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
#include <queue>

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

        queue<char *> noahsFiles;
        struct dirent *sourceFile;

        unordered_map<char *, int> fileCopyAttempts;

        while ((sourceFile = readdir(SRC)) != NULL) {
            noahsFiles.push(sourceFile -> d_name);
            fileCopyAttempts[sourceFile -> d_name] = 1;
        }

        // add files to queue
        // loop for each file in the given directory
        while (!(noahsFiles.empty())) {

            filename = noahsFiles.front();
            noahsFiles.pop();

            // skip the . and .. names
            // filename = sourceFile -> d_name;
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
            string srcFilename = makeFileName(argv[SRC_DIR], filename);

            if (!isFile(srcFilename)) {
                cerr << srcFilename << " is a not a valid file. Skipping" << endl;
                continue;
            }

            if (lstat(srcFilename.c_str(), &statbuf) != 0) {
                fprintf(stderr, "copyFile: Error stating supplied source file %s\n", srcFilename.c_str());
                continue;
            }

            sourceSize = statbuf.st_size;
            fileContent = (unsigned char *)malloc(sourceSize);
            NASTYFILE inputFile(atoi(argv[FILE_NAST_ARG]));

            void *filePtr    = NULL;
            size_t bytesRead = 0;
            int closeResult  = 0;
            

            float freq = 0;
            float tries = 0;
            unordered_map<string, float> hashCounts;
            unsigned char hash[21];
            do {

                // open file, try until success
                do {
                    filePtr = inputFile.fopen(srcFilename.c_str(), "rb");
                } while (filePtr == NULL);

                // read file, try until success
                do {
                    bytesRead = inputFile.fread(fileContent, 1, sourceSize);
                } while (bytesRead != sourceSize);

                // close file, try until success
                do {
                    closeResult = inputFile.fclose();
                } while (closeResult != 0 );

                // Create SHA1 hash for file from SRC Directory
                SHA1((const unsigned  char *)fileContent, bytesRead, hash);
                hash[20] = '\0';

                string hashString((char *)hash);
                tries += 1;

                int hits = hashCounts[hashString] += 1;
                freq = hits/tries;

            } while ((freq < 0.75 || tries < 50) && tries < 200);

            // if no successful read, try file later
            // if (tries == 200) {
            //     cerr << "Failed on: " << filename << endl;
            //     // noahsFiles.push(filename);
            //     *GRADING << "File: " << filename << " was not read correctly, attempt " << fileCopyAttempts[filename] << endl;
            //     fileCopyAttempts[filename] += 1;
            //     free(fileContent);
            //     continue;
            // }

            // Create packet alerting server of file to be copied
            packet fileInfoPacket = makeCopyPacket((int) sourceSize, filename, packetNumber);
            char *fileInfoPacketString = packetToString(fileInfoPacket);
            int packetLen = packetLength(fileInfoPacket);
            freePacket(fileInfoPacket);

            // Loop until server acknowledges C packet
            int attempts = 1;
            while (noResponse || unexpectedPacket) {
                sock -> write(fileInfoPacketString, packetLen);

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
                *GRADING << "File: " << filename << " attempt " << attempts << " to send file information to server in filecopy attempt " << fileCopyAttempts[filename] << endl;
                
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

            *GRADING << "File: " << filename << ", beginning transmission, attempt " << fileCopyAttempts[filename] << endl;

            do {
                filePtr = inputFile.fopen(srcFilename.c_str(), "rb");
            } while (filePtr == NULL);

            sock -> turnOnTimeouts(5);
            while ((offset < bytesRead) || !expectedAcks.empty()) {

                // there are still unsent bytes from the file
                if (offset < bytesRead) {
                    bytesToSend = min(bytesToSend, (int)(bytesRead - offset));

                    unsigned char *partialFileContent = (unsigned char *)malloc(bytesToSend);
                    if (safeFRead(bytesToSend, inputFile, 1, &partialFileContent, offset) == 1) {
                        cerr << "freq never reached 75%\n";
                    }
                    // ---------------------------------------------------------------------------------
                    // unsigned char partialFileContent[bytesToSend];
                    // size_t bytesReadPartial = 0;

                    // float freqPartial = 0;
                    // float triesPartial = 0;

                    // unordered_map<string, float> contentStrings;

                    // do {
                    //     do {
                    //         bytesReadPartial = inputFile.fread(partialFileContent, 1, bytesToSend);
                    //         cerr << "bytesReadPartial: " << bytesReadPartial << " bytesToSend: " << bytesToSend << endl;
                    //     } while ((int) bytesReadPartial != bytesToSend);
                        
                    //     string contentStr((const char *) partialFileContent, bytesReadPartial);
                    //     triesPartial += 1;

                    //     int hits = contentStrings[contentStr] += 1;
                    //     freqPartial = hits/triesPartial;
                    // } while ((freqPartial < 0.75 || triesPartial < 50) && triesPartial < 200);

                    // cerr << "got to 212\n";
                    // ---------------------------------------------------------------------------------

                    if (strcmp("warandpeace.txt", filename) == 0) {
                        for (int i = 0; i < bytesToSend; i++) {
                            cout << partialFileContent[i];
                        }
                    }

                    bytePacket = makeBytePacket(offset, filename, fileContent, 
                                                packetNumber, bytesToSend, 
                                                filenameLength);

                    // read bytes to send from <filename>
                    expectedAcks.push(packetNumber);
                    unAcked[packetNumber] = bytePacket;
                    // cout << "Sending " << bytesToSend << " bytes at offset " << offset << " for file " << filename << endl;
                    offset += bytesToSend;
                    packetNumber = (packetNumber == MAX_PACKET_NUM) ? 0 : (packetNumber + 1);

                // all bytes sent but not all packets acknowledged
                } else {
                    
                    while ((!expectedAcks.empty()) && (bytePacket == NULL)) {
                        int next = expectedAcks.top();
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

                    free(bytePacketString);
                }

                bytePacket = NULL;
            }

            // close file, try until success
            do {
                closeResult = inputFile.fclose();
            } while (closeResult != 0 );

            *GRADING << "File: " << filename << " transmission complete, waiting for end-to-end check, attempt " << fileCopyAttempts[filename] << endl;

            packetNumber = (packetNumber == MAX_PACKET_NUM) ? 0 : (packetNumber + 1);
            sock -> turnOnTimeouts(500);

            /****  Send packet to server requesting end-to-end check ****/
            packet fileCheckPacket = makeFileCheckPacket(filename, packetNumber);
            char *fileCheckPacketString = packetToString(fileCheckPacket);
            packetLen = packetLength(fileCheckPacket);

            freePacket(fileCheckPacket);

            noResponse = unexpectedPacket = true;
            attempts = 1;
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
                
                *GRADING << "File: " << filename << " attempt " << attempts << " to receive hash, in filecopy attempt " << fileCopyAttempts[filename] << endl;
                attempts++;
            }

            free(fileCheckPacketString);
            packetNumber = (packetNumber == MAX_PACKET_NUM) ? 0 : (packetNumber + 1);

            /** Send packet to server alerting of successful transfer or fail **/

            // // open file, try until success
            // do {
            //     filePtr = inputFile.fopen(srcFilename.c_str(), "rb");
            // } while (filePtr == NULL);

            // // read file, try until success
            // do {
            //     bytesRead = inputFile.fread(fileContent, 1, sourceSize);
            // } while (bytesRead != sourceSize);

            // // close file, try until success
            // do {
            //     closeResult = inputFile.fclose();
            // } while (closeResult != 0 );

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

            attempts = 1;
            noResponse = true, unexpectedPacket = true;

            while (noResponse || unexpectedPacket) {
                if (response != NULL) {
                    freePacket(response);
                    response = NULL;
                }

                if (statusPacketString[4] == 'S') {
                    cerr << "SUCCESS: " << filename << endl;
                    *GRADING << "File: " << filename << " end-to-end check succeeded, attempt " << fileCopyAttempts[filename] << endl;
                } else if (statusPacketString[4] == 'F') {
                    cerr << "FAILURE: " << filename << endl;
                    *GRADING << "File: " << filename << " end-to-end check failed, attempt " << fileCopyAttempts[filename] << endl;
                    fileCopyAttempts[filename] += 1;
                    noahsFiles.push(filename);
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
