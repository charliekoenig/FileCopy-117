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
bool parseHash(packet hashPacket, char *currFilename, unsigned char *parsedHash);

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
                }
                
                attempts++; 
            }

            // incremement packet number on success
            packetNumber = (packetNumber == MAX_PACKET_NUM) ? 0 : (packetNumber + 1);
            
            unsigned int offset = 0;
            int bytesToSend = MAX_READ - strlen(filename);
            packet bytePacket = NULL;

            // todo it might be that the packet number went back to 0 so it might be messing with the results
            // I switched the unanswered to an unordered map so this should fix this issue 
            int initialPacketNum = packetNumber;
            int numPackets = ceil((float) bytesRead / (float) bytesToSend);

            unordered_map<int, packet> unAcked;
            stack<int> expectedAcks;

            while ((offset < bytesRead) || !expectedAcks.empty()) {
                if (offset < bytesRead) {

                    // Send less than total bytes if last packet in file
                    // if (bytesRead < (bytesToSend + offset)) {
                    //     bytesToSend = bytesRead - offset;
                    // } 

                    // switched to min function
                    bytesToSend = min(bytesToSend, (int)(bytesRead - offset));

                    bytePacket = makeBytePacket(offset, filename, fileContent, 
                                                packetNumber, bytesToSend, 
                                                strlen(filename));

                    expectedAcks.push(packetNumber);
                    unAcked[packetNumber - initialPacketNum] = bytePacket;
                    // cout << "Sending " << bytesToSend << " bytes at offset " << offset << " for file " << filename << endl;
                    offset += bytesToSend;
                    packetNumber = (packetNumber == MAX_PACKET_NUM) ? 0 : (packetNumber + 1);
                } else {
                    int next = 0;
                    
                    while ((!expectedAcks.empty()) && (bytePacket == NULL)) {
                        next = expectedAcks.top();
                        if ((unAcked[next - initialPacketNum] == NULL)) {
                            expectedAcks.pop();
                        } else {
                            bytePacket = unAcked[next - initialPacketNum];
                        }
                    }

                }

                if (bytePacket != NULL) {
                    char *bytePacketString = packetToString(bytePacket);
                    int packetLen = packetLength(bytePacket);
                    sock -> write(bytePacketString, packetLen);

                    sock -> read(incomingMessage, sizeof(incomingMessage));
                    if (!(sock -> timedout())) {
                        response = stringToPacket((unsigned char *)incomingMessage);

                        if ((packetOpcode(response) == 'R') && (packetContent(response)[0] == 'B')) {
                            int packetIndex = packetNum(response) - initialPacketNum;
                            if (((packetIndex) < numPackets) && ((packetIndex) >= 0)) {
                                if (unAcked[packetIndex] != NULL) {
                                    freePacket(unAcked[packetIndex]);
                                    unAcked[packetIndex] = NULL;
                                }
                            }
                        }
                    }
                }

                bytePacket = NULL;
            }

            packetNumber = (packetNumber == MAX_PACKET_NUM) ? 0 : (packetNumber + 1);

            // send file check packet to server
            packet fileCheckPacket = makeFileCheckPacket(filename, packetNumber);
            char *fileCheckPacketString = packetToString(fileCheckPacket);
            packetLen = packetLength(fileCheckPacket);

            noResponse = unexpectedPacket = true;
            while (noResponse || unexpectedPacket) {
                sock -> write(fileCheckPacketString, packetLen);
                sock -> read(incomingMessage, sizeof(incomingMessage));
                noResponse = sock -> timedout();

                if (!noResponse) {
                    response = stringToPacket((unsigned char *)incomingMessage);
                    unexpectedPacket = (packetOpcode(response) != 'H' ||
                                        packetNum(response) != packetNumber);
                }
            }

            unsigned char localHash[20];
            SHA1((const unsigned  char *)fileContent, bytesRead, localHash);
            // free(fileContent);

            // printf("Client Hash: ");
            // for (int k = 0; k < 20; k++) {
            //     printf("%02x", localHash[k]);
            // }
            // cout << endl;

            unsigned char parsedHash[20];
            parseHash(response, filename, parsedHash);

            // printf("Server Hash: ");
            // for (int k = 0; k < 20; k++) {
            //     printf("%02x", parsedHash[k]);
            // }
            // cout << endl;
            
            char statusContent[strlen(filename) + 1];
            statusContent[0] = (strncmp((const char *) parsedHash, (const char *) localHash, 20) == 0) ? 'S' : 'F';
            
            for (size_t j = 1; j < strlen(filename) + 1; j++) {
                statusContent[j] = filename[j-1];
            }

            packet statusPacket = makeStatusPacket(parsedHash, localHash, filename, packetNumber);
            // packet statusPacket = makePacket('S', strlen(filename) + 1, packetNumber, statusContent);

            // printPacket(statusPacket2);
            // printPacket(statusPacket);
            char *statusPacketString = packetToString(statusPacket);
            packetLen = packetLength(statusPacket);
            freePacket(statusPacket);

            attempts = 0;
            noResponse = true, unexpectedPacket = true;

            while (noResponse || unexpectedPacket) {
                if (statusContent[0] == 'S') {
                    cout << "SUCCESS: " << filename << endl;
                    // *GRADING << "File: " << filename << " end-to-end check succeeded, attempt " << attempts << endl;
                } else if (statusContent[0] == 'F') {
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
            // cout << "_______________________________" << endl;
            // packetNumber = (packetNumber == MAX_PACKET_NUM) ? 0 : (packetNumber + 1);

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
/*
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
*/

// create DS (hashtable-like) filename -> *fileContent
// every time we receive a B packet, we parse and fill in the specific part in fileContent

// resB, integrating the endtoend to filecopy

// dealing with file nastiness (entirety of monday meet)

// optimization flags to the compiler