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

    // Debug output
    printf("Debug: makePacket created packet with length %d\n", newPacket->length);
    printf("Debug: Last character of content: %d\n", (int)newPacket->content[length]);
    printf("Debug: Null terminator present: %s\n\n", newPacket->content[length] == '\0' ? "Yes" : "No");


    return newPacket;
}

// note: cast to unsigned char when converting to integer for lengths
packet stringToPacket(char *packetString) {
    char opcode = packetString[0];
    int length = (packetString[1] - '\0') - 3;
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

    // Debug output
    cout << "P->S: Debug: packetToString created string with length " << length << endl;
    cout << "P->S: Debug: Opcode: " << (int)packetString[0] << endl;
    cout << "P->S: Debug: Length byte: " << (int)(unsigned char)packetString[1] << endl;
    cout << "P->S: Debug: Content: " << string(packetString + 2, length - 2) << endl;
    cout << "P->S: Debug: Last character of packet string: " << (int)packetString[length - 1] << endl << endl;

    return packetString;
}

void freePacket(packet packet) {
    free(packet->content);
    free(packet);
}