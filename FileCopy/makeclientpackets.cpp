#include "makeclientpackets.h"
#include <cstring>

#include <iostream>
using namespace std;

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

    int index = 0;
    for (index = 0; index < contentLength; index++) {
        if (index >= fileContentIndex) {
            content[index] = fileContent[(index - fileContentIndex) + offset];
        } else if (index >= filenameIndex) {
            content[index] = filename[index - filenameIndex];
        } else if (index == 3) {
            content[index] = (unsigned char)filenameLength;
        } else {
            content[index] = (unsigned char)(offset >> ((2 - index) * 8) & 0xFF);
        }
    }

    // cout << "First char in packet " << packetNumber << ": " << fileContent[offset] << endl;
    return makePacket('B', contentLength, packetNumber, content);
}