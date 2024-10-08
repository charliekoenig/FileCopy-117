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

using namespace C150NETWORK;

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
            packet packetIn = stringToPacket(incoming);
            packet packetOut = NULL;

            char *fileContent = NULL;

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
                        printf("%s and %ld\n", filenameRead, totalBytes);
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
                        free(filenameRead);
                        // free filecontent before the next request (next file)
                        fileContent = (char *)malloc(totalBytes);
                        free(fileContent);
                        break;
                    }
                // case 'B':
                //     {
                //         // after filename is parsed from the B packet
                //         if (filename != filenameRead) break;
                //         // otherwise just add to fileContent + offset based on parsing of the B packet
                //         fileContent + offset = fileContentParsed
                //         packetOut = makeResBPacket(packetIn);
                //         break;
                //     }
                default:
                    printPacket(packetIn);
                    packetOut = makePacket('U', 0, packetNum(packetIn), NULL);
            }

            sock -> write((const char *)packetToString(packetOut), packetLength(packetOut) + 2);
            freePacket(packetOut);
            freePacket(packetIn);
            i++;
        }
            
    } catch (C150NetworkException &e) {
        cerr << argv[0] << ": caught C150NetworkException: " << e.formattedExplanation() << endl;
    }

    return 0;
}
