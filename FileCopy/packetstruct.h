#include <stdlib.h>

#ifndef PACKET_STRUCT_H
#define PACKET_STRUCT_H

typedef struct packetStruct *packet;

packet makePacket(char opcode, int contentLength, int packetNum, char *content);
packet stringToPacket(char *packetString);

char *packetContent(packet packet);
int packetLength(packet packet);
char packetOpcode(packet packet);
int packetNum(packet packet);
ssize_t parseCPacket(packet packet, char **fileName);

char *packetToString(packet packet);
void freePacket(packet packet);

bool packetCompare(packet p1, packet p2);

#endif
