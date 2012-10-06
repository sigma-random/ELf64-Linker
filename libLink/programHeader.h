#include <string>
#include <vector>
#include "flags.h"
#include "elf.h"
#include "elfReader.h"
using namespace std;
#ifndef ProgramX86_64
#define ProgramX86_64

class Section;
class ProgramHeader {
public:
    ProgramHeader (ElfReader& , long , vector< Section *>& );
    virtual ~ProgramHeader (){};

    // Methods
    string WriteLink();
    void InitialiseFlags();

    // attributes
    size_t Size() { return sizeof(Elf64_Phdr);}
    Elf64_Off DataStart() { return data.p_offset; }
    Elf64_Xword& Alignment() { return data.p_align; }
    Elf64_Xword FileSize() { return data.p_filesz; }
    Elf64_Xword& SizeInMemory() { return data.p_memsz; }
    Elf64_Addr& Address() { return data.p_vaddr; }

    // Calculated values
    Elf64_Off DataEnd() { return DataStart() + FileSize(); }



private:
    Elf64_Phdr data;
    string sectionNames;
    string name;
    Flags flags;
    /* data */
};


#endif

