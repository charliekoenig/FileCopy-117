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
                    string targetName = makeTMPFileName(argv[TARGET_DIR], fname);

                    unsigned char hashTMP[20];
                    unsigned char memHash[20];

                    if (fileData[fname] == NULL) {
                        packetOut = makePacket('U', 0, packetNum(packetIn), NULL);
                    } else {
                        SHA1((const unsigned char *)fileData[fname], fileLengths[fname], memHash);
                        
                        // redo the entire write if hashes are different
                        int currAttempt = 1;
                        do {
                            // write to TMP file since file copy is done
                            while (outputFile.fopen(targetName.c_str(), "wb") == NULL) {
                                cerr << "Error opening input file " << targetName << 
                                    " errno=" << strerror(errno) << endl;
                            }

                            while ((int) outputFile.fwrite(fileData[fname], 1, fileLengths[fname]) != fileLengths[fname]) {
                                cerr << "Error writing file " << targetName << 
                                    "  errno=" << strerror(errno) << endl;
                            }
                            
                            while (outputFile.fclose() != 0) {
                                cerr << "Error closing output file " << targetName << 
                                    " errno=" << strerror(errno) << endl;
                            }

                            // read from TMP file for end to end check
                            while (outputFile.fopen(targetName.c_str(), "rb") == NULL) {
                                cerr << "Error opening input file " << targetName << 
                                    " errno=" << strerror(errno) << endl;
                            }

                            char *readFromTMP = (char *)malloc(fileLengths[fname]);

                            while ((ssize_t) outputFile.fread(readFromTMP, 1, fileLengths[fname]) != fileLengths[fname]) {
                                cerr << "Error reading file " << targetName << 
                                    "  errno=" << strerror(errno) << endl;
                            }
                            
                            while (outputFile.fclose() != 0) {
                                cerr << "Error closing output file " << targetName << 
                                    " errno=" << strerror(errno) << endl;
                            }

                            // cout << "Attempt " << currAttempt << " comparing hash\n";
                            currAttempt += 1;

                            // compare hashes of filecontent and readFromTMP
                            SHA1((const unsigned char *)readFromTMP, fileLengths[fname], hashTMP);

                            free(readFromTMP);
                            
                        } while (!(strncmp((const char *) hashTMP, (const char *) memHash, 20) == 0));

                        packetOut = makeHashPacket(packetIn, hashTMP);
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
                        rename((const char *) makeTMPFileName(argv[TARGET_DIR], fname).c_str(), (const char *)makeFileName(argv[TARGET_DIR], fname).c_str());
                    } else if (packetContent(packetIn)[0] == 'F') {
                        *GRADING << "File: " << fname << " end-to-end check failed" << endl;
                        cout << "File: " << fname << " end-to-end check failed" << endl;

                        // 0 means success
                        remove((const char *) makeTMPFileName(argv[TARGET_DIR], fname).c_str());
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

string
makeTMPFileName(string dir, string name) {
  stringstream ss;

  ss << dir;
  // make sure dir name ends in /
  if (dir.substr(dir.length()-1,1) != "/")
    ss << '/';
  ss << name;     // append file name to dir
  ss << ".TMP";
  return ss.str();  // return dir/name
  
}