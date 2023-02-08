// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include "Hdlc.h"

template<class PARENT>
struct ExtractorHdlc
{
    // uses got'Layer'() and skip'Tag'() calls where Layers are: [Ip4 Ip6] and Tags are: [Vlan Mpls]

    template<class PKT, class DEPTH = Depth<0>>
    typename PKT::BaseType extractHdlc(Hdlc<PKT>&& hdlc, DEPTH depth = DEPTH())
    {
        auto depth1 = depth.addDepth();
        auto mtp2 = hdlc.makeMtp();
        return move(mtp2);
    }
};
