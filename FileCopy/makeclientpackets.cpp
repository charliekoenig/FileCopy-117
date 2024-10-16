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

packet 
makeFileCheckPacket(char *filename, int packetNumber) {
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

    content[0] = (unsigned char)(offset >> (2 * 8) & 0xFF);
    content[1] = (unsigned char)(offset >> (1 * 8) & 0xFF);
    content[2] = (unsigned char)(offset & 0xFF);
    content[3] = (unsigned char)filenameLength;
    memcpy(content + filenameIndex, filename, filenameLength);
    memcpy(content + fileContentIndex, fileContent, bytesRead);

    // todo memcpy
    // for (int index = 0; index < contentLength; index++) {
    //     if (index >= fileContentIndex) {
    //         content[index] = fileContent[(index - fileContentIndex) + offset];
    //     } else if (index >= filenameIndex) {
    //         content[index] = filename[index - filenameIndex];
    //     } else if (index == 3) {
    //         content[index] = (unsigned char)filenameLength;
    //     } else {
    //         content[index] = (unsigned char)(offset >> ((2 - index) * 8) & 0xFF);
    //     }
    // }

    // cout << "First char in packet " << packetNumber << ": " << fileContent[offset] << endl;
    return makePacket('B', contentLength, packetNumber, content);
}