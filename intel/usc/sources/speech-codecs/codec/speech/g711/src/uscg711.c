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
// Purpose: G711 speech codec: USC functions.
//
*/

#include "usc.h"
#include "owng711.h"
#include "g711api.h"

static USC_Status GetInfoSize(Ipp32s *pSize);
static USC_Status GetInfoA(USC_Handle handle, void *pInfo);
static USC_Status GetInfoU(USC_Handle handle, void *pInfo);
static USC_Status NumAlloc(const void *options, Ipp32s *nbanks);
static USC_Status MemAlloc(const void *options, USC_MemBank *pBanks);
static USC_Status InitA(const void *options, const USC_MemBank *pBanks, USC_Handle *handle);
static USC_Status InitU(const void *options, const USC_MemBank *pBanks, USC_Handle *handle);
static USC_Status ReinitA(const void *modes, USC_Handle handle );
static USC_Status ReinitU(const void *modes, USC_Handle handle );
static USC_Status ControlA(const void *modes, USC_Handle handle );
static USC_Status ControlU(const void *modes, USC_Handle handle );
static USC_Status Encode(USC_Handle handle, USC_PCMStream *in, USC_Bitstream *out);
static USC_Status Decode(USC_Handle handle, USC_Bitstream *in, USC_PCMStream *out);
static USC_Status GetOutStreamSize(const USC_Option *options, Ipp32s bitrate, Ipp32s nbytesSrc, Ipp32s *nbytesDst);
static USC_Status SetFrameSize(const USC_Option *options, USC_Handle handle, Ipp32s frameSize);

#define BITSTREAM_SIZE  80
#define G711_NUM_RATES  1
#define G711_SPEECH_FRAME         80
#define G711_BITS_PER_SAMPLE      16
#define G711_SAMPLE_FREQUENCE     8000
#define G711_NCHANNELS            1

typedef struct {
    USC_Direction   direction;
    Ipp32s   pf;
    Ipp32s   vad;
    Ipp32s   law;        /* 0 - pcm, 1 - aLaw, 2 -muLaw */
} G711_Handle_Header;

/* global usc vector table */

USC_Fxns USC_G711A_Fxns=
{
   {
      USC_Codec,
      (GetInfoSize_func) GetInfoSize,
      (GetInfo_func)     GetInfoA,
      (NumAlloc_func)    NumAlloc,
      (MemAlloc_func)    MemAlloc,
      (Init_func)        InitA,
      (Reinit_func)      ReinitA,
      (Control_func)     ControlA
   },
   Encode,
   Decode,
   GetOutStreamSize,
   SetFrameSize

 };


USC_Fxns USC_G711U_Fxns=
{
   {
      USC_Codec,
      GetInfoSize,
      GetInfoU,
      NumAlloc,
      MemAlloc,
      InitU,
      ReinitU,
      ControlU
   },
   Encode,
   Decode,
   GetOutStreamSize,
   SetFrameSize
};

static __ALIGN32 CONST USC_Rates pTblRates_G711[G711_NUM_RATES]={
    {64000}
};
static __ALIGN32 CONST USC_PCMType pcmTypesTbl_G711[1]={
   {G711_SAMPLE_FREQUENCE,G711_BITS_PER_SAMPLE, G711_NCHANNELS}
};
static USC_Status GetInfoA(USC_Handle handle, void *pInf)
{
    USC_CodecInfo *pInfo=(USC_CodecInfo *)pInf;
    G711_Handle_Header *g711_header;

    USC_CHECK_PTR(pInf);

    pInfo->name = "IPP_G711A";
    pInfo->params.framesize = G711_SPEECH_FRAME*sizeof(Ipp16s);
    if (handle == NULL) {
        pInfo->params.direction = USC_DECODE;
        pInfo->params.law = G711_ALAW_CODEC;
        pInfo->params.modes.vad = 1;
        pInfo->params.modes.pf = 1;
    } else {
        g711_header = (G711_Handle_Header*)handle;
        pInfo->params.direction = g711_header->direction;
        pInfo->params.law = g711_header->law;
        pInfo->params.modes.vad = g711_header->vad;
        pInfo->params.modes.pf = g711_header->pf;
    }
    pInfo->maxbitsize = BITSTREAM_SIZE;
    pInfo->params.pcmType.sample_frequency = pcmTypesTbl_G711[0].sample_frequency;
    pInfo->params.pcmType.nChannels = pcmTypesTbl_G711[0].nChannels;
    pInfo->params.pcmType.bitPerSample = pcmTypesTbl_G711[0].bitPerSample;
    pInfo->nPcmTypes = 1;
    pInfo->pPcmTypesTbl = pcmTypesTbl_G711;
    pInfo->params.modes.truncate = 0;
    pInfo->params.modes.hpf = 0;
    pInfo->params.modes.outMode = USC_OUT_NO_CONTROL;
    pInfo->params.modes.bitrate = 64000;
    pInfo->nRates = G711_NUM_RATES;
    pInfo->pRateTbl = (const USC_Rates *)&pTblRates_G711;
    pInfo->params.nModes = sizeof(USC_Modes)/sizeof(Ipp32s);

    return USC_NoError;
}

static USC_Status GetInfoSize(Ipp32s *pSize)
{
   USC_CHECK_PTR(pSize);

   *pSize = sizeof(USC_CodecInfo);
   return USC_NoError;
}

static USC_Status GetInfoU(USC_Handle handle, void *pInf)
{
    USC_CodecInfo *pInfo=(USC_CodecInfo *)pInf;
    G711_Handle_Header *g711_header;

    USC_CHECK_PTR(pInf);

    pInfo->name = "IPP_G711U";
    pInfo->params.framesize = G711_SPEECH_FRAME*sizeof(Ipp16s);
    if (handle == NULL) {
        pInfo->params.direction = USC_DECODE;
        pInfo->params.law = G711_ALAW_CODEC;
        pInfo->params.modes.vad = 1;
         pInfo->params.modes.pf = 1;
    } else {
        g711_header = (G711_Handle_Header*)handle;
        pInfo->params.direction = g711_header->direction;
        pInfo->params.law = g711_header->law;
        pInfo->params.modes.vad = g711_header->vad;
        pInfo->params.modes.pf = g711_header->pf;
    }
    pInfo->maxbitsize = BITSTREAM_SIZE;
    pInfo->params.pcmType.sample_frequency = pcmTypesTbl_G711[0].sample_frequency;
    pInfo->params.pcmType.nChannels = pcmTypesTbl_G711[0].nChannels;
    pInfo->params.pcmType.bitPerSample = pcmTypesTbl_G711[0].bitPerSample;
    pInfo->nPcmTypes = 1;
    pInfo->pPcmTypesTbl = pcmTypesTbl_G711;
    pInfo->params.modes.truncate = 0;
    pInfo->params.modes.hpf = 0;
    pInfo->params.modes.outMode = USC_OUT_NO_CONTROL;
    pInfo->nRates = G711_NUM_RATES;
    pInfo->pRateTbl = (const USC_Rates *)&pTblRates_G711;
    pInfo->params.modes.bitrate = 64000;
    pInfo->params.nModes = sizeof(USC_Modes)/sizeof(Ipp32s);

    return USC_NoError;
}

static USC_Status NumAlloc(const void *opt, Ipp32s *nbanks)
{
   const USC_Option *options=(const USC_Option *) opt;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(nbanks);

   *nbanks = 2;

   return USC_NoError;
}

static USC_Status MemAlloc(const void *opt, USC_MemBank *pBanks)
{
   const USC_Option *options=(const USC_Option *) opt;
   Ipp32u nbytes;
    Ipp32s scratchMemSize = 0;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(pBanks);

   pBanks[0].pMem = NULL;
   pBanks[0].align = 32;
   pBanks[0].memType = USC_OBJECT;
   pBanks[0].memSpaceType = USC_NORMAL;

   if (options->direction == USC_ENCODE) { /* encode only */
      apiG711Encoder_Alloc((Ipp32s *)&nbytes);
      pBanks[0].nbytes = nbytes+sizeof(G711_Handle_Header); /* include direction in handle */
   } else if (options->direction == USC_DECODE) { /* decode only */
      apiG711Decoder_Alloc((Ipp32s *)&nbytes);
      pBanks[0].nbytes = nbytes+sizeof(G711_Handle_Header); /* include direction in handle */
   } else {
      return USC_NoOperation;
   }

    pBanks[1].pMem = NULL;
    pBanks[1].align = 32;
    pBanks[1].memType = USC_BUFFER;
    pBanks[1].memSpaceType = USC_NORMAL;

    apiG711Codec_ScratchMemoryAlloc(&scratchMemSize);

    pBanks[1].nbytes = scratchMemSize;

   return USC_NoError;
}

static USC_Status InitA(const void *opt, const USC_MemBank *pBanks, USC_Handle *handle)
{
   const USC_Option *options=(const USC_Option *) opt;
   G711_Handle_Header *g711_header;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(pBanks);
   USC_CHECK_HANDLE(handle);
   USC_CHECK_PTR(pBanks[0].pMem);
   USC_BADARG(pBanks[0].nbytes<=0, USC_NotInitialized);
   USC_CHECK_PTR(pBanks[1].pMem);
   USC_BADARG(pBanks[1].nbytes<=0, USC_NotInitialized);
   USC_BADARG(options->modes.vad > 1, USC_UnsupportedVADType);
   USC_BADARG(options->modes.vad < 0, USC_UnsupportedVADType);

   USC_BADARG(options->pcmType.bitPerSample!=G711_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
   USC_BADARG(options->pcmType.sample_frequency!=G711_SAMPLE_FREQUENCE, USC_UnsupportedPCMType);
   USC_BADARG(options->pcmType.nChannels!=G711_NCHANNELS, USC_UnsupportedPCMType);

   USC_BADARG(options->law < 0, USC_BadArgument);

   USC_BADARG(options->modes.bitrate != 64000, USC_UnsupportedBitRate);

   *handle = (USC_Handle*)pBanks[0].pMem;
   g711_header = (G711_Handle_Header*)*handle;
   g711_header->direction = options->direction;
   g711_header->law = G711_ALAW_CODEC;
   g711_header->vad = options->modes.vad;
   g711_header->pf = options->modes.pf;

   if (options->direction == USC_ENCODE) { /* encode only */
      G711Encoder_Obj *EncObj = (G711Encoder_Obj *)((Ipp8s*)*handle + sizeof(G711_Handle_Header));
      apiG711Encoder_Init((G711Encoder_Obj*)EncObj,
            (G711Codec_Type)G711_ALAW_CODEC,(G711Encode_Mode)options->modes.vad);
      apiG711Encoder_InitBuff(EncObj, pBanks[1].pMem);
   } else if (options->direction == USC_DECODE) { /* decode only */
      G711Decoder_Obj *DecObj = (G711Decoder_Obj *)((Ipp8s*)*handle + sizeof(G711_Handle_Header));
      apiG711Decoder_Init((G711Decoder_Obj*)DecObj,
            (G711Codec_Type)G711_ALAW_CODEC);
      apiG711Decoder_InitBuff(DecObj, pBanks[1].pMem);
      apiG711Decoder_Mode(DecObj, g711_header->pf);
   } else {
      return USC_NoOperation;
   }
   return USC_NoError;
}
static USC_Status InitU(const void *opt, const USC_MemBank *pBanks, USC_Handle *handle)
{
   const USC_Option *options=(const USC_Option *) opt;
   G711_Handle_Header *g711_header;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(pBanks);
   USC_CHECK_HANDLE(handle);
   USC_CHECK_PTR(pBanks[0].pMem);
   USC_BADARG(pBanks[0].nbytes<=0, USC_NotInitialized);
   USC_CHECK_PTR(pBanks[1].pMem);
   USC_BADARG(pBanks[1].nbytes<=0, USC_NotInitialized);
   USC_BADARG(options->modes.vad > 1, USC_UnsupportedVADType);
   USC_BADARG(options->modes.vad < 0, USC_UnsupportedVADType);

   USC_BADARG(options->pcmType.bitPerSample!=G711_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
   USC_BADARG(options->pcmType.sample_frequency!=G711_SAMPLE_FREQUENCE, USC_UnsupportedPCMType);
   USC_BADARG(options->pcmType.nChannels!=G711_NCHANNELS, USC_UnsupportedPCMType);

   USC_BADARG(options->law < 0, USC_BadArgument);

   USC_BADARG(options->modes.bitrate != 64000, USC_UnsupportedBitRate);

   *handle = (USC_Handle*)pBanks[0].pMem;
   g711_header = (G711_Handle_Header*)*handle;
   g711_header->direction = options->direction;
   g711_header->law = G711_MULAW_CODEC;
   g711_header->vad = options->modes.vad;
   g711_header->pf = options->modes.pf;

   if (options->direction == USC_ENCODE) { /* encode only */
      G711Encoder_Obj *EncObj = (G711Encoder_Obj *)((Ipp8s*)*handle + sizeof(G711_Handle_Header));
      apiG711Encoder_Init((G711Encoder_Obj*)EncObj,
            (G711Codec_Type)G711_MULAW_CODEC,(G711Encode_Mode)options->modes.vad);
      apiG711Encoder_InitBuff(EncObj, pBanks[1].pMem);
   }
   else if (options->direction == USC_DECODE) { /* decode only */
      G711Decoder_Obj *DecObj = (G711Decoder_Obj *)((Ipp8s*)*handle + sizeof(G711_Handle_Header));
      apiG711Decoder_Init((G711Decoder_Obj*)DecObj,
            (G711Codec_Type)G711_MULAW_CODEC);
      apiG711Decoder_InitBuff(DecObj, pBanks[1].pMem);
      apiG711Decoder_Mode(DecObj, g711_header->pf);
   } else {
      return USC_NoOperation;
   }
   return USC_NoError;
}
static USC_Status ReinitA(const void *mode, USC_Handle handle )
{
   const USC_Modes *modes=(const USC_Modes *)mode;
   G711_Handle_Header *g711_header;

   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);

   USC_BADARG(modes->vad > 1, USC_UnsupportedVADType);
   USC_BADARG(modes->vad < 0, USC_UnsupportedVADType);

   USC_BADARG(modes->bitrate != 64000, USC_UnsupportedBitRate);

   g711_header = (G711_Handle_Header*)handle;
   g711_header->law = G711_ALAW_CODEC;
   g711_header->vad = modes->vad;
   g711_header->pf = modes->pf;

   if (g711_header->direction == USC_ENCODE) { /* encode only */
      G711Encoder_Obj *EncObj = (G711Encoder_Obj *)((Ipp8s*)handle + sizeof(G711_Handle_Header));
      apiG711Encoder_Init((G711Encoder_Obj*)EncObj, (G711Codec_Type)g711_header->law, (G711Encode_Mode)modes->vad);
   } else if (g711_header->direction == USC_DECODE) { /* decode only */
      G711Decoder_Obj *DecObj = (G711Decoder_Obj *)((Ipp8s*)handle + sizeof(G711_Handle_Header));
      apiG711Decoder_Init((G711Decoder_Obj*)DecObj, (G711Codec_Type)g711_header->law);
      apiG711Decoder_Mode(DecObj, modes->pf);
   } else {
      return USC_NoOperation;
   }
   return USC_NoError;
}
static USC_Status ReinitU(const void *mode, USC_Handle handle )
{
   const USC_Modes *modes=(const USC_Modes *)mode;
   G711_Handle_Header *g711_header;

   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);

   USC_BADARG(modes->vad > 1, USC_UnsupportedVADType);
   USC_BADARG(modes->vad < 0, USC_UnsupportedVADType);

   USC_BADARG(modes->bitrate != 64000, USC_UnsupportedBitRate);

   g711_header = (G711_Handle_Header*)handle;
   g711_header->law = G711_MULAW_CODEC;
   g711_header->vad = modes->vad;
   g711_header->pf = modes->pf;

   if (g711_header->direction == USC_ENCODE) { /* encode only */
      G711Encoder_Obj *EncObj = (G711Encoder_Obj *)((Ipp8s*)handle + sizeof(G711_Handle_Header));
      apiG711Encoder_Init((G711Encoder_Obj*)EncObj, (G711Codec_Type)g711_header->law, (G711Encode_Mode)modes->vad);
   } else if (g711_header->direction == USC_DECODE) { /* decode only */
      G711Decoder_Obj *DecObj = (G711Decoder_Obj *)((Ipp8s*)handle + sizeof(G711_Handle_Header));
      apiG711Decoder_Init((G711Decoder_Obj*)DecObj, (G711Codec_Type)g711_header->law);
      apiG711Decoder_Mode(DecObj, modes->pf);
   } else {
      return USC_NoOperation;
   }
   return USC_NoError;
}
static USC_Status ControlA(const void *mode, USC_Handle handle )
{
   const USC_Modes *modes=(const USC_Modes *)mode;
   G711_Handle_Header *g711_header;

   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);

   USC_BADARG(modes->vad > 1, USC_UnsupportedVADType);
   USC_BADARG(modes->vad < 0, USC_UnsupportedVADType);

   USC_BADARG(modes->bitrate != 64000, USC_UnsupportedBitRate);

   g711_header = (G711_Handle_Header*)handle;

   g711_header->law = G711_ALAW_CODEC;
   g711_header->vad = modes->vad;
   g711_header->pf = modes->pf;

   if(g711_header->direction == USC_ENCODE) {
      G711Encoder_Obj *EncObj = (G711Encoder_Obj *)((Ipp8s*)handle + sizeof(G711_Handle_Header));
      apiG711Encoder_Mode(EncObj, (G711Encode_Mode)g711_header->vad);
   } else if(g711_header->direction == USC_DECODE) {
      G711Decoder_Obj *DecObj = (G711Decoder_Obj *)((Ipp8s*)handle + sizeof(G711_Handle_Header));
      apiG711Decoder_Mode(DecObj, g711_header->pf);
   } else
      return USC_NoOperation;

   return USC_NoError;
}
static USC_Status ControlU(const void *mode, USC_Handle handle )
{
   const USC_Modes *modes=(const USC_Modes *)mode;
   G711_Handle_Header *g711_header;

   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);

   USC_BADARG(modes->vad > 1, USC_UnsupportedVADType);
   USC_BADARG(modes->vad < 0, USC_UnsupportedVADType);

   USC_BADARG(modes->bitrate != 64000, USC_UnsupportedBitRate);

   g711_header = (G711_Handle_Header*)handle;

   g711_header->law = G711_MULAW_CODEC;
   g711_header->vad = modes->vad;
   g711_header->pf = modes->pf;

   if(g711_header->direction == USC_ENCODE) {
      G711Encoder_Obj *EncObj = (G711Encoder_Obj *)((Ipp8s*)handle + sizeof(G711_Handle_Header));
      apiG711Encoder_Mode(EncObj, (G711Encode_Mode)g711_header->vad);
   } else if(g711_header->direction == USC_DECODE) {
      G711Decoder_Obj *DecObj = (G711Decoder_Obj *)((Ipp8s*)handle + sizeof(G711_Handle_Header));
      apiG711Decoder_Mode(DecObj, g711_header->pf);
   } else
      return USC_NoOperation;

   return USC_NoError;
}

static Ipp32s GetBitstreamSize_G711(Ipp32s frametype)
{
   if(frametype==G711_Voice_Frame) return BITSTREAM_SIZE;
   else if(frametype==G711_Untransmitted_Frame) return 0;
   else return 2;
}

static USC_Status Encode(USC_Handle handle, USC_PCMStream *in, USC_Bitstream *out)
{
   G711_Handle_Header *g711_header;
   G711Encoder_Obj *EncObj;

   USC_CHECK_PTR(in);
   USC_CHECK_PTR(out);
   USC_CHECK_HANDLE(handle);

   USC_BADARG(in->pcmType.bitPerSample!=G711_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
   USC_BADARG(in->pcmType.sample_frequency!=G711_SAMPLE_FREQUENCE, USC_UnsupportedPCMType);
   USC_BADARG(in->pcmType.nChannels!=G711_NCHANNELS, USC_UnsupportedPCMType);

   USC_BADARG(in->bitrate != pTblRates_G711[0].bitrate, USC_UnsupportedBitRate);

   USC_BADARG(NULL == out->pBuffer, USC_NoOperation);

   if(in->nbytes < G711_SPEECH_FRAME*sizeof(Ipp16s)) {
      in->nbytes = 0;
      return USC_NoOperation;
   }

   g711_header = (G711_Handle_Header*)handle;

   if(g711_header->direction != USC_ENCODE) return USC_NoOperation;

   EncObj = (G711Encoder_Obj *)((Ipp8s*)handle + sizeof(G711_Handle_Header));

   if(apiG711Encode(EncObj,(const Ipp16s*)in->pBuffer,(Ipp8u *)out->pBuffer,
       &out->frametype) != APIG711_StsNoErr){
      return USC_NoOperation;
   }
   out->bitrate = in->bitrate;
   out->nbytes = GetBitstreamSize_G711(out->frametype);
   in->nbytes = G711_SPEECH_FRAME*sizeof(Ipp16s);
   return USC_NoError;
}

static USC_Status Decode(USC_Handle handle, USC_Bitstream *in, USC_PCMStream *out)
{
   G711_Handle_Header *g711_header;
   G711Decoder_Obj *DecObj;
   Ipp32s PLC = 0;

   USC_CHECK_PTR(out);
   USC_CHECK_HANDLE(handle);

   g711_header = (G711_Handle_Header*)handle;

   if(g711_header->direction != USC_DECODE) return USC_NoOperation;

   DecObj = (G711Decoder_Obj *)((Ipp8s*)handle + sizeof(G711_Handle_Header));

   if(in == NULL) {
      PLC = 1;
   } else if(in->pBuffer==NULL) {
      PLC = 1;
   }

   if(PLC) {
      if(apiG711Decode(DecObj,(const Ipp8u*)LostFrame,G711_Bad_Frame,
                       (Ipp16s*)out->pBuffer) != APIG711_StsNoErr){
         return USC_NoOperation;
      }
      out->bitrate = pTblRates_G711[0].bitrate;
   } else {

      USC_BADARG(in->nbytes < GetBitstreamSize_G711(in->frametype), USC_BadArgument);

      if(apiG711Decode(DecObj,(const Ipp8u*)in->pBuffer,in->frametype,
                              (Ipp16s*)out->pBuffer) != APIG711_StsNoErr){
         return USC_NoOperation;
      }
      out->bitrate = in->bitrate;
      in->nbytes = GetBitstreamSize_G711(in->frametype);
   }
   out->pcmType.bitPerSample = G711_BITS_PER_SAMPLE;
   out->pcmType.nChannels = G711_NCHANNELS;
   out->pcmType.sample_frequency = G711_SAMPLE_FREQUENCE;
   out->nbytes = G711_SPEECH_FRAME*sizeof(Ipp16s);
   return USC_NoError;
}

static USC_Status CalsOutStreamSize(const void *opt, Ipp32s nbytesSrc, Ipp32s *nbytesDst)
{
   const USC_Option *options=(const USC_Option *) opt;
   Ipp32s nBlocks, nSamples;

   if(options->direction==USC_ENCODE) { /*Encode: src - PCMStream, dst - bitstream*/
      if(options->modes.vad>1) return USC_UnsupportedVADType;

      nSamples = nbytesSrc / (G711_BITS_PER_SAMPLE >> 3);
      nBlocks = nSamples / G711_SPEECH_FRAME;

      if (0 == nBlocks) return USC_NoOperation;

      if (0 != nSamples % G711_SPEECH_FRAME) {
         nBlocks++;
      }

      *nbytesDst = nBlocks * GetBitstreamSize_G711(G711_Voice_Frame);
   } else if(options->direction==USC_DECODE) {/*Decode: src - bitstream, dst - PCMStream*/
      if(options->modes.vad==0) { /*VAD off*/
         nBlocks = nbytesSrc / GetBitstreamSize_G711(G711_Voice_Frame);
      } else if(options->modes.vad==1) { /*VAD on*/
         nBlocks = nbytesSrc / GetBitstreamSize_G711(G711_SID_Frame);
      } else return USC_UnsupportedVADType;

      if (0 == nBlocks) return USC_NoOperation;

      nSamples = nBlocks * G711_SPEECH_FRAME;
      *nbytesDst = nSamples * (G711_BITS_PER_SAMPLE >> 3);
   } else if(options->direction==USC_DUPLEX) {/* Both: src - PCMStream, dst - PCMStream*/
      nSamples = nbytesSrc / (G711_BITS_PER_SAMPLE >> 3);
      nBlocks = nSamples / G711_SPEECH_FRAME;

      if (0 == nBlocks) return USC_NoOperation;

      if (0 != nSamples % G711_SPEECH_FRAME) {
         nBlocks++;
      }
      *nbytesDst = nBlocks * G711_SPEECH_FRAME * (G711_BITS_PER_SAMPLE >> 3);
   } else return USC_NoOperation;

   return USC_NoError;
}
static USC_Status GetOutStreamSize(const USC_Option *options, Ipp32s bitrate, Ipp32s nbytesSrc, Ipp32s *nbytesDst)
{
   USC_CHECK_PTR(options);
   USC_CHECK_PTR(nbytesDst);
   USC_BADARG(nbytesSrc <= 0, USC_NoOperation);

   if(bitrate != pTblRates_G711[0].bitrate) return USC_UnsupportedBitRate;

   return CalsOutStreamSize(options, nbytesSrc, nbytesDst);
}

static USC_Status SetFrameSize(const USC_Option *options, USC_Handle handle, Ipp32s frameSize)
{
   USC_CHECK_PTR(options);
   USC_CHECK_HANDLE(handle);
   USC_BADARG(frameSize <= 0, USC_BadArgument);

   return USC_NoOperation;
}

