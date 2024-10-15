#include "packetstruct.h"

packet makeCopyPacket(ssize_t FileLen, char *filename, int packetNumber);
packet makeStatusPacket(unsigned char *clientHash, packet hashPacket, int packetNumber);
packet makeFileCheckPacket(char *filename, int packetNumber);
packet makeBytePacket(int offset, char *filename, unsigned char *fileContent, int packetNumber, int bytesRead, int filenameLength);