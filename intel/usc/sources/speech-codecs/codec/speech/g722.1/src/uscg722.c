/*/////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2012 Intel Corporation. All Rights Reserved.
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
// Purpose: G.722.1 speech codec: USC functions.
//
*/

#include "usc.h"
#include "g722.h"
#include "g722api.h"
#include "owng722.h"

#define  G722_1_NUM_RATES  4
#define  G722_BITS_PER_SAMPLE 16
#define  G722_NCHANNELS           1

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
    Ipp32s sample_frequency; // bandwidth
    Ipp32s frameSize;
    Ipp32s nRegions;
    Ipp32s reserved0; // for future extension
    Ipp32s reserved1; // for future extension
    Ipp32s reserved2; // for future extension
} G722_Handle_Header;

/* global usc vector table */
USCFUN USC_Fxns USC_G722_Fxns=
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

static __ALIGN32 CONST USC_Rates pTblRates_G722_1[G722_1_NUM_RATES]={
    {16000},
    {24000},
    {32000},
    {48000}
};
static __ALIGN32 CONST USC_PCMType pcmTypesTbl_G722_1[2]={
   {16000, G722_BITS_PER_SAMPLE, G722_NCHANNELS},
   {32000, G722_BITS_PER_SAMPLE, G722_NCHANNELS}
};
static USC_Status GetInfoSize(Ipp32s *pSize)
{
   USC_CHECK_PTR(pSize);
   *pSize = sizeof(USC_CodecInfo);
   return USC_NoError;
}

static USC_Status GetInfo(USC_Handle handle, USC_CodecInfo *pInfo)
{
    G722_Handle_Header *g722_header;

    USC_CHECK_PTR(pInfo);

    pInfo->name = "IPP_G722.1";
    if (handle == NULL) {
      pInfo->params.modes.bitrate = 24000;
      pInfo->params.direction = USC_DECODE;
      pInfo->params.pcmType.sample_frequency = pcmTypesTbl_G722_1[0].sample_frequency;
      pInfo->params.framesize = G722_FRAMESIZE*sizeof(Ipp16s);
    } else {
      g722_header = (G722_Handle_Header*)handle;
      pInfo->params.modes.bitrate = g722_header->bitrate;
      pInfo->params.direction = g722_header->direction;
      pInfo->params.pcmType.sample_frequency = g722_header->sample_frequency;
      pInfo->params.framesize = g722_header->frameSize*sizeof(Ipp16s);
    }
    pInfo->params.modes.vad = 0;
    pInfo->params.modes.truncate = 0;
    pInfo->maxbitsize = (G722_MAX_BITS_PER_FRAME/16)*sizeof(Ipp16s);
    pInfo->params.pcmType.nChannels = pcmTypesTbl_G722_1[0].nChannels;
    pInfo->params.pcmType.bitPerSample = pcmTypesTbl_G722_1[0].bitPerSample;
    pInfo->nPcmTypes = 2;
    pInfo->pPcmTypesTbl = pcmTypesTbl_G722_1;
    pInfo->params.modes.hpf = 0;
    pInfo->params.modes.pf = 0;
    pInfo->params.modes.outMode = USC_OUT_NO_CONTROL;
    pInfo->params.law = 0;
    pInfo->nRates = G722_1_NUM_RATES;
    pInfo->pRateTbl = (const USC_Rates *)&pTblRates_G722_1;
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
        apiG722Encoder_Alloc(&nbytes);
        pBanks->nbytes = nbytes + sizeof(G722_Handle_Header); /* room for USC header */
    }
    else if (options->direction == USC_DECODE) /* decode only */
    {
        apiG722Decoder_Alloc(&nbytes);
        pBanks->nbytes = nbytes + sizeof(G722_Handle_Header); /* room for USC header */
    } else {
        return USC_NoOperation;
    }
    return USC_NoError;
}

static Ipp32s CheckRate_G7221(Ipp32s rate_bps)
{
   Ipp32s rate;

   switch(rate_bps) {
      case 24000:  rate = 0; break;
      case 32000:  rate = 1; break;
      case 16000:  rate = 2; break;
      case 48000:  rate = 3; break;
      default: rate = -1; break;
   }
   return rate;
}

static USC_Status Init(const USC_Option *options, const USC_MemBank *pBanks, USC_Handle *handle)
{
   G722_Handle_Header *g722_header;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(pBanks);
   USC_CHECK_PTR(pBanks->pMem);
   USC_BADARG(pBanks->nbytes<=0, USC_NotInitialized);
   USC_CHECK_HANDLE(handle);

   USC_BADARG(options->pcmType.bitPerSample!=G722_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
   USC_BADARG(options->pcmType.nChannels!=G722_NCHANNELS, USC_UnsupportedPCMType);
   USC_BADARG(CheckRate_G7221(options->modes.bitrate) < 0, USC_UnsupportedBitRate);

   *handle = (USC_Handle*)pBanks->pMem;
   g722_header = (G722_Handle_Header*)*handle;

   if (options->pcmType.sample_frequency == 16000) {
      g722_header->frameSize = G722_FRAMESIZE;
      g722_header->nRegions = REG_NUM;
   } else if (options->pcmType.sample_frequency == 32000) {
      g722_header->frameSize = G722_FRAMESIZE_32kHz;
      g722_header->nRegions = REG_NUM_14kHz;
   } else {
      *handle = NULL;
      return USC_UnsupportedPCMType;
   }
   g722_header->sample_frequency = options->pcmType.sample_frequency;
   g722_header->direction = options->direction;
   g722_header->bitrate = options->modes.bitrate;

   if (options->direction == USC_ENCODE) { /* encode only */
      G722Encoder_Obj *EncObj = (G722Encoder_Obj *)((Ipp8s*)*handle + sizeof(G722_Handle_Header));
      apiG722Encoder_Init(EncObj);
   } else if (options->direction == USC_DECODE) { /* decode only */
      G722Decoder_Obj *DecObj = (G722Decoder_Obj *)((Ipp8s*)*handle + sizeof(G722_Handle_Header));
      apiG722Decoder_Init(DecObj);
   } else {
      *handle = NULL;
      return USC_NoOperation;
   }
   return USC_NoError;
}

static USC_Status Reinit(const USC_Modes *modes, USC_Handle handle )
{
    G722_Handle_Header *g722_header;

   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);

   USC_BADARG(CheckRate_G7221(modes->bitrate) < 0, USC_UnsupportedBitRate);

    g722_header = (G722_Handle_Header*)handle;
    g722_header->bitrate = modes->bitrate;

    if (g722_header->direction == USC_ENCODE) /* encode only */
    {
        G722Encoder_Obj *EncObj = (G722Encoder_Obj *)((Ipp8s*)handle + sizeof(G722_Handle_Header));
        apiG722Encoder_Init(EncObj);
    }
    else if (g722_header->direction == USC_DECODE) /* decode only */
    {
        G722Decoder_Obj *DecObj = (G722Decoder_Obj *)((Ipp8s*)handle + sizeof(G722_Handle_Header));
        apiG722Decoder_Init(DecObj);
    } else {
        return USC_NoOperation;
    }
    return USC_NoError;
}

static USC_Status Control(const USC_Modes *modes, USC_Handle handle )
{
   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);

   USC_BADARG(CheckRate_G7221(modes->bitrate) < 0, USC_UnsupportedBitRate);

   return USC_NoError;
}

static USC_Status Encode(USC_Handle handle, USC_PCMStream *in, USC_Bitstream *out)
{
    G722_Handle_Header *g722_header;
    G722Encoder_Obj *EncObj;

    USC_CHECK_PTR(in);
    USC_CHECK_PTR(out);
    USC_CHECK_HANDLE(handle);

    USC_BADARG(in->nbytes<G722_FRAMESIZE*sizeof(Ipp16s), USC_NoOperation);

    USC_BADARG(in->pcmType.bitPerSample!=G722_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
    USC_BADARG((in->pcmType.sample_frequency!=16000) &&
       (in->pcmType.sample_frequency!=32000), USC_UnsupportedPCMType);
    USC_BADARG(in->pcmType.nChannels!=G722_NCHANNELS, USC_UnsupportedPCMType);

    USC_BADARG(CheckRate_G7221(in->bitrate) < 0, USC_UnsupportedBitRate);

    USC_BADARG(NULL == in->pBuffer, USC_NoOperation);
    USC_BADARG(NULL == out->pBuffer, USC_NoOperation);

    g722_header = (G722_Handle_Header*)handle;

    if (USC_ENCODE != g722_header->direction) return USC_NoOperation;
    g722_header->bitrate = in->bitrate;
    EncObj = (G722Encoder_Obj *)((Ipp8s*)handle + sizeof(G722_Handle_Header));
    if(apiG722Encode(EncObj, in->bitrate/50, g722_header->frameSize, g722_header->nRegions,
       (Ipp16s*)in->pBuffer,(Ipp16s*)out->pBuffer) != APIG722_StsNoErr) {
       return USC_NoOperation;
    }

    out->frametype = 0;
    out->nbytes = (in->bitrate/50/8);
    out->bitrate = in->bitrate;

    in->nbytes = g722_header->frameSize*sizeof(Ipp16s);

    return USC_NoError;
}

static USC_Status Decode(USC_Handle handle, USC_Bitstream *in, USC_PCMStream *out)
{
   G722_Handle_Header *g722_header;
   G722Decoder_Obj *DecObj;
   Ipp32s PLC=0;

   USC_CHECK_PTR(out);
   USC_CHECK_HANDLE(handle);
   USC_BADARG(NULL == out->pBuffer, USC_NoOperation);

   g722_header = (G722_Handle_Header*)handle;

   if (USC_DECODE != g722_header->direction) return USC_NoOperation;

   DecObj = (G722Decoder_Obj *)((Ipp8s*)handle + sizeof(G722_Handle_Header));

   if(NULL == in) {
      PLC = 1;
   } else if(NULL == in->pBuffer) {
      PLC = 1;
   }
   if(PLC) {
      if(apiG722Decode(DecObj,1, g722_header->bitrate/50, g722_header->frameSize, g722_header->nRegions,
         (Ipp16s*)LostFrame, (Ipp16s*)out->pBuffer)!=APIG722_StsNoErr) {
         return USC_NoOperation;
      }
      out->bitrate = g722_header->bitrate;
   } else {
      USC_BADARG(NULL == in->pBuffer, USC_NoOperation);
      USC_BADARG(in->nbytes<=0, USC_NoOperation);

      g722_header->bitrate = in->bitrate;
      if(apiG722Decode(DecObj, (Ipp16s)(in->frametype), in->bitrate/50, g722_header->frameSize, g722_header->nRegions,
         (Ipp16s*)in->pBuffer, (Ipp16s*)out->pBuffer)!=APIG722_StsNoErr) {
         return USC_NoOperation;
      }
      out->bitrate = in->bitrate;
      in->nbytes = (in->bitrate/50/8);
   }
   out->nbytes = g722_header->frameSize*sizeof(Ipp16s);
   out->pcmType.bitPerSample = G722_BITS_PER_SAMPLE;
   out->pcmType.sample_frequency = g722_header->sample_frequency;
   out->pcmType.nChannels = G722_NCHANNELS;
   ippsAndC_16u((Ipp16u*)out->pBuffer, 0xfffc, (Ipp16u*)out->pBuffer, g722_header->frameSize);
   return USC_NoError;
}

static USC_Status CalsOutStreamSize(const USC_Option *options, Ipp32s bitrate, Ipp32s nbytesSrc, Ipp32s *nbytesDst)
{
   Ipp32s nBlocks, nSamples;

   if(options->modes.vad>0) return USC_UnsupportedVADType;

   if(options->direction==USC_ENCODE) { /*Encode: src - PCMStream, dst - bitstream*/
      nSamples = nbytesSrc / (G722_BITS_PER_SAMPLE >> 3);
      nBlocks = nSamples / G722_FRAMESIZE;

      if (0 == nBlocks) return USC_NoOperation;

      if (0 != nSamples % G722_FRAMESIZE) {
         /* Add another block to hold the last compressed fragment*/
         nBlocks++;
      }

      *nbytesDst = nBlocks * (bitrate/50/8);
   } else if(options->direction==USC_DECODE) {/*Decode: src - bitstream, dst - PCMStream*/
      nBlocks = nbytesSrc / (bitrate/50/8);

      if (0 == nBlocks) return USC_NoOperation;

      nSamples = nBlocks * G722_FRAMESIZE;
      *nbytesDst = nSamples * (G722_BITS_PER_SAMPLE >> 3);
   } else if(options->direction==USC_DUPLEX) {/* Both: src - PCMStream, dst - PCMStream*/
      nSamples = nbytesSrc / (G722_BITS_PER_SAMPLE >> 3);
      nBlocks = nSamples / G722_FRAMESIZE;

      if (0 == nBlocks) return USC_NoOperation;

      if (0 != nSamples % G722_FRAMESIZE) {
         /* Add another block to hold the last compressed fragment*/
         nBlocks++;
      }
      *nbytesDst = nBlocks * G722_FRAMESIZE * (G722_BITS_PER_SAMPLE >> 3);
   } else return USC_NoOperation;

   return USC_NoError;
}
static USC_Status GetOutStreamSize(const USC_Option *options, Ipp32s bitrate, Ipp32s nbytesSrc, Ipp32s *nbytesDst)
{
   Ipp32s bitrate_idx;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(nbytesDst);
   USC_BADARG(nbytesSrc <= 0, USC_NoOperation);

   bitrate_idx = CheckRate_G7221(bitrate);
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
