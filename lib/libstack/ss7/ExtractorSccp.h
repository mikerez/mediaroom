// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include "Sccp.h"

/*Q.713 (07 / 96)
 *
 *  A SCCP message consists of the following parts:
 *  - the message type code;
 *  - the mandatory fixed part;
 *  - the mandatory variable part;
 *  - the optional part, which may contain fixed length and variable length fields
 */

#pragma pack(push, 1)
struct ai // Address indicator
{
    uint8_t pci : 1;  // Point Code Indicator
    uint8_t ssni : 1; // SubSystem Number Indicator
    uint8_t gti : 4;  // Global Title Indicator
    uint8_t ri : 1;   // Routing Indicator
    uint8_t reserved : 1;
};
#pragma pack(pop)

#define SCCP_MSG_TYPE_LENGTH                    1

/* SCCP parameters */

#define SCCP_END_OF_OPTIONAL_PARAMETERS_LENGTH  1
#define SCCP_DST_LOCAL_REFERENCE_LENGTH         3
#define SCCP_SRC_LOCAL_REFERENCE_LENGTH         3
#define SCCP_PROTOCOL_CLASS_LENGTH              1
#define SCCP_SEGMENTING_REASSEMBLING_LENGTH     1
#define SCCP_RECEIVE_SEQUENCE_NUMBER_LENGTH     1
#define SCCP_SEQUENCING_SEGMENTING_LENGTH       2
#define SCCP_CREDIT_LENGTH                      1
#define SCCP_RELEASE_CAUSE_LENGTH               1
#define SCCP_RETURN_CAUSE_LENGTH                1
#define SCCP_RESET_CAUSE_LENGTH                 1
#define SCCP_ERROR_CAUSE_LENGTH                 1
#define SCCP_REFUSAL_CAUSE_LENGTH               1
#define SCCP_HOP_COUNTER_LENGTH                 1
#define SCCP_IMPORTANCE_LENGTH                  1

#define SCCP_PARAMETER_NAME_LENGTH              1
#define SCCP_PARAMETER_LENGTH_LENGTH            1
#define SCCP_LONG_PARAMETER_LENGTH_LENGTH       2
#define SCCP_POINTER_LENGTH                     1
#define SCCP_LONG_POINTER_LENGTH                2

/* Address indicator */

#define SCCP_ADDRESS_INDICATOR_LENGTH           1
#define SCCP_AI_PC_LENGTH                       2
#define SCCP_AI_SSN_LENGTH                      1

template<class PARENT>
struct ExtractorSccp
{
    template<class PKT>
    uint8_t getSsn(const uint8_t * parameter, uint16_t length)
    {
        ai address = *(ai *)parameter;
        uint8_t offset = SCCP_ADDRESS_INDICATOR_LENGTH;

        if (address.pci)
            offset += SCCP_AI_PC_LENGTH;
        if (address.ssni)
            return *(parameter + offset);

        return Sccp<PKT>::SubsystemNumber::SSN_UNKNOWN;
    }

    template<class PKT>
    typename PKT::BaseType gotOptionalParameter(Sccp<PKT>&& sccp, uint16_t offset)
    {
        for (uint8_t name = *(sccp.getWithOffset(offset++)); name != sccp.P_END;) {
            uint8_t length = *(sccp.getWithOffset(offset));
            offset += SCCP_PARAMETER_LENGTH_LENGTH;
            offset += length;
            name = *(sccp.getWithOffset(offset));
            offset += SCCP_PARAMETER_NAME_LENGTH;
        }
        offset += SCCP_PARAMETER_NAME_LENGTH;
        // TODO: sccp.getLength() > offset?
        return PktOffset();
    }

#define OPTIONAL_PARAMETER(sccp, offset)                   \
    uint8_t oParameterPtr = *(sccp.getWithOffset(offset)); \
    if (oParameterPtr != 0) {                              \
        offset += SCCP_POINTER_LENGTH;                     \
        return gotOptionalParameter(move(sccp), offset);   \
    }

#define VARIABLE_PARAMETER(sccp, var, offset)             \
    var = *(sccp.getWithOffset(offset));                  \
    LS_ASSERT(var);                                          \
    offset += SCCP_POINTER_LENGTH;                        \

    template<class PKT, class DEPTH = Depth<0>>
    typename PKT::BaseType extractSccp(Sccp<PKT>&& sccp, DEPTH depth = DEPTH())
    {
        auto depth1 = depth.addDepth();
        uint16_t offset = SCCP_MSG_TYPE_LENGTH;

        switch (sccp->mt) {
            case Sccp<PKT>::T_CR: {
                /* Table 3 / Q.713  - Message type: Connection request
                 * ---------------------------------------------------------
                 * Parameter                  | Reference | Type  | Length
                 *                            |           |(F V O)|(octets)
                 * ---------------------------------------------------------
                 * Message type code          |    2.1    |   F   |    1
                 * Source local reference     |    3.3    |   F   |    3
                 * Protocol class             |    3.6    |   F   |    1
                 * Called party address       |    3.4    |   V   |3 minimum
                 * Credit                     |    3.10   |   O   |    3
                 * Calling party address      |    3.5    |   O   |4 minimum
                 * Data                       |    3.16   |   O   | 3 - 130
                 * Hop counter                |    3.18   |   O   |    3
                 * Importance                 |    3.19   |   O   |    3
                 * End of optional parameters |    3.1    |   O   |    1
                 * ---------------------------------------------------------
                 * Contains two pointers
                 */
                offset += SCCP_SRC_LOCAL_REFERENCE_LENGTH;

                uint8_t protocolClass = *(sccp.getWithOffset(offset));
                LS_ASSERT(protocolClass == 2 || protocolClass == 3);
                offset += SCCP_PROTOCOL_CLASS_LENGTH;

                uint8_t vParameterPtr = *(sccp.getWithOffset(offset));
                LS_ASSERT(vParameterPtr); // Mandatory variable length parameters must be present
                offset += SCCP_POINTER_LENGTH;

                uint8_t oParameterPtr = *(sccp.getWithOffset(offset));
                offset += SCCP_POINTER_LENGTH;

                uint8_t vParameterLength = *(sccp.getWithOffset(offset));
                offset += SCCP_PARAMETER_LENGTH_LENGTH;
                offset += vParameterLength;

                if(oParameterPtr)
                    return gotOptionalParameter(move(sccp), offset);
                break;
            }
            case Sccp<PKT>::T_CC: {
                /* Table 4 / Q.713 - Message type: Connection confirm
                 * ----------------------------------------------------------
                 * Parameter                   | Reference | Type  | Length
                 *                             |           |(F V O)|(octets)
                 * ----------------------------------------------------------
                 * Message type                |    2.1    |   F   |    1
                 * Destination local reference |    3.2    |   F   |    3
                 * Source local reference      |    3.3    |   F   |    3
                 * Protocol class              |    3.6    |   F   |    1
                 * Credit                      |    3.10   |   O   |    3
                 * Called party address        |    3.4    |   O   |4 minimum
                 * Data                        |    3.16   |   O   | 3 - 130
                 * Importance                  |    3.19   |   O   |    3
                 * End of optional parameter   |    3.1    |   O   |    1
                 * ----------------------------------------------------------
                 * Contains one pointer
                 */
                offset += SCCP_DST_LOCAL_REFERENCE_LENGTH;
                offset += SCCP_SRC_LOCAL_REFERENCE_LENGTH;

                uint8_t protocolClass = *(sccp.getWithOffset(offset));
                LS_ASSERT(protocolClass == 2 || protocolClass == 3);
                offset += SCCP_PROTOCOL_CLASS_LENGTH;

                OPTIONAL_PARAMETER(sccp, offset)
                return PktOffset();
            }
            case Sccp<PKT>::T_CREF: {
                /* Table 5 / Q.713 - Message type: Connection refused
                 * ----------------------------------------------------------
                 * Parameter                   | Reference | Type  | Length
                 *                             |           |(F V O)|(octets)
                 * ----------------------------------------------------------
                 * Message type                |    2.1    |   F   |    1
                 * Destination local reference |    3.2    |   F   |    3
                 * Refusal cause               |    3.15   |   F   |    1
                 * Called party address        |    3.4    |   O   |4 minimum
                 * Data                        |    3.16   |   O   | 3-130
                 * Importance                  |    3.19   |   O   |    3
                 * End of optional parameter   |    3.1    |   O   |    1
                 * ----------------------------------------------------------
                 * Contains one pointer
                 */
                offset += SCCP_DST_LOCAL_REFERENCE_LENGTH;
                offset += SCCP_REFUSAL_CAUSE_LENGTH;

                OPTIONAL_PARAMETER(sccp, offset)
                return PktOffset();
            }
            case Sccp<PKT>::T_RLSD: {
                /* Table 4 / Q.713 - Message type: Connection confirm
                  * ----------------------------------------------------------
                  * Parameter                   | Reference | Type  | Length
                  *                             |           |(F V O)|(octets)
                  * ----------------------------------------------------------
                  * Message type                |    2.1    |   F   |    1
                  * Destination local reference |    3.2    |   F   |    3
                  * Source local reference      |    3.3    |   F   |    3
                  * Release cause               |    3.11   |   F   |    1
                  * Data                        |    3.16   |   O   |  3-130
                  * Importance                  |    3.19   |   O   |    3
                  * End of optional parameter   |    3.1    |   O   |    1
                  * ----------------------------------------------------------
                  * Contains one pointer
                  */
                offset += SCCP_DST_LOCAL_REFERENCE_LENGTH;
                offset += SCCP_SRC_LOCAL_REFERENCE_LENGTH;
                offset += SCCP_RELEASE_CAUSE_LENGTH;

                OPTIONAL_PARAMETER(sccp, offset)
                return PktOffset();
            }
            case Sccp<PKT>::T_RLC: {
                // No pointers - no need to do smth with offset
                return PktOffset();
            }
            case Sccp<PKT>::T_DT1: {
                /* Table 8 / Q.713 - Message type: Data form 1
                 * ----------------------------------------------------------
                 * Parameter                   | Reference | Type  | Length
                 *                             |           |(F V O)|(octets)
                 * ----------------------------------------------------------
                 * Message type                |    2.1    |   F   |    1
                 * Destination local reference |    3.2    |   F   |    3
                 * Segmenting/reassembling     |    3.7    |   F   |    1
                 * Data                        |    3.16   |   V   |  2-256
                 * ----------------------------------------------------------
                 * Contains one pointer
                 */
                offset += SCCP_DST_LOCAL_REFERENCE_LENGTH;
                offset += SCCP_SEGMENTING_REASSEMBLING_LENGTH;

                uint8_t vParameterPtr = *(sccp.getWithOffset(offset)); // pointer to mandatory variable length parameter
                LS_ASSERT(vParameterPtr); // Mandatory variable length parameters must be present
                offset += SCCP_POINTER_LENGTH;

                uint8_t vParameterLength = *(sccp.getWithOffset(offset)); // Data
                offset += SCCP_PARAMETER_LENGTH_LENGTH;
                offset += vParameterLength;

                return PktOffset();
            }
            case Sccp<PKT>::T_UDT:
            /* Table 11 / Q.713 - Message type: Unitdata
             * -----------------------------------------------------------
             * Parameter                   | Reference | Type  |  Length
             *                             |           |(F V O)| (octets)
             * -----------------------------------------------------------
             * Message type                |    2.1    |   F   |     1
             * Protocol class              |    3.6    |   F   |     1
             * Called party address        |    3.4    |   V   | 3 minimum
             * Calling party address       |    3.5    |   V   | 3 minimum
             * Data                        |    3.16   |   V   |2-X (Note)
             * -----------------------------------------------------------
             * Contains three pointers
             */
            case Sccp<PKT>::T_UDTS:
            /* Table 12 / Q.713 - Message type: Unitdata service
             * -----------------------------------------------------------
             * Parameter                   | Reference | Type  |  Length
             *                             |           |(F V O)| (octets)
             * -----------------------------------------------------------
             * Message type                |    2.1    |   F   |     1
             * Return cause                |    3.12   |   F   |     1
             * Called party address        |    3.4    |   V   | 3 minimum
             * Calling party address       |    3.5    |   V   | 3 minimum
             * Data                        |    3.16   |   V   |2-X (Note)
             * -----------------------------------------------------------
             * Contains three pointers
             */
            case Sccp<PKT>::T_XUDT:
            /* Table 19 / Q.713 - Message type: Extended unitdata
             * -----------------------------------------------------------
             * Parameter                   | Reference | Type  |  Length
             *                             |           |(F V O)| (octets)
             * -----------------------------------------------------------
             * Message type                |    2.1    |   F   |     1
             * Protocol class              |    3.6    |   F   |     1
             * Called party address        |    3.4    |   V   | 3 minimum
             * Calling party address       |    3.5    |   V   | 3 minimum
             * Data                        |    3.16   |   V   |    2-X
             * Segmentation                |    3.17   |   O   |     6
             * Importance                  |    3.19   |   O   |     3
             * End of optional parameters  |    3.1    |   O   |     1
             * -----------------------------------------------------------
             * Contains four pointers
             */
            case Sccp<PKT>::T_XUDTS: {
            /* Table 20 / Q.713 - Message type: Extended unitdata service
             * -----------------------------------------------------------
             * Parameter                   | Reference | Type  |  Length
             *                             |           |(F V O)| (octets)
             * -----------------------------------------------------------
             * Message type                |    2.1    |   F   |     1
             * Return cause                |    3.12   |   F   |     1
             * Called party address        |    3.4    |   V   | 3 minimum
             * Calling party address       |    3.5    |   V   | 3 minimum
             * Data                        |    3.16   |   V   |    2-X
             * Segmentation                |    3.17   |   O   |     6
             * Importance                  |    3.19   |   O   |     3
             * End of optional parameters  |    3.1    |   O   |     1
             * -----------------------------------------------------------
             * Contains four pointers
             */
                if(sccp->mt == Sccp<PKT>::T_UDT || sccp->mt == Sccp<PKT>::T_XUDT)
                    offset += SCCP_PROTOCOL_CLASS_LENGTH;
                else
                    offset += SCCP_RETURN_CAUSE_LENGTH;

                if(sccp->mt == Sccp<PKT>::T_XUDT || sccp->mt == Sccp<PKT>::T_XUDTS)
                    offset += SCCP_HOP_COUNTER_LENGTH;

                uint8_t vParameterPtr1, vParameterPtr2, vParameterPtr3;
                VARIABLE_PARAMETER(sccp, vParameterPtr1, offset);
                VARIABLE_PARAMETER(sccp, vParameterPtr2, offset);
                VARIABLE_PARAMETER(sccp, vParameterPtr3, offset);

                uint8_t oParameterPtr;
                if (sccp->mt == Sccp<PKT>::T_XUDT || sccp->mt == Sccp<PKT>::T_XUDTS) {
                    oParameterPtr = *(sccp.getWithOffset(offset));
                    offset += SCCP_POINTER_LENGTH;
                }

                uint8_t vParameterLength1 = *(sccp.getWithOffset(offset));
                offset += SCCP_PARAMETER_LENGTH_LENGTH;
                offset += vParameterLength1;

                uint8_t vParameterLength2 = *(sccp.getWithOffset(offset));
                offset += SCCP_PARAMETER_LENGTH_LENGTH;
                uint8_t ssn = getSsn<PKT>(sccp.getWithOffset(offset), vParameterLength2);
                offset += vParameterLength2;

                uint8_t vParameterLength3 = *(sccp.getWithOffset(offset));
                offset += SCCP_PARAMETER_LENGTH_LENGTH;

                if (ssn) {
                    uint16_t length = sccp.getLength() - offset;
                    offset += sccp.getOffset();

                    Tcap<PktOffset> tcap = PktOffset(move(sccp.makeTcap().getPtr()), length, offset);

                    return static_cast<PARENT*>(this)->gotTcap(move(tcap));
                }

                offset += vParameterLength3;

                if ((sccp->mt == Sccp<PKT>::T_XUDT || sccp->mt == Sccp<PKT>::T_XUDTS) && oParameterPtr)
                    return gotOptionalParameter(move(sccp), offset);
                // length checks
                break; 
            }
        case Sccp<PKT>::T_ERR:
        case Sccp<PKT>::T_IT:  {
            // No pointers - no need to do smth with offset
            return PktOffset();
        }
        case Sccp<PKT>::T_LUDT:
        case Sccp<PKT>::T_LUDTS:
            // Class-3 is never actually used in the real world
        case Sccp<PKT>::T_DT2:
        case Sccp<PKT>::T_AK:
        case Sccp<PKT>::T_ED:
        case Sccp<PKT>::T_EA:
        case Sccp<PKT>::T_RSR:
        case Sccp<PKT>::T_RSC:
            return PktOffset();
        default:
            // Unknown message type
            return PktOffset();
        }

        return move(sccp);
    }
};
