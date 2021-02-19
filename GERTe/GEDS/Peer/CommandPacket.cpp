#include "CommandPacket.h"

CommandPacket::CommandPacket(unsigned char cmd, int commandSize): command(cmd), ReentrantPacket(commandSize) {}
