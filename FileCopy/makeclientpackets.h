#include "packetstruct.h"

packet makeCopyPacket(ssize_t FileLen, char *filename, int packetNumber);
packet makeStatusPacket(char *serverHash, char *clientHash, char *filename, int packetNum);
packet makeFileCheckPacket(char *filename, int packetNumber);