/**********************************************************
                 packetstruct.h - 10/17/2024
    Authors:
        * Charlie Koenig
        * Idil Kolabas

    The packetstruct interface, useful for sending data
    over the C150NETWORK
    
***********************************************************/

#include <stdlib.h>

#ifndef PACKET_STRUCT_H
#define PACKET_STRUCT_H

typedef struct packetStruct *packet;

packet makePacket(char opcode, int contentLength, int packetNum, char *content);

packet stringToPacket(unsigned char *packetString);
char *packetToString(packet packet);

char *packetContent(packet packet);
int packetLength(packet packet);
char packetOpcode(packet packet);
int packetNum(packet packet);

void freePacket(packet packet);

bool packetCompare(packet p1, packet p2);
void printContent(packet packet);
void printPacket(packet packet);

#endif
