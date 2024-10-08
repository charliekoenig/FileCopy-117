#include "packetstruct.h"

packet makeCPacket(ssize_t FileLen, char *fileName, int packetNumber);
packet makeSPacket(char *serverHash, char *clientHash, char *filename, int packetNum);
    //char *fileName, int packetNumber);
packet makeFPacket(char *filename, int packetNumber);