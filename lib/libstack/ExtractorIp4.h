// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include "Ip4.h"

template<class PARENT>
struct ExtractorIp4
{
    template<class PKT, class DEPTH = Depth<0>>
    typename PKT::BaseType extractIp4(Ip4<PKT>&& ip, DEPTH depth = DEPTH())
    {
        auto depth1 = depth.addDepth();
        if (ip.getLength() >= ip.size() && LS_ntohs(ip->total_length) <= ip.getLength()) {
            switch (ip->protocol) {
            case Ip4<PKT>::Ip4Ip4:
                return static_cast<PARENT*>(this)->gotIp4(move(ip.makeIp4().rebase()));
            case Ip4<PKT>::Ip4Tcp:
                return static_cast<PARENT*>(this)->gotTcp(move(ip.makeTcp().rebase()));
            case Ip4<PKT>::Ip4Udp:
                return static_cast<PARENT*>(this)->gotUdp(move(ip.makeUdp().rebase()));
            case Ip4<PKT>::Ip4Sctp:
                return static_cast<PARENT*>(this)->gotSctp(move(ip.makeSctp().rebase()));
            }
        }
        return move(ip);
    }
};
