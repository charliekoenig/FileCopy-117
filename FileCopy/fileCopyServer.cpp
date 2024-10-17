#include "c150nastydgmsocket.h"
#include "c150debug.h"
#include "c150grading.h"
#include "nastyfileops.h"
#include "packetstruct.h"
#include "makeserverpackets.h"
#include <cstdlib>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <unordered_set>
#include <openssl/sha.h>
#include <stdio.h>
#include <cstdio>

using namespace C150NETWORK;
using namespace std;

const int NETWORK_NAST_ARG = 1;
const int FILE_NAST_ARG    = 2;
const int TARGET_DIR       = 3;

string makeTMPFileName(string dir, string name);

int
main(int argc, char *argv[]) {
    GRADEME(argc, argv);

    if (argc != 4) {
        cerr << "Correct syntax is " << argv[0] 
             << " <networknastiness> <filenastiness> <targetdir>" << endl;
        exit(1);
    }

    ssize_t readLen;
    char incoming[512];
    int serverNastiness = atoi(argv[NETWORK_NAST_ARG]);
    int fileNastiness   = atoi(argv[FILE_NAST_ARG]);

    unordered_map<string, unsigned char *> fileData;
    unordered_map<string, ssize_t> fileLengths;
    unordered_map<string, unordered_set<int>> fileBytesFilled;
    unordered_set<string> fileCopyData;

    int i = 0;
    try {
        C150DgmSocket *sock = new C150NastyDgmSocket(serverNastiness);
        while(1) {
            readLen = sock -> read(incoming, sizeof(incoming));
            if (readLen == 0) {
                cerr << "Empty message on " << i << " iteration. Trying again" << endl;
                i++;
                continue;
            }    

            // convert incoming file check request packet to a string
            packet packetIn = stringToPacket((unsigned char *)incoming);
            packet packetOut = NULL;

            // handles one read which is capable of handling any type of packet 
            switch (packetOpcode(packetIn)) {
                case 'F':
                {
                    string fname(packetContent(packetIn));
                    *GRADING << "File: " << fname << " received, beginning end-to-end check" << endl;
                    
                    NASTYFILE outputFile(fileNastiness);
                    string targetName = makeFileName(argv[TARGET_DIR], (fname + ".TMP"));


                    if (fileData[fname] == NULL) {
                        packetOut = makePacket('U', 0, packetNum(packetIn), NULL);
                    } else {
                        
                        // redo the entire write if hashes are different
                        int currAttempt = 1;
                        bool successfulWrite = true;
                        int fileSize = fileLengths[fname];
                        int offset = 0;
                        unsigned char *content  = fileData[fname];
                        unsigned char *partialContent = NULL;

                        do {
                            int bytesToWrite = min(512, fileSize - offset);
                            partialContent = content + offset;

                            successfulWrite = safeFWrite(bytesToWrite, outputFile, 1, 
                                                        partialContent, offset, targetName);

                            offset += bytesToWrite;
                            
                        } while (successfulWrite && (offset < fileSize));
                        
                        *GRADING << "File: " << fname << " written to TMP" << currAttempt << endl;
                        *GRADING << "File: " << fname << " sending sha1 to client" << endl;

                        unsigned char memHash[20];
                        SHA1((const unsigned char *)content, fileLengths[fname], memHash);

                        packetOut = makeHashPacket(packetIn, memHash);
                    }

                    break;
                }
                case 'S':
                {
                    string fname(packetContent(packetIn) + 1);
                    if (packetContent(packetIn)[0] == 'S') {
                        *GRADING << "File: " << fname << " end-to-end check succeeded" << endl;
                        cout<< "File: " << fname << " end-to-end check succeeded" << endl;

                        // returns an int, do we want to do while?
                        rename((const char *) makeFileName(argv[TARGET_DIR], (fname + ".TMP")).c_str(), 
                               (const char *)makeFileName(argv[TARGET_DIR], fname).c_str());
                    } else if (packetContent(packetIn)[0] == 'F') {
                        *GRADING << "File: " << fname << " end-to-end check failed" << endl;
                        cout << "File: " << fname << " end-to-end check failed" << endl;

                        // 0 means success
                        remove((const char *) makeFileName(argv[TARGET_DIR], (fname + ".TMP")).c_str());
                    }

                    if (fileData[fname] == NULL) {
                        packetOut = makePacket('U', 0, packetNum(packetIn), NULL);
                    } else {
                        free(fileData[fname]);
                        fileData[fname] = NULL;
                        fileBytesFilled[fname].clear();

                        packetOut = makeAckPacket(packetIn);
                    }

                    break;
                }
                case 'C':
                    {
                        // malloc the bytes based on the parseCpacket
                        char *filenameRead = NULL;
                        ssize_t totalBytes = parseCPacket(packetIn, &filenameRead);
                        string filenameReadString(filenameRead);

                        *GRADING << "File: " << filenameRead << " starting to receive file" << endl;
                        
                        if (fileData[filenameReadString] == NULL) {
                            fileData[filenameReadString] = (unsigned char *)malloc(totalBytes);
                            fileLengths[filenameReadString] = totalBytes;
                            packetOut = makeResCPacket(packetIn);
                        } else {
                            packetOut = makePacket('U', 0, packetNum(packetIn), NULL);
                        }
                        free(filenameRead);
                        break;
                    }
                case 'B':
                    {
                        int offset = parseByteOffset(packetIn);
                        int bytesRead = parseBytesRead(packetIn);
                        char *fileContent = parseBytesContent(packetIn, bytesRead);
                        string filename = parseBytesFilename(packetIn);

                        // if file is active
                        if (fileData[filename] != NULL) {
                            // if bytes at offset have not been copied
                            if (fileBytesFilled[filename].count(offset) == 0) {
                                fileBytesFilled[filename].insert(offset);
                                memcpy(fileData[filename] + offset, fileContent, bytesRead);
                            }

                            packetOut = makeResBPacket(packetIn);
                        } else {
                            packetOut = makePacket('U', 0, packetNum(packetIn), NULL);
                        }

                        break;
                    }
                default:
                    printPacket(packetIn);
                    packetOut = makePacket('U', 0, packetNum(packetIn), NULL);
            }

            const char *outgoingContent = packetToString(packetOut);

            sock -> write(outgoingContent, packetLength(packetOut));
            freePacket(packetOut);
            freePacket(packetIn);
            free((void *)outgoingContent);
            i++;
        }
            
    } catch (C150NetworkException &e) {
        cerr << argv[0] << ": caught C150NetworkException: " << e.formattedExplanation() << endl;
    }

    return 0;
}
