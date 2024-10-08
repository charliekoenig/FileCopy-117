#include "makeclientpackets.h"
#include <openssl/sha.h>
#include <cstring>

packet 
makeCopyPacket(ssize_t fileLen, char *filename, int packetNumber) {
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

    return makePacket('C', contentLen, packetNumber, content);
}

packet 
makeStatusPacket(char *serverHash, char *clientHash, char *filename, int packetNumber) {
    int contentLength = strlen(filename) + 1;
    char content[contentLength];

    content[0] = (memcmp(serverHash, clientHash, 20) == 0) ? 'S' : 'F';
    for (int index = 1; index < contentLength; index++) {
        content[index] = filename[index - 1];
    }

    return makePacket('S', contentLength, packetNumber, content);
}

packet 
makeFilecheckPacket(char *filename, int packetNumber) {
    return makePacket('F', strlen(filename), packetNumber, filename);
}