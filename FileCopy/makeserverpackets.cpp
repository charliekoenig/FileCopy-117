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
makeHashPacket(packet incomingPacket, unsigned char *fileContent) {
    int fNameLen = packetLength(incomingPacket) - 1;
    int contentLength = 20 + fNameLen;

    char outgoingContent[contentLength];
    char *incomingConent = packetContent(incomingPacket);

    // Output buffer populated with SHA1 hash
    unsigned char obuff[20];
    SHA1((const unsigned char *)fileContent, fNameLen, obuff);

    // populate packet content with hash followed by filename
    for (int j = 0; j < contentLength; j++) {
        outgoingContent[j] = j < 20 ? obuff[j] : incomingConent[j-20];
    }

    // return a hash packet
    return makePacket('H', contentLength, outgoingContent);
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
    return makePacket('A', strlen(fileName), fileName);
}