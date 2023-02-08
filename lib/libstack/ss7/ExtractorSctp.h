// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include "Sctp.h"

template<class Parent>
struct ExtractorSctp
{
    template<class PKT, class DEPTH = Depth<0>>
    typename PKT::BaseType extractSctpChunk(SctpChunk<PKT>&& chunk, DEPTH depth = DEPTH())
    {
        auto depth1 = depth.addDepth();
        if (chunk.getLength() < chunk.size())
            return move(chunk);  // broken

        switch (chunk->type) {
            case SctpChunk<PKT>::SCTP_DATA: {
                auto chunkData = SctpChunkData<PKT>(move(chunk));
                if (chunkData->proto == SctpChunkData<PKT>::SctpM2ua)
                    return static_cast<Parent*>(this)->gotM2ua(move(chunkData.makeM2ua().rebase()));
                else if(chunkData->proto == SctpChunkData<PKT>::SctpM3ua)
                    return static_cast<Parent*>(this)->gotM3ua(move(chunkData.makeM3ua().rebase()));

                break;
            }
            default: return PktOffset();

        }
        return move(chunk);  // unknown (like error)
    }
};
