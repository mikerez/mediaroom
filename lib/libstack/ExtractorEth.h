// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include "Eth.h"

template<class PARENT>
struct ExtractorEth
{
    // uses got'Layer'() and skip'Tag'() calls where Layers are: [Ip4 Ip6] and Tags are: [Vlan Mpls]

    template<class PKT, class DEPTH = Depth<0>>
    typename PKT::BaseType extractEth(Eth<PKT>&& eth, DEPTH depth = DEPTH())
    {
        auto depth1 = depth.addDepth();
        auto type = eth->type;
        switch (type) {
        case Eth<PKT>::EthIP4:
            return static_cast<PARENT*>(this)->gotIp4(move(eth.makeIp4().rebase()));
        case Eth<PKT>::EthVlan:
        case Eth<PKT>::EthQinQ: {
            auto vlan1 = move(eth.makeVlan());
            if (static_cast<PARENT*>(this)->skipVlan(vlan1)) {
                auto tpid = vlan1->tpid;
                switch (tpid) {
                case vlan1.VlanIP4:
                    return static_cast<PARENT*>(this)->gotIp4(move(vlan1.makeIp4().rebase()));
                case vlan1.VlanVlan: {
                    auto vlan2 = move(vlan1.makeVlan());
                    if (static_cast<PARENT*>(this)->skipVlan(vlan2)) {
                        switch (vlan2->tpid) {
                        case vlan2.VlanIP4:
                            return static_cast<PARENT*>(this)->gotIp4(move(vlan2.makeIp4().rebase()));
                        case vlan2.VlanMplsU:
                        case vlan2.VlanMplsM:
                            return extractMpls(move(vlan2.makeMpls()), depth1);
                        }
                    }
                    return move(vlan2);
                }
                case vlan1.VlanMplsU:
                case vlan1.VlanMplsM:
                    return extractMpls(move(vlan1.makeMpls()), depth1);
                }
            }
            return move(vlan1);
        }
        case Eth<PKT>::EthMplsU:
        case Eth<PKT>::EthMplsM:
            return extractMpls(move(eth.makeMpls()), depth1);
        }
        return move(eth);
    }

    template<class PKT, class DEPTH = Depth<0>>
    typename PKT::BaseType extractMpls(Mpls<PKT>&& mpls1, DEPTH depth = DEPTH())
    {
        auto depth1 = depth.addDepth();
        if (static_cast<PARENT*>(this)->skipMpls(mpls1)) {
            if (mpls1->getBs() == mpls1.MplsIP4) {
                return static_cast<PARENT*>(this)->gotIp4(move(mpls1.makeIp4().rebase()));
            }
            else {
                auto mpls2(move(mpls1.makeMpls()));
                if (static_cast<PARENT*>(this)->skipMpls(mpls2)) {
                    if (mpls2->getBs() == mpls2.MplsIP4) {
                        return static_cast<PARENT*>(this)->gotIp4(move(mpls2.makeIp4().rebase()));
                    }
                    else {
                        auto mpls3(move(mpls2.makeMpls()));
                        if (static_cast<PARENT*>(this)->skipMpls(mpls3)) {
                            if (mpls3->getBs() == mpls3.MplsIP4) {
                                return static_cast<PARENT*>(this)->gotIp4(move(mpls3.makeIp4().rebase()));
                            }
                            else { /*TODO CHECK MPLSL2TUN*/
                                return static_cast<PARENT*>(this)->gotEth(move(mpls3.makeEth().rebase()));
                            }
                        }
                        return move(mpls3);
                    }
                }
                return move(mpls2);
            }
        }
        return move(mpls1);
    }
};
