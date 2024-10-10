#include "c150nastydgmsocket.h"
#include "c150debug.h"
#include "c150grading.h"
#include "nastyfileops.h"
#include "packetstruct.h"
#include "makeclientpackets.h"
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

    char *filename;
    char incomingMessage[512];
    int packetNumber = 0;
    packet response;

    // TODO: clean this up
    try {
        C150DgmSocket *sock = new C150DgmSocket();
        sock -> setServerName(argv[SERVER_ARG]);
        sock -> turnOnTimeouts(3000);

        // loop for each file in the given directory
        while ((sourceFile = readdir(SRC)) != NULL) {
            filename = sourceFile -> d_name;
            // skip the . and .. names
            if ((strcmp(filename, ".") == 0) || (strcmp(filename, "..")  == 0 )) 
                continue;

            bool noResponse = sock -> timedout();
            bool unexpectedPacket = true;

            printf("filename: %s\n", filename);
            
            // PREP PACKET C
            // get file information
            unsigned char *fileContent;
            ssize_t bytesRead = readFile(argv[SRC_DIR], filename, atoi(argv[FILE_NAST_ARG]), &fileContent);
            // unsigned int bytesReadSize = sizeof(bytesRead);
            // unsigned char bytesReadCharRep[bytesReadSize];
            // for (unsigned int offset = 0; offset < bytesReadSize; offset++) {
            //     bytesReadCharRep[bytesReadSize - 1 - offset] = ((bytesRead >> (offset * 8)) & ~0);
            // }

            // int cContentLen = strlen(filename) + bytesReadSize;
            // char cContent[cContentLen];
            // for (int k = 0; k < cContentLen; k++) {
            //     if (k < (int) bytesReadSize) {
            //         cContent[k] = bytesReadCharRep[k];
            //     } else {
            //         cContent[k] = filename[k - (int) bytesReadSize];
            //     }
            // }

            // create packet and write to server
            packet fileInfoPacket = makeCopyPacket(bytesRead, filename, packetNumber);

            char *fileInfoPacketString = packetToString(fileInfoPacket);
            
            char *filenameRead = NULL;
            parseCPacket(fileInfoPacket, &filenameRead);
            printf("%s\n", filenameRead);
            free(filenameRead);
            int packetLen = packetLength(fileInfoPacket);
            freePacket(fileInfoPacket);

            // LOOP TIL CLIENT RECEVES R PACKET FOR C FROM SERVER
            int attempts = 0;
            while (noResponse || unexpectedPacket) {
                sock -> write(fileInfoPacketString, packetLen);
                // *GRADING << "File: " << filename << " transmission complete, waiting for end-to-end check, attempt " << attempts << endl;

                sock -> read(incomingMessage, sizeof(incomingMessage));
                noResponse = sock -> timedout();

                if (!noResponse) {
                    response = stringToPacket(incomingMessage);
                    unexpectedPacket = (packetOpcode(response) != 'R' ||
                                        packetNum(response) != packetNumber);
                }
                
                attempts++; 
            }
            // printf("Hash Response: ");
            printPacket(response);
            packetNumber = (packetNumber == 255) ? 0 : (packetNumber + 1);

            // unsigned char parsedHash[20];
            // parseHash(response, fileName, parsedHash);
            // for (int k = 0; k < 20; k++) {
            //     printf("%02x", parsedHash[k]);
            // }
            // cout << endl;

            // unsigned char *fileContent;
            // ssize_t bytesRead = readFile(argv[SRC_DIR], fileName, atoi(argv[FILE_NAST_ARG]), &fileContent);
            
            // unsigned char obuff[20];
            // SHA1((const unsigned  char *)fileContent, bytesRead, obuff);
            // free(fileContent);
            // printf("Client Hash: ");
            // for (int k = 0; k < 20; k++) {
            //     printf("%02x", obuff[k]);
            // }
            // cout << endl;

            // char statusContent[strlen(fileName) + 1];
            // statusContent[0] = (strncmp((const char *) parsedHash, (const char *) obuff, 20) == 0) ? 'S' : 'F';
            
            // for (size_t j = 1; j < strlen(fileName) + 1; j++) {
            //     statusContent[j] = fileName[j-1];
            // }

            // packet statusPacket = makePacket('S', strlen(fileName) + 1, packetNumber, statusContent);
            // packetString = packetToString(statusPacket);
            // packetLen = packetLength(statusPacket) + 3;
            // freePacket(statusPacket);

            // attempts = 0;
            // noResponse = true, unexpectedPacket = true;

            // while (noResponse || unexpectedPacket) {
            //     if (statusContent[0] == 'S') {
            //         cout << "SUCCESS: " << fileName << endl;
            //         *GRADING << "File: " << fileName << " end-to-end check succeeded, attempt " << attempts << endl;
            //     } else if (statusContent[0] == 'F') {
            //         cout << "FAILURE: " << fileName << endl;
            //         *GRADING << "File: " << fileName << " end-to-end check failed, attempt " << attempts << endl;
            //     }

            //     sock -> write(packetString, packetLen);

            //     sock -> read(incomingMessage, sizeof(incomingMessage));
            //     noResponse = sock -> timedout();

            //     if (!noResponse) {
            //         response = stringToPacket(incomingMessage);
            //         unexpectedPacket = (packetOpcode(response) != 'A' ||
            //                             packetNum(response) != packetNumber);
            //     }
            //     attempts++; 
            // }
            // printf("Ack Response: %s\n", packetContent(response));
            // cout << "_______________________________" << endl;
            // packetNumber = (packetNumber == 255) ? 0 : (packetNumber + 1);

            printf("--------------------\n");
        }
    } catch (C150NetworkException &e) {
        cerr << argv[0] << ": caught C150NetworkException: " << e.formattedExplanation() << endl;
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