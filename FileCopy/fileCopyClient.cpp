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
#include <cmath>

using namespace std;
using namespace C150NETWORK;

const int SERVER_ARG       = 1;
const int NETWORK_NAST_ARG = 2;
const int FILE_NAST_ARG    = 3;
const int SRC_DIR          = 4;

const int MAX_READ         = 503;
const int MAX_PACKET_NUM   = 0x3FFF;

void checkDirectory(char *dirname);
void parseHash(packet hashPacket, char *currFilename, unsigned char *parsedHash);

int 
main(int argc, char *argv[]) {

    GRADEME(argc, argv);

    if (argc != 5) {
        cerr << "Correct syntax is " << argv[0] 
             << " <server> <networknastiness> <filenastiness> <srcdir>" << endl;

        exit(1);
    }

    // File to be transferred 
    struct dirent *sourceFile;

    // make sure input given is a valid directory and open it
    checkDirectory(argv[SRC_DIR]);
    DIR *SRC = opendir(argv[SRC_DIR]);
    if (SRC == NULL) {
        fprintf(stderr,"Error opening source directory %s\n", argv[2]);     
        exit(8);
    }

    char *filename;
    int filenameLength;
    char incomingMessage[512];
    int packetNumber = 0;
    packet response = NULL;

    try {
        C150DgmSocket *sock = new C150DgmSocket();
        sock -> setServerName(argv[SERVER_ARG]);
        sock -> turnOnTimeouts(500);

        // loop for each file in the given directory
        while ((sourceFile = readdir(SRC)) != NULL) {
            // skip the . and .. names
            filename = sourceFile -> d_name;
            filenameLength = strlen(filename);
            if ((strcmp(filename, ".") == 0) || (strcmp(filename, "..")  == 0 )) 
                continue;

            bool noResponse = sock -> timedout();
            bool unexpectedPacket = true;
            
            // PREP PACKET C
            // get file information
            unsigned char *fileContent = NULL;

            struct stat statbuf;
            size_t sourceSize;
            string sourceFile = makeFileName(argv[SRC_DIR], filename);

            if (!isFile(sourceFile)) {
                cerr << sourceFile << " is a not a valid file. Skipping" << endl;
                continue;
            }

            if (lstat(sourceFile.c_str(), &statbuf) != 0) {
                fprintf(stderr, "copyFile: Error stating supplied source file %s\n", sourceFile.c_str());
                continue;
            }

            sourceSize = statbuf.st_size;
            fileContent = (unsigned char *)malloc(sourceSize);
            NASTYFILE inputFile(atoi(argv[FILE_NAST_ARG]));

            void *filePtr    = NULL;
            size_t bytesRead = 0;
            int closeResult  = 0;

            // open file, try until success
            do {
                filePtr = inputFile.fopen(sourceFile.c_str(), "rb");
            } while (filePtr == NULL);

            // read file, try until success
            do {
                bytesRead = inputFile.fread(fileContent, 1, sourceSize);
            } while (bytesRead != sourceSize);
        
            // close file, try until success
            do {
                closeResult = inputFile.fclose();
            } while (closeResult != 0 );



            // Create packet alerting server of file to be copied
            packet fileInfoPacket = makeCopyPacket(bytesRead, filename, packetNumber);
            char *fileInfoPacketString = packetToString(fileInfoPacket);
            int packetLen = packetLength(fileInfoPacket);
            freePacket(fileInfoPacket);

            // Loop until server acknowledges C packet
            int attempts = 0;
            while (noResponse || unexpectedPacket) {
                sock -> write(fileInfoPacketString, packetLen);
                // *GRADING << "File: " << filename << " transmission complete, waiting for end-to-end check, attempt " << attempts << endl;

                sock -> read(incomingMessage, sizeof(incomingMessage));
                noResponse = sock -> timedout();

                // Confirm response was the expected packet
                if (!noResponse) {
                    response = stringToPacket((unsigned char *)incomingMessage);
                    unexpectedPacket = (packetOpcode(response) != 'R' ||
                                        packetNum(response) != packetNumber ||
                                        packetContent(response)[0] != 'C');
                    freePacket(response);
                    response = NULL;
                }
                
                attempts++; 
            }

            free(fileInfoPacketString);
            packetNumber = (packetNumber == MAX_PACKET_NUM) ? 0 : (packetNumber + 1);
            
            /****  Send packets of file data to server ****/

            // intialize intial offset in file and the # of bytes that can be sent
            unsigned int offset = 0;
            int bytesToSend = MAX_READ - filenameLength;

            unordered_map<int, packet> unAcked;
            stack<int> expectedAcks;
            packet bytePacket = NULL;

            sock -> turnOnTimeouts(5);
            while ((offset < bytesRead) || !expectedAcks.empty()) {

                // there are still unsent bytes from the file
                if (offset < bytesRead) {
                    bytesToSend = min(bytesToSend, (int)(bytesRead - offset));
                    bytePacket = makeBytePacket(offset, filename, fileContent, 
                                                packetNumber, bytesToSend, 
                                                filenameLength);

                    expectedAcks.push(packetNumber);
                    unAcked[packetNumber] = bytePacket;
                    // cout << "Sending " << bytesToSend << " bytes at offset " << offset << " for file " << filename << endl;
                    offset += bytesToSend;
                    packetNumber = (packetNumber == MAX_PACKET_NUM) ? 0 : (packetNumber + 1);

                // all bytes sent but not all packets acknowledged
                } else {
                    int next = 0;
                    
                    while ((!expectedAcks.empty()) && (bytePacket == NULL)) {
                        next = expectedAcks.top();
                        if ((unAcked[next] == NULL)) {
                            expectedAcks.pop();
                        } else {
                            bytePacket = unAcked[next];
                        }
                    }

                }

                if (bytePacket != NULL) {
                    char *bytePacketString = packetToString(bytePacket);
                    int packetLen = packetLength(bytePacket);

                    sock -> write(bytePacketString, packetLen);
                    
                    // read until timeout to avoid overloading server
                    do {
                        sock -> read(incomingMessage, sizeof(incomingMessage));
                        noResponse = sock -> timedout();

                        if (!noResponse) {
                            response = stringToPacket((unsigned char *)incomingMessage);

                            if ((packetOpcode(response) == 'R') && (packetContent(response)[0] == 'B')) {
                                int sentPacketNumber = packetNum(response);
                                if (unAcked[sentPacketNumber] != NULL) {
                                    freePacket(unAcked[sentPacketNumber]);
                                    unAcked[sentPacketNumber] = NULL;
                                }

                            }

                            if (response != NULL) {
                                freePacket(response);
                                response = NULL;
                            }
                        }
                    } while (!noResponse);

                    /*
                    sock -> read(incomingMessage, sizeof(incomingMessage));
                    noResponse = sock -> timedout();

                    if (!noResponse) {
                        response = stringToPacket((unsigned char *)incomingMessage);

                        if ((packetOpcode(response) == 'R') && (packetContent(response)[0] == 'B')) {
                            int sentPacketNumber = packetNum(response);
                            if (unAcked[sentPacketNumber] != NULL) {
                                freePacket(unAcked[sentPacketNumber]);
                                unAcked[sentPacketNumber] = NULL;
                            }

                            if (response != NULL) {
                                freePacket(response);
                                response = NULL;
                            }
                        }
                    }
                    */

                    free(bytePacketString);
                }

                bytePacket = NULL;
            }

            packetNumber = (packetNumber == MAX_PACKET_NUM) ? 0 : (packetNumber + 1);
            sock -> turnOnTimeouts(500);

            /****  Send packet to server requesting end-to-end check ****/
            packet fileCheckPacket = makeFileCheckPacket(filename, packetNumber);
            char *fileCheckPacketString = packetToString(fileCheckPacket);
            packetLen = packetLength(fileCheckPacket);

            freePacket(fileCheckPacket);

            noResponse = unexpectedPacket = true;
            while (noResponse || unexpectedPacket) {
                if (response != NULL) {
                    freePacket(response);
                    response = NULL;
                }

                sock -> write(fileCheckPacketString, packetLen);
                sock -> read(incomingMessage, sizeof(incomingMessage));
                noResponse = sock -> timedout();

                if (!noResponse) {
                    response = stringToPacket((unsigned char *)incomingMessage);
                    unexpectedPacket = (packetOpcode(response) != 'H' ||
                                        packetNum(response) != packetNumber);
                }
            }

            free(fileCheckPacketString);
            packetNumber = (packetNumber == MAX_PACKET_NUM) ? 0 : (packetNumber + 1);


            /** Send packet to server alerting of successful transfer or fail **/

            // Create SHA1 hash for file from SRC Directory
            unsigned char localHash[20];
            SHA1((const unsigned  char *)fileContent, bytesRead, localHash);

            // Create file status packet - marks success or fail 
            packet statusPacket = makeStatusPacket(localHash, response, packetNumber);
            char *statusPacketString = packetToString(statusPacket);
            packetLen = packetLength(statusPacket);
            freePacket(statusPacket);

            if (response != NULL) {
                freePacket(response);
                response = NULL;
            }

            attempts = 0;
            noResponse = true, unexpectedPacket = true;

            while (noResponse || unexpectedPacket) {
                if (response != NULL) {
                    freePacket(response);
                    response = NULL;
                }

                if (statusPacketString[4] == 'S') {
                    cout << "SUCCESS: " << filename << endl;
                    // *GRADING << "File: " << filename << " end-to-end check succeeded, attempt " << attempts << endl;
                } else if (statusPacketString[4] == 'F') {
                    cout << "FAILURE: " << filename << endl;
                    // *GRADING << "File: " << filename << " end-to-end check failed, attempt " << attempts << endl;
                }

                sock -> write(statusPacketString, packetLen);
                sock -> read(incomingMessage, sizeof(incomingMessage));
                noResponse = sock -> timedout();

                if (!noResponse) {
                    response = stringToPacket((unsigned char *) incomingMessage);
                    unexpectedPacket = (packetOpcode(response) != 'A' ||
                                        packetNum(response) != packetNumber);
                }
                attempts++;
            }
            
            printf("Ack Response: %s\n", packetContent(response));
            if (response != NULL) {
                    freePacket(response);
                    response = NULL;
            }

            free(statusPacketString);
            packetNumber = (packetNumber == MAX_PACKET_NUM) ? 0 : (packetNumber + 1);

            free(fileContent);
            fileContent = NULL;

            // cout << "_______________________________" << endl;
            // packetNumber = (packetNumber == MAX_PACKET_NUM) ? 0 : (packetNumber + 1);

        }

        delete sock;

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

void
parseHash(packet hashPacket, char *currFilename, unsigned char *parsedHash) {

    int hashLength = 20;
    char *hashContent = packetContent(hashPacket);
    memcpy(parsedHash, hashContent, hashLength);
    (void) currFilename;
}
