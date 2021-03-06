#include "elfParser.h"
#include "elfReader.h"
#include <iostream>
#include "buildElf.h"
#include "stdWriter.h"
#include <sstream>
#include "tester.h"
#include <elf.h>
#include "dataLump.h"
#include "defer.h"
#include "binaryDescribe.h"

unsigned char * buf = new unsigned char[50000000];
DEFER ( delete [] buf;)
DataIO outfile(buf,5000000);
SectionHeader* stringheaders;
Elf64_Shdr* sections;
SectionHeader* stringTableHeader;
std::map<string, int>* sectionMap;
std::vector<Section *>* sectionHeaders;

int ValidHeader(testLogger& log );
int SectionNames(testLogger& log );

int main(int argc, const char *argv[])
{
    /*
     * Tests to validate the string table written by ElfFile
     */
    ElfFileReader f("isYes/a.out");
    
    ElfParser p(f);
    
    //parse the input file
    ElfContent content = p.Content();
    sectionMap = &content.sectionMap;

    ElfFile file( content);
    
    // Write out the data
    outfile.Fill(0,'\0',5000);
    file.WriteToFile(outfile);

    // read in the section data
    BinaryReader readPos(outfile);
    ElfHeaderX86_64 header(readPos);
    sections = new Elf64_Shdr[header.Sections()];
    DEFER ( delete [] sections; ) 

    readPos = header.SectionTableStart();

    for ( int i=0; i< header.Sections(); i++ ) {
        readPos >> sections[i];
    }

    stringTableHeader = new SectionHeader(sections[header.StringTableIndex()]);
    DEFER(delete stringTableHeader;)
    sectionHeaders = &content.sections;

    Test("Header format",(loggedTest)ValidHeader).RunTest();
    Test("Header format",(loggedTest)SectionNames).RunTest();
    return 0;
}

int ValidHeader(testLogger& log ) {
    int retCode = 0;

    if ( stringTableHeader->sh_type != SHT_STRTAB ) {
        log << stringTableHeader->sh_type << " , " << SHT_STRTAB << endl;
        log << "Invalid section type" << endl;
        log << "Header Block: " << endl;
        log << stringTableHeader->Descripe() << endl;
        retCode = 1;
    }

    return retCode;
}

int SectionNames(testLogger& log ) {
    // new string table 
    BinaryReader newSReader(outfile,stringTableHeader->DataStart());

    for ( auto& pair: *sectionMap) {
        // get the first and second elements
        string originalName = pair.first;
        SectionHeader newHdr(sections[pair.second]);
        Section& oldHdr = *(*sectionHeaders)[pair.second];

        BinaryReader strTable(outfile,stringTableHeader->DataStart() + newHdr.NameOffset());
        string newName = strTable.ReadString();
        if ( newName != originalName ) {
            log << "(" << pair.second << ") Name miss match: ";
            log  << originalName << " -> " << newName << endl;
            log << hex << oldHdr.DataStart() << " -> " << newHdr.DataStart() << endl;
            return 1;
        } else {
            log << "(" << pair.second << ") Name match: ";
            log <<  originalName << " -> " << newName << endl;
        }
    }
    return 0;
}
