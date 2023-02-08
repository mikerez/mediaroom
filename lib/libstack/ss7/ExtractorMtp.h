// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include "Mtp3.h"
#include "Mtp.h"


template<class PARENT>
struct ExtractorMtp
{
    template<class PKT, class DEPTH = Depth<0>>
    typename PKT::BaseType extractMtp(Mtp<PKT>&& mtp, DEPTH depth = DEPTH())
    {
        auto depth1 = depth.addDepth();
        switch (mtp->sio.si) {
            case Mtp<PKT>::ISUP:
                return static_cast<PARENT*>(this)->gotIsup(move(mtp.makeIsup().rebase()));
            case Mtp<PKT>::SCCP: {
                auto sccp = move(mtp.makeSccp());
                return move(sccp);
                break;
            }
            default:
                break;
        }
        return move(mtp);
    }

    template<class PKT, class DEPTH = Depth<0>>
    typename PKT::BaseType extractMtp3(Mtp3<PKT>&& mtp3, DEPTH depth = DEPTH())
    {
        auto depth1 = depth.addDepth();
        switch (mtp3->sio.si) {
        case Mtp3<PKT>::ISUP:
            return static_cast<PARENT*>(this)->gotIsup(move(mtp3.makeIsup().rebase()));
        case Mtp3<PKT>::SCCP: {
            return static_cast<PARENT*>(this)->gotSccp(move(mtp3.makeSccp().rebase()));
        }
        default:
            break;
        }
        return move(mtp3);
    }
};
