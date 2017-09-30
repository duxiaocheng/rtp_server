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
// Purpose: G.722 speech codec: internal definitions.
//
*/

#ifndef __OWNG722SB_H__
#define __OWNG722SB_H__

#if defined( _WIN32_WCE)
#pragma warning( disable : 4505 )
#endif

#include <ipps.h>
#if defined( _NO_IPPSC_LIB )
#include "_ippsc.h"
#else
#include <ippsc.h>
#endif
#include "g722sb.h"
#include "g722sbapi.h"
#include "scratchmem.h"

#include "aux_fnxs.h"

#define FACT                        4                /* decimation factor for pitch analysis */
#define FACTLOG2                    2                /* log2(FACT) */
#define FEC_L_FIR_FILTER_LTP        9                /* length of decimation filter */
#define FEC_L_FIR_FILTER_LTP_M1     (FEC_L_FIR_FILTER_LTP-1) /* length of decimation filter - 1 */
#define MAXPIT                      144              /* maximal pitch lag (20ms @8kHz) => 50 Hz */
#define MAXPIT2                     (2*MAXPIT)
#define MAXPIT2P1                   (MAXPIT2+1)
#define MAXPIT_S2                   (MAXPIT/2)
#define MAXPIT_P2                   (MAXPIT+2)
#define MAXPIT_DS                   (MAXPIT/FACT)
#define MAXPIT_DSP1                 (MAXPIT_DS+1)
#define MAXPIT_DSM1                 (MAXPIT_DS-1)
#define MAXPIT2_DS                  (MAXPIT2/FACT)
#define MAXPIT2_DSM1                (MAXPIT2_DS-1)
#define MAXPIT_S2_DS                (MAXPIT_S2/FACT)
#define MINPIT                      16               /* minimal pitch lag (2ms @8kHz) => 500 Hz */
#define MINPIT_DS                   (MINPIT/FACT)
#define GAMMA                       15401            /* 0.94 in Q14 */
#define GAMMA2                      14477            /* 0.94^2 */
#define MAX_CORR_VOICED             22936
#define ORD_LPC                     8                /* LPC order */
#define HAMWINDLEN                  80                /* length of the assymetrical hamming window */
#define LEN_MEM_SPEECH              (MAXPIT2P1+ORD_LPC)
#define T0_PLUS                     2
#define CROSSFADELEN                80               /* length of crossfade (10 ms @8kHz) */
#define CROSSFADELEN16              160              /* length of crossfade (10 ms @16kHz) */
#define END_3RD_PART                320              /* attenuation range: 40ms @ 8kHz */
#define LEN_HB_MEM                  160

extern CONST short G722PLC_lpc_win_80[HAMWINDLEN];
extern CONST short G722PLC_b_hp[2];
extern CONST short G722PLC_a_hp[2];
extern CONST short tblWeightFade[CROSSFADELEN-20];
extern CONST short tblWeightInvFade[CROSSFADELEN];
extern CONST short tblWeight_UV[END_3RD_PART];
extern CONST short tblWeight_V[END_3RD_PART];
extern CONST short tblWeight_VR[END_3RD_PART];

#define   G722SB_CODECFUN(type,name,arg)                extern type name arg

#define MIN(a,b) (a < b ? a : b)
#define MAX(a,b) (a > b ? a : b)

struct _G722SB_Encoder_Obj {
   G722SB_Obj_t objPrm;

   Ipp16s  qmf_tx_delayx[SBADPCM_G722_SIZE_QMFDELAY];
   IppsEncoderState_G722_16s  *stateEnc;

};

struct _G722SB_Decoder_Obj {
  G722SB_Obj_t objPrm;

  Ipp16s   mem_speech_lb[MAXPIT2+16];                 /* lower-band speech buffer */
  Ipp16s   mem_speech_hb[LEN_HB_MEM];                 /* higher-band speech buffer */
  Ipp16s   mem_exc_lb[MAXPIT2+8];                     /* past LPC residual */
  Ipp16s   crossfade_buf[CROSSFADELEN];
  Ipp16s   qmf_rx_delayx[SBADPCM_G722_SIZE_QMFDELAY];
  Ipp16s   mem_syn[ORD_LPC];                          /* past synthesis */
  Ipp16s   lpc[ORD_LPC + 1];                          /* LPC coefficients */
  Ipp16s   t0;                                        /* pitch delay */
  Ipp16s   prev_bfi;                                  /* bad frame indicator of previous frame */
  Ipp16s   good1stFrame;
  Ipp16s   frameSize;
  Ipp16s   inc_att;
  Ipp16s   count_lb;                                  /* counter for lower-band attenuation (number of samples) */
  Ipp16s   count_hb;                                  /* counter for higher-band attenuation (number of samples) */
  Ipp16s   mem_hpf[2];
  Ipp32s   count_hpf;
  Ipp16s   *classWeight;
  IppG722SBClass  frameClass;
  IppsDecoderState_G722_16s *stateDec;
};

/******************************** own-functions ********************************/
void ownCrossCorrInv_G722_16s32s(const Ipp16s *pSrc1, const Ipp16s *pSrc2, int len, Ipp32s *pDst, int lenDst);
void ownDCRemoveFilter_G722_16s(const Ipp16s *pSrc, int len, Ipp16s *pDst);
void ownSynthesisLowbandPLC_G722_16s(G722SB_Decoder_Obj *state, Ipp16s *syn, short n);
Ipp16s* ownSynthesisHighbandPLC_G722_16s(G722SB_Decoder_Obj *state);
Ipp16s ownOpenLoopPitchSearchPLC_G722_16s(short *signal, short *maxco);

void g722Encode(G722SB_Encoder_Obj *state, short *code, char *outcode, int len);
void g722NQMFEncode(G722SB_Encoder_Obj *state, short *code, char *outcode, int len);

void g722PLCDecode(G722SB_Decoder_Obj *state, char *code, short *outcode, int len, short mode);
void g722PLCConceal(G722SB_Decoder_Obj *state, Ipp16s *outcode);
void g722NQMFDecode(G722SB_Decoder_Obj *state, char *code, short *outcode, int len, short mode);
/******************************************************************************/
#endif /* __OWNG722SB_H__ */
