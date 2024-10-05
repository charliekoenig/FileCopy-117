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
bool parseHash(packet hashPacket, char *currFilename, unsigned char *parsedHash);

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
            
            sock -> write(packetString, packetLength(diskData) + 2);

            *GRADING << "File: " << fileName << " transmission complete, waiting for end-to-end check, attempt " << 0 << endl;
            
            freePacket(diskData);
            // receive computed hash of file in target directory from server
            sock -> read(incomingMessage, sizeof(incomingMessage));
            packet hashFromServer = stringToPacket(incomingMessage);
            // if (packetOpcode(filenamePacket) != 'H') { JUST_ASKING_FOR_HASH; }
            unsigned char parsedHash[20];
            parseHash(hashFromServer, fileName, parsedHash);
            // todo: discard if filenames not matching
            
            // read file content
            unsigned char *fileContent;
            ssize_t bytesRead = readFile(argv[SRC_DIR], fileName, atoi(argv[FILE_NAST_ARG]), &fileContent);
            // TODO: error handling in readFile

            // compute hash for file in source directory
            unsigned char obuff[20];
            SHA1((const unsigned  char *)fileContent, bytesRead, obuff);

            free(fileContent);

            // set status as success or failure based on hash comparison
            char statusContent[strlen(fileName) + 1];
            statusContent[0] = (strncmp((const char *) parsedHash, (const char *) obuff, 20) == 0) ? 'S' : 'F';

            if (statusContent[0] == 'S') {
                *GRADING << "File: " << fileName << " end-to-end check succeeded, attempt " << 0 << endl;
            } else if (statusContent[0] == 'F') {
                *GRADING << "File: " << fileName << " end-to-end check failed, attempt " << 0 << endl;
            }
            
            for (size_t j = 1; j < strlen(fileName) + 1; j++) {
                statusContent[j] = fileName[j-1];
            }

            packet statusPacket = makePacket('S', strlen(fileName) + 1, statusContent);
            sock -> write(packetToString(statusPacket), packetLength(statusPacket) + 2);

            // read acknowledgement packet from server
            sock -> read(incomingMessage, sizeof(incomingMessage));
            packet ackPacket = stringToPacket(incomingMessage);
            
            // todo: make sure filenames match
            (void) ackPacket;
            printf("--------------------\n");

        } catch (C150NetworkException &e) {
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

bool
parseHash(packet hashPacket, char *currFilename, unsigned char *parsedHash) {
    // parse hashPacket's packetContent and store first 20 in hash and rest in filenamePacket
    char packetsFilename[packetLength(hashPacket) - 2 - 20];
    for (int i = 0; i < packetLength(hashPacket); i++) {
        if (i < 20) {
            // update parsedHash with hash that we parse from hashPacket's packetContent
            parsedHash[i] =  packetContent(hashPacket)[i];
        } else {
            packetsFilename[i - 20] = packetContent(hashPacket)[i];
        }
    }

    // compare filenamepacket with currFilename
    return strcmp((const char *) currFilename, (const char *) packetsFilename) == 0;
}

// to do for fri: getting things out of main in both client & server, gradelog & debug prints