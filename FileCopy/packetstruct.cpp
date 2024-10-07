#include "packetstruct.h"
#include <cstring>
#include <string>

//testing
#include <ostream>
#include <iostream>

using namespace std;

struct packetStruct {
    char opcode;
    int length;
    int packetNum;
    char *content;
};

packet makePacket(char opcode, int length, int packetNum, char *content) {
    packet newPacket = (packet)malloc(sizeof(*newPacket));

    newPacket->opcode = opcode;
    newPacket->length = length + 1;
    newPacket->packetNum = packetNum;

    newPacket->content = (char *)malloc(length + 1);

    for (int i = 0; i < length; i++) {
        newPacket->content[i] = content[i];
    }
    newPacket->content[length] = '\0';

    return newPacket;
}

packet stringToPacket(char *packetString) {
    char opcode = packetString[0];
    int length = ((unsigned char)packetString[1] - '\0') - 4;
    char *content = (char *)malloc(length);
    int packetNum = ((unsigned char)packetString[2] - '\0');

    for (int i = 3; i < 3 + length; i++) {
        content[i - 3] = packetString[i];
    }

    return makePacket(opcode, length, packetNum, content);
}

char *
packetContent(packet packet) {
    return packet->content;
}

int  
packetLength(packet packet) {
    return packet->length;
}

int  
packetNum(packet packet) {
    return packet->packetNum;
}

char 
packetOpcode(packet packet) {
    return packet->opcode;
}

ssize_t
parseCPacket(packet packet, char **fileName) {
    ssize_t unpackedLen = 0;
    *fileName = (char *)malloc(packetLength(packet) - sizeof(ssize_t) + 1);
    for (int i = 0; i < packetLength(packet); i++) {
        if ((unsigned) i < sizeof(ssize_t)) {
            unpackedLen = (unpackedLen << 8) + (ssize_t)(packetContent(packet)[i]);
        } else {
            (*fileName)[(unsigned) i - sizeof(ssize_t)] = packetContent(packet)[i];
        }
    }

    (*fileName)[packetLength(packet) - sizeof(ssize_t)] = '\0';

    printf("bytesRead: %ld\n", unpackedLen);

    return unpackedLen;
}

char *
packetToString(packet packet) {
    int length = packetLength(packet) + 3;
    char *packetString = (char *)malloc(length);
    
    packetString[0] = packetOpcode(packet);
    packetString[1] = length + '\0';
    packetString[2] = packetNum(packet) + '\0';

    char *content = packetContent(packet);

    for (int i = 3; i < length; i++) {
        packetString[i] = content[i - 3];

    }

    return packetString;
}

void freePacket(packet packet) {
    free(packet->content);
    free(packet);
}