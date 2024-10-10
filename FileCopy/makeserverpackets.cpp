#include "makeserverpackets.h"
#include <openssl/sha.h>
#include <cstring>

/**********************************************************
 * Function: makeHashPacket

 * Parameters: 
    * packet incomingPacket -> Message from client encoded 
                               as a packet struct
    * unsigned char *fileContent -> a point to a character 
                                    array containing an
                                    entire file

 * Return: A hash packet

 * Notes: 
    * Allocates memory for return packet that must be freed 
      by caller
    * Uses SHA1 function to hash the *fileContent
***********************************************************/
packet 
makeHashPacket(packet incomingPacket, unsigned char *fileContent, size_t fContentLen) {
    int fNameLen = packetLength(incomingPacket) - 1;
    int contentLength = 20 + fNameLen;
    int packetNumber = packetNum(incomingPacket);

    char outgoingContent[contentLength];
    char *incomingContent = packetContent(incomingPacket);

    // Output buffer populated with SHA1 hash
    unsigned char obuff[20];
    SHA1((const unsigned char *)fileContent, fContentLen, obuff);

    // populate packet content with hash followed by filename
    for (int j = 0; j < contentLength; j++) {
        outgoingContent[j] = j < 20 ? obuff[j] : incomingContent[j-20];
    }

    // return a hash packet
    return makePacket('H', contentLength, packetNumber, outgoingContent);
}

/**********************************************************
 * Function: makeAckPacket

 * Parameters: 
    * packet incomingPacket -> Message from client encoded 
                               as a packet struct

 * Return: An acknowledgement packet

 * Notes: 
    * Allocates memory for return packet that must be freed 
      by caller
***********************************************************/
packet 
makeAckPacket(packet incomingPacket) {
    char *fileName = packetContent(incomingPacket) + 1;
    int packetNumber = packetNum(incomingPacket);

    return makePacket('A', strlen(fileName), packetNumber, fileName);
}

/**********************************************************
 * Function: makeResPacket

 * Parameters: 
    * packet incomingPacket -> Message from client encoded 
                               as a packet struct

 * Return: A response packet acknowledging the request packet

 * Notes: 
    * Allocates memory for return packet that must be freed 
      by caller
***********************************************************/


// todo: (idil) i feel like the length is off by one:
/*
PACKET CONTENTS
R
13
1
Cdata10
 */
packet 
makeResCPacket(packet incomingPacket) {
    int packetNumber = packetNum(incomingPacket);

    // get total bytes out
    // get the opcode, length, num out
    // (will be B for B acks, we can modularize this later on)
    int filenameLength = packetLength(incomingPacket) - sizeof(ssize_t) - 4;

    char *filename = NULL;
    parseCPacket(incomingPacket, &filename);

    char cAndFilename[filenameLength + 1];
    cAndFilename[0] = 'C';
    for (int i = 1; i < filenameLength + 1; i++) {
        cAndFilename[i] = filename[i - 1];
    }

    return makePacket('R', filenameLength + 1, packetNumber, cAndFilename);
}