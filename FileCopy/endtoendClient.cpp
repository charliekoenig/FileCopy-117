#include "c150dgmsocket.h"
#include "c150debug.h"
#include "c150grading.h"
#include <fstream>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <openssl/sha.h>
#include "c150nastyfile.h"


using namespace std;
using namespace C150NETWORK;

const int SERVER_ARG       = 1;
const int NETWORK_NAST_ARG = 2;
const int FILE_NAST_ARG    = 3;
const int SRC_DIR          = 4;

void checkDirectory(char *dirname);
void checkAndPrintMessage(ssize_t readlen, char *msg, ssize_t bufferlen);
ssize_t readFile(string sourceDir, string fileName, int nastiness, unsigned char **obuff);
string makeFileName(string dir, string name);
bool isFile(string fname);

int 
main(int argc, char *argv[]) {

    GRADEME(argc, argv);

    if (argc != 5) {
        cerr << "Correct syntax is " << argv[0] << " <server> <networknastiness> <filenastiness> <srcdir>" << endl;
        exit(1);
    }

    struct dirent *sourceFile;
    DIR *SRC;

    checkDirectory(argv[SRC_DIR]);
    SRC = opendir(argv[SRC_DIR]);
    if (SRC == NULL) {
        fprintf(stderr,"Error opening source directory %s\n", argv[2]);     
        exit(8);
    }

    char *fileName;
    ssize_t readLen;
    char incomingMessage[512];
    while ((sourceFile = readdir(SRC)) != NULL) {
        fileName = sourceFile -> d_name;
        // skip the . and .. names
        if ((strcmp(fileName, ".") == 0) || (strcmp(fileName, "..")  == 0 )) 
            continue;          // never copy . or ..

        try { 
            C150DgmSocket *sock = new C150DgmSocket();
            sock -> setServerName(argv[SERVER_ARG]);

            sock -> write(fileName, strlen(fileName) + 1);
            
            readLen = sock -> read(incomingMessage, sizeof(incomingMessage));
            checkAndPrintMessage(readLen, incomingMessage, sizeof(incomingMessage));

            // adding on 9/30
            unsigned char *fileContent;
            ssize_t bytesRead = readFile(argv[SRC_DIR], fileName, atoi(argv[FILE_NAST_ARG]), &fileContent);
            // TODO: Check for -1?

            unsigned char obuff[20];
            SHA1((const unsigned  char *)fileContent, bytesRead, obuff);
            printf("75      : ");
            for (int j = 0; j < 20; j++) {
                printf("%02x", (unsigned int) obuff[j]);
            }
            cout << endl;

            

            free(fileContent);
            // until here - 9/30

        } catch (C150NetworkException &e) {
            // In case we're logging to a file, write to the console too
            cerr << argv[0] << ": caught C150NetworkException: " << e.formattedExplanation() << endl;
        }
    }

    closedir(SRC);
    return 0;
}


void
checkDirectory(char *dirname) {
    struct stat statbuf;  
    if (lstat(dirname, &statbuf) != 0) {
        fprintf(stderr,"Error stating supplied source directory %s\n", dirname);
        exit(8);
    }

    if (!S_ISDIR(statbuf.st_mode)) {
        fprintf(stderr,"File %s exists but is not a directory\n", dirname);
        exit(8);
    }
}

void
checkAndPrintMessage(ssize_t readlen, char *msg, ssize_t bufferlen) {
    // 
    // Except in case of timeouts, we're not expecting
    // a zero length read
    //
    if (readlen == 0) {
        throw C150NetworkException("Unexpected zero length read in client");
    }

    // DEFENSIVE PROGRAMMING: we aren't even trying to read this much
    // We're just being extra careful to check this
    if (readlen > (int)(bufferlen)) {
        throw C150NetworkException("Unexpected over length read in client");
    }

    //
    // Make sure server followed the rules and
    // sent a null-terminated string (well, we could
    // check that it's all legal characters, but 
    // at least we look for the null)
    //
    if(msg[readlen-1] != '\0') {
        throw C150NetworkException("Client received message that was not null terminated");     
    };

    //
    // Use a routine provided in c150utility.cpp to change any control
    // or non-printing characters to "." (this is just defensive programming:
    // if the server maliciously or inadvertently sent us junk characters, then we 
    // won't send them to our terminal -- some 
    // control characters can do nasty things!)
    //
    // Note: cleanString wants a C++ string, not a char*, so we make a temporary one
    // here. Not super-fast, but this is just a demo program.
    string s(msg, readlen);
    // cout << s << endl;
    cleanString(s);
    // cout << s << endl << endl;
    // Echo the response on the console
    printf("Response: ");
    c150debug->printf(C150APPLICATION,"PRINTING RESPONSE: Response received is \"%s\"\n", s.c_str());
    // printf("Response received is \"%02x\"\n", (unsigned int) s);
    for (int h = 0; h < readlen - 1; h++) {
        printf("%02x", (unsigned int) (unsigned char) msg[h]);
    }
    printf("\n");
}

ssize_t
readFile(string sourceDir, string fileName, int nastiness, unsigned char **obuff) {
    struct stat statbuff;
    void *fopenretval;
    size_t length;
    size_t sourceSize;

    string fullFilePath = makeFileName(sourceDir, fileName);

    if (!isFile(fullFilePath)) {
        cerr << "Input file " << fullFilePath << " is a directory or other non-regular file. Skipping" << endl;
        return -1; 
    }

    try {

        if (lstat(fullFilePath.c_str(), &statbuff) != 0) {
            fprintf(stderr,"copyFile: Error stating supplied source file %s\n", fullFilePath.c_str());
            exit(20);
        }

        sourceSize = statbuff.st_size;
        *obuff = (unsigned char *)malloc(sourceSize);
        
        NASTYFILE inputFile(nastiness); 

        fopenretval = inputFile.fopen(fullFilePath.c_str(), "rb");  
        if (fopenretval == NULL) {
            cerr << "Error opening input file " << fullFilePath << " errno=" << strerror(errno) << endl;
            exit(12);
        }

        length = inputFile.fread(*obuff, 1, sourceSize);
        if (length != sourceSize) {
            cerr << "Error reading file " << fullFilePath << " errno=" << strerror(errno) << endl;
            exit(16);
        }

        if (inputFile.fclose() != 0 ) {
            cerr << "Error closing input file " << fullFilePath << " errno=" << strerror(errno) << endl;
            exit(16);
        }


        return length;

    } catch (C150Exception& e) {
        cerr << "endtoendserver:readfile(): Caught C150Exception: " << e.formattedExplanation() << endl;
    } 

    return -1;
}


string
makeFileName(string dir, string name) {
    stringstream ss;

    ss << dir;
    // make sure dir name ends in /
    if (dir.substr(dir.length()-1,1) != "/")
        ss << '/';
    ss << name;     // append file name to dir
    return ss.str();  // return dir/name
  
}

bool
isFile(string fname) {
    const char *filename = fname.c_str();
    struct stat statbuf;  
    if (lstat(filename, &statbuf) != 0) {
        fprintf(stderr,"isFile: Error stating supplied source file %s\n", filename);
        return false;
    }

    if (!S_ISREG(statbuf.st_mode)) {
        fprintf(stderr,"isFile: %s exists but is not a regular file\n", filename);
        return false;
    }
    return true;
}