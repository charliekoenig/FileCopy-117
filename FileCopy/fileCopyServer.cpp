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
