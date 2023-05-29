// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include "Packet.h"
#include <vector>

class Chain
{
public:
    Chain( Chain *next = nullptr ) : _next(next)
    {
    }
    virtual ~Chain() {}

    virtual void putPackets(Packet** packets, size_t count) {}
    virtual void putPacket(Packet* packet) {}

protected:
    Chain* _next;
};
