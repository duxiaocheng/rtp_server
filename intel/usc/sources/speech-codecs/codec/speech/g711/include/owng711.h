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
// Purpose: G711 speech codec: main own header file.
//
*/

#ifndef __OWNG711_H__
#define __OWNG711_H__

#if defined( _WIN32_WCE)
#pragma warning( disable : 4505 )
#endif

#include <ipps.h>
#include <ippsc.h>
#include "g711api.h"
#include "scratchmem.h"

#define ENC_KEY 0xecd711
#define DEC_KEY 0xdec711

#define G711_CODECFUN(type,name,arg) extern type name arg

#define LP_FRAME_DIM        80
#define LP_SUBFRAME_DIM     40
#define BWF_HARMONIC        (1<<14)
#define BWF_HARMONIC_E      (1<<13)

#define PITCH_MIN       40                    /* minimum allowed pitch, 200 Hz */
#define PITCH_MAX       120                   /* maximum allowed pitch, 66 Hz */
#define POVERLAPMAX     (PITCH_MAX >> 2)      /* maximum pitch OLA window */
#define HISTORYLEN      (PITCH_MAX*3 + POVERLAPMAX) /* history buffer length*/

typedef struct _SynthesisFilterState {
    Ipp32s nTaps;
    Ipp16s *buffer;
} SynthesisFilterState;

typedef struct _G711Coder_Obj {
    Ipp32s              objSize;
    Ipp32s              key;
    Ipp32u        mode;
    G711Codec_Type      codecType;
} G711Coder_Obj;

typedef struct { /* State of Packet Loss Concealment */
  Ipp32s     _erasecnt;                     /* consecutive erased frames */
  Ipp32s     _poverlap;                     /* overlap based on pitch */
  Ipp32s     _poffset;                      /* offset into pitch period */
  Ipp32s     _pitch;                        /* pitch estimate */
  Ipp32s     _pitchblen;                    /* current pitch buffer length */
  Ipp16s     *_pitchbufend;                 /* end of pitch buffer */
  Ipp16s     *_pitchbufstart;               /* start of pitch buffer */
  Ipp16s     _pitchbuf[HISTORYLEN];         /* buffer for cycles of speech */
  Ipp16s     _lastq[POVERLAPMAX];           /* saved last quarter wavelength */
  Ipp16s     _history[HISTORYLEN];          /* history buffer */
} G711_PLC_state;

struct _G711Decoder_Obj {
    G711Coder_Obj  objPrm;
    G711_PLC_state PLCstate;
    G711Codec_Type codecType;
    void     *g729_obj;
    Ipp32s   lastGoodFrame;
    ScratchMem_Obj Mem;
};

struct _G711Encoder_Obj {
    G711Coder_Obj  objPrm;
    G711Codec_Type codecType;
    void     *g729_obj;
    Ipp16s   inputHistory[LP_SUBFRAME_DIM];
    ScratchMem_Obj Mem;
};

void PLC_init(G711_PLC_state *PLCstate);
void PLC_dofe(G711_PLC_state *PLCstate, Ipp16s *s); /* synthesize speech for erasure */
void PLC_addtohistory(G711_PLC_state *PLCstate, Ipp16s *s);  /* add a good frame to history buffer */

#include "aux_fnxs.h"

#endif /* __OWNG711_H__ */
