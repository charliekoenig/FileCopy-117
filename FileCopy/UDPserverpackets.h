/**********************************************************
               UDPserverpackets.h - 10/17/2024
    Authors:
        * Charlie Koenig
        * Idil Kolabas

    This module is an interdace for the UDPserverpackets 
    operations, useful for sending data over the C150NETWORK 
    from the server to the client and for parsing data sent 
    by the client to the server
    
***********************************************************/

#include "packetstruct.h"
#include <string>

using namespace std;

packet makeHashPacket(packet incomingPacket, unsigned char obuff [20]);

packet makeAckPacket(packet incomingPacket);
packet makeResCPacket(packet incomingPacket);
packet makeResBPacket(packet incomingPacket);

ssize_t parseCPacket(packet packet, char **filename);

int parseByteOffset(packet packet);
int parseBytesRead(packet packet);
char *parseBytesContent(packet packet, int bytesRead);
string parseBytesFilename(packet packet);