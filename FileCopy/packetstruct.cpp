#include "packetstruct.h"

struct packetStruct {
    char opcode;
    char length;
    char *content;
};

packet makePacket(char opcode, char length, char *content) {
    packet newPacket = (packet)malloc(sizeof(*newPacket));

    newPacket->opcode = opcode;
    newPacket->length = length;
    newPacket->content = (char *)malloc(length);

    for (int i = 0; i < length; i++) {
        newPacket->content[i] = content[i];
    }

    return newPacket;
}


char *
packetContent(packet packet) {
    return packet->content;
}

char  
packetLength(packet packet) {
    return packet->length;
}

char  
packetOpcode(packet packet) {
    return packet->opcode;
}


char *
packetToString(packet packet) {
    char *packetString = (char *)malloc(packet->length + 2);
    int length = packetLength(packet);

    packetString[0] = packetOpcode(packet);
    packetString[1] = length;

    for (int i = 2; i < length; i++) {
        packetString[i] = packetContent(packet)[i - 2];
    }

    return packetString;
}
