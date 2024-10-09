#include "packetstruct.h"

packet makeCopyPacket(ssize_t FileLen, char *filename, int packetNumber);
packet makeStatusPacket(char *serverHash, char *clientHash, char *filename, int packetNumber);
packet makeFileCheckPacket(char *filename, int packetNumber);
packet makeBytePacket(int offset, char *filename, char *fileContent, int packetNumber);