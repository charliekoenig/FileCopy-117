#include <stdlib.h>

#ifndef PACKET_STRUCT_H
#define PACKET_STRUCT_H

typedef struct packetStruct *packet;

packet makePacket(char opcode, char packetLength, char *content);
packet packetToString(char *packetString);

char *packetContent(packet packet);
char  packetLength(packet packet);
char  packetOpcode(packet packet);

char *packetToString(packet packet);

#endif
