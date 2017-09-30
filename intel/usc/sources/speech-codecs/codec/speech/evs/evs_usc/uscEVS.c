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
// Purpose: EVS speech codec: USC functions.
//
*/
//lint -save -e10 -e2
#include <stddef.h>  //for size_t
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//lint -restore
#include "typedefs.h"
#include "usc.h"
#include "evs_types.h"
#include "ownevs.h"
#include "evsapi.h"
#include "ippsc.h"


#define EVS_Frame	160

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
static USC_Status FeedJBMFrame(USC_Handle handle, USC_MemBank *pBanks, USC_Bitstream *in);
static USC_Status GetJBMFrame(USC_Handle handle, USC_MemBank *pBanks, USC_PCMStream *out, USC_Modes *modes);

typedef struct {
    USC_Direction direction;
    Ipp32s trunc;
    Ipp32s bitrate;
    Ipp32s vad;
    Ipp32s reset_flag;
    Ipp32s reset_flag_old;
    Ipp32s bitrate_old;
    Ipp32s usedRate;
	Ipp32s bitstreamformat;			/* input bitstream format for EVS decoder*/
	Ipp32s use_parital_copy;		/* a flag that indicates if partial frame is used, EVS RTP only*/
	Ipp32s next_coder_type;			/* next frame coder type information, for EVS RTP format only*/
    //SIDSyncState sid_state;
    Ipp32s reserved0; // for future extension
    Ipp32s reserved1; // for future extension
} EVS_Handle_Header;


/* global usc vector table */
USCFUN USC_Fxns USC_EVS_Fxns={
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
    SetFrameSize,
	FeedJBMFrame,
	GetJBMFrame
};


static __ALIGN32 CONST Ipp32s BitsLenTbl[EVS_NUM_RATES] = {
	132, 177, 253, 285, 317, 365, 397, 461, 477,
	118, 144, 192, 264, 328, 488, 640, 960, 1280, 1920, 2560 
};

static __ALIGN32 CONST USC_Rates pTblRates_EVS[EVS_NUM_RATES] = {
	{ 23850 },
	{ 23050 },
	{ 19850 },
	{ 18250 },
	{ 15850 },
	{ 14250 },
	{ 12650 },
	{ 8850 },
	{ 6600 },
	{ 5900 },
	{7200 },
	{8000},
	{9600},
	{13200},
	{16400},
	{24400},
	{32000},
	{48000},
	{64000},
	{96000},
	{128000},
};

static __ALIGN32 CONST USC_PCMType pcmTypesTbl_EVS[4]={
   {EVS_SAMPLE_FREQUENCY_8,EVS_BITS_PER_SAMPLE, EVS_NCHANNELS},
   {EVS_SAMPLE_FREQUENCY_16, EVS_BITS_PER_SAMPLE, EVS_NCHANNELS},
   {EVS_SAMPLE_FREQUENCY_32, EVS_BITS_PER_SAMPLE, EVS_NCHANNELS},
   {EVS_SAMPLE_FREQUENCY_48, EVS_BITS_PER_SAMPLE, EVS_NCHANNELS},
};
Ipp16s EVS_enc_delay(Ipp32s Fs)
{

	return(NS2SA(Fs, evs_get_delay_fx(ENC, Fs)));
}
Ipp16s EVS_dec_delay(Ipp32s Fs)
{

	return(NS2SA(Fs, evs_get_delay_fx(DEC, Fs)));
}

static USC_Status GetInfoSize(Ipp32s *pSize)
{
   USC_CHECK_PTR(pSize);

   *pSize = sizeof(USC_CodecInfo);
   return USC_NoError;
}

static USC_Status GetInfo(USC_Handle handle, USC_CodecInfo *pInfo)
{
    EVS_Handle_Header *evs_header;

    USC_CHECK_PTR(pInfo);

    pInfo->name = "IPP_EVS";

    if (handle == NULL) {
		pInfo->params.framesize = EVS_Frame*sizeof(Ipp16s);
        pInfo->params.modes.bitrate = 6600;
        pInfo->params.modes.truncate = 1;
        pInfo->params.direction = USC_DECODE;
        pInfo->params.modes.vad = 1;
		pInfo->params.pcmType.sample_frequency = pcmTypesTbl_EVS[1].sample_frequency;  // set fot 16000 for now
		pInfo->params.pcmType.nChannels = pcmTypesTbl_EVS[1].nChannels;
		pInfo->params.pcmType.bitPerSample = pcmTypesTbl_EVS[1].bitPerSample;
    } else {
      evs_header = (EVS_Handle_Header*)handle;

	  if (evs_header->direction == USC_ENCODE)
	  {
		  Intel_EVS_Encoder_Cntrl_t EnCntrl;
		  void *EncObj = (void *)((Ipp8s*)handle + sizeof(EVS_Handle_Header));

		  Intel_EVS_Encoder_GetControl(EncObj, &EnCntrl);

		  pInfo->params.pcmType.sample_frequency = EnCntrl.input_Fs_fx;
		  pInfo->params.framesize = EnCntrl.input_Fs_fx / 50 * sizeof(Ipp16s);					/*frame size in bytes, 20ms frames */
		  pInfo->params.modes.vad = EnCntrl.Opt_DTX_ON_fx;
		  pInfo->params.modes.Opt_RF_ON = EnCntrl.Opt_RF_ON;
		  pInfo->params.modes.rf_fec_offset = EnCntrl.rf_fec_offset;
		  pInfo->params.modes.rf_fec_indicator = EnCntrl.rf_fec_indicator;
		  pInfo->params.modes.max_bwidth_fx = EnCntrl.max_bwidth_fx;
		  pInfo->params.modes.interval_SID_fx = EnCntrl.interval_SID_fx;
	  }
	  else
	  {
		  Intel_EVS_Decoder_Cntrl_t DeCntrl;
		  void *DecObj = (void *)((Ipp8s*)handle + sizeof(EVS_Handle_Header));

		  Intel_EVS_Decoder_GetControl(DecObj, &DeCntrl);
		  pInfo->params.pcmType.sample_frequency = DeCntrl.output_Fs_fx;
		  pInfo->params.framesize = DeCntrl.output_Fs_fx / 50 * sizeof(Ipp16s);					/*frame size in bytes, 20ms frames */
	  }
      pInfo->params.modes.bitrate = evs_header->bitrate;
      pInfo->params.modes.truncate = evs_header->trunc;
      pInfo->params.direction = evs_header->direction;
      pInfo->params.modes.vad = evs_header->vad;
    }
	pInfo->maxbitsize = MAX_AU_SIZE;
    pInfo->nPcmTypes = 4;
    pInfo->pPcmTypesTbl = pcmTypesTbl_EVS;
    pInfo->params.modes.hpf = 0;
    pInfo->params.modes.pf = 0;
    pInfo->params.modes.outMode = USC_OUT_NO_CONTROL;
    pInfo->params.law = 0;
    pInfo->nRates = EVS_NUM_RATES;
    pInfo->pRateTbl = (const USC_Rates *)&pTblRates_EVS;
    pInfo->params.nModes = sizeof(USC_Modes)/sizeof(Ipp32s);

    return USC_NoError;
}

static Ipp32s CheckRate_EVS(Ipp32s rate_bps)
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
	  case 5900:  rate = 9; break;
	  case 7200:  rate = 10; break;
	  case 8000:  rate = 11; break;
	  case 9600:  rate = 12; break;
	  case 13200: rate = 13; break;
	  case 16400: rate = 14; break;
	  case 24400: rate = 15; break;
	  case 32000: rate = 16; break;
	  case 48000: rate = 17; break;
	  case 64000: rate = 18; break;
	  case 96000: rate = 19; break;
	  case 128000:rate = 20; break;
      default: rate = -1; break;
    }
    return rate;
}

static USC_Status NumAlloc(const USC_Option *options, Ipp32s *nbanks)
{
   USC_CHECK_PTR(options);
   USC_CHECK_PTR(nbanks);

   if ((options->modes.jbm_enabled == 1) && (options->direction == USC_DECODE))
   {
	   *nbanks = 2;			/* the 2nd memory block is for jitter buffer module */
   }
   else
   {
	   *nbanks = 1;
   }
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
	  nbytes = Intel_EVS_Encoder_Init(NULL, NULL);
	  pBanks->nbytes = nbytes + sizeof(EVS_Handle_Header); /* include header in handle */
   }
   else if (options->direction == USC_DECODE) /* decode only */
   {
	  nbytes = Intel_EVS_Decoder_Init(NULL, NULL);
	  pBanks->nbytes = nbytes + sizeof(EVS_Handle_Header); /* include header in handle */

	  if (options->modes.jbm_enabled == 1)
	  {
		  USC_MemBank *p2ndBank = pBanks + 1;

		  p2ndBank->pMem = NULL;
		  p2ndBank->align = 32;
		  p2ndBank->memType = USC_OBJECT;
		  p2ndBank->memSpaceType = USC_NORMAL;
		  p2ndBank->nbytes = Intel_EVS_Decoder_JB_GetSize();
	  }
   } 
   else 
   {
      return USC_NoOperation;
   }

   return USC_NoError;
}

static USC_Status Init(const USC_Option *options, const USC_MemBank *pBanks, USC_Handle *handle)
{
   EVS_Handle_Header *evs_header;
   Ipp32s bitrate_idx;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(pBanks);
   USC_CHECK_PTR(pBanks->pMem);
   USC_BADARG(pBanks->nbytes<=0, USC_NotInitialized);
   USC_CHECK_HANDLE(handle);
   USC_BADARG(options->modes.vad > 1, USC_UnsupportedVADType);
   USC_BADARG(options->modes.vad < 0, USC_UnsupportedVADType);

   USC_BADARG(options->pcmType.bitPerSample!=EVS_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
   USC_BADARG((options->pcmType.sample_frequency!=EVS_SAMPLE_FREQUENCY_8) 
	   && (options->pcmType.sample_frequency != EVS_SAMPLE_FREQUENCY_16)
	   && (options->pcmType.sample_frequency != EVS_SAMPLE_FREQUENCY_32)
	   && (options->pcmType.sample_frequency != EVS_SAMPLE_FREQUENCY_48),
		USC_UnsupportedPCMType);
   USC_BADARG(options->pcmType.nChannels!=EVS_NCHANNELS, USC_UnsupportedPCMType);

   bitrate_idx = CheckRate_EVS(options->modes.bitrate);
   if(bitrate_idx < 0) return USC_UnsupportedBitRate;

   *handle = (USC_Handle*)pBanks->pMem;
   evs_header = (EVS_Handle_Header*)*handle;

   evs_header->direction = options->direction;
   evs_header->trunc = options->modes.truncate;
   evs_header->vad = options->modes.vad;
   evs_header->bitrate = options->modes.bitrate;
   evs_header->bitrate_old = evs_header->bitrate;

   if (options->direction == USC_ENCODE) /* encode only */
   {
	   Intel_EVS_Encoder_Cntrl_t EnCntrl = { 0 };
       void *EncObj = (void *)((Ipp8s*)*handle + sizeof(EVS_Handle_Header));

	   EnCntrl.input_Fs_fx = options->pcmType.sample_frequency;
	   EnCntrl.total_brate_fx = options->modes.bitrate;
	   EnCntrl.Opt_DTX_ON_fx = options->modes.vad;

	   EnCntrl.Opt_RF_ON = options->modes.Opt_RF_ON;
	   EnCntrl.rf_fec_offset = options->modes.rf_fec_offset;
	   EnCntrl.rf_fec_indicator = options->modes.rf_fec_indicator;
	   EnCntrl.max_bwidth_fx =  options->modes.max_bwidth_fx;
	   EnCntrl.interval_SID_fx = 1; // FIXED_SID_RATE;

	   //EnCntrl.Opt_AMR_WB_fx = 1; //remove later
	   //EnCntrl.codec_mode = 1;    //remove later

	   EnCntrl.brate_bw_update = EVS_UPDATE_BRATE | EVS_UPDATE_BWIDTH; 
       Intel_EVS_Encoder_Init(EncObj, &EnCntrl);
   }
   else if (options->direction == USC_DECODE) /* decode only */
   {
	   Intel_EVS_Decoder_Cntrl_t DeCntrl = { 0 };
       void *DecObj = (void *)((Ipp8s*)*handle + sizeof(EVS_Handle_Header));

	   DeCntrl.output_Fs_fx = options->pcmType.sample_frequency;
	   DeCntrl.total_brate_fx = options->modes.bitrate;
	   DeCntrl.bitstreamformat = options->modes.bitstreamformat;

	   if (options->modes.jbm_enabled)
	   {
		   USC_CHECK_PTR(pBanks[1].pMem);
		   USC_BADARG(pBanks[1].nbytes <= 0, USC_NotInitialized);

		   memset(pBanks[1].pMem, 0, pBanks[1].nbytes);
		   DeCntrl.bitstreamformat = 2;						// RTP format 
		   /* Initialize the jitter buffer module */
		   Intel_EVS_Decoder_JB_Init(pBanks[1].pMem, DecObj, &DeCntrl);
	   }

	   Intel_EVS_Decoder_Init(DecObj, &DeCntrl);
	   evs_header->usedRate = options->modes.bitrate;
       evs_header->reset_flag = 0;
       evs_header->reset_flag_old = 1;
	   evs_header->bitstreamformat = options->modes.bitstreamformat;
   } 
   else 
   {
      *handle=NULL;
      return USC_NoOperation;
   }
   return USC_NoError;
}

static USC_Status Reinit(const USC_Modes *modes, USC_Handle handle )
{
    EVS_Handle_Header *evs_header;
    Ipp32s bitrate_idx;

    USC_CHECK_PTR(modes);
    USC_CHECK_HANDLE(handle);

    USC_BADARG(modes->vad > 1, USC_UnsupportedVADType);
    USC_BADARG(modes->vad < 0, USC_UnsupportedVADType);

    evs_header = (EVS_Handle_Header*)handle;

    bitrate_idx = CheckRate_EVS(modes->bitrate);
    if(bitrate_idx < 0) return USC_UnsupportedBitRate;
    evs_header->vad = modes->vad;
    evs_header->bitrate = modes->bitrate;
    evs_header->bitrate_old = evs_header->bitrate;

    if (evs_header->direction == USC_ENCODE) /* encode only */
    {
		Intel_EVS_Encoder_Cntrl_t EnCntrl = { 0 };
		void *EncObj = (void *)((Ipp8s*)handle + sizeof(EVS_Handle_Header));

		Intel_EVS_Encoder_GetControl(EncObj, &EnCntrl);
		//EnCntrl.input_Fs_fx = options->pcmType.sample_frequency;
		EnCntrl.total_brate_fx = modes->bitrate;
		EnCntrl.Opt_DTX_ON_fx = modes->vad;

		EnCntrl.Opt_RF_ON = modes->Opt_RF_ON;
		EnCntrl.rf_fec_offset = modes->rf_fec_offset;
		EnCntrl.rf_fec_indicator = modes->rf_fec_indicator;
		EnCntrl.max_bwidth_fx = modes->max_bwidth_fx;
		EnCntrl.interval_SID_fx = FIXED_SID_RATE;

		//EnCntrl.Opt_AMR_WB_fx = 1; //remove later
		//EnCntrl.codec_mode = 1;    //remove later
		EnCntrl.brate_bw_update = EVS_UPDATE_BRATE | EVS_UPDATE_BWIDTH;
		Intel_EVS_Encoder_Init(EncObj, &EnCntrl);
        evs_header->reset_flag = 0;
    }
    else if (evs_header->direction == USC_DECODE) /* decode only */
    {
		Intel_EVS_Decoder_Cntrl_t DeCntrl = { 0 };
		void *DecObj = (void *)((Ipp8s*)handle + sizeof(EVS_Handle_Header));

		Intel_EVS_Decoder_GetControl(DecObj, &DeCntrl);
		DeCntrl.total_brate_fx = modes->bitrate;
		DeCntrl.bitstreamformat = modes->bitstreamformat;
		Intel_EVS_Decoder_Init(DecObj, &DeCntrl);
        evs_header->usedRate = 6600;
        evs_header->reset_flag = 0;
		evs_header->reset_flag_old = 1;
		evs_header->bitstreamformat = modes->bitstreamformat;
    } else {
        return USC_NoOperation;
    }
    return USC_NoError;
}

static USC_Status Control(const USC_Modes *modes, USC_Handle handle )
{
   EVS_Handle_Header *evs_header;
   Ipp32s bitrate_idx;

   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);

   USC_BADARG(modes->vad > 1, USC_UnsupportedVADType);
   USC_BADARG(modes->vad < 0, USC_UnsupportedVADType);

   bitrate_idx = CheckRate_EVS(modes->bitrate);
   USC_BADARG(bitrate_idx < 0,USC_UnsupportedBitRate);

   evs_header = (EVS_Handle_Header*)handle;

   evs_header->trunc = modes->truncate;
   evs_header->vad = modes->vad;
   evs_header->bitrate = modes->bitrate;

   if (evs_header->direction == USC_ENCODE) /* encode only */
   {
	   Intel_EVS_Encoder_Cntrl_t EnCntrl;
	   void *EncObj = (void *)((Ipp8s*)handle + sizeof(EVS_Handle_Header));

	   Intel_EVS_Encoder_GetControl(EncObj, &EnCntrl);
	   EnCntrl.total_brate_fx = modes->bitrate;
	   EnCntrl.Opt_DTX_ON_fx = modes->vad;

	   EnCntrl.Opt_RF_ON = modes->Opt_RF_ON;
	   EnCntrl.rf_fec_offset = modes->rf_fec_offset;
	   EnCntrl.rf_fec_indicator = modes->rf_fec_indicator;
	   EnCntrl.max_bwidth_fx = modes->max_bwidth_fx;
	   EnCntrl.interval_SID_fx = modes->interval_SID_fx; // FIXED_SID_RATE;

	   EnCntrl.brate_bw_update = 0;

	   if (modes->update_brate_on)
	   {
		   EnCntrl.brate_bw_update = EVS_UPDATE_BRATE;
	   }

	   if (modes->update_bwidth_on)
	   {
		   EnCntrl.brate_bw_update |= EVS_UPDATE_BWIDTH;
	   }

	   Intel_EVS_Encoder_Set_Params(EncObj, &EnCntrl);
   }
   else if (evs_header->direction == USC_DECODE) /* decode only */
   {
	   //Intel_EVS_Decoder_Cntrl_t DeCntrl;
	   //void *DecObj = (void *)((Ipp8s*)handle + sizeof(EVS_Handle_Header));

	   //Intel_EVS_Decoder_GetControl(DecObj, &DeCntrl);
	   //DeCntrl.total_brate_fx = modes->bitrate;
	   evs_header->use_parital_copy = modes->use_parital_copy;
	   evs_header->next_coder_type = modes->next_coder_type;
	   evs_header->bitstreamformat = modes->bitstreamformat;
	   //Intel_EVS_Decoder_Set_Params(DecObj, &DeCntrl);
   }

   return USC_NoError;
}


static Ipp32s getBitstreamSize(Ipp32s rate, Ipp32s frametype, Ipp32s *outRate)
{
    Ipp32s nbytes;
    Ipp32s usedRate = rate;

	/* not sure why we need this, set rate index to 0 for now*/
	rate = 0;

    nbytes = ((BitsLenTbl[usedRate] + 7) / 8);
    *outRate = rate;

    return nbytes;
}

static USC_Status Encode(USC_Handle handle, USC_PCMStream *in, USC_Bitstream *out)
{
    EVS_Handle_Header *evs_header;
	Ipp32s bitrate_idx;
	Ipp16s total_bits = 0;
    void *EncObj;
	//Ipp16u work_buf[EVS_SAMPLE_FREQUENCY_48*2/50];
	Intel_EVS_Encoder_Cntrl_t EnCntrl;
	Intel_EVS_Encoder_Status_t EnStatus;
	Indice_fx ind_list[MAX_NUM_INDICES];

    USC_CHECK_PTR(in);
    USC_CHECK_PTR(out);
    USC_CHECK_HANDLE(handle);

    USC_BADARG(NULL == in->pBuffer, USC_NoOperation);
    USC_BADARG(NULL == out->pBuffer, USC_NoOperation);

//    USC_BADARG(in->nbytes<EVS_Frame*sizeof(Ipp16s), USC_NoOperation);

    USC_BADARG(in->pcmType.bitPerSample!=EVS_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
    USC_BADARG((in->pcmType.sample_frequency!=EVS_SAMPLE_FREQUENCY_8) 
	   && (in->pcmType.sample_frequency != EVS_SAMPLE_FREQUENCY_16)
	   && (in->pcmType.sample_frequency != EVS_SAMPLE_FREQUENCY_32)
	   && (in->pcmType.sample_frequency != EVS_SAMPLE_FREQUENCY_48),
		USC_UnsupportedPCMType);
    USC_BADARG(in->pcmType.nChannels!=EVS_NCHANNELS, USC_UnsupportedPCMType);

    evs_header = (EVS_Handle_Header*)handle;

    if(evs_header == NULL) return USC_InvalidHandler;

    if (evs_header->direction != USC_ENCODE) return USC_NoOperation;

    bitrate_idx = CheckRate_EVS(in->bitrate);
    USC_BADARG(bitrate_idx < 0,USC_UnsupportedBitRate);
    evs_header->bitrate = in->bitrate;

    EncObj = (void *)((Ipp8s*)handle + sizeof(EVS_Handle_Header));

#if(0)
    if (evs_header->trunc) {
       /* Delete the LSBs */
		ippsAndC_16u((Ipp16u*)in->pBuffer, (Ipp16u)IO_MASK, work_buf, in->pcmType.sample_frequency *2 / 50);
    } else {
		ippsCopy_16s((const Ipp16s*)in->pBuffer, (Ipp16s*)work_buf, in->pcmType.sample_frequency * 2 / 50);
    }
#endif
	/* update control parameters */
	Intel_EVS_Encoder_GetControl(EncObj, &EnCntrl);
	EnCntrl.total_brate_fx = in->bitrate;
#if(0)
	EnCntrl.Opt_RF_ON = in->Opt_RF_ON;
	EnCntrl.rf_fec_offset = in->rf_fec_offset;
	EnCntrl.rf_fec_indicator = in->rf_fec_indicator;
	EnCntrl.max_bwidth_fx = in->max_bwidth_fx;
	EnCntrl.interval_SID_fx = FIXED_SID_RATE;
#endif
	Intel_EVS_Encoder_Set_Params(EncObj, &EnCntrl);
	
	Intel_EVS_Encoder_Exec(EncObj, (Ipp16s *)in->pBuffer, (in->nbytes >> 1), ind_list);

	Intel_EVS_Encoder_GetStatus(EncObj, &EnStatus);

	evs_indices_to_serial_generic(ind_list, MAX_NUM_INDICES, (UWord8 *)out->pBuffer, &total_bits);

	out->nbytes = (total_bits + 7) >> 3;
	//out->nbytes = total_bits;

	out->bitrate = total_bits * 50;         // EnStatus.total_brate_fx_in_use;
	out->frametype = 0;						// need to give the frame type !!!!!

    //in->nbytes = EVS_Frame*sizeof(Ipp16s);
    return USC_NoError;
}

static USC_Status Decode(USC_Handle handle, USC_Bitstream *in, USC_PCMStream *out)
{
    EVS_Handle_Header *evs_header;
    void  *DecObj;
    Ipp32s bitrate_idx;
	Intel_EVS_Decoder_Cntrl_t DeCntrl;
	Intel_EVS_Decoder_Status_t DeStatus;
	Ipp8u *pInputStream;

    USC_CHECK_PTR(out);
    USC_BADARG(NULL == out->pBuffer, USC_NoOperation);
    USC_CHECK_HANDLE(handle);

    evs_header = (EVS_Handle_Header*)handle;
    DecObj = (void *)((Ipp8s*)handle + sizeof(EVS_Handle_Header));

    if (USC_DECODE != evs_header->direction) return USC_NoOperation;

	Intel_EVS_Decoder_GetControl(DecObj, &DeCntrl);

	if (in)
	{
		bitrate_idx = evs_rate2EVSmode(in->bitrate);
		if (bitrate_idx < 0) return USC_UnsupportedBitRate;
		evs_header->bitrate = in->bitrate;
		out->bitrate = in->bitrate;
		DeCntrl.total_brate_fx = in->bitrate;

		if (in->frametype == -1)
		{
			DeCntrl.bfi_fx = 1;
		}
		else
		{
			DeCntrl.bfi_fx = 0;
		}

		if (in->pBuffer == NULL)
		{
			DeCntrl.bfi_fx = 1;
			pInputStream = (Ipp8u*)LostFrame;
		}
		else
		{
			pInputStream = (Ipp8u*)(in->pBuffer);
		}
	}
	else
	{
		/* for packet lost */
		evs_header->bitrate = DeCntrl.total_brate_fx;
		out->bitrate = DeCntrl.total_brate_fx;
		DeCntrl.bfi_fx = 1;
		pInputStream = (Ipp8u*)LostFrame;
	}

	//DeCntrl.bitstreamformat = 0; // G.1912 for now
	//DeCntrl.output_Fs_fx = EVS_SAMPLE_FREQUENCY_48; // for now
	DeCntrl.amrwb_rfc4867_flag = 0;
	DeCntrl.use_partial_copy = evs_header->use_parital_copy;
	DeCntrl.next_coder_type = evs_header->next_coder_type;
	DeCntrl.bitstreamformat = evs_header->bitstreamformat;

	Intel_EVS_Decoder_Set_Params(DecObj, &DeCntrl);
	Intel_EVS_Decoder_Exec(DecObj, (Ipp8u *)pInputStream, (Ipp16s *)out->pBuffer);
	Intel_EVS_Decoder_GetStatus(DecObj, &DeStatus);

	out->nbytes = DeCntrl.output_Fs_fx*2/50;    

    out->pcmType.bitPerSample = EVS_BITS_PER_SAMPLE;
    out->pcmType.nChannels = EVS_NCHANNELS;
    out->pcmType.sample_frequency = DeCntrl.output_Fs_fx;

    return USC_NoError;
}

#if(1)
static USC_Status CalsOutStreamSize(const USC_Option *options, Ipp32s bitrate_idx, Ipp32s nbytesSrc, Ipp32s *nbytesDst)
{
   Ipp32s nBlocks, nSamples, foo;

   if(options->direction==USC_ENCODE) { /*Encode: src - PCMStream, dst - bitstream*/
      if(options->modes.vad>1) return USC_UnsupportedVADType;

      nSamples = nbytesSrc / (EVS_BITS_PER_SAMPLE >> 3);
      nBlocks = nSamples / EVS_Frame;

      if (0 == nBlocks) return USC_NoOperation;

      if (0 != nSamples % EVS_Frame) {
         /* Add another block to hold the last compressed fragment*/
         nBlocks++;
      }

      *nbytesDst = nBlocks * getBitstreamSize(bitrate_idx, 0, &foo);
   } else if(options->direction==USC_DECODE) {/*Decode: src - bitstream, dst - PCMStream*/
      if(options->modes.vad==0) { /*VAD off*/
         nBlocks = nbytesSrc / getBitstreamSize(bitrate_idx, 0, &foo);
      } else if(options->modes.vad==1) { /*VAD on*/
         nBlocks = nbytesSrc / getBitstreamSize(bitrate_idx, 0, &foo);
      } else return USC_UnsupportedVADType;

      if (0 == nBlocks) return USC_NoOperation;

      nSamples = nBlocks * EVS_Frame;
      *nbytesDst = nSamples * (EVS_BITS_PER_SAMPLE >> 3);
   } else return USC_NoOperation;

   return USC_NoError;
}

static USC_Status GetOutStreamSize(const USC_Option *options, Ipp32s bitrate, Ipp32s nbytesSrc, Ipp32s *nbytesDst)
{
   Ipp32s bitrate_idx;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(nbytesDst);
   USC_BADARG(nbytesSrc <= 0, USC_NoOperation);

   bitrate_idx = CheckRate_EVS(bitrate);
    if(bitrate_idx < 0) return USC_UnsupportedBitRate;

   return CalsOutStreamSize(options, bitrate_idx, nbytesSrc, nbytesDst);
}
#endif

USC_Status SetFrameSize(const USC_Option *options, USC_Handle handle, Ipp32s frameSize)
{
   USC_CHECK_PTR(options);
   USC_CHECK_HANDLE(handle);
   USC_BADARG(frameSize <= 0, USC_BadArgument);

   return USC_NoOperation;
}

static USC_Status FeedJBMFrame(USC_Handle handle, USC_MemBank *pBanks, USC_Bitstream *in)
{
	EVS_Handle_Header *evs_header;
	void  *DecObj;
	Intel_EVS_Decoder_Cntrl_t DeCntrl;
	RTP_Frame rtp_data;

	USC_CHECK_HANDLE(handle);

	evs_header = (EVS_Handle_Header*)handle;
	DecObj = (void *)((Ipp8s*)handle + sizeof(EVS_Handle_Header));

	if (USC_DECODE != evs_header->direction) return USC_NoOperation;

	Intel_EVS_Decoder_GetControl(DecObj, &DeCntrl);

	if (in)
	{
		rtp_data.frame = in->pBuffer;
		rtp_data.frameSize = in->nbytes;
		rtp_data.rtpSequenceNumber = in->rtp_info.rtpSequenceNumber;
		rtp_data.rtpTimeStamp = in->rtp_info.rtpTimeStamp;
		rtp_data.RcvTime_ms = in->rtp_info.RcvTime_ms;
	}
	else
	{
		/* error! */
		return(1);
	}

	Intel_EVS_Decoder_JB_FeedFrame(pBanks[1].pMem, &rtp_data);

	return USC_NoError;
}

static USC_Status GetJBMFrame(USC_Handle handle, USC_MemBank *pBanks, USC_PCMStream *out, USC_Modes *modes)
{
	EVS_Handle_Header *evs_header;
	void  *DecObj;
	Intel_EVS_Decoder_Cntrl_t DeCntrl;
	Intel_EVS_Decoder_Status_t DeStatus;
	Ipp16s nSamples;

	USC_CHECK_PTR(out);
	USC_BADARG(NULL == out->pBuffer, USC_NoOperation);
	USC_CHECK_HANDLE(handle);

	evs_header = (EVS_Handle_Header*)handle;
	DecObj = (void *)((Ipp8s*)handle + sizeof(EVS_Handle_Header));

	if (USC_DECODE != evs_header->direction) return USC_NoOperation;

	Intel_EVS_Decoder_GetControl(DecObj, &DeCntrl);

	DeCntrl.bitstreamformat = 2; // G.1912 for now
	//	DeCntrl.output_Fs_fx = EVS_SAMPLE_FREQUENCY_48; // for now
	DeCntrl.amrwb_rfc4867_flag = 0;

	Intel_EVS_Decoder_JB_GetSamples(pBanks[1].pMem, &nSamples, (Ipp16s *)out->pBuffer, out->nbytes, modes->system_time_ms, &DeCntrl, &DeStatus);

	out->nbytes = nSamples * 2; // DeCntrl.output_Fs_fx * 2 / 50;

	out->pcmType.bitPerSample = EVS_BITS_PER_SAMPLE;
	out->pcmType.nChannels = EVS_NCHANNELS;
	out->pcmType.sample_frequency = DeCntrl.output_Fs_fx;

	modes->rf_fec_indicator = DeStatus.fec_hi;
	modes->rf_fec_offset = DeStatus.fec_offset;
	modes->jbIsEmpty = DeStatus.jbIsEmpty;

	return USC_NoError;
}
