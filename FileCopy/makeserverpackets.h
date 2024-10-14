#include "packetstruct.h"
#include <string>

using namespace std;

packet makeHashPacket(packet incomingPacket, unsigned char obuff [20]);
packet makeAckPacket(packet incomingPacket);
packet makeResCPacket(packet incomingPacket);
packet makeResBPacket(packet incomingPacket);

ssize_t parseCPacket(packet packet, char **fileName);

int parseByteOffset(packet packet);
int parseBytesRead(packet packet);
char *parseBytesContent(packet packet, int bytesRead);
string parseBytesFilename(packet packet);