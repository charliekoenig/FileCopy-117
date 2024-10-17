/**********************************************************
               UDPclientpackets.h - 10/17/2024
    Authors:
        * Charlie Koenig
        * Idil Kolabas

    This module is an interdace for the UDPclientpackets 
    operations, useful for sending data over the C150NETWORK 
    from the client to the server and for parsing data sent 
    by the server to the client
    
***********************************************************/
#include "packetstruct.h"

packet makeCopyPacket(ssize_t FileLen, char *filename, int packetNumber);
packet makeStatusPacket(unsigned char *clientHash, packet hashPacket, int packetNumber);
packet makeFileCheckPacket(char *filename, int packetNumber);
packet makeBytePacket(int offset, char *filename, unsigned char *fileContent, 
                      int packetNumber, int bytesRead, int filenameLength);