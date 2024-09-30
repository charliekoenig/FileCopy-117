#include "c150nastydgmsocket.h"
#include "c150debug.h"
#include "c150dgmsocket.h"
#include "c150grading.h"
#include "c150nastyfile.h"
#include <fstream>
#include <cstdlib>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <iostream>

using namespace C150NETWORK;

const int NETWORK_NAST_ARG = 1;
const int FILE_NAST_ARG    = 2;
const int TARGET_DIR       = 3;


char *testRead(string sourceDir, string fileName, int nastiness);

int
main(int argc, char *argv[]) {

    GRADEME(argc, argv);

    if (argc != 4) {
        cerr << "Correct syntax is " << argv[0] << " <networknastiness> <filenastiness> <targetdir>" << endl;
        exit(1);
    }


    ssize_t readLen;
    char incomingMessage[512];
    int nastiness = atoi(argv[NETWORK_NAST_ARG]);

    int i = 0;
    try {
        C150DgmSocket *sock = new C150NastyDgmSocket(nastiness);

        while(1) {
            readLen = sock -> read(incomingMessage, sizeof(incomingMessage));
            if (readLen == 0) {
                cerr << "Read zero message on " << i << " iteration. Trying again" << endl;
                continue;
            }

            incomingMessage[readLen] = '\0';  // make sure null terminated
            string incoming(incomingMessage); // Convert to C++ string ...it's slightly
                                              // easier to work with, and cleanString
                                              // expects it


            char *response = testRead(argv[TARGET_DIR], incoming, nastiness);
            string finalResponse(response);
            if (response[0] == '\0') {
                free(response);
                continue;
            }
            cleanString(finalResponse);            // c150ids-supplied utility: changes
                                              // non-printing characters to .
            
            sock -> write(finalResponse.c_str(), finalResponse.length()+1);
            free(response);
            i++;
        }
            
    } catch (C150NetworkException &e) {
        // In case we're logging to a file, write to the console too
        cerr << argv[0] << ": caught C150NetworkException: " << e.formattedExplanation() << endl;
    }

    return 0;
}



bool isFile(string fname);
string makeFileName(string dir, string name);

char *
testRead(string sourceDir, string fileName, int nastiness) {

    //
    //  Misc variables, mostly for return codes
    //
    void *fopenretval;
    size_t len;
    string errorString;
    char *buffer;
    struct stat statbuf;  
    size_t sourceSize;

    // temporary
    char *empty;


    //
    // Put together directory and filenames SRC/file TARGET/file
    //
    string sourceName = makeFileName(sourceDir, fileName);

    //
    // make sure the file we're copying is not a directory
    // 
    if (!isFile(sourceName)) {
            cerr << "Input file " << sourceName << " is a directory or other non-regular file. Skipping" << endl;
            empty = (char *)malloc(1);
            empty[0] = '\0';
            return empty;
    }
    
    try {

        //
        // Read whole input file 
        //
        if (lstat(sourceName.c_str(), &statbuf) != 0) {
            fprintf(stderr,"copyFile: Error stating supplied source file %s\n", sourceName.c_str());
            exit(20);
        }
    
        //
        // Make an input buffer large enough for
        // the whole file
        //


        // sourceSize = statbuf.st_size;
        sourceSize = 11;
        buffer = (char *)malloc(sourceSize);

        //
        // Define the wrapped file descriptors
        //
        // All the operations on outputFile are the same
        // ones you get documented by doing "man 3 fread", etc.
        // except that the file descriptor arguments must
        // be left off.
        //
        // Note: the NASTYFILE type is meant to be similar
        //       to the Unix FILE type
        //
        NASTYFILE inputFile(nastiness);     // See c150nastyfile.h for interface
                                            // NASTYFILE is supposed to
                                            // remind you of FILE
                                            //  It's defined as: 
                                            // typedef C150NastyFile NASTYFILE
    
        // do an fopen on the input file
        fopenretval = inputFile.fopen(sourceName.c_str(), "rb");  
                                            // wraps Unix fopen
                                            // Note rb gives "read, binary"
                                            // which avoids line end munging
    
        if (fopenretval == NULL) {
            cerr << "Error opening input file " << sourceName << " errno=" << strerror(errno) << endl;
            exit(12);
        }
    

        // 
        // Read the whole file
        //
        len = inputFile.fread(buffer, 1, sourceSize);
        if (len != sourceSize) {
            cerr << "Error reading file " << sourceName << 
                "  errno=" << strerror(errno) << endl;
            exit(16);
        }
    
        if (inputFile.fclose() != 0 ) {
            cerr << "Error closing input file " << sourceName << 
                " errno=" << strerror(errno) << endl;
            exit(16);
        }

        buffer[10] = '\0';

        //
        // Free the input buffer to avoid memory leaks
        //

        return buffer;
        //
        // Handle any errors thrown by the file framekwork
        //
    } catch (C150Exception& e) {
        cerr << "nastyfiletest:copyfile(): Caught C150Exception: " << e.formattedExplanation() << endl;
    } 

    empty = (char *)malloc(1);
    empty[0] = '\0';
    return empty;
}


// ------------------------------------------------------
//
//                   makeFileName
//
// Put together a directory and a file name, making
// sure there's a / in between
//
// ------------------------------------------------------

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



// ------------------------------------------------------
//
//                   isFile
//
//  Make sure the supplied file is not a directory or
//  other non-regular file.
//     
// ------------------------------------------------------

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