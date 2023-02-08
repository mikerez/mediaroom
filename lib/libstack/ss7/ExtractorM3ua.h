// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once
#include "M3ua.h"

template<class PARENT>
struct ExtractorM3ua
{
    template<class PKT, class DEPTH = Depth<0>>
    typename PKT::BaseType extractM3ua(M3ua<PKT>&& m3ua, DEPTH depth = DEPTH())
    {
        auto depth1 = depth.addDepth();
        bool got_details = false;

        uint16_t typeMask = m3ua->type;
        typeMask |= m3ua->msgClass << 8;

        if (m3ua.size() > sizeof(M3uaHdr)) {
            uint16_t offset = sizeof(M3uaHdr);

            while (offset <= m3ua.size()) {
                M3uaParamter param = *(M3uaParamter *)(m3ua.getWithOffset(offset));

                switch (param.getTag()) {
                    case M3ua<PKT>::PDATA: {
                        M3uaProtocolData pd = *(M3uaProtocolData *)(m3ua.getWithOffset(offset));
                        PacketDetails* details = (*m3ua).getDetails();
                        details->pushM3ua(m3ua, offset + sizeof(M3uaParamter)/*offset to opc*/);
                        got_details = true;

                        if (pd.si == pd.SCCP) {
                            offset += sizeof(M3uaProtocolData);

                            uint16_t length = m3ua.getLength() - offset;
                            offset += m3ua.getOffset();

                            Sccp<PktOffset> sccp = PktOffset(move(m3ua.makeSccp().getPtr()), length, offset);
                            // TODO: smth with padding
                            return static_cast<PARENT*>(this)->gotSccp(move(sccp));
                        }
                        else if (pd.si == pd.ISUP) {
                            offset += sizeof(M3uaProtocolData);

                            uint16_t length = m3ua.getLength() - offset;
                            offset += m3ua.getOffset();

                            Isup<PktOffset> isup = PktOffset(move(m3ua.makeIsup().getPtr()), length, offset);
                            // TODO: smth with padding,
                            // M3UA + ISUP
                            // return static_cast<PARENT*>(this)->gotIsup(move(isup));
                            return PktOffset();
                        }
                        else return PktOffset();
                    }
                    default:
                    {
                        offset += param.getLength();
                        break;
                    }
                }
            }
        }

        if (!got_details) {
            PacketDetails* details = (*m3ua).getDetails();
            details->pushM3ua(m3ua);
            got_details = true;
        }

        return move(m3ua);
    }
};
