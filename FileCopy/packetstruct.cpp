#include "packetstruct.h"
#include <cstring>
#include <string>

using namespace std;

struct packetStruct {
    char *opcode;
    char *length;
    char *content;
};

packet makePacket(char opcode, char length, char *content) {
    packet newPacket = (packet)malloc(sizeof(*newPacket));

    newPacket->opcode = (char *)malloc(1);
    *(newPacket->opcode) = opcode;
    newPacket->length = (char *)malloc(1);
    *(newPacket->length) = length;
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
    return *(packet->length);
}

char  
packetOpcode(packet packet) {
    return *(packet->opcode);
}


char *
packetToString(packet packet) {
    int length = packetLength(packet);
    string lengthString = to_string(length);
    int lengthOfLength = strlen(lengthString.c_str()); // we have to rename this omg

    char *packetString = (char *)malloc(length + 1 + lengthOfLength);
    
    packetString[0] = packetOpcode(packet);
   
    for (int i = 1; i < 1 + lengthOfLength; i++) {
        packetString[i] = lengthString[i - 1];
    }

    for (int i = 1 + lengthOfLength; i < length + 1 + lengthOfLength; i++) {
        packetString[i] = packetContent(packet)[i - (1 + lengthOfLength)];
    }

    return packetString;
}

void freePacket(packet packet) {
    free(packet->opcode);
    free(packet->length);
    free(packet->content);
    free(packet);
}