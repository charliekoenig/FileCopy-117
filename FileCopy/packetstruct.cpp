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

packet makePacket(char opcode, int contentLength, int packetNum, char *content) {
    packet newPacket = (packet)malloc(sizeof(*newPacket));

    newPacket->opcode = opcode;
    newPacket->length = contentLength + 5;
    newPacket->packetNum = packetNum;

    newPacket->content = (char *)malloc(contentLength + 1);

    for (int i = 0; i < contentLength; i++) {
        newPacket->content[i] = content[i];
    }
    newPacket->content[contentLength] = '\0';

    return newPacket;
}

packet stringToPacket(char *packetString) {
    char opcode = packetString[0];
    int contentLength = (((unsigned char)packetString[1] << 8) + (unsigned char)packetString[2]) - 5;

    // should be array not malloc'd
    char *content = (char *)malloc(contentLength);
    int packetNum = ((unsigned char)packetString[3] - '\0');

    for (int i = 4; i < (contentLength + 4); i++) {
        content[i - 4] = packetString[i];
    }

    return makePacket(opcode, contentLength, packetNum, content);
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
    int contentLength = packetLength(packet) - 4;
    *fileName = (char *)malloc(contentLength - sizeof(ssize_t));
    for (int i = 0; i < contentLength; i++) {
        if ((unsigned) i < sizeof(ssize_t)) {
            unpackedLen = (unpackedLen << 8) + (ssize_t)(packetContent(packet)[i]);
        } else {
            (*fileName)[(unsigned) i - sizeof(ssize_t)] = packetContent(packet)[i];
        }
    }

    (*fileName)[contentLength - sizeof(ssize_t)] = '\0';

    printf("bytesRead: %ld\n", unpackedLen);

    return unpackedLen;
}

char *
packetToString(packet packet) {
    int length = packetLength(packet);
    char *packetString = (char *)malloc(length);
    
    packetString[0] = packetOpcode(packet);
    packetString[1] = (length >> 8) & 0xFF;
    packetString[2] = length & 0xFF;
    packetString[3] = packetNum(packet) + '\0';

    char *content = packetContent(packet);

    for (int i = 4; i < length; i++) {
        packetString[i] = content[i - 4];
    }

    return packetString;
}

bool 
packetCompare(packet p1, packet p2) {
    int p1Len = packetLength(p1);
    int p2Len = packetLength(p2);

    if (p1Len != p2Len) {
        cout << "Length wrong\n";
        return false;
    }

    if (packetOpcode(p1) != packetOpcode(p2)) {
        cout << "Opcode wrong\n";
        return false;
    }


    if ((memcmp(packetContent(p1), packetContent(p2), p1Len - 4) != 0)) {
        cout << "Cotent Wrong\n";
        return false;
    }

    return true;
}


void freePacket(packet packet) {
    free(packet->content);
    free(packet);
}

/*
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
}*/