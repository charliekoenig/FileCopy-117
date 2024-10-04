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
    int nastiness = atoi(argv[NETWORK_NAST_ARG]);

    int i = 0;
    try {
        C150DgmSocket *sock = new C150NastyDgmSocket(nastiness);

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

            cout << packetToString(packetIn) << endl;
            cout << packetOpcode(packetIn) << endl;
            cout << packetLength(packetIn) << endl;
            cout << packetContent(packetIn) << endl;


            // Should be engineered in a way that allows us to continue loop, no guarantee the next packet is the confirmation
                // Each loop should handle one read which is capable of handling any type of packet 
            switch (packetOpcode(packetIn)) {
                case 'F':
                {
                    string fName(packetContent(packetIn));
                    
                    // read file from client, set fContent to file data
                    unsigned char *fContent;
                    ssize_t fNameLen = readFile(argv[TARGET_DIR], fName, nastiness, &fContent);

                    if (fNameLen == -1) { 
                        // TODO: Handle negative 1 case? Resend request packet?
                        cerr << "Error while reading " << fName; 
                        packetOut = makeHashPacket(packetIn, fContent);
                    } else {
                        packetOut = makeHashPacket(packetIn, fContent);
                    }
                    break;
                }
                case 'S':
                {
                    // TODO: print to debug (success or failure)
                    packetOut = makeAckPacket(packetIn);
                    break;
                }
                default:
                    packetOut = makePacket('U', 0, NULL);
            }

            cout << packetToString(packetOut) << endl;
            cout << packetOpcode(packetOut) << endl;
            cout << packetLength(packetOut) << endl;
            cout << packetContent(packetOut) << endl;
            printf("--------------------\n");

            sock -> write((const char *)packetToString(packetOut), packetLength(packetOut) + 2);
            freePacket(packetOut);
            freePacket(packetIn);
            i++;
        }
            
    } catch (C150NetworkException &e) {
        // In case we're logging to a file, write to the console too
        cerr << argv[0] << ": caught C150NetworkException: " << e.formattedExplanation() << endl;
    }

    return 0;
}
