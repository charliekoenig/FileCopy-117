#include <stdlib.h>

#ifndef PACKET_STRUCT_H
#define PACKET_STRUCT_H

typedef struct packetStruct *packet;

packet makePacket(char opcode, int packetLength, char *content);
packet packetToString(char *packetString);

char *packetContent(packet packet);
int  packetLength(packet packet);
char  packetOpcode(packet packet);

char *packetToString(packet packet);
void freePacket(packet packet);

#endif
