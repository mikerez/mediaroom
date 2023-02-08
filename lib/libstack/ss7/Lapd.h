// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

/*
 * Link Access Procedures on the D-channel (LAPD) - Q.921
 *
 */
#pragma once

#include "HeaderIf.h"

/*
 * 2 Frame structure for peer-to-peer communication
 *
 * Each frame consists of the following fields
 * (transmission sequence left to right):
 *
 *  Flag     | Address |  Control  | Info. | FCS     | Flag
 *  ---------------------------------------------------------
 *  01111110 | 16 bits | 8/16 bits | *     | 16 bits | 01111110
 *
 *  An unspecified number of bits which in some cases may be a multiple of a
 *  particular character size; for example, an octet.
 *  where
 *      Flag = flag sequence
 *      Address = data station address field
 *      Control = control field
 *      Information = information field
 *      FCS = frame checking sequence field
 */

namespace LapdHdrTypes
{
/*
 * 3.2 Address field format
 * The address field format shown in Figure 5 contains the address field
 * extension bits, a command/response indication bit, a data link layer
 * Service Access Point Identifier (SAPI) subfield, and a Terminal Endpoint
 * Identifier (TEI) subfield
 *
 *          Figure 5/Q.921 – Address field format
 *           -----------------------------------
 *          | 8 | 7 | 6 | 5 | 4 | 3 |  2  |  1  |
 *           -----------------------------------
 *  Octet 1 |         SAPI          | C/R | EA0 |
 *           -----------------------------------
 *  Octet 2 |            TEI              | EA1 |
 *           -----------------------------------
 *
 *  EA    Address field extension bit
 *  C/R   Command/response field bit
 *  SAPI  Service Access Point Identifier
 *  TEI   Terminal Endpoint Identifier
 */
    struct address_t
    {
        struct
        {
            uint8_t ea0  : 1; // always 0
            uint8_t cr   : 1;
            uint8_t sapi : 6;
        } octet_1;
        struct
        {
            uint8_t ea1  : 1; // always 1
            uint8_t tei  : 7;
        } octet_2;

        uint8_t sapi() const    { return octet_1.sapi; }
    };

/*
 * 3.4 Control field formats
 * The control field identifies the type of frame which will be either
 * a command or response. The control field will contain sequence numbers,
 * where applicable
 * Three types of control field formats are specified:
 *  - numbered information transfer (I format)
 *  - supervisory functions (S format)
 *  - unnumbered information transfers and control functions (U format)
 *
 *           Table 4/Q.921 – Control field formats
 *           -----------------------------------------------------------
 *          |                      | 8 | 7 | 6 |  5  | 4 | 3 | 2 |  1  |
 *           -----------------------------------------------------------
 *  Octet 1 |       I format       |             N(S)            |  0  |
 *  Octet 2 |                      |             N(R)            |  1  |
 *           -----------------------------------------------------------
 *  Octet 1 |       S format       | X | X | X |  X  | S | S | 0 |  1  |
 *  Octet 2 |                      |             N(R)            | P/F |
 *           -----------------------------------------------------------
 *  Octet 1 |       U format       | M | M | M | P/F | M | M | 1 |  1  |
 *           -----------------------------------------------------------
 *
 *  N(S)  Transmitter send sequence number
 *  N(R)  Transmitter receive sequence number
 *  S     Supervisory function bit
 *  M     Modifier function bit
 *  P/F   Poll bit when issued as a command, final bit when issued as a response
 *  X     Reserved and set to 0
 */
    struct control_t
    {
        struct
        {
            uint8_t bit_1  : 1;
            uint8_t bit_2  : 1;
            uint8_t unused : 6;
        } octet_1; // octet 1 only for check format

        uint8_t bit1() const    { return octet_1.bit_1; }
        uint8_t bit2() const    { return octet_1.bit_2; }

        bool is_I_type() const  { return bit1() == 0; }
        bool is_S_type() const  { return bit1() == 1 && bit2() == 0; }
        bool is_U_type() const  { return bit1() == 1 && bit2() == 1; }
    };

    struct control_I_t
    {
        struct
        {
            uint8_t bit_1 : 1; // always 0
            uint8_t ns    : 7;
        } octet_1;
        struct
        {
            uint8_t bit_1 : 1; // always 1
            uint8_t nr    : 7;
        } octet_2;

        bool check_type() const { return ((const control_t *)this)->is_I_type(); }
    };

    struct control_S_t
    {
        struct
        {
            uint8_t bit_1 : 1; // always 1
            uint8_t bit_2 : 1; // always 0
            uint8_t s     : 2;
            uint8_t x     : 4; // always 0s
        } octet_1;
        struct
        {
            uint8_t pf    : 1;
            uint8_t nr    : 7;
        } octet_2;

        bool check_type() const { return ((const control_t *)this)->is_S_type(); }
    };

    struct control_U_t
    {
        struct
        {
            uint8_t bit_1 : 1; // always 1
            uint8_t bit_2 : 1; // always 1
            uint8_t m_34  : 2;
            uint8_t pf    : 1;
            uint8_t m_678 : 3;
        } octet_1;

        bool check_type() const { return ((const control_t *)this)->is_U_type(); }
    };
};

BUILD_ASSERT(sizeof(LapdHdrTypes::address_t)   == 2);
BUILD_ASSERT(sizeof(LapdHdrTypes::control_t)   == 1);
BUILD_ASSERT(sizeof(LapdHdrTypes::control_I_t) == 2);
BUILD_ASSERT(sizeof(LapdHdrTypes::control_S_t) == 2);
BUILD_ASSERT(sizeof(LapdHdrTypes::control_U_t) == 1);

struct LapdHdr
{
    typedef LapdHdrTypes::address_t address_t;
    typedef LapdHdrTypes::control_t control_t;

    address_t   address;
    control_t   control;

    uint8_t sapi()  const   { return address.sapi(); }
    uint8_t bit1() const    { return control.bit1(); }
    uint8_t bit2() const    { return control.bit2(); }

    bool is_q931() const    { return sapi() == 0;  }
    bool is_x25()  const    { return sapi() == 16; }

    bool is_I_type() const  { return control.is_I_type(); }
    bool is_S_type() const  { return control.is_S_type(); }
    bool is_U_type() const  { return control.is_U_type(); }
};

template< typename CONTROL_TYPE >
struct LapdHdrTempl
{
    typedef LapdHdrTypes::address_t address_t;
    typedef CONTROL_TYPE            control_t;

    address_t   address;
    control_t   control;

    bool is_q931() const    { return ((const LapdHdr *)this)->is_q931(); }
    bool check_type() const { return control.check_type(); }
};

typedef LapdHdrTempl<LapdHdrTypes::control_I_t> LapdHdrI;
typedef LapdHdrTempl<LapdHdrTypes::control_S_t> LapdHdrS;
typedef LapdHdrTempl<LapdHdrTypes::control_U_t> LapdHdrU;

BUILD_ASSERT(sizeof(LapdHdr)  == 3);
BUILD_ASSERT(sizeof(LapdHdrI) == 4);
BUILD_ASSERT(sizeof(LapdHdrS) == 4);
BUILD_ASSERT(sizeof(LapdHdrU) == 3);

template<class SUBLAYER>
struct Lapd : public HeaderIf<LapdHdrI,Lapd<SUBLAYER>,SUBLAYER>
{
    typedef HeaderIf<LapdHdrI, Lapd<SUBLAYER>, SUBLAYER> Sub;
    Lapd(SUBLAYER&& pkt) : Sub(move(pkt)) {
    }

    uint16_t getHash() const
    {
        return getStmE1IdBase();
    }

    uint16_t getOpc() const
    {
        return stmE1IdToPc(getStmE1Id());
    }

    uint16_t getDpc() const
    {
        return stmE1IdToPc(getStmE1IdPair());
    }

    uint16_t getCic() const
    {
        return getTs();
    }

private:
    uint16_t getStmE1Id() const
    {
        return Sub::get()->chanid >> 5;
    }

    uint16_t getTs() const
    {
        return Sub::get()->chanid % 32;
    }

    uint16_t getStmE1IdBase() const
    {
        uint16_t stmE1Id = getStmE1Id();
        return stmE1Id >= (16 * 63) ? stmE1Id - (16 * 63) : stmE1Id;
    }

    uint16_t getStmE1IdPair() const
    {
        uint16_t stmE1Id = getStmE1Id();
        return stmE1Id >= (16 * 63) ? stmE1Id - (16 * 63) : stmE1Id + (16 * 63);
    }

    static uint16_t stmE1IdToPc(uint16_t stmE1Id)
    {
        return stmE1Id | 0x4000;
    }
};
