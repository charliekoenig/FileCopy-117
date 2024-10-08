#include "makeclientpackets.h"
#include <openssl/sha.h>
#include <cstring>

packet 
makeCPacket(ssize_t fileLen, char *filename, int packetNumber) {
    int bytes = sizeof(fileLen);
    int contentLen = strlen(filename) + bytes;
    char content[contentLen];

    for (int index = 0; index < contentLen; index++) {
        if (index < bytes) {
            content[index] = (fileLen >> ((bytes - 1 - index) * 8)) & 0xFF;
        } else {
            content[index] = filename[index - bytes];
        }
    }

    return makePacket('C', contentLen, packetNumber, cContent);
}

packet 
makeSPacket(char *serverHash, char *clientHash, char *filename, int packetNum) {
    int contentLength = strlen(filename) + 1;
    char content[contentLength];

    // use memcmp in case of null char in hash
    content[0] = (memcmp(serverHash, clientHash, 20) == 0) ? 'S' : 'F';
    for (int index = 1; index < contentLength; index++) {
        statusContent[index] = filename[index - 1];
    }

    return makePacket('S', contentLength, packetNumber, statusContent);
}

packet 
makeFPacket(char *filename, int packetNumber) {
    return makePacket('F', strlen(filename), packetNumber, filename);
}