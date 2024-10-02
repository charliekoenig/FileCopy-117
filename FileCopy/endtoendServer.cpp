#include "c150nastydgmsocket.h"
#include "c150debug.h"
#include "c150grading.h"
#include "nastyfileops.h"
#include <cstdlib>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <openssl/sha.h>

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
                cerr << "Read zero message on " << i 
                     << " iteration. Trying again" << endl;
                continue;
            }

            // null terminate response and convert to C++ string
            incoming[readLen] = '\0';  
            string incomingString(incoming); 

            // read file given from client and set fileContent to array of file characters 
            unsigned char *fileContent;
            ssize_t bytesRead = readFile(argv[TARGET_DIR], incomingString, nastiness, &fileContent);
            // TODO: Check for -1?

            // Output buffer populated with SHA1 hash
            unsigned char obuff[20];
            SHA1((const unsigned char *)fileContent, bytesRead, obuff);
            
            // Temporary: Print hash to terminal
            for (int j = 0; j < 20; j++) {
                printf("%02x", (unsigned int) obuff[j]);
            }
            cout << endl;
            free(fileContent);
            
            // send hash to client as entire response
            string finalResponse((char *)obuff, 20);
            sock -> write((const char *)obuff, finalResponse.length()+1);
            
            // TODO: Read SUCCESS/FAILURE from client
                // sock -> read() (retransmisison or not)
                // print filename + status
                // send acknoledgement to client
            // Should be engineered in a way that allows us to continue loop, no guarantee the next packet is the confirmation
                // Each loop should handle one read which is capable of handling any type of packet 
            i++;
        }
            
    } catch (C150NetworkException &e) {
        // In case we're logging to a file, write to the console too
        cerr << argv[0] << ": caught C150NetworkException: " << e.formattedExplanation() << endl;
    }

    return 0;
}
