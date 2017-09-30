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
// Purpose: G.728 speech codec: USC functions.
//
*/

#include <ipps.h>
#include "usc.h"
#include "owng728.h"
#include "g728api.h"

#define G728_VEC_PER_FRAME  4
#define G728_VECTOR         5
#define G728_FRAME          (G728_VEC_PER_FRAME * G728_VECTOR)
#define BITSTREAM_SIZE_G728 (G728_VECTOR)
#define G728_NUM_RATES      4
#define G728_NUM_FRAMES_PER_PACKET 4
#define G728_BITS_PER_SAMPLE 16
#define G728_SAMPLE_FREQ     8000
#define G728_NCHANNELS          1

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
    Ipp32s pst;
    Ipp32s G728Type;
} G728_Handle_Header;

/* global usc vector table */
USCFUN USC_Fxns USC_G728_Fxns=
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

static __ALIGN32 CONST USC_Rates pTblRates_G728[G728_NUM_RATES]={
    {16000},
    {12800},
    {9600},
    {40000}
};
static __ALIGN32 CONST USC_PCMType pcmTypesTbl_G728[1]={
   {G728_SAMPLE_FREQ,G728_BITS_PER_SAMPLE,G728_NCHANNELS}
};

static Ipp32s CheckRate_G728(Ipp32s rate_bps)
{
   Ipp32s rate;

    switch(rate_bps) {
      case 16000: rate = 0; break;
      case 12800: rate = 1; break;
      case 9600:  rate = 2; break;
      case 40000:  rate = 3; break;
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
   G728_Handle_Header *g728_header;

   USC_CHECK_PTR(pInfo);

   pInfo->name = "IPP_G728";
   pInfo->params.framesize = G728_NUM_FRAMES_PER_PACKET*G728_FRAME*sizeof(Ipp16s);
   if (handle == NULL) {
      pInfo->params.modes.bitrate = 16000;
      pInfo->params.direction = USC_DECODE;
      pInfo->params.modes.pf = 1;
   } else {
      g728_header = (G728_Handle_Header*)handle;
      pInfo->params.modes.bitrate = g728_header->bitrate;
      pInfo->params.direction = g728_header->direction;
      pInfo->params.modes.pf = g728_header->pst;
   }
   pInfo->params.modes.truncate = 0;
   pInfo->params.modes.vad = 0;
   pInfo->params.modes.outMode = USC_OUT_NO_CONTROL;
   pInfo->maxbitsize = 50;//G728_NUM_FRAMES_PER_PACKET*BITSTREAM_SIZE_G728*sizeof(Ipp16s);
   pInfo->params.pcmType.sample_frequency = pcmTypesTbl_G728[0].sample_frequency;
   pInfo->params.pcmType.nChannels = pcmTypesTbl_G728[0].nChannels;
   pInfo->params.pcmType.bitPerSample = pcmTypesTbl_G728[0].bitPerSample;
   pInfo->nPcmTypes = 1;
   pInfo->pPcmTypesTbl = pcmTypesTbl_G728;
   pInfo->params.modes.hpf = 0;
   pInfo->params.law = 0;
   pInfo->nRates = G728_NUM_RATES;
   pInfo->pRateTbl = (const USC_Rates *)&pTblRates_G728;
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
      apiG728Encoder_Alloc(&nbytes);
   } else if (options->direction == USC_DECODE) /* decode only */
   {
      apiG728Decoder_Alloc(&nbytes);
   } else {
      return USC_NoOperation;
   }
   pBanks->nbytes = nbytes + sizeof(G728_Handle_Header); /* room for USC header */
   return USC_NoError;

}

G728_Rate usc2apiRate[4] = {
   G728_Rate_16000,
   G728_Rate_12800,
   G728_Rate_9600,
   G728_Rate_40000
};

static USC_Status Init(const USC_Option *options, const USC_MemBank *pBanks, USC_Handle *handle)
{
   G728_Handle_Header *g728_header;
   Ipp32s bitrate_idx;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(pBanks);
   USC_CHECK_PTR(pBanks->pMem);
   USC_BADARG(pBanks->nbytes<=0, USC_NotInitialized);
   USC_CHECK_HANDLE(handle);

   USC_BADARG(options->pcmType.bitPerSample!=G728_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
   USC_BADARG(options->pcmType.sample_frequency!=G728_SAMPLE_FREQ, USC_UnsupportedPCMType);
   USC_BADARG(options->pcmType.nChannels!=G728_NCHANNELS, USC_UnsupportedPCMType);

   bitrate_idx = CheckRate_G728(options->modes.bitrate);
   USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);

   *handle = (USC_Handle*)pBanks->pMem;
   g728_header = (G728_Handle_Header*)*handle;

   g728_header->direction = options->direction;
   g728_header->bitrate = options->modes.bitrate;
   g728_header->pst=options->modes.pf;

   if (options->direction == USC_ENCODE) /* encode only */
   {
      G728Encoder_Obj *EncObj = (G728Encoder_Obj *)((Ipp8s*)*handle + sizeof(G728_Handle_Header));
      apiG728Encoder_Init(EncObj, usc2apiRate[bitrate_idx]);
      g728_header->G728Type=G728_Pure;
   } else if (options->direction == USC_DECODE) /* decode only */
   {
      G728Decoder_Obj *DecObj = (G728Decoder_Obj *)((Ipp8s*)*handle + sizeof(G728_Handle_Header));
      g728_header->G728Type=G728_Annex_I;
      apiG728Decoder_Init(DecObj, (G728_Type)g728_header->G728Type, usc2apiRate[bitrate_idx], g728_header->pst);
   } else {
      *handle = NULL;
      return USC_NoOperation;
   }
   return USC_NoError;
}

static USC_Status Reinit(const USC_Modes *modes, USC_Handle handle )
{
   G728_Handle_Header *g728_header;
   Ipp32s bitrate_idx;

   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);

   g728_header = (G728_Handle_Header*)handle;

   bitrate_idx = CheckRate_G728(modes->bitrate);
   USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);
   g728_header->bitrate = modes->bitrate;

   if (g728_header->direction == USC_ENCODE) /* encode only */
   {
      G728Encoder_Obj *EncObj = (G728Encoder_Obj *)((Ipp8s*)handle + sizeof(G728_Handle_Header));
      g728_header->G728Type = G728_Pure;
      apiG728Encoder_Init(EncObj, usc2apiRate[bitrate_idx]);
   }
   else if (g728_header->direction == USC_DECODE) /* decode only */
   {
      G728Decoder_Obj *DecObj = (G728Decoder_Obj *)((Ipp8s*)handle + sizeof(G728_Handle_Header));
      g728_header->G728Type = G728_Annex_I;
      g728_header->pst = modes->pf;
      apiG728Decoder_Init(DecObj, (G728_Type)g728_header->G728Type, usc2apiRate[bitrate_idx], g728_header->pst);
   } else {
      return USC_NoOperation;
   }
   return USC_NoError;
}

static USC_Status Control(const USC_Modes *modes, USC_Handle handle )
{
   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);

   return Reinit(modes, handle);
}

static Ipp32s outFrameSize(Ipp32s bitRate)
{
   Ipp32s size=0;
   if(bitRate==0) {
      size=5*sizeof(Ipp8s);
   } else if(bitRate==1) {
      size=4*sizeof(Ipp8s);
   } else  if(bitRate==2) {
      size=3*sizeof(Ipp8s);
   } else  if(bitRate==3) {
      size=20*sizeof(Ipp8s);
   }
   return size;
}

static USC_Status Encode(USC_Handle handle, USC_PCMStream *in, USC_Bitstream *out)
{
   G728_Handle_Header *g728_header;
   G728Encoder_Obj *EncObj;
   Ipp32s i, bitrate_idx, inputLen;
   IPP_ALIGNED_ARRAY(16, Ipp16s, work_buf, G728_NUM_FRAMES_PER_PACKET*G728_FRAME);

   USC_CHECK_PTR(in);
   USC_CHECK_PTR(out);
   USC_CHECK_HANDLE(handle);

   USC_BADARG(in->pcmType.bitPerSample!=G728_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
   USC_BADARG(in->pcmType.sample_frequency!=G728_SAMPLE_FREQ, USC_UnsupportedPCMType);
   USC_BADARG(in->pcmType.nChannels!=G728_NCHANNELS, USC_UnsupportedPCMType);

   USC_BADARG(NULL == in->pBuffer, USC_NoOperation);
   USC_BADARG(NULL == out->pBuffer, USC_NoOperation);

   g728_header = (G728_Handle_Header*)handle;

   USC_BADARG(USC_ENCODE != g728_header->direction, USC_NoOperation);

   EncObj = (G728Encoder_Obj *)((Ipp8s*)handle + sizeof(G728_Handle_Header));

   if(in->nbytes >= G728_NUM_FRAMES_PER_PACKET*G728_FRAME*sizeof(Ipp16s))
      inputLen = G728_NUM_FRAMES_PER_PACKET*G728_FRAME*sizeof(Ipp16s);
   else
      inputLen = (in->nbytes/(G728_VECTOR*sizeof(Ipp16s)))*(G728_VECTOR*sizeof(Ipp16s));
   USC_BADARG(inputLen <= 0, USC_NoOperation);

   in->nbytes = inputLen;

   if(g728_header->bitrate != in->bitrate) {
      bitrate_idx = CheckRate_G728(in->bitrate);
      USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);
      apiG728Encoder_Init(EncObj, usc2apiRate[bitrate_idx]);
      g728_header->bitrate = in->bitrate;
   } else {
      bitrate_idx = CheckRate_G728(g728_header->bitrate);
      USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);
   }

   /* scale down to 14-bit uniform PCM*/
   ippsRShiftC_16s((Ipp16s*)in->pBuffer, 1, (Ipp16s*)work_buf, in->nbytes/sizeof(Ipp16s));

   if(in->bitrate==40000) {
      if(apiG728Encode(EncObj, work_buf,(Ipp8u*)out->pBuffer) != APIG728_StsNoErr){
         return USC_NoOperation;
      }
      out->nbytes = 50;
   } else {
      for(i=0;i<G728_NUM_FRAMES_PER_PACKET;i++) {
         if(apiG728Encode(EncObj, work_buf+i*G728_FRAME,(Ipp8u*)(out->pBuffer+i*outFrameSize(bitrate_idx))) != APIG728_StsNoErr){
            return USC_NoOperation;
         }
      }
      out->nbytes = G728_NUM_FRAMES_PER_PACKET*outFrameSize(bitrate_idx);
   }

   out->bitrate = in->bitrate;
   out->frametype = 0;

   return USC_NoError;
}

static __ALIGN32 CONST Ipp8u g728LostFrame[6] = {
   1, 0, 0, 0, 0, 0
};

static USC_Status Decode(USC_Handle handle, USC_Bitstream *in, USC_PCMStream *out)
{
   Ipp32s i, j, sbrfLen, bitrate_idx;
   G728_Handle_Header *g728_header;
   G728Decoder_Obj *DecObj;
   Ipp8s packet[51/*6*/];
   Ipp32s PLC = 0;

   USC_CHECK_PTR(out);
   USC_CHECK_HANDLE(handle);

   USC_BADARG(NULL == out->pBuffer, USC_NoOperation);

   g728_header = (G728_Handle_Header*)handle;

   USC_BADARG(USC_DECODE != g728_header->direction, USC_NoOperation);

   DecObj = (G728Decoder_Obj *)((Ipp8s*)handle + sizeof(G728_Handle_Header));

   if(in == NULL) {
      PLC = 1;
   } else if(in->pBuffer==NULL) {
      PLC = 1;
   }

   if(PLC) {
      for(i=0;i<G728_NUM_FRAMES_PER_PACKET;i++) {
         if(apiG728Decode(DecObj,g728LostFrame, (Ipp16s*)out->pBuffer+i*20)!=APIG728_StsNoErr){
            return USC_NoOperation;
         }
      }
      out->bitrate = g728_header->bitrate;
   } else {
      USC_BADARG(NULL == in->pBuffer, USC_NoOperation);

      bitrate_idx = CheckRate_G728(in->bitrate);
      USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);
      if(g728_header->bitrate != in->bitrate) {
        apiG728Decoder_Init(DecObj, (G728_Type)g728_header->G728Type, usc2apiRate[bitrate_idx], g728_header->pst);
        g728_header->bitrate = in->bitrate;
      }

      if(in->bitrate == 40000) {
         packet[0] = (Ipp8s)in->frametype;
         for(j=0;j<50;j++)
            packet[j+1] = in->pBuffer[j];
         if(apiG728Decode(DecObj,(Ipp8u*)packet, (Ipp16s*)out->pBuffer)!=APIG728_StsNoErr){
            return USC_NoOperation;
         }
         in->nbytes = 50;
      } else {
         sbrfLen = outFrameSize(bitrate_idx);
         if(in->nbytes < sbrfLen*G728_NUM_FRAMES_PER_PACKET) return USC_NoOperation;

         for(i=0;i<G728_NUM_FRAMES_PER_PACKET;i++) {
            packet[0] = (Ipp8s)in->frametype;
            for(j=0;j<sbrfLen;j++)
               packet[j+1] = in->pBuffer[i*sbrfLen + j];
            if(apiG728Decode(DecObj,(Ipp8u*)packet, (Ipp16s*)out->pBuffer+i*20)!=APIG728_StsNoErr){
               return USC_NoOperation;
            }
         }
         in->nbytes = sbrfLen*G728_NUM_FRAMES_PER_PACKET;
      }
      out->bitrate = in->bitrate;
   }

   out->nbytes = G728_NUM_FRAMES_PER_PACKET*G728_FRAME*sizeof(Ipp16s);
   out->pcmType.sample_frequency = G728_SAMPLE_FREQ;
   out->pcmType.bitPerSample = G728_BITS_PER_SAMPLE;
   out->pcmType.nChannels = G728_NCHANNELS;

   return USC_NoError;
}

static USC_Status CalsOutStreamSize(const USC_Option *options, Ipp32s bitrate_idx, Ipp32s nbytesSrc, Ipp32s *nbytesDst)
{
   Ipp32s nBlocks, nSamples;

   if(options->direction==USC_ENCODE) { /*Encode: src - PCMStream, dst - bitstream*/
      USC_BADARG(options->modes.vad!=0, USC_UnsupportedVADType);

      nSamples = nbytesSrc / (G728_BITS_PER_SAMPLE >> 3);
      nBlocks = nSamples / (G728_NUM_FRAMES_PER_PACKET*G728_FRAME);

      if (0 == nBlocks) return USC_NoOperation;

      if (0 != nSamples % (G728_NUM_FRAMES_PER_PACKET*G728_FRAME)) {
         /* Add another block to hold the last compressed fragment*/
         nBlocks++;
      }

      *nbytesDst = nBlocks * outFrameSize(bitrate_idx)*G728_NUM_FRAMES_PER_PACKET;
   } else if(options->direction==USC_DECODE) {/*Decode: src - bitstream, dst - PCMStream*/
      USC_BADARG(options->modes.vad!=0, USC_UnsupportedVADType);

      nBlocks = nbytesSrc / (outFrameSize(bitrate_idx)*G728_NUM_FRAMES_PER_PACKET);
      if (0 == nBlocks) return USC_NoOperation;

      nSamples = nBlocks * (G728_NUM_FRAMES_PER_PACKET*G728_FRAME);
      *nbytesDst = nSamples * (G728_BITS_PER_SAMPLE >> 3);
   } else if(options->direction==USC_DUPLEX) {/* Both: src - PCMStream, dst - PCMStream*/
      nSamples = nbytesSrc / (G728_BITS_PER_SAMPLE >> 3);
      nBlocks = nSamples / (G728_NUM_FRAMES_PER_PACKET*G728_FRAME);

      if (0 == nBlocks) return USC_NoOperation;

      if (0 != nSamples % (G728_NUM_FRAMES_PER_PACKET*G728_FRAME)) {
         /* Add another block to hold the last compressed fragment*/
         nBlocks++;
      }
      *nbytesDst = nBlocks * (G728_NUM_FRAMES_PER_PACKET*G728_FRAME) * (G728_BITS_PER_SAMPLE >> 3);
   } else return USC_NoOperation;

   return USC_NoError;
}
static USC_Status GetOutStreamSize(const USC_Option *options, Ipp32s bitrate, Ipp32s nbytesSrc, Ipp32s *nbytesDst)
{
   Ipp32s bitrate_idx;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(nbytesDst);
   USC_BADARG(nbytesSrc <= 0, USC_NoOperation);

   bitrate_idx = CheckRate_G728(bitrate);
   USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);

   return CalsOutStreamSize(options, bitrate_idx, nbytesSrc, nbytesDst);
}

USC_Status SetFrameSize(const USC_Option *options, USC_Handle handle, Ipp32s frameSize)
{
   USC_CHECK_PTR(options);
   USC_CHECK_HANDLE(handle);
   USC_BADARG(frameSize <= 0, USC_BadArgument);

   return USC_NoOperation;
}

