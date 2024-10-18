/**********************************************************
               UDPclientpackets.cpp - 10/17/2024
    Authors:
        * Charlie Koenig
        * Idil Kolabas

    This module provides an implementation for the 
    UDPclientpackets interface, useful for sending data
    over the C150NETWORK from the client to the server
    and for parsing data sent by the server to the client
    
***********************************************************/

#include "UDPclientpackets.h"
#include <cstring>

using namespace std;


/***************************************************************
 * Function: makeCopyPacket

 * Parameters: 
   * ssize_t fileLen  : Length of the file reffered to in bytes
   * char *filename   : null terminated array hold the filename
   * int packetNumber : unique packet identifier

 * Return: A pointer to a packetStruct

 * Notes: 
   * Allocates memory for packetStruct in call to makePacket()
****************************************************************/
packet 
makeCopyPacket(ssize_t fileLen, char *filename, int packetNumber) {
    int bytes = sizeof(fileLen);
    int contentLen = strlen(filename) + bytes;
    char content[contentLen];

    for (int index = 0; index < contentLen; index++) {
        if (index < bytes) {
            content[index] = (unsigned char)(fileLen >> ((bytes - 1 - index) * 8));
        } else {
            content[index] = filename[index - bytes];
        }
    }

    return makePacket('C', contentLen, packetNumber, content);
}


/***************************************************************
 * Function: makeStatusPacket

 * Parameters: 
   * unsigned char *clientHash : the clients SHA1 hash
   * packet hashPacket         : a packetStruct pointer holding
                                 the server's SHA1 hash
   * int packetNumber          : unique packet identifier

 * Return: A pointer to a packetStruct

 * Notes: 
   * Allocates memory for packetStruct in call to makePacket()
****************************************************************/
packet 
makeStatusPacket(unsigned char *clientHash, packet hashPacket, int packetNumber) {
    int incomingContentLength = packetLength(hashPacket) - 5;
    int hashLength = 20;
    char *hashContent = packetContent(hashPacket);
    
    // response will be filename + status
    int outgoingContentLength = incomingContentLength - hashLength + 1;
    char content[outgoingContentLength];

    // copy status into content at index 0
    content[0] = (memcmp(clientHash, hashContent, 20) == 0) ? 'S' : 'F';

    // remaining array is the filename
    memcpy((content + 1), (hashContent + hashLength), (outgoingContentLength - 1));

    return makePacket('S', outgoingContentLength, packetNumber, content);
}


/***************************************************************
 * Function: makeFileCheckPacket

 * Parameters: 
   * char *filename   : name of file for checksum 
   * int packetNumber : unique packet identifier

 * Return: A pointer to a packetStruct

 * Notes: 
   * Allocates memory for packetStruct in call to makePacket()
****************************************************************/
packet 
makeFileCheckPacket(char *filename, int packetNumber) {
    return makePacket('F', strlen(filename), packetNumber, filename);
}


/***************************************************************
 * Function: makeBytePacket

 * Parameters: 
   * int offset                 : offset of bytes in original file 
   * char *filename             : name of file being transferred
   * unsigned char *fileContent : pointer to content to copy and send
   * int packetNumber           : unique packet identifier
   * int bytesRead              : # of bytes transferred in this packet
   * int filenameLength         : length of filename

 * Return: A pointer to a packetStruct

 * Notes: 
   * Allocates memory for packetStruct in call to makePacket()
****************************************************************/
packet
makeBytePacket(int offset, char *filename, unsigned char *fileContent, int packetNumber,
                                            int bytesRead, int filenameLength) {
    int filenameLengthBytes = 1;
    int offsetBytes         = 3;

    // index of specific content in output array
    int fileContentIndex = (offsetBytes + filenameLengthBytes + filenameLength);
    int filenameIndex    = (offsetBytes + filenameLengthBytes);

    // length of output array (not including null termination)
    int contentLength = filenameLength + bytesRead + offsetBytes + filenameLengthBytes;
    char content[contentLength];

    content[0] = (unsigned char)(offset >> (2 * 8) & 0xFF);
    content[1] = (unsigned char)(offset >> (1 * 8) & 0xFF);
    content[2] = (unsigned char)(offset & 0xFF);
    content[3] = (unsigned char)filenameLength;

    // copy filename into indicies 4 through (4 + filenameLength)
    memcpy(content + filenameIndex, filename, filenameLength);

    // copy content starting at index (4 + filenameLength)
    memcpy(content + fileContentIndex, fileContent, bytesRead);

    return makePacket('B', contentLength, packetNumber, content);
}