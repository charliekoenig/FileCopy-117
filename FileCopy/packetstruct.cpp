#include "packetstruct.h"
#include <cstring>
#include <string>
#include <cassert>
#include <iostream>

using namespace std;


struct packetStruct {
    char opcode;               // 8  bits
    unsigned int length;       // 10 bits
    unsigned int packetNum;    // 14 bits
    char *content;             // Max 508 bytes
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

packet stringToPacket(unsigned char *packetString) {
    assert(packetString != NULL);

    char opcode = packetString[0];
    int contentLength = ((packetString[1] << 2) + (packetString[2] >> 6)) - 5;
    int packetNumber = ((packetString[2] & 0x3F) << 8) + packetString[3];

    char content[contentLength];
    for (int i = 4; i < (contentLength + 4); i++) {
        content[i - 4] = packetString[i];
    }

    return makePacket(opcode, contentLength, packetNumber, content);
}

char *
packetContent(packet packet) {
    assert(packet != NULL);
    return packet->content;
}

int  
packetLength(packet packet) {
    assert(packet != NULL);
    return packet->length;
}

int  
packetNum(packet packet) {
    assert(packet != NULL);
    return packet->packetNum;
}

char 
packetOpcode(packet packet) {
    assert(packet != NULL);
    return packet->opcode;
}

char *
packetToString(packet packet) {
    assert(packet != NULL);

    int length = packetLength(packet);
    int packetNumber = packetNum(packet);
    char *packetString = (char *)malloc(length);
    
    packetString[0] = packetOpcode(packet);
    packetString[1] = (length >> 2) & 0xFF;
    packetString[2] = ((length & 0x03) << 6) + ((packetNumber >> 8) & 0x3F);
    packetString[3] = (packetNumber & 0xFF);

    char *content = packetContent(packet);

    for (int i = 4; i < length; i++) {
        packetString[i] = content[i - 4];
    }

    return packetString;
}

bool 
packetCompare(packet p1, packet p2) {
    assert((p1 != NULL) && (p2 != NULL));

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
    assert(packet != NULL);


    int contentLength = packetLength(packet) - 5;
    for (int i = 0; i < contentLength; i++) {
        printf("%c", packetContent(packet)[i]);
    }
    cout << endl;
}

void
printPacket(packet packet) {
    assert(packet != NULL);

    printf("PACKET CONTENTS\n");
    cout << packetOpcode(packet) << endl;
    printf("%d\n", packetLength(packet));
    printf("%d\n", packetNum(packet));
    printContent(packet);
    cout << "_________________________" << endl; 
}


void freePacket(packet packet) {
    assert(packet != NULL);

    free(packet->content);
    free(packet);
}
