using namespace std;
#include <cstring>
#include <iostream>
#include "tester.h"
#include <sstream>
#include "stdReader.h"
#include <cstdio>
#include <vector>
#include "defer.h"

//Tests
template<class Reader>
long VerifyRead(stringstream& log);

string quote = R"(
There is a computer disease that anybody who works with computers knows about. 
It's a very serious disease and it interferes completely with the work. 
The trouble with computers is that you 'play' with them!
Richard P. Feynman 
)";

//Validators
bool ValidateMatch( const char *str1, const char *str2, 
                                      long len,
                                      stringstream& log);

template<class Reader>
long VerifyGet(stringstream& log);
template<class Reader>
long VerifyRead(stringstream& log);

int main(int argc, const char *argv[])
{
    Test("Basic gets",  (loggedTest)VerifyGet<IFStreamReader>).RunTest();
    Test("Read hunks",  (loggedTest)VerifyRead<IFStreamReader>).RunTest();
    return 0;
}

bool ValidateMatch( const char *str1, const char *str2, 
                                      long len,
                                      stringstream& log)  
{
    bool match = true;
    long i;
    for (i = 0; i < len; i++) {
        log << i << ": " << str1[i] << " , "  << str2[i] << endl;
        if ( str1[i] != str2[i] ) {
            log << "Character missmatch! (" << i << ")" << endl;
            log << (int)str1[i] << " -> " << (int)str2[i] << endl;
            match = false;
            break;
        }
    }
    if ( match ) {
         log << "arrays matched " << endl;
    }
    return match;
}


// Define a generator for each class to be tested
IFStreamReader* Generator() {

    static long count = 0;
    count++;

    //prepare the file
    stringstream fname;
    fname << "stdReaderTest" << count << ".tmp";
    fname.flush();
    ofstream writer(fname.str().c_str(), ios_base::binary | ios_base::out);
    writer << quote;
    writer.put('\0');
    writer.close();

    return new IFStreamReader(fname.str().c_str());
}


// Define a Delete for each class
void Delete ( IFStreamReader* reader ) { remove(reader->Fname().c_str()); delete reader;
}


template<class Reader>
long VerifyGet(stringstream& log) {
    long ret = 0;

	Reader *reader = Generator();
    // remember to clean up after ourselves:
    DEFER(Delete(reader);)

    vector<int> testInds  ({
         0, 
         (int)quote.length(),
         10,
         50
    });

    for (int& ind: testInds) {
        char q = quote[ind];
        char r = reader->Get(ind);
        log << ind << ": " << q << " , " << r << endl;
        if (quote[ind] != reader->Get(ind) ){
            log << "MISSMATCH" << endl;
            return 1;
        }
    }
    return 0;
}

template<class Reader>
long VerifyRead(stringstream& log) {
    long ret = 0;

	Reader *reader = Generator();
    // remember to clean up after ourselves:
    DEFER(Delete(reader);)

    char* buffer = new char[quote.length()];

    vector<int> testInds  ({
         0, 
         200,
         10,
         0,
         50
    });

    for (int& ind: testInds) {
        memset(buffer,'\0',quote.length());
        const char *src = quote.c_str() + ind;
        reader->Read(ind, buffer,20);
        if ( !ValidateMatch(src,buffer,20,log) ) {
            log << "Match failure at index: " << ind << endl;
            log << "Quote: " << quote.c_str() + ind << endl;
            log << "Read: " << buffer << endl;
            return 1;
        }
    }
    return 0;
}