#include "ReentrantPacket.h"

ReentrantPacket::ReentrantPacket(int neededOffset): offset(neededOffset) {}

int ReentrantPacket::needed() const {
    return (raw.capacity() - raw.length()) - partial.length() + offset;
}
