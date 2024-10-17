/**********************************************************
               UDPserverpackets.cpp - 10/17/2024
    Authors:
        * Charlie Koenig
        * Idil Kolabas

    This module provides an implementation for the 
    UDPserverpackets interface, useful for sending data
    over the C150NETWORK from the server to the client
    and for parsing data sent by the client to the server
    
***********************************************************/

#include "UDPserverpackets.h"
#include <openssl/sha.h>
#include <cstring>
#include <cassert>



/***************************************************************
 * Function: makeHashPacket

 * Parameters: 
   * packet incomingPacket  : Client message encoded as a packet struct
   * unsigned char obuff[20]: An array holding a 20 char SHA1 hash

 * Return: A pointer to a packetStruct

 * Notes: 
   * Allocates memory for return packet that must be freed 
     by caller
   * Uses SHA1 function to hash the *fileContent
****************************************************************/
packet 
makeHashPacket(packet incomingPacket, unsigned char obuff[20]) {
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
   * packet incomingPacket : Client message encoded as a packet struct

 * Return: A pointer to a packetStruct

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
   * packet incomingPacket : Client message encoded as a packet struct

 * Return: A pointer to a packetStruct

 * Notes: 
    * Allocates memory for return packet that must be freed 
      by caller
    * Frees memory for filename which is allocated in parseCpacket
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
   * packet incomingPacket : Client message encoded as a packet struct

 * Return: A pointer to a packetStruct

 * Notes: 
   * Allocates memory for packet that must be freed by caller
***********************************************************/
packet 
makeResBPacket(packet incomingPacket) {
    int packetNumber = packetNum(incomingPacket);
    char content[1] = {'B'};

    return makePacket('R', 1, packetNumber, content);
}


/**********************************************************
 * Function: parseCPacket

 * Parameters: 
   * packet packet   : Client message encoded as a packet struct
   * char **fileName : pointer to pointer to char array which
                       is populated by parsing packet content

 * Return: A ssize_t representing the length of the file to be
           transmitted

 * Notes: 
   * CRE for filename to be NULL
   * Allocates space for *filename that must be freed by caller
***********************************************************/
ssize_t
parseCPacket(packet packet, char **filename) {
    assert(filename != NULL);

    ssize_t unpackedLen = 0;

    // includes null character
    int contentLength = packetLength(packet) - 4;

    *filename = (char *)malloc(contentLength - sizeof(ssize_t));
    for (int i = 0; i < contentLength; i++) {
        if ((unsigned) i < sizeof(ssize_t)) {
            unpackedLen = (unpackedLen << 8) + (unsigned char)(packetContent(packet)[i]);
        } else {
            (*filename)[(unsigned) i - sizeof(ssize_t)] = packetContent(packet)[i];
        }
    }

    (*filename)[contentLength - sizeof(ssize_t) - 1] = '\0';

    return unpackedLen;
}


/**********************************************************
 * Function: parseByteOffset

 * Parameters: 
   * packet packet : A pointer to a packetStruct with opcode 'B'

 * Return: An int representing the byte offset with the file
           of transmitted data

 * Notes:
***********************************************************/
int
parseByteOffset(packet packet) {
    char *data = packetContent(packet);
    int offset = 0;
    for (int i = 0; i < 3; i++) {
        offset = (offset << 8) + (unsigned char)(data[i]);
    }

    return offset;
}


/**********************************************************
 * Function: parseBytesRead

 * Parameters: 
   * packet packet : A pointer to a packetStruct with opcode 'B'

 * Return: An int representing the number of bytes read and sent
           in the transmitted packet

 * Notes: 
***********************************************************/
int 
parseBytesRead(packet packet) {
    char *data = packetContent(packet);

    int contentLen = packetLength(packet) - 5;
    int filenameLen = (unsigned char)data[3];
    int bytesForFilenameLen = 1;
    int bytesForOffset = 3;
    
    return (contentLen - (filenameLen + bytesForFilenameLen + bytesForOffset));
}


/**********************************************************
 * Function: parseBytesContent

 * Parameters: 
   * packet packet : A pointer to a packetStruct with opcode 'B'
   * int bytesRead : Number of bytes transmitted in the given
                     packetStruct

 * Return: A pointer to the first character in the sequence
           of bytesRead characters from the file

 * Notes: 
***********************************************************/
char *
parseBytesContent(packet packet, int bytesRead) {
    int contentLen = packetLength(packet) - 5;
    int contentStart = contentLen - bytesRead;

    char *data = packetContent(packet);

    return &(data[contentStart]);
}


/**********************************************************
 * Function: parseBytesFilename

 * Parameters: 
   * packet packet : A pointer to a packetStruct with opcode 'B'

 * Return: A string that is the filename sent in the packetStruct

 * Notes: 
***********************************************************/
string
parseBytesFilename(packet packet) {
    char *packetBContent = packetContent(packet);
    int filenameLen = packetBContent[3];
    char filename[filenameLen + 1];

    for (int i = 0; i < filenameLen; i++) {
        filename[i] = packetBContent[i + 4];
    }
    filename[filenameLen] = '\0';

    string filenameStr(filename);
    return filenameStr;
}