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
    unordered_set<string> fileCopyData;
    int i = 0;
    try {
        C150DgmSocket *sock = new C150NastyDgmSocket(serverNastiness);
        ssize_t totalBytes = 0;
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
                    string fName(packetContent(packetIn));
                    *GRADING << "File: " << fName << " received, beginning end-to-end check" << endl;
                    
                    // read file from client, set fContent to file data
                    // unsigned char *fContent;
                    // ssize_t fContentLen = readFile(argv[TARGET_DIR], fName, fileNastiness, &fContent);

                    NASTYFILE outputFile(fileNastiness);
                    string targetName = makeTMPFileName(argv[TARGET_DIR], fName);

                    unsigned char hashTMP[20];
                    unsigned char obuff[20];
                    SHA1((const unsigned char *)fileData[fName], totalBytes, obuff);
                    
                    // redo the entire write if hashes are different
                    do {
                        // write to TMP file since file copy is done
                        while (outputFile.fopen(targetName.c_str(), "wb") == NULL) {
                            cerr << "Error opening input file " << targetName << 
                                " errno=" << strerror(errno) << endl;
                        }

                        while ((int) outputFile.fwrite(fileData[fName], 1, totalBytes) != totalBytes) {
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

                        char *readFromTMP = (char *)malloc(totalBytes);

                        while ((int) outputFile.fread(readFromTMP, 1, totalBytes) != totalBytes) {
                            cerr << "Error reading file " << targetName << 
                                "  errno=" << strerror(errno) << endl;
                        }
                        
                        while (outputFile.fclose() != 0) {
                            cerr << "Error closing output file " << targetName << 
                                " errno=" << strerror(errno) << endl;
                        }

                        // compare hashes of filecontent and readFromTMP
                        SHA1((const unsigned char *)readFromTMP, totalBytes, hashTMP);
                    } while (!(strncmp((const char *) hashTMP, (const char *) obuff, 20) == 0));


                    // todo: server keeps track of all hashes it has computed thus far
                    //       to deal with a possible drop of a H packet that would make the C resend F
                    packetOut = makeHashPacket(packetIn, hashTMP);
                    break;
                }
                case 'S':
                {
                    string fName(packetContent(packetIn) + 1);
                    if (packetContent(packetIn)[0] == 'S') {
                        *GRADING << "File: " << fName << " end-to-end check succeeded" << endl;
                    } else if (packetContent(packetIn)[0] == 'F') {
                        *GRADING << "File: " << fName << " end-to-end check failed" << endl;
                    }

                    packetOut = makeAckPacket(packetIn);
                    break;
                }
                case 'C':
                    {
                        packetOut = makeResCPacket(packetIn);
                        // malloc the bytes based on the parseCpacket
                        char *filenameRead = NULL;
                        totalBytes = parseCPacket(packetIn, &filenameRead);
                        

                        // filenameread wont be freed til this one is done
                        
                        
                        if (fileData[filenameRead] == NULL) {
                            fileData[filenameRead] = (unsigned char *)malloc(totalBytes);
                            printf("filename is %s\n", filenameRead);
                        }
                        

                        break;
                    }
                case 'B':
                    {
                        int offset = parseByteOffset(packetIn);
                        int bytesRead = parseBytesRead(packetIn);
                        char *fileContent = parseBytesContent(packetIn, bytesRead);
                        string filename = parseBytesFilename(packetIn);

                        if (fileData[filename] != NULL) {
                            if (fileCopyData.count(filename + to_string(offset)) == 0) {
                                fileCopyData.insert(filename + to_string(offset));
                                for (int byte = 0; byte < bytesRead; byte++) {
                                    (fileData[filename])[offset + byte] = fileContent[byte];
                                }
                            }
                        }

                        // cout << "First char in packet " << packetNum(packetIn) << ": " << *fileContent << endl;
                        (void) fileContent;
                        (void) offset;

                        // temp
                        packetOut = makeResBPacket(packetIn);

                        // // after filename is parsed from the B packet
                        // if (filename != filenameRead) break;
                        // // otherwise just add to fileContent + offset based on parsing of the B packet
                        // fileContent + offset = fileContentParsed
                        // packetOut = makeResBPacket(packetIn);
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