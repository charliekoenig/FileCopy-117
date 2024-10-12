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

    char content[contentLength];
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
        cout << "Content Wrong\n";
        return false;
    }

    return true;
}

void
printContent(packet packet) {
    int contentLength = packetLength(packet) - 5;
    for (int i = 0; i < contentLength; i++) {
        printf("%c", packetContent(packet)[i]);
    }
    cout << endl;
}

void
printPacket(packet packet) {
    printf("PACKET CONTENTS\n");
    cout << packetOpcode(packet) << endl;
    printf("%d\n", packetLength(packet));
    printf("%d\n", packetNum(packet));
    printContent(packet);
    cout << "_________________________" << endl; 
}

void freePacket(packet packet) {
    free(packet->content);
    free(packet);
}
