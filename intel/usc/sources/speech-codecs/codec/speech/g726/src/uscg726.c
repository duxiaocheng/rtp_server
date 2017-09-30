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
// Purpose: G.726 speech codec: USC functions.
//
*/

#if defined( _WIN32_WCE)
#pragma warning( disable : 4505 )
#endif


#include <ipps.h>
#include <ippsc.h>
#include "usc.h"
#include "aux_fnxs.h"
#include "scratchmem.h"

#define  BITSTREAM_SIZE   5
#define  G726_NUM_RATES   4
#define  G726_FRAME_SIZE  8
#define  G726_BITS_PER_SAMPLE 16
#define  G726_SAMPLE_FREQ 8000
#define  G726_NCHANNELS   1

#define  G726_10MS_FRAME_SIZE  (G726_FRAME_SIZE*10)
#define  G726_MAX_BIT_SIZE     (BITSTREAM_SIZE*10)

static USC_Status GetInfoSize(Ipp32s *pSize);
static USC_Status GetInfo(USC_Handle handle, USC_CodecInfo *pInfo);
static USC_Status NumAlloc(const USC_Option *options, Ipp32s *nbanks);
static USC_Status MemAlloc(const USC_Option *options, USC_MemBank *pBanks);
static USC_Status Init(const USC_Option *options, const USC_MemBank *pBanks, USC_Handle *handle);
static USC_Status Reinit(const USC_Modes *modes, USC_Handle handle );
static USC_Status Control(const USC_Modes *modes, USC_Handle handle );
static USC_Status Encode(USC_Handle handle, USC_PCMStream *in, USC_Bitstream *out);
static USC_Status Decode(USC_Handle handle, USC_Bitstream *in, USC_PCMStream *out);
static USC_Status GetOutStreamSize(const USC_Option *options, Ipp32s bitrate, Ipp32s nbytesSrc, Ipp32s *nbytesDst);
static USC_Status SetFrameSize(const USC_Option *options, USC_Handle handle, Ipp32s frameSize);

typedef struct {
    USC_Direction direction;
    Ipp32s bitrate;
    Ipp32s law;
    Ipp32s reserved; // for future extension
} G726_Handle_Header;

/* global usc vector table */
USCFUN USC_Fxns USC_G726_Fxns=
{
    {
        USC_Codec,
        (GetInfoSize_func) GetInfoSize,
        (GetInfo_func)     GetInfo,
        (NumAlloc_func)    NumAlloc,
        (MemAlloc_func)    MemAlloc,
        (Init_func)        Init,
        (Reinit_func)      Reinit,
        (Control_func)     Control
    },
    Encode,
    Decode,
    GetOutStreamSize,
    SetFrameSize
};

static __ALIGN32 CONST USC_Rates pTblRates_G726[G726_NUM_RATES]={
    {40000},
    {32000},
    {24000},
    {16000}
};

static __ALIGN32 CONST USC_PCMType pcmTypesTbl_G726[1]={
   {G726_SAMPLE_FREQ,G726_BITS_PER_SAMPLE,G726_NCHANNELS}
};

static Ipp32s CheckRate_G726(Ipp32s rate_bps)
{
    Ipp32s rate;

    switch(rate_bps) {
      case 16000: rate = 0; break;
      case 24000: rate = 1; break;
      case 32000: rate = 2; break;
      case 40000: rate = 3; break;
      default: rate = -1; break;
    }
    return rate;
}

static USC_Status GetInfoSize(Ipp32s *pSize)
{
   USC_CHECK_PTR(pSize);
   *pSize = sizeof(USC_CodecInfo);
   return USC_NoError;
}

static USC_Status GetInfo(USC_Handle handle, USC_CodecInfo *pInfo)
{
    G726_Handle_Header *g726_header;

    USC_CHECK_PTR(pInfo);

    pInfo->name = "IPP_G726";
    if (handle == NULL) {
      pInfo->params.modes.bitrate = 16000;
      pInfo->params.direction = USC_DECODE;
      pInfo->params.law = 2;
    } else {
      g726_header = (G726_Handle_Header*)handle;
      pInfo->params.modes.bitrate = g726_header->bitrate;
      pInfo->params.direction = g726_header->direction;
      pInfo->params.law = g726_header->law;
    }

    pInfo->params.framesize = G726_10MS_FRAME_SIZE*sizeof(Ipp16s);
    pInfo->maxbitsize = G726_MAX_BIT_SIZE;

    pInfo->params.modes.vad = 0;
    pInfo->params.modes.truncate = 0;
    pInfo->params.modes.hpf = 0;
    pInfo->params.modes.pf = 0;
    pInfo->params.modes.outMode = USC_OUT_NO_CONTROL;
    pInfo->params.pcmType.sample_frequency = pcmTypesTbl_G726[0].sample_frequency;
    pInfo->params.pcmType.nChannels = pcmTypesTbl_G726[0].nChannels;
    pInfo->params.pcmType.bitPerSample = pcmTypesTbl_G726[0].bitPerSample;
    pInfo->nPcmTypes = 1;
    pInfo->pPcmTypesTbl = pcmTypesTbl_G726;
    pInfo->nRates = G726_NUM_RATES;
    pInfo->pRateTbl = (const USC_Rates *)&pTblRates_G726;
    pInfo->params.nModes = sizeof(USC_Modes)/sizeof(Ipp32s);

    return USC_NoError;
}

static USC_Status NumAlloc(const USC_Option *options, Ipp32s *nbanks)
{
   USC_CHECK_PTR(options);
   USC_CHECK_PTR(nbanks);
   *nbanks = 1;
   return USC_NoError;
}

static USC_Status MemAlloc(const USC_Option *options, USC_MemBank *pBanks)
{
    Ipp32u nbytes;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(pBanks);

    pBanks->pMem = NULL;
    pBanks->align = 32;
    pBanks->memType = USC_OBJECT;
    pBanks->memSpaceType = USC_NORMAL;

    if (options->direction == USC_ENCODE) /* encode only */
    {
        ippsEncodeGetStateSize_G726_16s8u(&nbytes);
    }
    else if (options->direction == USC_DECODE) /* decode only */
    {
        ippsDecodeGetStateSize_G726_8u16s(&nbytes);
    } else {
        return USC_NoOperation;
    }
    pBanks->nbytes = nbytes + sizeof(G726_Handle_Header); /* room for USC header */
    return USC_NoError;
}
static IppPCMLaw Usc2IppLaw(Ipp32s uscLaw)
{
   if(uscLaw==0) return IPP_PCM_LINEAR;
   else if(uscLaw==1) return IPP_PCM_ALAW;
   else return IPP_PCM_MULAW;
}
static __ALIGN32 CONST IppSpchBitRate usc2g726Rate[4]={
    IPP_SPCHBR_16000,
    IPP_SPCHBR_24000,
    IPP_SPCHBR_32000,
    IPP_SPCHBR_40000
};

static USC_Status Init(const USC_Option *options, const USC_MemBank *pBanks, USC_Handle *handle)
{
    G726_Handle_Header *g726_header;
    Ipp32s bitrate_idx;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(pBanks);
   USC_CHECK_PTR(pBanks->pMem);
   USC_BADARG(pBanks->nbytes<=0, USC_NotInitialized);
   USC_CHECK_HANDLE(handle);
   USC_BADARG(options->modes.vad!=0, USC_UnsupportedVADType);

   USC_BADARG(options->pcmType.bitPerSample!=G726_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
   USC_BADARG(options->pcmType.sample_frequency!=G726_SAMPLE_FREQ, USC_UnsupportedPCMType);
   USC_BADARG(options->pcmType.nChannels!=G726_NCHANNELS, USC_UnsupportedPCMType);

   USC_BADARG(options->law < 0, USC_BadArgument);
   USC_BADARG(options->law > 2, USC_BadArgument);

   bitrate_idx = CheckRate_G726(options->modes.bitrate);
   USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);

   *handle = (USC_Handle*)pBanks->pMem;
   g726_header = (G726_Handle_Header*)*handle;

    g726_header->direction = options->direction;
    g726_header->bitrate = options->modes.bitrate;
    g726_header->law = options->law;
    g726_header->reserved = 0;

    if (options->direction == USC_ENCODE) /* encode only */
    {
        IppsEncoderState_G726_16s *EncObj = (IppsEncoderState_G726_16s *)((Ipp8s*)*handle + sizeof(G726_Handle_Header));
        ippsEncodeInit_G726_16s8u(EncObj, usc2g726Rate[bitrate_idx]);
    }
    else if (options->direction == USC_DECODE) /* decode only */
    {
        IppsDecoderState_G726_16s *DecObj = (IppsDecoderState_G726_16s *)((Ipp8s*)*handle + sizeof(G726_Handle_Header));
        ippsDecodeInit_G726_8u16s(DecObj, usc2g726Rate[bitrate_idx],Usc2IppLaw(options->law));

    } else {
        *handle = NULL;
        return USC_NoOperation;
    }
    return USC_NoError;
}

static USC_Status Reinit(const USC_Modes *modes, USC_Handle handle )
{
   G726_Handle_Header *g726_header;
   Ipp32s bitrate_idx;

   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);

    g726_header = (G726_Handle_Header*)handle;

    bitrate_idx = CheckRate_G726(modes->bitrate);
    if(bitrate_idx < 0) return USC_UnsupportedBitRate;
    g726_header->bitrate = modes->bitrate;
    g726_header->reserved = 0;

    if (g726_header->direction == USC_ENCODE) /* encode only */
    {
        IppsEncoderState_G726_16s *EncObj = (IppsEncoderState_G726_16s *)((Ipp8s*)handle + sizeof(G726_Handle_Header));
        ippsEncodeInit_G726_16s8u(EncObj, usc2g726Rate[bitrate_idx]);///////////////////////////
    }
    else if (g726_header->direction == USC_DECODE) /* decode only */
    {
        IppsDecoderState_G726_16s *DecObj = (IppsDecoderState_G726_16s *)((Ipp8s*)handle + sizeof(G726_Handle_Header));
        ippsDecodeInit_G726_8u16s(DecObj, usc2g726Rate[bitrate_idx],Usc2IppLaw(g726_header->law));////////////////
    } else {
        return USC_NoOperation;
    }
    return USC_NoError;
}

static USC_Status Control(const USC_Modes *modes, USC_Handle handle )
{
   Ipp32s bitrate_idx;
   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);
   bitrate_idx = CheckRate_G726(modes->bitrate);
   USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);
   return USC_NoError;
}

static Ipp32s OutFrameSize(Ipp32s bitrate_bps)
{
   switch(bitrate_bps) {
      case 16000: return 2;
      case 24000: return 3;
      case 32000: return 4;
      case 40000: return 5;
      default: return 0;
   }
}

static void PackCodeword(const Ipp8s* inCode, Ipp8s* outCode, Ipp32s bitrate, Ipp32s inLen, Ipp32s *outLen)
{
   Ipp32s i, frmLen, packedLen = 0;
   frmLen = OutFrameSize(bitrate);
   if(frmLen==0) return;

   if(frmLen == 2) {
      for(i=0;i<inLen;i+=8) {
         outCode[0] = (Ipp8s)((inCode[3] << 6) | ((inCode[2] & 0x3) << 4) | ((inCode[1] & 0x3) << 2) | (inCode[0] & 0x3));
         outCode[1] = (Ipp8s)((inCode[7] << 6) | ((inCode[6] & 0x3) << 4) | ((inCode[5] & 0x3) << 2) | (inCode[4] & 0x3));
         outCode += 2;
         inCode += 8;
         packedLen += 2;
      }
   } else if(frmLen == 3) {
      for(i=0;i<inLen;i+=8) {
         outCode[0] = (Ipp8s)((inCode[0] & 0x7) | ((inCode[1] & 0x7)<<3) | ((inCode[2] & 0x3)<<6));
         outCode[1] = (Ipp8s)(((inCode[2] & 0x4)>>2) | ((inCode[3] & 0x7)<<1) | ((inCode[4] & 0x7)<<4) | ((inCode[5] & 0x1)<<7));
         outCode[2] = (Ipp8s)(((inCode[5] & 0x7)>>1) | ((inCode[6] & 0x7)<<2) | ((inCode[7] & 0x7)<<5));
         outCode += 3;
         inCode += 8;
         packedLen += 3;
      }
   } else if(frmLen == 4) {
      for(i=0;i<inLen;i+=8) {
         outCode[0] = (Ipp8s)((inCode[1] << 4) | (inCode[0] & 0xf));
         outCode[1] = (Ipp8s)((inCode[3] << 4) | (inCode[2] & 0xf));
         outCode[2] = (Ipp8s)((inCode[5] << 4) | (inCode[4] & 0xf));
         outCode[3] = (Ipp8s)((inCode[7] << 4) | (inCode[6] & 0xf));
         outCode += 4;
         inCode += 8;
         packedLen += 4;
      }
   } else if(frmLen == 5) {
      for(i=0;i<inLen;i+=8) {
         outCode[0] = (Ipp8s)((inCode[0] & 0x1f) | ((inCode[1] & 0x7)<<5));
         outCode[1] = (Ipp8s)(((inCode[1] & 0x18)>>3) | ((inCode[2] & 0x1f)<<2) | ((inCode[3] & 0x1)<<7));
         outCode[2] = (Ipp8s)(((inCode[3] & 0x1e)>>1) | ((inCode[4] & 0xf)<<4));
         outCode[3] = (Ipp8s)(((inCode[4] & 0x10)>>4) | ((inCode[5] & 0x1f)<<1) | ((inCode[6] & 0x3)<<6));
         outCode[4] = (Ipp8s)(((inCode[6] & 0x1c)>>2) | ((inCode[7] & 0x1f)<<3));
         outCode += 5;
         inCode += 8;
         packedLen += 5;
      }
   }
   *outLen = packedLen;
}

static USC_Status Encode(USC_Handle handle, USC_PCMStream *in, USC_Bitstream *out)
{
   G726_Handle_Header *g726_header;
   Ipp32s bitrate_idx, inputLen, outLen = 0, foo = 0;
   IPP_ALIGNED_ARRAY(16, Ipp16s, work_buf, 10*G726_FRAME_SIZE);/*10 ms*/
   IPP_ALIGNED_ARRAY(16, Ipp8u, outBuffer, 10*G726_FRAME_SIZE);/*10 ms*/
   IppsEncoderState_G726_16s *EncObj;
   Ipp16s *pPCMPtr;
   Ipp8s *pBitstrPtr;

   USC_CHECK_PTR(in);
   USC_CHECK_PTR(out);
   USC_CHECK_HANDLE(handle);
   USC_BADARG(in->pcmType.bitPerSample!=G726_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
   USC_BADARG(in->pcmType.sample_frequency!=G726_SAMPLE_FREQ, USC_UnsupportedPCMType);
   USC_BADARG(in->pcmType.nChannels!=G726_NCHANNELS, USC_UnsupportedPCMType);

   USC_BADARG( NULL==in->pBuffer, USC_NoOperation );
   USC_BADARG( NULL==out->pBuffer, USC_NoOperation );

   g726_header = (G726_Handle_Header*)handle;

   USC_BADARG(USC_ENCODE != g726_header->direction, USC_NoOperation);

   EncObj = (IppsEncoderState_G726_16s *)((Ipp8s*)handle + sizeof(G726_Handle_Header));

   inputLen = in->nbytes;
   USC_BADARG(inputLen <= 0, USC_NoOperation);

   if(in->nbytes > G726_10MS_FRAME_SIZE*sizeof(Ipp16s))
      inputLen = G726_10MS_FRAME_SIZE*sizeof(Ipp16s);
   else {
      inputLen = (in->nbytes / (G726_FRAME_SIZE*sizeof(Ipp16s))) * (G726_FRAME_SIZE*sizeof(Ipp16s));
   }

   in->nbytes = inputLen;

    if(g726_header->bitrate != in->bitrate) {
      bitrate_idx = CheckRate_G726(in->bitrate);
      USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);
      ippsEncodeInit_G726_16s8u(EncObj, usc2g726Rate[bitrate_idx]);
      g726_header->bitrate = in->bitrate;
    }

    //inputLen = in->nbytes;
    pPCMPtr = (Ipp16s*)in->pBuffer;
    pBitstrPtr = out->pBuffer;
    if(inputLen == G726_10MS_FRAME_SIZE*sizeof(Ipp16s)) {
       ippsRShiftC_16s(pPCMPtr, 2, work_buf, G726_10MS_FRAME_SIZE);
       if(ippsEncode_G726_16s8u(EncObj,work_buf,(Ipp8u*)outBuffer,G726_10MS_FRAME_SIZE) != ippStsNoErr){
         return USC_NoOperation;
       }
       PackCodeword((const Ipp8s*)outBuffer, pBitstrPtr, in->bitrate, G726_10MS_FRAME_SIZE, &foo);

       outLen += foo;
    } else {
      while(inputLen > 0) {
         ippsRShiftC_16s(pPCMPtr, 2, work_buf, G726_FRAME_SIZE);
         if(ippsEncode_G726_16s8u(EncObj,work_buf,(Ipp8u*)outBuffer,G726_FRAME_SIZE) != ippStsNoErr){
            return USC_NoOperation;
         }
         PackCodeword((const Ipp8s*)outBuffer, pBitstrPtr, in->bitrate, G726_FRAME_SIZE, &foo);
         outLen += foo;
         pBitstrPtr += foo;
         inputLen -= (G726_FRAME_SIZE*sizeof(Ipp16s));
         pPCMPtr += (G726_FRAME_SIZE);
      }
    }
    out->frametype = 0;
    out->bitrate = in->bitrate;
    out->nbytes = outLen;

    return USC_NoError;
}

static Ipp32s UnpackCodeword(const Ipp8s* inCode, Ipp8u* outCode, Ipp32s bitrate, Ipp32s bitstrLen)
{

   Ipp32s len, i;
   len = OutFrameSize(bitrate);
   if(len==0) return 0;

   switch(bitrate) {
      case 16000: {
         for(i = 0;i < bitstrLen;i+=len) {
            outCode[3] = (Ipp8s)(((inCode[0] >> 6) & 0x3));
            outCode[2] = (Ipp8s)(((inCode[0] >> 4) & 0x3));
            outCode[1] = (Ipp8s)(((inCode[0] >> 2) & 0x3));
            outCode[0] = (Ipp8s)((inCode[0] & 0x3));
            outCode[7] = (Ipp8s)(((inCode[1] >> 6) & 0x3));
            outCode[6] = (Ipp8s)(((inCode[1] >> 4) & 0x3));
            outCode[5] = (Ipp8s)(((inCode[1] >> 2) & 0x3));
            outCode[4] = (Ipp8s)((inCode[1] & 0x3));
            inCode += len;
            outCode += 8;
         }
         break;
      }
      case 24000: {
         for(i = 0;i < bitstrLen;i+=len) {
            /*outCode[0] = ((inCode[0] >> 5) & 0x7);
            outCode[1] = ((inCode[0] >> 2) & 0x7);
            outCode[2] = ((inCode[0] << 1) & 0x7) | ((inCode[1] >> 7) & 0x1);
            outCode[3] = ((inCode[1] >> 4) & 0x7);
            outCode[4] = ((inCode[1] >> 1) & 0x7);
            outCode[5] = ((inCode[1] << 2) & 0x7) | ((inCode[2] >> 6) & 0x3);
            outCode[6] = ((inCode[2] >> 3) & 0x7);
            outCode[7] = (inCode[2] & 0x7);*/
            outCode[0] = (Ipp8s)((inCode[0] & 0x7));
            outCode[1] = (Ipp8s)((inCode[0] >> 3) & 0x7);
            outCode[2] = (Ipp8s)(((inCode[0] >> 6) & 0x3) | ((inCode[1] & 0x1))<<2);
            outCode[3] = (Ipp8s)((inCode[1] >> 1) & 0x7);
            outCode[4] = (Ipp8s)((inCode[1] >> 4) & 0x7);
            outCode[5] = (Ipp8s)(((inCode[1] >> 7) & 0x1) | (inCode[2] & 0x3)<<1);
            outCode[6] = (Ipp8s)((inCode[2] >> 2) & 0x7);
            outCode[7] = (Ipp8s)((inCode[2] >> 5) & 0x7);
            inCode += len;
            outCode += 8;
         }
         break;
      }
      case 32000: {
         for(i = 0;i < bitstrLen;i+=len) {
            outCode[1] = (Ipp8s)(((inCode[0] >> 4) & 0xf));
            outCode[0] = (Ipp8s)((inCode[0] & 0xf));
            outCode[3] = (Ipp8s)(((inCode[1] >> 4) & 0xf));
            outCode[2] = (Ipp8s)((inCode[1] & 0xf));
            outCode[5] = (Ipp8s)(((inCode[2] >> 4) & 0xf));
            outCode[4] = (Ipp8s)((inCode[2] & 0xf));
            outCode[7] = (Ipp8s)(((inCode[3] >> 4) & 0xf));
            outCode[6] = (Ipp8s)((inCode[3] & 0xf));
            inCode += len;
            outCode += 8;
         }
         break;
      }
      case 40000: {
         for(i = 0;i < bitstrLen;i+=len) {
            outCode[0] = (Ipp8s)((inCode[0] & 0x1f));
            outCode[1] = (Ipp8s)(((inCode[0] >> 5) & 0x7) | ((inCode[1] & 0x3)<<3));
            outCode[2] = (Ipp8s)((inCode[1] >> 2) & 0x1f);
            outCode[3] = (Ipp8s)(((inCode[1] >> 7) & 0x1) | ((inCode[2] & 0xf)<<1));
            outCode[4] = (Ipp8s)(((inCode[2] >> 4) & 0xf) | ((inCode[3] & 0x1)<<4));
            outCode[5] = (Ipp8s)((inCode[3] >> 1) & 0x1f);
            outCode[6] = (Ipp8s)(((inCode[3] >> 6) & 0x3) | ((inCode[4] & 0x7)<<2));
            outCode[7] = (Ipp8s)((inCode[4]  >>3) & 0x1f);
            inCode += len;
            outCode += 8;
         }
         break;
      }
   }

   return 1;
}

static USC_Status Decode(USC_Handle handle, USC_Bitstream *in, USC_PCMStream *out)
{
   G726_Handle_Header *g726_header;
   Ipp32s bitrate_idx, inputLen, frmLen;
   IPP_ALIGNED_ARRAY(16, Ipp8u, inBuffer, 10*G726_FRAME_SIZE);
   IppsDecoderState_G726_16s *DecObj;
   Ipp8s *pBitstrPtr;
   Ipp16s *pPCMPtr;
   Ipp32s PLC = 0;

   USC_CHECK_PTR(out);
   USC_CHECK_HANDLE(handle);

   USC_BADARG( NULL==out->pBuffer, USC_NoOperation );

   g726_header = (G726_Handle_Header*)handle;

   USC_BADARG(USC_DECODE != g726_header->direction, USC_NoOperation);

   DecObj = (IppsDecoderState_G726_16s *)((Ipp8s*)handle + sizeof(G726_Handle_Header));

   if(in == NULL) {
      PLC = 1;
   } else if(in->pBuffer==NULL) {
      PLC = 1;
   }

   if(PLC) {
     /* Lost frame */

     if(ippsDecode_G726_8u16s(DecObj, (const Ipp8u*)LostFrame, (Ipp16s *)out->pBuffer, G726_10MS_FRAME_SIZE)!=ippStsNoErr){
        return USC_NoOperation;
     }

     out->bitrate = g726_header->bitrate;
     out->nbytes  = G726_10MS_FRAME_SIZE*sizeof(Ipp16s);
   } else {
     USC_BADARG(in->nbytes <= 0, USC_NoOperation);

     if(g726_header->bitrate != in->bitrate) {
       bitrate_idx = CheckRate_G726(in->bitrate);
       if(bitrate_idx < 0) return USC_UnsupportedBitRate;
       ippsDecodeInit_G726_8u16s(DecObj, usc2g726Rate[bitrate_idx], Usc2IppLaw(g726_header->law));
       g726_header->bitrate = in->bitrate;
     }

     frmLen = OutFrameSize(in->bitrate);

     inputLen = in->nbytes;
     in->nbytes = (inputLen / frmLen) * frmLen;

     if((Ipp32s)((in->nbytes/frmLen)*(G726_FRAME_SIZE*sizeof(Ipp16s))) > G726_10MS_FRAME_SIZE*sizeof(Ipp16s))
         in->nbytes = (G726_10MS_FRAME_SIZE*sizeof(Ipp16s) / (G726_FRAME_SIZE*sizeof(Ipp16s)))*frmLen;

     inputLen = (in->nbytes / frmLen) * G726_FRAME_SIZE;/*Convert to the unpucked bitstream len*/

     pBitstrPtr = in->pBuffer;
     pPCMPtr = (Ipp16s *)out->pBuffer;
     out->nbytes = 0;

     if(inputLen == (10*G726_FRAME_SIZE)) {/*10 ms loop*/
        UnpackCodeword((const Ipp8s*)pBitstrPtr, inBuffer, in->bitrate, 10*frmLen);

        if(ippsDecode_G726_8u16s(DecObj, (const Ipp8u*)inBuffer, pPCMPtr, 10*G726_FRAME_SIZE)!=ippStsNoErr){
            return USC_NoOperation;
        }
        out->nbytes += (10*G726_FRAME_SIZE*sizeof(Ipp16s));
     } else {
      while(inputLen >= G726_FRAME_SIZE) {/*1 ms loop*/
         UnpackCodeword((const Ipp8s*)pBitstrPtr, inBuffer, in->bitrate, frmLen);

         if(ippsDecode_G726_8u16s(DecObj, (const Ipp8u*)inBuffer, pPCMPtr, G726_FRAME_SIZE)!=ippStsNoErr){
               return USC_NoOperation;
         }
         pBitstrPtr += frmLen;
         pPCMPtr += G726_FRAME_SIZE;
         out->nbytes += (G726_FRAME_SIZE*sizeof(Ipp16s));
         inputLen -= (G726_FRAME_SIZE);
      }
     }

     out->bitrate = in->bitrate;
   }

   out->pcmType.bitPerSample = G726_BITS_PER_SAMPLE;
   out->pcmType.sample_frequency = G726_SAMPLE_FREQ;
   out->pcmType.nChannels=G726_NCHANNELS;

   return USC_NoError;
}

static USC_Status CalsOutStreamSize(const USC_Option *options, Ipp32s bitrate_bps, Ipp32s nbytesSrc, Ipp32s *nbytesDst)
{
   Ipp32s nBlocks, nSamples;

   USC_BADARG(options->modes.vad>0, USC_UnsupportedVADType);

   if(options->direction==USC_ENCODE) { /*Encode: src - PCMStream, dst - bitstream*/
      nSamples = nbytesSrc / (G726_BITS_PER_SAMPLE >> 3);
      nBlocks = nSamples / G726_10MS_FRAME_SIZE;

      if (0 == nBlocks) return USC_NoOperation;

      if (0 != nSamples % G726_10MS_FRAME_SIZE) {
         /* Add another block to hold the last compressed fragment*/
         nBlocks++;
      }

      *nbytesDst = nBlocks * OutFrameSize(bitrate_bps) * 10;
   } else if(options->direction==USC_DECODE) {/*Decode: src - bitstream, dst - PCMStream*/
      nBlocks = nbytesSrc / (OutFrameSize(bitrate_bps) * 10);

      if (0 == nBlocks) return USC_NoOperation;

      nSamples = nBlocks * G726_10MS_FRAME_SIZE;
      *nbytesDst = nSamples * (G726_BITS_PER_SAMPLE >> 3);
   } else if(options->direction==USC_DUPLEX) {/* Both: src - PCMStream, dst - PCMStream*/
      nSamples = nbytesSrc / (G726_BITS_PER_SAMPLE >> 3);
      nBlocks = nSamples / G726_10MS_FRAME_SIZE;

      if (0 == nBlocks) return USC_NoOperation;

      if (0 != nSamples % G726_10MS_FRAME_SIZE) {
         /* Add another block to hold the last compressed fragment*/
         nBlocks++;
      }
      *nbytesDst = nBlocks * G726_10MS_FRAME_SIZE * (G726_BITS_PER_SAMPLE >> 3);
   } else return USC_NoOperation;

   return USC_NoError;
}
static USC_Status GetOutStreamSize(const USC_Option *options, Ipp32s bitrate, Ipp32s nbytesSrc, Ipp32s *nbytesDst)
{
   Ipp32s bitrate_idx;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(nbytesDst);
   USC_BADARG(nbytesSrc <= 0, USC_NoOperation);

   bitrate_idx = CheckRate_G726(bitrate);
   USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);

   return CalsOutStreamSize(options, bitrate, nbytesSrc, nbytesDst);
}

USC_Status SetFrameSize(const USC_Option *options, USC_Handle handle, Ipp32s frameSize)
{

   USC_CHECK_PTR(options);
   USC_CHECK_HANDLE(handle);
   USC_BADARG(frameSize <= 0, USC_BadArgument);

   return USC_NoOperation;
}
