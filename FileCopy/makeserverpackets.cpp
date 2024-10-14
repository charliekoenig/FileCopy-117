#include "makeserverpackets.h"
#include <openssl/sha.h>
#include <cstring>

// testing
#include <iostream>
using namespace std;

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
makeHashPacket(packet incomingPacket, unsigned char obuff [20]) {
    int fNameLen = packetLength(incomingPacket) - 5;
    int contentLength = 20 + fNameLen;
    int packetNumber = packetNum(incomingPacket);

    char outgoingContent[contentLength];
    char *incomingContent = packetContent(incomingPacket);

    // populate packet content with hash followed by filename
    for (int j = 0; j < contentLength; j++) {
        outgoingContent[j] = j < 20 ? obuff[j] : incomingContent[j - 20];
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
 * Function: makeResCPacket

 * Parameters: 
    * packet incomingPacket -> Message from client encoded 
                               as a packet struct

 * Return: A response packet acknowledging the request packet

 * Notes: 
    * Allocates memory for return packet that must be freed 
      by caller
***********************************************************/
packet 
makeResCPacket(packet incomingPacket) {
    int packetNumber = packetNum(incomingPacket);

    int filenameLength = packetLength(incomingPacket) - sizeof(ssize_t) - 5;

    char *filename = NULL;
    parseCPacket(incomingPacket, &filename);

    char cAndFilename[filenameLength + 1];
    cAndFilename[0] = 'C';
    for (int i = 1; i < filenameLength + 1; i++) {
        cAndFilename[i] = filename[i - 1];
    }

    free(filename);

    return makePacket('R', filenameLength + 1, packetNumber, cAndFilename);
}

/**********************************************************
 * Function: makeResBPacket

 * Parameters: 
    * packet incomingPacket -> Message from client encoded 
                               as a packet struct

 * Return: A response packet acknowledging the byte packet

 * Notes: 
    * Allocates memory for return packet that must be freed 
      by caller
***********************************************************/
packet 
makeResBPacket(packet incomingPacket) {
    int packetNumber = packetNum(incomingPacket);

    char content[1] = {'B'};

    return makePacket('R', 1, packetNumber, content);
}

ssize_t
parseCPacket(packet packet, char **fileName) {
    ssize_t unpackedLen = 0;

    // includes null character
    int contentLength = packetLength(packet) - 4;

    *fileName = (char *)malloc(contentLength - sizeof(ssize_t));
    for (int i = 0; i < contentLength; i++) {
        if ((unsigned) i < sizeof(ssize_t)) {
            unpackedLen = (unpackedLen << 8) + (unsigned char)(packetContent(packet)[i]);
        } else {
            (*fileName)[(unsigned) i - sizeof(ssize_t)] = packetContent(packet)[i];
        }
    }

    (*fileName)[contentLength - sizeof(ssize_t) - 1] = '\0';

    return unpackedLen;
}


int
parseByteOffset(packet packet) {
    char *data = packetContent(packet);
    int offset = 0;
    for (int i = 0; i < 3; i++) {
        offset = (offset << 8) + (unsigned char)(data[i]);
    }

    return offset;
}

int 
parseBytesRead(packet packet) {
    char *data = packetContent(packet);

    int contentLen = packetLength(packet) - 5;
    int filenameLen = (unsigned char)data[3];
    int bytesForFilenameLen = 1;
    int bytesForOffset = 3;
    
    return (contentLen - (filenameLen + bytesForFilenameLen + bytesForOffset));
}

char *
parseBytesContent(packet packet, int bytesRead) {
    int contentLen = packetLength(packet) - 5;
    int contentStart = contentLen - bytesRead;

    char *data = packetContent(packet);

    return &(data[contentStart]);
}

string
parseBytesFilename(packet packet) {
    char *packetBContent = packetContent(packet);
    int filenameLen = packetBContent[3];
    char *filename = (char *)malloc(filenameLen + 1);

    for (int i = 0; i < filenameLen; i++) {
        filename[i] = packetBContent[i + 4];
    }
    filename[filenameLen] = '\0';

    string filenameStr(filename);
    free(filename);
    return filenameStr;
}