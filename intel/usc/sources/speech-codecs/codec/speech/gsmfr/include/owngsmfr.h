/*/////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2004-2011 Intel Corporation. All Rights Reserved.
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
// Purpose: GSM FR 06.10: main own header filen.
//
*/

#ifndef __OWNGSMFR_H__
#define __OWNGSMFR_H__

#if defined( _WIN32_WCE)
#pragma warning( disable : 4505 )
#endif

#include <ipps.h>
#include <ippsc.h>
#include "gsmfrapi.h"
#include "scratchmem.h"

typedef struct _GSMFC_Obj {
   Ipp32s       objSize;
   GSMFR_LTP_t  cutLTPFlag;
   Ipp32s       key;
   Ipp32s       reserved;
}GSMFRCoder_Obj;

struct _GSMFREncoder_Obj {
   GSMFRCoder_Obj objPrm;

   Ipp16s         dp0[ 280 ];

   Ipp32s         L_sacf[27];
   Ipp32s         L_sav0[36];
   Ipp32s         L_lastdm;

   Ipp32s         L_av0[9];
   Ipp32s         stat;
   Ipp32s         L_av1[9];
   Ipp32s         ptch;

   Ipp16s         LARpp[2][8];

   Ipp16s         ep[50];
   Ipp16s         e_pvad;
   Ipp16s         e_acf0;

   Ipp16s         u[8];

   Ipp16s         rvad[9];
   Ipp16s         m_acf0;

   Ipp16s         rav1[9];
   Ipp16s         m_pvad;

   Ipp16s         saveLAR[32];
   Ipp16s         saveXMax[16];
   Ipp16s         sof[160];

   uchar          oldSID[33];
   Ipp16s         normrvad;
   GSMFR_VAD_t    vadMode;
   GSMFR_VAD_t    vadDirection;
   GSMFR_VAD_t    nextFrameVadMode;
   GSMFR_VAD_t    nextFrameVadDirection;
   Ipp32s         hpMem[2];
   Ipp16s         mp;
   Ipp32s         vvad,vad;
   Ipp32s         oldestLAR;
   Ipp32s         oldestxmax;
   Ipp32s         SIDAveragePeriodCount;
   Ipp32s         scalauto;
   Ipp16s         pt_sacf;        /* ??? 32 pointer on the delay line L_sacf*/
   Ipp16s         pt_sav0;        /* ??? 32 pointer on the delay line L_sav0*/
   Ipp16s         oldlagcount;    /* periodicity counter*/
   Ipp16s         veryoldlagcount;/* periodicity counter*/
   Ipp16s         e_thvad;        /* adaptive threshold exponent */
   Ipp16s         m_thvad;        /* adaptive threshold mantissa */
   Ipp16s         adaptcount;     /* counter for adaptation */
   Ipp16s         burstcount;     /* hangover flags */
   Ipp16s         hangcount;      /* hangover flags */
   Ipp16s         oldlag;         /* LTP lag memory */
   Ipp16s         tone;           /* tone detection */
   Ipp16s         scalvad;
   Ipp16s         normrav1,lagcount;
   Ipp16s         myburstcount;
   Ipp16s         idx;

};
struct _GSMFRDecoder_Obj {
   GSMFRCoder_Obj objPrm;
   Ipp16s         dp0[ 280 ];
   Ipp16s         LARpp[2][8];
   Ipp16s         v[9];
   Ipp16s         nrp;
   Ipp16s         msr;
   Ipp16s         idx;
};
#define M            10
#define DTX_HIST_SIZE 4
#define DTX_MAX_EMPTY_THRESH 50
#define DTX_ELAPSED_FRAMES_THRESH 23

#define GSMFR_CODECFUN(type, name, arg) extern type name arg
extern Ipp32s  GsmRPEDec(Ipp16s xmaxcr, Ipp16s Mcr, Ipp16s *xMcr, Ipp16s *erp);
extern void Ref2bits(const Ipp16s *parm,uchar *bitStream);
extern void Bits2Ref(uchar *bitStream, Ipp16s *parm);
extern void Ltaf(Ipp16s *dp,const Ipp16s *d,Ipp16s *e,Ipp16s bp,Ipp16s Nc);
extern void Shift_32s16s(const Ipp32s *pSrc, Ipp16s *pDst,Ipp32s len,Ipp32s shift);
extern Ipp32s  RPEGridPos( Ipp16s *xMp, Ipp16s Mc, Ipp16s *ep );
extern Ipp32s  APCMQuant( Ipp16s xmaxc, Ipp16s *exp, Ipp16s *mant);
extern Ipp32s  Ref2LAR(Ipp16s *pSrc,Ipp32s len);
extern Ipp32s  QuantCoding(Ipp16s *pSrc,Ipp32s len);
extern void RPEGrid( Ipp16s *pSrc, Ipp16s *pDst1, Ipp16s *pDst2);
extern void DecCodLAR (const Ipp16s *LARc, Ipp16s *LAR,Ipp32s len);
extern void LARp2RC(Ipp16s *LARp, Ipp32s len);
extern Ipp32s  APCMQuant2(const Ipp16s *xM, Ipp16s *xMc, Ipp16s *mant, Ipp16s *exp, Ipp16s *xmaxc);
extern void Interpolate_GSMFR_16s(const Ipp16s *pSrc1, const Ipp16s *pSrc2, Ipp16s *pDst,Ipp32s len);
extern Ipp32s  predValComp(const Ipp16s *pSrc,Ipp32s *pDst,Ipp32s len);
#define DTX_ELAPSED_FRAMES 23
#define DTX_HANG_CONST     5
#define ENC_KEY 0xecdaaa
#define DEC_KEY 0xdecaaa

#include "aux_fnxs.h"

#endif /* __OWNGSMFR_H__ */
