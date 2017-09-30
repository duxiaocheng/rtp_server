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
// Purpose: G.723.1 speech codec: USC functions.
//
*/

#include "usc.h"
#include "owng723.h"
#include "g723api.h"


#define BITSTREAM_SIZE            24
#define G723_SPEECH_FRAME        240
#define G723_BITS_PER_SAMPLE      16
#define G723_UNTR_FRAMESIZE        1
#define G723_NUM_RATES             2
#define G723_SAMPLE_FREQUENCE   8000
#define G723_NCHANNELS             1


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
    Ipp32s vad;
    Ipp32s hpf;
    Ipp32s pf;
    Ipp32s reserved0; // for future extension
    Ipp32s reserved1; // for future extension
    Ipp32s reserved2; // for future extension
} G723_Handle_Header;

/* global usc vector table */
USCFUN USC_Fxns USC_G723_Fxns=
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

static __ALIGN32 CONST USC_Rates pTblRates_G723[G723_NUM_RATES]={
    {6300},
    {5300}
};
static __ALIGN32 CONST USC_PCMType pcmTypesTbl_G723[1]={
   {G723_SAMPLE_FREQUENCE,G723_BITS_PER_SAMPLE,G723_NCHANNELS}
};

static Ipp32s CheckRate_G723(Ipp32s rate_bps)
{
    Ipp32s rate;

    switch(rate_bps) {
      case 6300:  rate = 0; break;
      case 5300:  rate = 1; break;
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
   G723_Handle_Header *g723_header;

   USC_CHECK_PTR(pInfo);

   pInfo->name = "IPP_G723.1";
   pInfo->params.framesize = G723_SPEECH_FRAME*sizeof(Ipp16s);
   if (handle == NULL) {
      pInfo->params.modes.bitrate = 6300;
      pInfo->params.direction = USC_DECODE;
      pInfo->params.modes.vad = 1;
      pInfo->params.modes.hpf = 1;
      pInfo->params.modes.pf = 1;
   } else {
      g723_header = (G723_Handle_Header*)handle;
      pInfo->params.modes.bitrate = g723_header->bitrate;
      pInfo->params.direction = g723_header->direction;
      pInfo->params.modes.vad = g723_header->vad;
      pInfo->params.modes.hpf = g723_header->hpf;
      pInfo->params.modes.pf = g723_header->pf;
   }
   pInfo->params.modes.truncate = 0;
   pInfo->params.modes.outMode = USC_OUT_NO_CONTROL;
   pInfo->maxbitsize = BITSTREAM_SIZE*sizeof(Ipp16s);
   pInfo->params.pcmType.sample_frequency = pcmTypesTbl_G723[0].sample_frequency;
   pInfo->params.pcmType.nChannels = pcmTypesTbl_G723[0].nChannels;
   pInfo->params.pcmType.bitPerSample = pcmTypesTbl_G723[0].bitPerSample;
   pInfo->nPcmTypes = 1;
   pInfo->pPcmTypesTbl = pcmTypesTbl_G723;
   pInfo->params.law = 0;
   pInfo->nRates = G723_NUM_RATES;
   pInfo->pRateTbl = (const USC_Rates *)&pTblRates_G723;
   pInfo->params.nModes = sizeof(USC_Modes)/sizeof(Ipp32s);

   return USC_NoError;
}

static USC_Status NumAlloc(const USC_Option *options, Ipp32s *nbanks)
{
   USC_CHECK_PTR(options);
   USC_CHECK_PTR(nbanks);

   *nbanks = 2;

   return USC_NoError;
}

static USC_Status MemAlloc(const USC_Option *options, USC_MemBank *pBanks)
{
   Ipp32s nbytes;
    Ipp32s scratchMemSize = 0;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(pBanks);

    pBanks[0].pMem = NULL;
    pBanks[0].align = 32;
    pBanks[0].memType = USC_OBJECT;
    pBanks[0].memSpaceType = USC_NORMAL;

    if (options->direction == USC_ENCODE) /* encode only */
    {
        apiG723Encoder_Alloc(&nbytes);
        pBanks[0].nbytes = nbytes+sizeof(G723_Handle_Header); /* include direction in handle */
    }
    else if (options->direction == USC_DECODE) /* decode only */
    {
        apiG723Decoder_Alloc(&nbytes);
        pBanks[0].nbytes = nbytes+sizeof(G723_Handle_Header); /* include direction in handle */
    } else {
        return USC_NoOperation;
    }

    pBanks[1].pMem = NULL;
    pBanks[1].align = 32;
    pBanks[1].memType = USC_BUFFER;
    pBanks[1].memSpaceType = USC_NORMAL;

    apiG723Codec_ScratchMemoryAlloc(&scratchMemSize);

    pBanks[1].nbytes = scratchMemSize;

    return USC_NoError;
}

static USC_Status Init(const USC_Option *options, const USC_MemBank *pBanks, USC_Handle *handle)
{
   Ipp32s mode=0;
   Ipp32s bitrate_idx;
   G723_Handle_Header *g723_header;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(pBanks);
   USC_CHECK_HANDLE(handle);
   USC_CHECK_PTR(pBanks[0].pMem);
   USC_BADARG(pBanks[0].nbytes<=0, USC_NotInitialized);
   USC_CHECK_PTR(pBanks[1].pMem);
   USC_BADARG(pBanks[1].nbytes<=0, USC_NotInitialized);
   USC_BADARG(options->modes.vad > 1, USC_UnsupportedVADType);
   USC_BADARG(options->modes.vad < 0, USC_UnsupportedVADType);

   USC_BADARG(options->pcmType.bitPerSample!=G723_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
   USC_BADARG(options->pcmType.sample_frequency!=G723_SAMPLE_FREQUENCE, USC_UnsupportedPCMType);
   USC_BADARG(options->pcmType.nChannels!=G723_NCHANNELS, USC_UnsupportedPCMType);

   bitrate_idx = CheckRate_G723(options->modes.bitrate);
   USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);

   *handle = (USC_Handle*)pBanks[0].pMem;
   g723_header = (G723_Handle_Header*)*handle;

     g723_header->hpf = options->modes.hpf;
     g723_header->pf = options->modes.pf;
     g723_header->vad = options->modes.vad;
     g723_header->bitrate = options->modes.bitrate;
    g723_header->direction = options->direction;

    if (options->direction == USC_ENCODE) /* encode only */
    {
      G723Encoder_Obj *EncObj = (G723Encoder_Obj *)((Ipp8s*)*handle + sizeof(G723_Handle_Header));
      apiG723Encoder_InitBuff(EncObj, pBanks[1].pMem);

        if(options->modes.hpf) mode |= 2;
        if(options->modes.vad) mode |= 1;
        apiG723Encoder_Init(EncObj, mode);
    }
    else if (options->direction == USC_DECODE) /* decode only */
    {
      G723Decoder_Obj *DecObj = (G723Decoder_Obj *)((Ipp8s*)*handle + sizeof(G723_Handle_Header));
      apiG723Decoder_InitBuff(DecObj, pBanks[1].pMem);

        if(options->modes.pf) mode = 1;
        apiG723Decoder_Init(DecObj, mode);
    } else {
      *handle = NULL;
      return USC_NoOperation;
    }
    return USC_NoError;
}

static USC_Status Reinit(const USC_Modes *modes, USC_Handle handle )
{
     Ipp32s mode=0;
     Ipp32s bitrate_idx;
    G723_Handle_Header *g723_header;

   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);
   USC_BADARG(modes->vad > 1, USC_UnsupportedVADType);
   USC_BADARG(modes->vad < 0, USC_UnsupportedVADType);

    g723_header = (G723_Handle_Header*)handle;

     bitrate_idx = CheckRate_G723(modes->bitrate);
     if(bitrate_idx < 0) return USC_UnsupportedBitRate;
     g723_header->hpf = modes->hpf;
     g723_header->pf = modes->pf;
     g723_header->vad = modes->vad;
     g723_header->bitrate = modes->bitrate;

    if (g723_header->direction == USC_ENCODE) /* encode only */
    {
      G723Encoder_Obj *EncObj = (G723Encoder_Obj *)((Ipp8s*)handle + sizeof(G723_Handle_Header));
        if(modes->hpf) mode |= 2;
        if(modes->vad) mode |= 1;
        apiG723Encoder_Init(EncObj, mode);
    }
    else if (g723_header->direction == USC_DECODE) /* decode only */
    {
      G723Decoder_Obj *DecObj = (G723Decoder_Obj *)((Ipp8s*)handle + sizeof(G723_Handle_Header));
        if(modes->pf) mode = 1;
        apiG723Decoder_Init(DecObj, mode);
    } else {
        return USC_NoOperation;
    }
    return USC_NoError;
}

static USC_Status Control(const USC_Modes *modes, USC_Handle handle )
{
   Ipp32s mode=0;
   Ipp32s bitrate_idx;
   G723_Handle_Header *g723_header;

   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);
   USC_BADARG(modes->vad > 1, USC_UnsupportedVADType);
   USC_BADARG(modes->vad < 0, USC_UnsupportedVADType);

   g723_header = (G723_Handle_Header*)handle;

   bitrate_idx = CheckRate_G723(modes->bitrate);
   if(bitrate_idx < 0) return USC_UnsupportedBitRate;

   g723_header->hpf = modes->hpf;
   g723_header->pf = modes->pf;
   g723_header->vad = modes->vad;
   g723_header->bitrate = modes->bitrate;

   if (g723_header->direction == USC_ENCODE) { /* encode only */
      G723Encoder_Obj *EncObj = (G723Encoder_Obj *)((Ipp8s*)handle + sizeof(G723_Handle_Header));
      if(modes->hpf) mode |= 2;
      if(modes->vad) mode |= 1;
      apiG723Encoder_ControlMode(EncObj, mode);
   } else if (g723_header->direction == USC_DECODE) {/* decode only */
      G723Decoder_Obj *DecObj = (G723Decoder_Obj *)((Ipp8s*)handle + sizeof(G723_Handle_Header));
      if(modes->pf) mode = 1;
      apiG723Decoder_ControlMode(DecObj, mode);
   }
   return USC_NoError;
}

static Ipp32s getBitstreamSize(Ipp8s *pBitstream)
{
    Ipp16s Info,size ;

    Info = (Ipp16s)(pBitstream[0] & (Ipp8s)0x3) ;
    /* Check frame type and rate information */
     size=24;
    switch (Info) {
        case 0x0002 : {   /* SID frame */
            size=4;
            break;
        }
        case 0x0003 : {  /* untransmitted silence frame */
            size=1;
            break;
        }
        case 0x0001 : {   /* active frame, low rate */
            size=20;
            break;
        }
        default : {  /* active frame, high rate */
            break;
        }
    }

    return size;
}

static USC_Status Encode(USC_Handle handle, USC_PCMStream *in, USC_Bitstream *out)
{
    G723_Handle_Header *g723_header;
    Ipp32s bitrate_idx;
    G723Encoder_Obj *EncObj;

    USC_CHECK_PTR(in);
    USC_CHECK_PTR(out);
    USC_CHECK_HANDLE(handle);

    USC_BADARG(in->pcmType.bitPerSample!=G723_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
    USC_BADARG(in->pcmType.sample_frequency!=G723_SAMPLE_FREQUENCE, USC_UnsupportedPCMType);
    USC_BADARG(in->pcmType.nChannels!=G723_NCHANNELS, USC_UnsupportedPCMType);
    USC_BADARG(in->nbytes < G723_SPEECH_FRAME*sizeof(Ipp16s), USC_NoOperation);

    g723_header = (G723_Handle_Header*)handle;

    USC_BADARG(USC_ENCODE != g723_header->direction, USC_NoOperation);

    bitrate_idx = CheckRate_G723(in->bitrate);
    if(bitrate_idx < 0) return USC_UnsupportedBitRate;
    g723_header->bitrate = in->bitrate;
    EncObj = (G723Encoder_Obj *)((Ipp8s*)handle + sizeof(G723_Handle_Header));

    if(apiG723Encode(EncObj,(const Ipp16s*)in->pBuffer, (Ipp16s)bitrate_idx, out->pBuffer) != APIG723_StsNoErr){
       return USC_NoOperation;
    }
    out->frametype = 0;
    out->nbytes = getBitstreamSize(out->pBuffer);
    out->bitrate = in->bitrate;

    in->nbytes = G723_SPEECH_FRAME*sizeof(Ipp16s);

    return USC_NoError;
}

static USC_Status Decode(USC_Handle handle, USC_Bitstream *in, USC_PCMStream *out)
{
    G723_Handle_Header *g723_header;
    G723Decoder_Obj *DecObj;
    Ipp32s bitrate_idx;
    Ipp32s PLC = 0;

    USC_CHECK_PTR(out);
    USC_CHECK_HANDLE(handle);

    g723_header = (G723_Handle_Header*)handle;

    USC_BADARG(USC_DECODE != g723_header->direction, USC_NoOperation);

    DecObj = (G723Decoder_Obj *)((Ipp8s*)handle + sizeof(G723_Handle_Header));

    if(in == NULL) {
      PLC = 1;
    } else if(in->pBuffer == NULL) {
      PLC = 1;
    }

    if(PLC) {
       /* Lost frame */
       if(apiG723Decode(DecObj,(const Ipp8s*)LostFrame,1,(Ipp16s*)out->pBuffer) != APIG723_StsNoErr){
        return USC_NoOperation;
      }
      out->bitrate = g723_header->bitrate;
    } else {
       bitrate_idx = CheckRate_G723(in->bitrate);
       USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);
       g723_header->bitrate = in->bitrate;
       in->nbytes = getBitstreamSize(in->pBuffer);
       if(apiG723Decode(DecObj,(const Ipp8s*)in->pBuffer, (Ipp16s)in->frametype, (Ipp16s*)out->pBuffer) != APIG723_StsNoErr){
        return USC_NoOperation;
      }
      out->bitrate = in->bitrate;
    }
    out->nbytes = G723_SPEECH_FRAME*sizeof(Ipp16s);
    out->pcmType.bitPerSample = G723_BITS_PER_SAMPLE;
    out->pcmType.sample_frequency = G723_SAMPLE_FREQUENCE;
    out->pcmType.nChannels=G723_NCHANNELS;

    return USC_NoError;
}

static __ALIGN32 CONST Ipp32s pFrameSize_G723[G723_NUM_RATES]={
    24,
    20
};

static USC_Status GetOutStreamSize(const USC_Option *options, Ipp32s bitrate, Ipp32s nbytesSrc, Ipp32s *nbytesDst)
{
   Ipp32s bitrate_idx;
   Ipp32s nBlocks, nSamples;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(nbytesDst);
   USC_BADARG(nbytesSrc <= 0, USC_NoOperation);

   bitrate_idx = CheckRate_G723(bitrate);
   USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);

   if(options->direction==USC_ENCODE) { /*Encode: src - PCMStream, dst - bitstream*/
      USC_BADARG(options->modes.vad>1, USC_UnsupportedVADType);

      nSamples = nbytesSrc / (G723_BITS_PER_SAMPLE >> 3);
      nBlocks = nSamples / G723_SPEECH_FRAME;

      if (0 == nBlocks) return USC_NoOperation;

      if (0 != nSamples % G723_SPEECH_FRAME) {
         /* Add another block to hold the last compressed fragment*/
         nBlocks++;
      }

      *nbytesDst = nBlocks * pFrameSize_G723[bitrate_idx];
   } else if(options->direction==USC_DECODE) {/*Decode: src - bitstream, dst - PCMStream*/
      if(options->modes.vad==0) { /*VAD off*/
         nBlocks = nbytesSrc / pFrameSize_G723[bitrate_idx];
      } else if(options->modes.vad==1) { /*VAD on*/
         nBlocks = nbytesSrc / G723_UNTR_FRAMESIZE;
      } else return USC_UnsupportedVADType;

      if (0 == nBlocks) return USC_NoOperation;

      nSamples = nBlocks * G723_SPEECH_FRAME;
      *nbytesDst = nSamples * (G723_BITS_PER_SAMPLE >> 3);
   } else if(options->direction==USC_DUPLEX) {/* Both: src - PCMStream, dst - PCMStream*/
      nSamples = nbytesSrc / (G723_BITS_PER_SAMPLE >> 3);
      nBlocks = nSamples / G723_SPEECH_FRAME;

      if (0 == nBlocks) return USC_NoOperation;

      if (0 != nSamples % G723_SPEECH_FRAME) {
         /* Add another block to hold the last compressed fragment*/
         nBlocks++;
      }
      *nbytesDst = nBlocks * G723_SPEECH_FRAME * (G723_BITS_PER_SAMPLE >> 3);
   } else return USC_NoOperation;
   return USC_NoError;
}

USC_Status SetFrameSize(const USC_Option *options, USC_Handle handle, Ipp32s frameSize)
{
   USC_CHECK_PTR(options);
   USC_CHECK_HANDLE(handle);
   USC_BADARG(frameSize <= 0, USC_BadArgument);

   return USC_NoOperation;
}
