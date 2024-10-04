#include "packetstruct.h"

packet makeHashPacket(packet incomingPacket, unsigned char *fileContent);
packet makeAckPacket(packet incomingPacket);