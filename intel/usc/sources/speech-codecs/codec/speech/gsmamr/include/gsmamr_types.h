/*/////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2011 Intel Corporation. All Rights Reserved.
//
//     Intel(R) Integrated Performance Primitives
//     USC - Unified Speech Codec interface library
//
// By downloading and installing USC codec, you hereby agree that the
// accompanying Materials are being provided to you under the terms and
// conditions of the End User License Agreement for the Intel(R) Integrated
// Performance Primitives product previously accepted by you. Please refer
// to the file ippEULA.rtf or ippEULA.txt located in the root directory of your Intel(R) IPP
// product installation for more information.
//
// A speech coding standards promoted by ITU, ETSI, 3GPP and other
// organizations. Implementations of these standards, or the standard enabled
// platforms may require licenses from various entities, including
// Intel Corporation.
//
//
// Purpose: GSMAMR speech codec: types declaration.
//
*/

#ifndef __GSMAMR_TYPES_H__
#define __GSMAMR_TYPES_H__

#include <ippdefs.h>

/*************************************************************************
       GSM AMR recieved and transmitted frame types
**************************************************************************/

typedef enum  {
   RX_NON_SPEECH = -1,
   RX_SPEECH_GOOD = 0,
   RX_SPEECH_DEGRADED = 1,
   RX_ONSET = 2,
   RX_SPEECH_BAD = 3,
   RX_SID_FIRST = 4,
   RX_SID_UPDATE = 5,
   RX_SID_BAD = 6,
   RX_NO_DATA = 7,
   RX_SID_INVALID = 8,
   RX_SPEECH_BAD_TAF = 9,
   RX_N_FRAMETYPES = 10
}RXFrameType;

typedef enum  {
   TX_NON_SPEECH = -1,
   TX_SPEECH_GOOD = 0,
   TX_SID_FIRST = 1,
   TX_SID_UPDATE = 2,
   TX_NO_DATA = 3,
   TX_SPEECH_DEGRADED = 4,
   TX_SPEECH_BAD = 5,
   TX_SID_BAD = 6,
   TX_ONSET = 7,
   TX_N_FRAMETYPES = 8     /* number of frame types */
}TXFrameType;

/* GSM AMR has nine possible bit rates... */

typedef enum {
    GSMAMR_RATE_UNDEFINED = -1,
    GSMAMR_RATE_4750 = 0,  /* MR475  4.75 kbps  */
    GSMAMR_RATE_5150,      /* MR515  5.15 kbps  */
    GSMAMR_RATE_5900,      /* MR59   5.9  kbps  */
    GSMAMR_RATE_6700,      /* MR67   6.7  kbps  */
    GSMAMR_RATE_7400,      /* MR74   7.4  kbps  */
    GSMAMR_RATE_7950,      /* MR795  7.95 kbps  */
    GSMAMR_RATE_10200,     /* MR102 10.2  kbps  */
    GSMAMR_RATE_12200,     /* MR122 12.2  kbps  (GSM EFR) */
    GSMAMR_RATE_DTX,        /* MRDTX Discontinuous TX mode */
    GSMAMR_RATE_DTE        /* For AMR-EFR */ 
} GSMAMR_Rate_t;

#define EHF_MASK 0x0008        /* encoder homing frame pattern             */

/* frame size in serial bitstream file (frame type + serial stream + flags) */
#define MAX_SERIAL_SIZE 244    /* max. num. of serial bits                 */
#define BITSTREAM_SIZE  ((MAX_SERIAL_SIZE+7)>>3)  /* max. num. of serial bytes                 */
#define SERIAL_FRAMESIZE (1+MAX_SERIAL_SIZE+5)
#define  GSMAMR_Frame       160
#define N_MODES     10


#endif /* __GSMAMR_TYPES_H__ */
