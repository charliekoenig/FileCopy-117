#include "c150nastydgmsocket.h"
#include "c150debug.h"
#include "c150grading.h"
#include "nastyfileops.h"
#include "packetstruct.h"
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <openssl/sha.h>

using namespace std;
using namespace C150NETWORK;

const int SERVER_ARG       = 1;
const int NETWORK_NAST_ARG = 2;
const int FILE_NAST_ARG    = 3;
const int SRC_DIR          = 4;

void checkDirectory(char *dirname);

int 
main(int argc, char *argv[]) {

    GRADEME(argc, argv);

    if (argc != 5) {
        cerr << "Correct syntax is " << argv[0] << " <server> <networknastiness> <filenastiness> <srcdir>" << endl;
        exit(1);
    }

    struct dirent *sourceFile;
    DIR *SRC;

    // make sure input given is a valid directory and open it
    checkDirectory(argv[SRC_DIR]);
    SRC = opendir(argv[SRC_DIR]);
    if (SRC == NULL) {
        fprintf(stderr,"Error opening source directory %s\n", argv[2]);     
        exit(8);
    }

    char *fileName;
    ssize_t readLen;
    char incomingMessage[512];

    // loop for each file in the given directory
    while ((sourceFile = readdir(SRC)) != NULL) {
        fileName = sourceFile -> d_name;
        // skip the . and .. names
        if ((strcmp(fileName, ".") == 0) || (strcmp(fileName, "..")  == 0 )) 
            continue;          // never copy . or ..

        try { 
            C150DgmSocket *sock = new C150DgmSocket();
            sock -> setServerName(argv[SERVER_ARG]);

            printf("FILENAME: %s\n", fileName);
            
            packet diskData = makePacket('F', strlen(fileName), fileName);
            char *packetString = packetToString(diskData);
            packet packetPacket = stringToPacket(packetString);
            freePacket(diskData);
            freePacket(packetPacket);
            (void)packetString;

            sock -> write(fileName, strlen(fileName) + 1);
            
            // receive computed hash of file in target directory from server
            readLen = sock -> read(incomingMessage, sizeof(incomingMessage));
            (void)readLen;

            // read file content
            unsigned char *fileContent;
            ssize_t bytesRead = readFile(argv[SRC_DIR], fileName, atoi(argv[FILE_NAST_ARG]), &fileContent);
            // TODO: Check for -1?

            // compute hash for file in source directory
            unsigned char obuff[20];
            SHA1((const unsigned  char *)fileContent, bytesRead, obuff);

            free(fileContent);
            printf("--------------------\n");

        } catch (C150NetworkException &e) {
            // write error to console
            cerr << argv[0] << ": caught C150NetworkException: " << e.formattedExplanation() << endl;
        }
    }

    closedir(SRC);
    return 0;
}

/**********************************************************
 * Function: checkDirectory
 
 * Parameters: 
    * char *dirname   -> possible directory name

 * Return: nothing

 * Side Effects: prints error and exits program if dirname
                 is not a valid directory name

 * written by: Noah Mendelsohn 
   from: nastyfiletest.cpp
***********************************************************/
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
