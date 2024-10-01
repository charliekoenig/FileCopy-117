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
        cerr << "Correct syntax is " << argv[0] << " <networknastiness> <filenastiness> <targetdir>" << endl;
        exit(1);
    }


    ssize_t readLen;
    char incomingMessage[512];
    int nastiness = atoi(argv[NETWORK_NAST_ARG]);

    int i = 0;
    try {
        C150DgmSocket *sock = new C150NastyDgmSocket(nastiness);

        while(1) {
            readLen = sock -> read(incomingMessage, sizeof(incomingMessage));
            if (readLen == 0) {
                cerr << "Read zero message on " << i << " iteration. Trying again" << endl;
                continue;
            }

            incomingMessage[readLen] = '\0';  // make sure null terminated
            string incoming(incomingMessage); // Convert to C++ string ...it's slightly
                                              // easier to work with, and cleanString
                                              // expects it

            // one function to read file (PBR const unsigned char *), return length buffer (DONE)
            unsigned char *fileContent;
            ssize_t bytesRead = readFile(argv[TARGET_DIR], incoming, nastiness, &fileContent);
            // TODO: Check for -1?

            // one function to create hash, calls SHA1 on content (char *), strenlen(above), output buff
                    // Create output buffer in main
            unsigned char obuff[20];
            SHA1((const unsigned  char *)fileContent, bytesRead, obuff);
            for (int j = 0; j < 20; j++) {
                printf("%02x", (unsigned int) obuff[j]);
            }
            cout << endl;
            free(fileContent);
            
            // send hash to client as entire response
            string finalResponse((char *)obuff, 20);
            sock -> write((const char *)obuff, finalResponse.length()+1);
            
            // stay in same iteration of loop waiting for SUCCESS/FAILURE from client
                // sock -> read() (retransmisison or not)
                // print filename + status
                // send acknoledgement to client

            i++;
        }
            
    } catch (C150NetworkException &e) {
        // In case we're logging to a file, write to the console too
        cerr << argv[0] << ": caught C150NetworkException: " << e.formattedExplanation() << endl;
    }

    return 0;
}
