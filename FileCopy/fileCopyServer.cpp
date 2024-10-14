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

using namespace C150NETWORK;
using namespace std;

const int NETWORK_NAST_ARG = 1;
const int FILE_NAST_ARG    = 2;
const int TARGET_DIR       = 3;

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
    char *targetDir = argv[TARGET_DIR];

    unordered_map<string, unsigned char *> fileData;
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
                    string fName(packetContent(packetIn));
                    *GRADING << "File: " << fName << " received, beginning end-to-end check" << endl;
                    
                    // read file from client, set fContent to file data
                    unsigned char *fContent;
                    ssize_t fContentLen = readFile(argv[TARGET_DIR], fName, fileNastiness, &fContent);

                    if (fContentLen == -1) { 
                        // TODO: Handle -> Resend request packet? -> error cases in readFile
                        cerr << "Error while reading " << fName << endl; 
                    } else {
                        packetOut = makeHashPacket(packetIn, fContent, fContentLen);
                        free(fContent);
                    }
                    
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
                        ssize_t totalBytes = parseCPacket(packetIn, &filenameRead);
                        // create TMP file
                        /*
                            actually, it might be easier to write to the TMP file each time
                            i think we were concerned abt the time aspect of it though...
                        */
                        ofstream fileTMP;
                        char filenameTMP[strlen(filenameRead) + strlen(targetDir) + 6];
                        snprintf(filenameTMP, sizeof(filenameTMP), "%s/%s.TMP", targetDir, filenameRead);
                        fileTMP.open(filenameTMP);
                        fileTMP << "testing for " << filenameRead << endl;
                        fileTMP.close();

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
