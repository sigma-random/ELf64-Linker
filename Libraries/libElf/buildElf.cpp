#include "buildElf.h"
#include "programHeader.h"
#include <cstring>
#include <iostream>



ElfFile::ElfFile(ElfContent& data) 
     : file(1024) , 
       programHeadersStart(file),
       dataSectionStart(file),
       sectionHeadersStart(file),
       header(ElfHeaderX86_64::NewObjectFile())
{
    InitialiseHeader(data);
    InitialiseFile(data);
      
    // Process the program headers
    header.SectionTableStart() =  (long)ProcessProgHeaders( data);

    WriteSectionHeaders(data);

    // Finally write the header
    file.Writer().Write(&header,header.Size());

    // clean up any extra data
    file.resize( (long)sectionHeadersStart + 
                 header.Sections() * header.SectionHeaderSize() );
}

void ElfFile::InitialiseFile(ElfContent& data) {
    long sectionDataLength = 0;
    
    programHeadersStart = header.Size();
    dataSectionStart = (long) programHeadersStart + 
                       (  header.ProgramHeaderSize() 
                        * data.progHeaders.size() );
                         
    // Reserve space for data, plus room for alignment 
    // ( the idea is to allocate too much and resize back down )

    // guess the length of the sectionDataLength
    for( auto section : data.sections)
        sectionDataLength += section->Size();
    // allow room for alignments of loadable segments
    sectionDataLength += 16 * header.ProgramHeaders();

    file.resize(  (long) dataSectionStart
                + sectionDataLength
                + header.SectionHeaderSize() * header.Sections());
}

void ElfFile::InitialiseHeader(ElfContent &data) {
    
    header.ProgramHeaders() = data.progHeaders.size();
    header.Sections()  = data.sections.size();
    
    // If there is no load table, progheader start is defined to be 0 by 
    // the elf standard
    if ( header.ProgramHeaders() == 0 ) {
        header.ProgramHeadersStart() = 0;
    } else {
        header.ProgramHeadersStart() = header.Size();
    }
}

BinaryWriter ElfFile::ProcessProgHeaders(ElfContent &data ) {
    // we need a local copy to sort
    auto headers = data.progHeaders;

    BinaryWriter dataWritePos = dataSectionStart;
    auto dataEnd = dataWritePos;
    auto pheaderWritePos = programHeadersStart;

    headers.reserve(headers.size());

    SortByAddress(headers.begin(),headers.end());

    for ( auto ph: headers ) {
        // We need to align the program segment: (see comment in .h)
        // Calculate boundrary location
        dataEnd= (long)dataWritePos.NextBoundrary( ph->Alignment()) 
                + (ph->Address() % ph->Alignment());
        // move to the boundrary
        dataWritePos.FillTo(dataEnd);
        dataWritePos = (long)dataEnd;

        // write the section data
        dataEnd = (long)WriteDataSections( data, *ph, dataWritePos);

        // Set the file position in the program header
        ph->DataStart() = dataWritePos;
        ph->FileSize() = dataEnd - dataWritePos;

        pheaderWritePos.Write(ph,header.ProgramHeaderSize());
        pheaderWritePos += header.ProgramHeaderSize();
    }
    return pheaderWritePos;
}


BinaryWriter ElfFile::WriteDataSections( ElfContent &data, 
                                         ProgramHeader& prog,
                                         BinaryWriter& writer) {
    BinaryWriter end = writer;
    BinaryWriter writePos = writer;
    Section * section;
    for ( auto sname: prog.SectionNames() ) {
        section = data.sections[data.sectionMap[sname]];
        if ( prog.Address() != 0 ) {
            if ( section->DataSize() == 0 ) { 
                // don't care
                continue;
            }
            if ( section->Address() == 0 ) {
                string error = "FATAL ERROR: A section in a ";
                error += "loadable segment MUST specify a load ";
                error += "address. Segment table is currupt, I ";
                error += "cannot continue";
                error += "( " + sname + " )";
                throw error;
            }
            writePos = (long)writer + section->Address() 
                                    - prog.Address();
            section->WriteRawData(writePos);
            if (end <=  section->Size() + writePos)
                 end =  section->Size() + writePos;

        } else {
            writePos = (long)end;
            section->DataStart() = writePos;
            section->WriteRawData(writePos);
            // These are not loadable, we have no responsibility to 
            // align them
            end = section->Size() + writePos;
        }
    }
    writePos = (long)end;
    return writePos;
}

bool ElfFile::IsSpecialSection(Section& s) {
    bool special = false;
    special |= s.Name() == ".shstrtab";
    special |= s.Name() == ".symtab";
    special |= s.Name() == ".strtab";
    return special;
}

void ElfFile::WriteSectionHeaders(ElfContent &data ) {
    auto writer = dataSectionStart;
    long idx = 0;
    // Write the standard header sections
    for ( auto sec : data.sections ) {
        if ( ! IsSpecialSection( *sec ) ) {
            writer.Write(&sec,header.SectionHeaderSize());
            writer += header.SectionHeaderSize();
            ++idx;
        }
    }

    WriteSpecial(data, ".shstrtab" ,idx, writer);
    WriteSpecial(data, ".symtab", idx, writer);
    WriteSpecial(data, ".strtab", idx, writer);
}

void ElfFile::WriteSpecial(ElfContent& data, string name,
                                             long& idx,
                                             BinaryWriter& writer) {
    if ( name == ".shstrtab" )  {
        this->header.StringTableIndex() = idx;
    }
    auto loc = data.sectionMap.find(name);
    if ( loc != data.sectionMap.end() ) {
        auto sec = data.sections[loc->second];
        writer.Write(sec,header.SectionHeaderSize());
        writer += header.SectionHeaderSize();
        ++idx;
    }
}

void ElfFile::WriteToFile(BinaryWriter& w) {
    this->file.Reader().Read(w,this->file.Size());
}
