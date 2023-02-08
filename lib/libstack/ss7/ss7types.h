// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

/*
 * Common types of Signalling System No. 7 – Message transfer part implementation
 *
 * ITU-T Q.703 (07/96)
 * ITU-T Q.704 (07/96)
 *
 * SERIES Q: SWITCHING AND SIGNALLING
 * Specifications of Signalling System No. 7 – Message transfer part
 *
 * ----------------------------------------------------------------------------
 *
 */

#pragma once

 /*
  * ITU-T Q.704
  *
  * 14 Common characteristics of message signal unit formats
  *
  * 14.1 General
  *
  * The basic signal unit format which is common to all message signal units is
  * described in clause 2/Q.703. From the point of view of the Message Transfer
  * Part level 3 functions, common characteristics of the message signal units
  * are the presence of:
  * - the service information octet;
  * - the label, contained in the signalling information field, and, in
  *   particular, the routing label.
  *
  * 14.2 Service information octet
  *
  * The service information octet of message signal units contains the service
  * indicator and the sub-service field. The structure of the service information
  * octet is shown in Figure 13.
  * -----------------------------------------
  * | Sub-service field | Service indicator |
  * -----------------------------------------
  * |  D |  C |  B |  A |  D |  C |  B |  A |
  * -----------------------------------------
  * |         4         |         4         | First bit transmitted
  * -----------------------------------------
  *  Figure 13/Q.704 – Service information octet
  *
  *  14.2.1 Service indicator
  *
  *  The service indicator is used by signalling handling functions to perform
  *  message distribution (see 2.4) and, in some special applications, to perform
  *  message routing (see 2.3).
  *  The service indicator codes for the international signalling network are
  *  allocated as follows:
  *
  *  bits D C B A
  *       0 0 0 0 Signalling network management messages
  *       0 0 0 1 Signalling network testing and maintenance messages
  *       0 0 1 0 Spare
  *       0 0 1 1 SCCP
  *       0 1 0 0 Telephone User Part
  *       0 1 0 1 ISDN User Part
  *       0 1 1 0 Data User Part (call and circuit-related messages)
  *       0 1 1 1 Data User Part (facility registration and cancellation messages)
  *       1 0 0 0 Reserved for MTP Testing User Part
  *       1 0 0 1 Broadband ISDN User Part
  *       1 0 1 0 Satellite ISDN User Part
  *       1 0 1 1 )
  *       1 1 0 0 )
  *       1 1 0 1 ) Spare
  *       1 1 1 0 )
  *       1 1 1 1 )
  *
  *  The allocation of the service indicator codes for national signalling
  *  networks is a national matter.  *  However, it is suggested to allocate the
  *  same service indicator code to a User Part which performs
  *  similar functions as in the international network.
  */
typedef struct sio {
    uint8_t si : 4;
    uint8_t prio : 2;
    uint8_t ni : 2;
} sio_t;

typedef struct li {
    uint8_t li : 6;
    uint8_t spare : 2;
} li_t;

/*
 * ITU-T Q.704
 *
 * 2.2.2 The standard routing label has a length of 32 bits and is placed at the
 * beginning of the Signalling Information Field. Its structure appears in Figure 3.
 *
 * 2.2.3 The Destination Point Code (DPC) indicates the destination point of the
 * message. The Originating Point Code (OPC) indicates the originating point of
 * the message. The coding of these codes is pure binary. Within each field, the
 * least significant bit occupies the first position and is transmitted first.
 *
 * A unique numbering scheme for the coding of the fields will be used for the
 * signalling points of the international network, irrespective of the User Parts
 * connected to each signalling point.
 *            -------------------------------------------
 *            |                       | SLS | OPC | DPC |
 *            -------------------------------------------
 *            |                       |  4  | 14  | 14  |
 *            -------------------------------------------
 *            | Length          n × 8 |                 |
 *            | (bit)          (n ≥ 0)|  Routing label  |
 *            -------------------------------------------
 *            |                   Label                 | First bit transmitted
 *            -------------------------------------------
 *
 *   DPC - Destination Point Code
 *   OPC - Originating Point Code
 *   SLS - Signalling Link Selection
 *
 *                  Figure 3/Q.704 – Routing label structure
 */
typedef struct itu_rl {
    uint32_t dpc : 14;
    uint32_t opc : 14;
    uint32_t sls : 4;
} routing_label_t;
