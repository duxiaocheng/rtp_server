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
// Purpose: AMRWB speech codec: USC functions.
//
*/

#include "usc.h"
#include "ownamrwb.h"
#include "amrwbapi.h"

#define  AMRWB_NUM_RATES  9
#define  AMRWB_SAMPLE_FREQUENCY 16000
#define  AMRWB_BITS_PER_SAMPLE 16
#define  AMRWB_NCHANNELS 1

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

typedef struct
{
    Ipp16s updateSidCount;
    TXFrameType prevFrameType;
} SIDSyncState;

typedef struct {
    USC_Direction direction;
    Ipp32s trunc;
    Ipp32s bitrate;
    Ipp32s vad;
    Ipp32s reset_flag;
    Ipp32s reset_flag_old;
    Ipp32s bitrate_old;
    Ipp32s usedRate;
    SIDSyncState sid_state;
    Ipp32s reserved0; // for future extension
    Ipp32s reserved1; // for future extension
} AMRWB_Handle_Header;

static Ipp32s ownSidSyncInit(SIDSyncState *st);
static void   ownSidSync(SIDSyncState *st, Ipp32s valRate, TXFrameType *pTXFrameType);
static Ipp32s ownTestPCMFrameHoming(const Ipp16s *pSrc);
static Ipp32s ownTestBitstreamFrameHoming(Ipp16s* pPrmsvec, Ipp32s valRate);

/* global usc vector table */
USCFUN USC_Fxns USC_AMRWB_Fxns={
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

static __ALIGN32 CONST Ipp32s BitsLenTbl[NUM_OF_MODES]={
   132, 177, 253, 285, 317, 365, 397, 461, 477, 35
};

static __ALIGN32 CONST USC_Rates pTblRates_AMRWB[AMRWB_NUM_RATES]={
    {23850},
    {23050},
    {19850},
    {18250},
    {15850},
    {14250},
    {12650},
    {8850},
    {6600}
};

static __ALIGN32 CONST USC_PCMType pcmTypesTbl_AMRWB[1]={
   {AMRWB_SAMPLE_FREQUENCY,AMRWB_BITS_PER_SAMPLE, AMRWB_NCHANNELS}
};

static void ownConvert15(Ipp32s valRate, const Ipp8s *pSerialvec, Ipp16s* pPrmvec)
{
    Ipp16s i, j, tmp;
    Ipp32s valNumPrms = BitsLenTbl[valRate];
    Ipp32s nbit=0;

    j = 0;
    i = 0;

    tmp = (Ipp16s)(valNumPrms - 15);
    while (tmp > j)
    {
        pPrmvec[i] = amrExtractBits(&pSerialvec,&nbit,15);
        j += 15;
        i++;
    }
    tmp = (Ipp16s)(valNumPrms - j);
    pPrmvec[i] = amrExtractBits(&pSerialvec,&nbit,tmp);
    pPrmvec[i] <<= (15 - tmp);
}

static USC_Status GetInfoSize(Ipp32s *pSize)
{
   USC_CHECK_PTR(pSize);

   *pSize = sizeof(USC_CodecInfo);
   return USC_NoError;
}

static USC_Status GetInfo(USC_Handle handle, USC_CodecInfo *pInfo)
{
    AMRWB_Handle_Header *amrwb_header;

    USC_CHECK_PTR(pInfo);

    pInfo->name = "IPP_AMRWB";
    pInfo->params.framesize = AMRWB_Frame*sizeof(Ipp16s);
    if (handle == NULL) {
        pInfo->params.modes.bitrate = 6600;
        pInfo->params.modes.truncate = 1;
        pInfo->params.direction = USC_DECODE;
        pInfo->params.modes.vad = 1;
    } else {
      amrwb_header = (AMRWB_Handle_Header*)handle;
      pInfo->params.modes.bitrate = amrwb_header->bitrate;
      pInfo->params.modes.truncate = amrwb_header->trunc;
      pInfo->params.direction = amrwb_header->direction;
      pInfo->params.modes.vad = amrwb_header->vad;
    }
    pInfo->maxbitsize = BITSTREAM_SIZE;
    pInfo->params.pcmType.sample_frequency = pcmTypesTbl_AMRWB[0].sample_frequency;
    pInfo->params.pcmType.nChannels = pcmTypesTbl_AMRWB[0].nChannels;
    pInfo->params.pcmType.bitPerSample = pcmTypesTbl_AMRWB[0].bitPerSample;
    pInfo->nPcmTypes = 1;
    pInfo->pPcmTypesTbl = pcmTypesTbl_AMRWB;
    pInfo->params.modes.hpf = 0;
    pInfo->params.modes.pf = 0;
    pInfo->params.modes.outMode = USC_OUT_NO_CONTROL;
    pInfo->params.law = 0;
    pInfo->nRates = AMRWB_NUM_RATES;
    pInfo->pRateTbl = (const USC_Rates *)&pTblRates_AMRWB;
    pInfo->params.nModes = sizeof(USC_Modes)/sizeof(Ipp32s);

    return USC_NoError;
}

static Ipp32s CheckRate_AMRWB(Ipp32s rate_bps)
{
    Ipp32s rate;

    switch(rate_bps) {
      case 6600:  rate = 0; break;
      case 8850:  rate = 1; break;
      case 12650: rate = 2; break;
      case 14250: rate = 3; break;
      case 15850: rate = 4; break;
      case 18250: rate = 5; break;
      case 19850: rate = 6; break;
      case 23050: rate = 7; break;
      case 23850: rate = 8; break;
      default: rate = -1; break;
    }
    return rate;
}

static Ipp32s CheckIdxRate_AMRWB(Ipp32s idx_rate)
{
    Ipp32s rate;

    switch(idx_rate) {
      case 0: rate = 6600; break;
      case 1: rate = 8850; break;
      case 2: rate = 12650; break;
      case 3: rate = 14250; break;
      case 4: rate = 15850; break;
      case 5: rate = 18250; break;
      case 6: rate = 19850; break;
      case 7: rate = 23050; break;
      case 8: rate = 23850; break;
      default: rate = -1; break;
    }
    return rate;
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
   USC_BADARG(options->modes.vad > 1, USC_UnsupportedVADType);

   pBanks->pMem = NULL;
   pBanks->align = 32;
   pBanks->memType = USC_OBJECT;
   pBanks->memSpaceType = USC_NORMAL;

   if (options->direction == USC_ENCODE) /* encode only */
   {
      AMRWBEnc_Params enc_params;
      enc_params.codecType = (AMRWBCodec_Type)0;
      enc_params.mode = options->modes.vad;
      apiAMRWBEncoder_Alloc(&enc_params, &nbytes);
   }
   else if (options->direction == USC_DECODE) /* decode only */
   {
      AMRWBDec_Params dec_params;
      dec_params.codecType = (AMRWBCodec_Type)0;
      dec_params.mode = options->modes.vad;
      apiAMRWBDecoder_Alloc(&dec_params, &nbytes);
   } else {
      return USC_NoOperation;
   }
   pBanks->nbytes = nbytes + sizeof(AMRWB_Handle_Header); /* include header in handle */
   return USC_NoError;
}

static USC_Status Init(const USC_Option *options, const USC_MemBank *pBanks, USC_Handle *handle)
{
   AMRWB_Handle_Header *amrwb_header;
   Ipp32s bitrate_idx;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(pBanks);
   USC_CHECK_PTR(pBanks->pMem);
   USC_BADARG(pBanks->nbytes<=0, USC_NotInitialized);
   USC_CHECK_HANDLE(handle);
   USC_BADARG(options->modes.vad > 1, USC_UnsupportedVADType);
   USC_BADARG(options->modes.vad < 0, USC_UnsupportedVADType);

   USC_BADARG(options->pcmType.bitPerSample!=AMRWB_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
   USC_BADARG(options->pcmType.sample_frequency!=AMRWB_SAMPLE_FREQUENCY, USC_UnsupportedPCMType);
   USC_BADARG(options->pcmType.nChannels!=AMRWB_NCHANNELS, USC_UnsupportedPCMType);

   bitrate_idx = CheckRate_AMRWB(options->modes.bitrate);
   if(bitrate_idx < 0) return USC_UnsupportedBitRate;

   *handle = (USC_Handle*)pBanks->pMem;
   amrwb_header = (AMRWB_Handle_Header*)*handle;

   amrwb_header->direction = options->direction;
   amrwb_header->trunc = options->modes.truncate;
   amrwb_header->vad = options->modes.vad;
   amrwb_header->bitrate = options->modes.bitrate;
   amrwb_header->bitrate_old = amrwb_header->bitrate;

   if (options->direction == USC_ENCODE) /* encode only */
   {
       AMRWBEncoder_Obj *EncObj = (AMRWBEncoder_Obj *)((Ipp8s*)*handle + sizeof(AMRWB_Handle_Header));
       apiAMRWBEncoder_Init((AMRWBEncoder_Obj*)EncObj, amrwb_header->vad);
   }
   else if (options->direction == USC_DECODE) /* decode only */
   {
       AMRWBDecoder_Obj *DecObj = (AMRWBDecoder_Obj *)((Ipp8s*)*handle + sizeof(AMRWB_Handle_Header));
       apiAMRWBDecoder_Init((AMRWBDecoder_Obj*)DecObj);
       amrwb_header->usedRate = 6600;
       amrwb_header->reset_flag = 0;
       amrwb_header->reset_flag_old = 1;
   } else {
      *handle=NULL;
      return USC_NoOperation;
   }
   return USC_NoError;
}

static USC_Status Reinit(const USC_Modes *modes, USC_Handle handle )
{
    AMRWB_Handle_Header *amrwb_header;
    Ipp32s bitrate_idx;

    USC_CHECK_PTR(modes);
    USC_CHECK_HANDLE(handle);

    USC_BADARG(modes->vad > 1, USC_UnsupportedVADType);
    USC_BADARG(modes->vad < 0, USC_UnsupportedVADType);

    amrwb_header = (AMRWB_Handle_Header*)handle;

    bitrate_idx = CheckRate_AMRWB(modes->bitrate);
    if(bitrate_idx < 0) return USC_UnsupportedBitRate;
    amrwb_header->vad = modes->vad;
    amrwb_header->bitrate = modes->bitrate;
    amrwb_header->bitrate_old = amrwb_header->bitrate;

    if (amrwb_header->direction == USC_ENCODE) /* encode only */
    {
        AMRWBEncoder_Obj *EncObj = (AMRWBEncoder_Obj *)((Ipp8s*)handle + sizeof(AMRWB_Handle_Header));
        amrwb_header->reset_flag = 0;
        ownSidSyncInit(&amrwb_header->sid_state);
        apiAMRWBEncoder_Init((AMRWBEncoder_Obj*)EncObj, modes->vad);
    }
    else if (amrwb_header->direction == USC_DECODE) /* decode only */
    {
        AMRWBDecoder_Obj *DecObj = (AMRWBDecoder_Obj *)((Ipp8s*)handle + sizeof(AMRWB_Handle_Header));
        apiAMRWBDecoder_Init((AMRWBDecoder_Obj*)DecObj);
        amrwb_header->usedRate = 6600;
        amrwb_header->reset_flag = 0;
        amrwb_header->reset_flag_old = 1;
    } else {
        return USC_NoOperation;
    }
    return USC_NoError;
}

static USC_Status Control(const USC_Modes *modes, USC_Handle handle )
{
   AMRWB_Handle_Header *amrwb_header;
   Ipp32s bitrate_idx;

   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);

   USC_BADARG(modes->vad > 1, USC_UnsupportedVADType);
   USC_BADARG(modes->vad < 0, USC_UnsupportedVADType);

   bitrate_idx = CheckRate_AMRWB(modes->bitrate);
   USC_BADARG(bitrate_idx < 0,USC_UnsupportedBitRate);

   amrwb_header = (AMRWB_Handle_Header*)handle;

   amrwb_header->trunc = modes->truncate;
   amrwb_header->vad = modes->vad;
   amrwb_header->bitrate = modes->bitrate;

   if (amrwb_header->direction == USC_ENCODE) { /* encode only */
      AMRWBEncoder_Obj *EncObj;
      EncObj = (AMRWBEncoder_Obj *)((Ipp8s*)handle + sizeof(AMRWB_Handle_Header));
      apiAMRWBEncoder_Mode(EncObj, amrwb_header->vad);
   }

   return USC_NoError;
}

static __ALIGN32 CONST AMRWB_Rate_t usc2amrwb[9]={
    AMRWB_RATE_6600,
    AMRWB_RATE_8850,
    AMRWB_RATE_12650,
    AMRWB_RATE_14250,
    AMRWB_RATE_15850,
    AMRWB_RATE_18250,
    AMRWB_RATE_19850,
    AMRWB_RATE_23050,
    AMRWB_RATE_23850
};

static Ipp32s getBitstreamSize(Ipp32s rate, Ipp32s frametype, Ipp32s *outRate)
{
    Ipp32s nbytes;
    Ipp32s usedRate = rate;

    if (frametype != TX_SPEECH) {
        usedRate = AMRWB_RATE_DTX;
    }

    nbytes = ((BitsLenTbl[usedRate] + 7) / 8);
    *outRate = rate;

    return nbytes;
}

static USC_Status Encode(USC_Handle handle, USC_PCMStream *in, USC_Bitstream *out)
{
    AMRWB_Handle_Header *amrwb_header;
    Ipp32s bitrate_idx;
    AMRWBEncoder_Obj *EncObj;
    Ipp16u work_buf[AMRWB_Frame];
    Ipp32s WrkRate, out_WrkRate;

    USC_CHECK_PTR(in);
    USC_CHECK_PTR(out);
    USC_CHECK_HANDLE(handle);

    USC_BADARG(NULL == in->pBuffer, USC_NoOperation);
    USC_BADARG(NULL == out->pBuffer, USC_NoOperation);

    USC_BADARG(in->nbytes<AMRWB_Frame*sizeof(Ipp16s), USC_NoOperation);

    USC_BADARG(in->pcmType.bitPerSample!=AMRWB_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
    USC_BADARG(in->pcmType.sample_frequency!=AMRWB_SAMPLE_FREQUENCY, USC_UnsupportedPCMType);
    USC_BADARG(in->pcmType.nChannels!=AMRWB_NCHANNELS, USC_UnsupportedPCMType);

    amrwb_header = (AMRWB_Handle_Header*)handle;

    if(amrwb_header == NULL) return USC_InvalidHandler;

    if (amrwb_header->direction != USC_ENCODE) return USC_NoOperation;

    bitrate_idx = CheckRate_AMRWB(in->bitrate);
    USC_BADARG(bitrate_idx < 0,USC_UnsupportedBitRate);
    amrwb_header->bitrate = in->bitrate;

    EncObj = (AMRWBEncoder_Obj *)((Ipp8s*)handle + sizeof(AMRWB_Handle_Header));

    /* check for homing frame */
    amrwb_header->reset_flag = ownTestPCMFrameHoming((Ipp16s*)in->pBuffer);

    if (amrwb_header->trunc) {
       /* Delete the LSBs */
       ippsAndC_16u((Ipp16u*)in->pBuffer, (Ipp16u)IO_MASK, work_buf, AMRWB_Frame);
    } else {
       ippsCopy_16s((const Ipp16s*)in->pBuffer, (Ipp16s*)work_buf, AMRWB_Frame);
    }

    WrkRate = usc2amrwb[bitrate_idx];
	
	/* do a check before the encoding */
    {
        Ipp32u codeSize;
		if (apiAMRWBEncoder_GetSize(EncObj, &codeSize) != APIAMRWB_StsNoErr)
			return USC_NoOperation;
	}
	
    if(apiAMRWBEncode(EncObj,(const Ipp16u*)work_buf,(Ipp8u*)out->pBuffer,
       (AMRWB_Rate_t *)&WrkRate,EncObj->iDtx, 0) != APIAMRWB_StsNoErr){
       return USC_NoOperation;
    }
    /* include frame type and mode information in serial bitstream */
    ownSidSync(&amrwb_header->sid_state, WrkRate, (TXFrameType *)&out->frametype);

    out->nbytes = getBitstreamSize(bitrate_idx, out->frametype, &out_WrkRate);
    out->bitrate = CheckIdxRate_AMRWB(out_WrkRate);

    if (amrwb_header->reset_flag != 0) {
        ownSidSyncInit(&amrwb_header->sid_state);
        apiAMRWBEncoder_Init(EncObj, EncObj->iDtx);
    }

    in->nbytes = AMRWB_Frame*sizeof(Ipp16s);
    return USC_NoError;
}

static RXFrameType ownTX2RX (TXFrameType valTXType)
{
    switch (valTXType) {
      case TX_SPEECH:                    return RX_SPEECH_GOOD;
      case TX_SID_FIRST:                 return RX_SID_FIRST;
      case TX_SID_UPDATE:                return RX_SID_UPDATE;
      case TX_NO_DATA:                   return RX_NO_DATA;
      case TX_SPEECH_PROBABLY_DEGRADED:  return RX_SPEECH_PROBABLY_DEGRADED;
      case TX_SPEECH_LOST:               return RX_SPEECH_LOST;
      case TX_SPEECH_BAD:                return RX_SPEECH_BAD;
      case TX_SID_BAD:                   return RX_SID_BAD;
      default:
                                         return (RXFrameType)(-1);
    }
}

static USC_Status Decode(USC_Handle handle, USC_Bitstream *in, USC_PCMStream *out)
{
    AMRWB_Handle_Header *amrwb_header;
    AMRWBDecoder_Obj *DecObj;
    Ipp32s bitrate_idx;
    Ipp32s mode;
    RXFrameType rx_type;
    Ipp16s prm[56];
    Ipp32s PLC=0;

    USC_CHECK_PTR(out);
    USC_BADARG(NULL == out->pBuffer, USC_NoOperation);
    USC_CHECK_HANDLE(handle);

    amrwb_header = (AMRWB_Handle_Header*)handle;
    DecObj = (AMRWBDecoder_Obj *)((Ipp8s*)handle + sizeof(AMRWB_Handle_Header));

    if (USC_DECODE != amrwb_header->direction) return USC_NoOperation;

    if(in == NULL) {
       PLC=1;
    } else if(in->pBuffer==NULL){
       PLC=1;
    }

    if(PLC) {
       bitrate_idx = CheckRate_AMRWB(amrwb_header->bitrate_old);
       if(bitrate_idx < 0) return USC_UnsupportedBitRate;

       amrwb_header->bitrate = amrwb_header->bitrate_old;
       out->bitrate = amrwb_header->bitrate_old;
       rx_type = RX_SPEECH_LOST;
	
	    /* do a check before the decoding */
	   {
	        Ipp32u codeSize;
			if (apiAMRWBDecoder_GetSize(DecObj, &codeSize) != APIAMRWB_StsNoErr)
				return USC_NoOperation;
	   }
	
       if(apiAMRWBDecode(DecObj,(const Ipp8u*)LostFrame, usc2amrwb[bitrate_idx],
          rx_type,(Ipp16u*)out->pBuffer, 0) != APIAMRWB_StsNoErr){
            return USC_NoOperation;
      }
      if(amrwb_header->trunc) {
          ippsAndC_16u((Ipp16u*)out->pBuffer, (Ipp16u)IO_MASK, (Ipp16u*)out->pBuffer, AMRWB_Frame);
      }
      amrwb_header->reset_flag = 0;
      amrwb_header->reset_flag_old = 1;

      out->nbytes = AMRWB_Frame*sizeof(Ipp16s);

    } else {
      USC_BADARG(((in->nbytes<=0) && (in->frametype != 3)), USC_NoOperation);

      bitrate_idx = CheckRate_AMRWB(in->bitrate);
      if(bitrate_idx < 0) return USC_UnsupportedBitRate;
      amrwb_header->bitrate = in->bitrate;
      out->bitrate = in->bitrate;
      rx_type = ownTX2RX((TXFrameType)in->frametype);

      if ((rx_type == RX_SID_BAD) || (rx_type == RX_SID_UPDATE) || (rx_type == RX_NO_DATA)) {
         mode = AMRWB_RATE_DTX;
      } else {
         mode = bitrate_idx;
      }
      if ((rx_type == RX_SPEECH_LOST)) {
         out->bitrate = amrwb_header->usedRate;
         amrwb_header->reset_flag = 0;
      } else {
         amrwb_header->usedRate = out->bitrate;
         /* if homed: check if this frame is another homing frame */
         ownConvert15(mode,(const Ipp8s *)in->pBuffer, prm);
         amrwb_header->reset_flag = ownTestBitstreamFrameHoming(prm, mode);
      }

      /* produce encoder homing frame if homed & input=decoder homing frame */
      if ((amrwb_header->reset_flag != 0) && (amrwb_header->reset_flag_old != 0))
      {
         ippsSet_16s(EHF_MASK, (Ipp16s*)out->pBuffer, AMRWB_Frame);
      }
      else
      {
     
         /* do a check before the decoding */
	     {
	        Ipp32u codeSize;
			if (apiAMRWBDecoder_GetSize(DecObj, &codeSize) != APIAMRWB_StsNoErr)
				return USC_NoOperation;
		 }
	 	  
         if(apiAMRWBDecode(DecObj,(const Ipp8u*)in->pBuffer,usc2amrwb[bitrate_idx],
            rx_type,(Ipp16u*)out->pBuffer, 0) != APIAMRWB_StsNoErr){
            return USC_NoOperation;
         }
         if (amrwb_header->trunc) {
            /* Truncate LSBs */
            ippsAndC_16u((Ipp16u*)out->pBuffer, (Ipp16u)IO_MASK, (Ipp16u*)out->pBuffer, AMRWB_Frame);
         }
      }
      /* reset decoder if current frame is a homing frame */
      if (amrwb_header->reset_flag != 0)
      {
         apiAMRWBDecoder_Init((AMRWBDecoder_Obj*)DecObj);
         amrwb_header->usedRate = 6600;
      }
      amrwb_header->reset_flag_old = amrwb_header->reset_flag;

      out->nbytes = AMRWB_Frame*sizeof(Ipp16s);
      {
          Ipp32s foo;
          in->nbytes = getBitstreamSize(bitrate_idx, in->frametype, &foo);
      }
    }

   out->pcmType.bitPerSample = AMRWB_BITS_PER_SAMPLE;
   out->pcmType.nChannels = AMRWB_NCHANNELS;
   out->pcmType.sample_frequency = AMRWB_SAMPLE_FREQUENCY;

   return USC_NoError;
}

static Ipp32s ownSidSyncInit(SIDSyncState *st)
{
    st->updateSidCount = 3;
    st->prevFrameType = TX_SPEECH;
    return 0;
}

static void ownSidSync(SIDSyncState *st, Ipp32s valRate, TXFrameType *pTXFrameType)
{
    if ( valRate == AMRWB_RATE_DTX)
    {
        st->updateSidCount--;
        if (st->prevFrameType == TX_SPEECH)
        {
           *pTXFrameType = TX_SID_FIRST;
           st->updateSidCount = 3;
        }
        else
        {
           if (st->updateSidCount == 0)
           {
              *pTXFrameType = TX_SID_UPDATE;
              st->updateSidCount = 8;
           } else {
              *pTXFrameType = TX_NO_DATA;
           }
        }
    }
    else
    {
       st->updateSidCount = 8 ;
       *pTXFrameType = TX_SPEECH;
    }
    st->prevFrameType = *pTXFrameType;
}

static Ipp32s ownTestPCMFrameHoming(const Ipp16s *pSrc)
{
    Ipp32s i, fl = 0;

    for (i = 0; i < AMRWB_Frame; i++)
    {
        fl = pSrc[i] ^ EHF_MASK;
        if (fl) break;
    }

    return (!fl);
}

#define PRML 15
#define NUMPRM6600 NUMBITS6600/PRML + 1
#define NUMPRM8850 NUMBITS8850/PRML + 1
#define NUMPRM12650 NUMBITS12650/PRML + 1
#define NUMPRM14250 NUMBITS14250/PRML + 1
#define NUMPRM15850 NUMBITS15850/PRML + 1
#define NUMPRM18250 NUMBITS18250/PRML + 1
#define NUMPRM19850 NUMBITS19850/PRML + 1
#define NUMPRM23050 NUMBITS23050/PRML + 1
#define NUMPRM23850 NUMBITS23850/PRML + 1

static __ALIGN32 CONST Ipp16s DecHomFrm6600Tbl[NUMPRM6600] =
{
   3168, 29954, 29213, 16121, 64,
  13440, 30624, 16430, 19008
};

static __ALIGN32 CONST Ipp16s DecHomFrm8850Tbl[NUMPRM8850] =
{
   3168, 31665, 9943,  9123, 15599,  4358,
  20248, 2048, 17040, 27787, 16816, 13888
};

static __ALIGN32 CONST Ipp16s DecHomFrm12650Tbl[NUMPRM12650] =
{
  3168, 31665,  9943,  9128,  3647,
  8129, 30930, 27926, 18880, 12319,
   496,  1042,  4061, 20446, 25629,
 28069, 13948
};

static __ALIGN32 CONST Ipp16s DecHomFrm14250Tbl[NUMPRM14250] =
{
    3168, 31665,  9943,  9131, 24815,
     655, 26616, 26764,  7238, 19136,
    6144,    88,  4158, 25733, 30567,
    30494,  221, 20321, 17823
};

static __ALIGN32 CONST Ipp16s DecHomFrm15850Tbl[NUMPRM15850] =
{
    3168, 31665,  9943,  9131, 24815,
     700,  3824,  7271, 26400,  9528,
    6594, 26112,   108,  2068, 12867,
   16317, 23035, 24632,  7528,  1752,
    6759, 24576
};

static __ALIGN32 CONST Ipp16s DecHomFrm18250Tbl[NUMPRM18250] =
{
     3168, 31665,  9943,  9135, 14787,
    14423, 30477, 24927, 25345, 30154,
      916,  5728, 18978,  2048,   528,
    16449,  2436,  3581, 23527, 29479,
     8237, 16810, 27091, 19052,     0
};

static __ALIGN32 CONST Ipp16s DecHomFrm19850Tbl[NUMPRM19850] =
{
     3168, 31665,  9943,  9129,  8637, 31807,
    24646,   736, 28643,  2977,  2566, 25564,
    12930, 13960,  2048,   834,  3270,  4100,
    26920, 16237, 31227, 17667, 15059, 20589,
    30249, 29123,     0
};

static __ALIGN32 CONST Ipp16s DecHomFrm23050Tbl[NUMPRM23050] =
{
     3168, 31665,  9943,  9132, 16748,  3202,  28179,
    16317, 30590, 15857, 19960,  8818, 21711,  21538,
     4260, 16690, 20224,  3666,  4194,  9497,  16320,
    15388,  5755, 31551, 14080, 3574,  15932,     50,
    23392, 26053, 31216
};

static __ALIGN32 CONST Ipp16s DecHomFrm23850Tbl[NUMPRM23850] =
{
     3168, 31665,  9943,  9134, 24776,  5857, 18475,
    28535, 29662, 14321, 16725,  4396, 29353, 10003,
    17068, 20504,   720,     0,  8465, 12581, 28863,
    24774,  9709, 26043,  7941, 27649, 13965, 15236,
    18026, 22047, 16681,  3968
};

static __ALIGN32 CONST Ipp16s *DecHomFrmTbl[] =
{
  DecHomFrm6600Tbl,
  DecHomFrm8850Tbl,
  DecHomFrm12650Tbl,
  DecHomFrm14250Tbl,
  DecHomFrm15850Tbl,
  DecHomFrm18250Tbl,
  DecHomFrm19850Tbl,
  DecHomFrm23050Tbl,
  DecHomFrm23850Tbl,
  DecHomFrm23850Tbl
};

/* check if the parameters matches the parameters of the corresponding decoder homing frame */
static Ipp32s ownTestBitstreamFrameHoming (Ipp16s* pPrmsvec, Ipp32s valRate)
{
    Ipp32s i, valFlag;
    Ipp16s tmp, valSht;
    Ipp32s valNumBytes = (BitsLenTbl[valRate] + 14) / 15;
    valSht  = (Ipp16s)(valNumBytes * 15 - BitsLenTbl[valRate]);

    valFlag = 0;
    if (valRate != AMRWB_RATE_DTX)
    {
        if (valRate != AMRWB_RATE_23850)
        {
            for (i = 0; i < valNumBytes-1; i++)
            {
                valFlag = (Ipp16s)(pPrmsvec[i] ^ DecHomFrmTbl[valRate][i]);
                if (valFlag) break;
            }
        } else
        {
            valSht = 0;
            for (i = 0; i < valNumBytes-1; i++)
            {
                switch (i)
                {
                case 10:
                    tmp = (Ipp16s)(pPrmsvec[i] & 0x61FF);
                    break;
                case 17:
                    tmp = (Ipp16s)(pPrmsvec[i] & 0xE0FF);
                    break;
                case 24:
                    tmp = (Ipp16s)(pPrmsvec[i] & 0x7F0F);
                    break;
                default:
                    tmp = pPrmsvec[i];
                    break;
                }
                valFlag = (Ipp16s) (tmp ^ DecHomFrmTbl[valRate][i]);
                if (valFlag) break;
            }
        }
        tmp = 0x7fff;
        tmp >>= valSht;
        tmp <<= valSht;
        tmp = (Ipp16s) (DecHomFrmTbl[valRate][i] & tmp);
        tmp = (Ipp16s) (pPrmsvec[i] ^ tmp);
        valFlag = (Ipp16s) (valFlag | tmp);
    }
    else
    {
        valFlag = 1;
    }
    return (!valFlag);
}

static USC_Status CalsOutStreamSize(const USC_Option *options, Ipp32s bitrate_idx, Ipp32s nbytesSrc, Ipp32s *nbytesDst)
{
   Ipp32s nBlocks, nSamples, foo;

   if(options->direction==USC_ENCODE) { /*Encode: src - PCMStream, dst - bitstream*/
      if(options->modes.vad>1) return USC_UnsupportedVADType;

      nSamples = nbytesSrc / (AMRWB_BITS_PER_SAMPLE >> 3);
      nBlocks = nSamples / AMRWB_Frame;

      if (0 == nBlocks) return USC_NoOperation;

      if (0 != nSamples % AMRWB_Frame) {
         /* Add another block to hold the last compressed fragment*/
         nBlocks++;
      }

      *nbytesDst = nBlocks * getBitstreamSize(bitrate_idx, TX_SPEECH, &foo);
   } else if(options->direction==USC_DECODE) {/*Decode: src - bitstream, dst - PCMStream*/
      if(options->modes.vad==0) { /*VAD off*/
         nBlocks = nbytesSrc / getBitstreamSize(bitrate_idx, TX_SPEECH, &foo);
      } else if(options->modes.vad==1) { /*VAD on*/
         nBlocks = nbytesSrc / getBitstreamSize(bitrate_idx, TX_SID_FIRST, &foo);
      } else return USC_UnsupportedVADType;

      if (0 == nBlocks) return USC_NoOperation;

      nSamples = nBlocks * AMRWB_Frame;
      *nbytesDst = nSamples * (AMRWB_BITS_PER_SAMPLE >> 3);
   } 
#if(0)   
   else if(options->direction==USC_DUPLEX) {/* Both: src - PCMStream, dst - PCMStream*/
      nSamples = nbytesSrc / (AMRWB_BITS_PER_SAMPLE >> 3);
      nBlocks = nSamples / AMRWB_Frame;

      if (0 == nBlocks) return USC_NoOperation;

      if (0 != nSamples % AMRWB_Frame) {
         /* Add another block to hold the last compressed fragment*/
         nBlocks++;
      }
      *nbytesDst = nBlocks * AMRWB_Frame * (AMRWB_BITS_PER_SAMPLE >> 3);
   } 
#endif   
   else return USC_NoOperation;

   return USC_NoError;
}

static USC_Status GetOutStreamSize(const USC_Option *options, Ipp32s bitrate, Ipp32s nbytesSrc, Ipp32s *nbytesDst)
{
   Ipp32s bitrate_idx;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(nbytesDst);
   USC_BADARG(nbytesSrc <= 0, USC_NoOperation);

   bitrate_idx = CheckRate_AMRWB(bitrate);
    if(bitrate_idx < 0) return USC_UnsupportedBitRate;

   return CalsOutStreamSize(options, bitrate_idx, nbytesSrc, nbytesDst);
}

USC_Status SetFrameSize(const USC_Option *options, USC_Handle handle, Ipp32s frameSize)
{
   USC_CHECK_PTR(options);
   USC_CHECK_HANDLE(handle);
   USC_BADARG(frameSize <= 0, USC_BadArgument);

   return USC_NoOperation;
}
