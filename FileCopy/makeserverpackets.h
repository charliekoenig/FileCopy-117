#include "packetstruct.h"

packet makeHashPacket(packet incomingPacket, unsigned char *fileContent, size_t fContentLen);
packet makeAckPacket(packet incomingPacket);
packet makeResPacket(packet incomingPacket);