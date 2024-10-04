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
    char *content;
};

packet makePacket(char opcode, int length, char *content) {
    packet newPacket = (packet)malloc(sizeof(*newPacket));

    newPacket->opcode = opcode;
    newPacket->length = length + 1;

    newPacket->content = (char *)malloc(length + 1);

    for (int i = 0; i < length; i++) {
        newPacket->content[i] = content[i];
    }
    newPacket->content[length] = '\0';

    return newPacket;
}

packet stringToPacket(char *packetString) {
    char opcode = packetString[0];
    int length = ((unsigned char)packetString[1] - '\0') - 3;
    char *content = (char *)malloc(length);

    for (int i = 2; i < 2 + length; i++) {
        content[i - 2] = packetString[i];
    }

    return makePacket(opcode, length, content);
}

char *
packetContent(packet packet) {
    return packet->content;
}

int  
packetLength(packet packet) {
    return packet->length;
}

char 
packetOpcode(packet packet) {
    return packet->opcode;
}

char *
packetToString(packet packet) {
    int length = packetLength(packet) + 2;
    char *packetString = (char *)malloc(length);
    
    packetString[0] = packetOpcode(packet);
    packetString[1] = length + '\0';

    char *content = packetContent(packet);

    for (int i = 2; i < length; i++) {
        packetString[i] = content[i - 2];

    }

    return packetString;
}

void freePacket(packet packet) {
    free(packet->content);
    free(packet);
}