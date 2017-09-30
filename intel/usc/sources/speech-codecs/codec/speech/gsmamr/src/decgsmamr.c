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
// Purpose: GSMAMR speech codec: decoder API.
//
*/

#include "owngsmamr.h"

/* filter coefficients (fc = 100 Hz) */

static __ALIGN32 CONST Ipp16s b100[3] = {7699, -15398, 7699};      /* Q13 */
static __ALIGN32 CONST Ipp16s a100[3] = {8192, 15836, -7667};      /* Q13 */


GSMAMR_CODECFUN( APIGSMAMR_Status, apiGSMAMRDecoder_GetSize,
         (GSMAMRDecoder_Obj* decoderObj, Ipp32u *pCodecSize))
{
   if(NULL == decoderObj)
      return APIGSMAMR_StsBadArgErr;
   if(NULL == pCodecSize)
      return APIGSMAMR_StsBadArgErr;
   if(decoderObj->objPrm.key != DEC_KEY)
      return APIGSMAMR_StsNotInitialized;

   *pCodecSize = decoderObj->objPrm.objSize;
   return APIGSMAMR_StsNoErr;
}


GSMAMR_CODECFUN( APIGSMAMR_Status, apiGSMAMRDecoder_Alloc,
         (const GSMAMRDec_Params *gsm_Params, Ipp32u *pSizeTab))
{
   Ipp32s fltSize;
   Ipp32s allocSize=sizeof(GSMAMRDecoder_Obj);
   if(GSMAMR_CODEC != gsm_Params->codecType)
      return APIGSMAMR_StsBadCodecType;
   ippsHighPassFilterSize_G729(&fltSize);
   allocSize += fltSize; /* to provide memory for postprocessing high pass filter with upscaling */
   pSizeTab[0] =  allocSize;
   return APIGSMAMR_StsNoErr;
}

Ipp32s ownDtxDecoderInit_GSMAMR(sDTXDecoderSt* dtxState)
{
   Ipp32s i;

   dtxState->vLastSidFrame = 0;
   dtxState->vSidPeriodInv = (1 << 13);
   dtxState->vLogEnergy = 3500;
   dtxState->vLogEnergyOld = 3500;
   /* low level noise for better performance in  DTX handover cases*/

   dtxState->vPerfSeedDTX_long = PSEUDO_NOISE_SEED;

   /* Initialize state->a_LSP [] and state->a_LSP_Old [] */
   ippsCopy_16s(TableLSPInitData, &dtxState->a_LSP[0], LP_ORDER_SIZE);
   ippsCopy_16s(TableLSPInitData, &dtxState->a_LSP_Old[0], LP_ORDER_SIZE);
   dtxState->vLogMean = 0;
   dtxState->vLogEnergyHistory = 0;

   /* initialize decoder lsf history */
   ippsCopy_16s(TableMeanLSF_5, &dtxState->a_LSFHistory[0], LP_ORDER_SIZE);

   for (i = 1; i < DTX_HIST_SIZE; i++)
      ippsCopy_16s(&dtxState->a_LSFHistory[0], &dtxState->a_LSFHistory[LP_ORDER_SIZE*i], LP_ORDER_SIZE);

   ippsZero_16s(dtxState->a_LSFHistoryMean, LP_ORDER_SIZE*DTX_HIST_SIZE);

   /* initialize decoder log frame energy */
   ippsSet_16s(dtxState->vLogEnergy, dtxState->a_LogEnergyHistory, DTX_HIST_SIZE);
   dtxState->vLogEnergyCorrect = 0;
   dtxState->vDTXHangoverCt = DTX_HANG_PERIOD;
   dtxState->vDecExpireCt = 32767;
   dtxState->vFlagSidFrame = 0;
   dtxState->vFlagValidData = 0;
   dtxState->vDTXHangAdd = 0;
   dtxState->eDTXPrevState = DTX;
   dtxState->vFlagDataUpdate = 0;
   return 1;
}

Ipp32s ownPhDispInit_GSMAMR(sPhaseDispSt* PhState)
{
   ippsZero_16s(PhState->a_GainMemory, LTP_GAIN_MEM_SIZE);
   PhState->vPrevState = 0;
   PhState->vPrevGain = 0;
   PhState->vFlagLockGain = 0;
   PhState->vOnSetGain = 0;          /* assume no onset in start */

   return 1;
}

Ipp32s ownPostFilterInit_GSMAMR(sPostFilterSt *PostState)
{
   ippsZero_16s(PostState->a_MemSynPst, LP_ORDER_SIZE);
   ippsZero_16s(PostState->a_SynthBuf, LP_ORDER_SIZE);
   PostState->vPastGainScale = 4096;
   PostState->vMemPrevRes = 0;

  return 1;
}

/*************************************************************************
 *
 *   FUNCTION NAME: amr_ef_dtx_dec_reset
 *
 *   PURPOSE:  Resets the static variables of the RX DTX handler to their
 *             initial values
 *
 *************************************************************************/

void amr_ef_dtx_dec_reset(
    AMR_ef_dtx_rx_State_t *dtx_ef_rx,
    Ipp16s taf_count
)
{
    Ipp16s i, j;

    /* suppose infinitely long speech period before start */

    for (i = 0; i < 4 * EF_DTX_HANGOVER; i++)
    {
        dtx_ef_rx->gain_code_old_rx[i] = 0;
    }

    dtx_ef_rx->prev_SID_frames_lost = 0;
    dtx_ef_rx->next_lsf_old = 0;
    dtx_ef_rx->buf_p_rx = 0;

    /* Variable in decoder.c: */
    dtx_ef_rx->gcode0_CN = 0;              /* CNI */
    dtx_ef_rx->gain_code_old_CN = 0;       /* CNI */
    dtx_ef_rx->gain_code_new_CN = 0;       /* CNI */
    dtx_ef_rx->gain_code_muting_CN = 0;    /* CNI */

    /* reset all the decoder state variables */
    /* ------------------------------------- */

    dtx_ef_rx->taf_count = taf_count;

    dtx_ef_rx->rxdtx_ctrl = EF_RX_SP_FLAG | EF_RX_ACTIVE;
    dtx_ef_rx->rxdtx_aver_period = EF_DTX_HANGOVER;
    dtx_ef_rx->rxdtx_N_elapsed = 0x7fff;

    for (i = 0; i < EF_DTX_HANGOVER; i++)
        for (j = 0; j < EF_LPCORDER; j++)
            dtx_ef_rx->lsf_old_rx[i][j] = TableMeanLSF_5[j];

    dtx_ef_rx->L_pn_seed_rx = EF_PN_INITIAL_SEED;
    dtx_ef_rx->rx_dtx_state = EF_CN_INT_PERIOD - 1;

    /* Variables in d_plsf_5.c: */
    for (i = 0; i < EF_LPCORDER; i++)
    {
        dtx_ef_rx->lsf_p_CN[i]   = TableMeanLSF_5[i];    /* CNI */
        dtx_ef_rx->lsf_new_CN[i] = TableMeanLSF_5[i];    /* CNI */
        dtx_ef_rx->lsf_old_CN[i] = TableMeanLSF_5[i];    /* CNI */
    }
    
}

Ipp32s ownDecoderInit_GSMAMR(sDecoderState_GSMAMR* decState, GSMAMR_Rate_t rate, Ipp16s EFR_flag, Ipp16s taf_count)
{

	/* Initialize static pointer */
   decState->pExcVec = decState->a_ExcVecOld + PITCH_MAX_LAG + FLT_INTER_SIZE;

   /* Static vectors to zero */
   ippsZero_16s(decState->a_ExcVecOld, PITCH_MAX_LAG + FLT_INTER_SIZE);

   if (rate != GSMAMR_RATE_DTX)
     ippsZero_16s(decState->a_MemorySyn, LP_ORDER_SIZE);

   /* initialize pitch sharpening */
   decState->vFlagSharp = PITCH_SHARP_MIN;
   decState->vPrevPitchLag = 40;

   decState->EFR_flag = EFR_flag;

   /* Initialize decState->lsp_old [] */

   if (rate != GSMAMR_RATE_DTX) {
      ippsCopy_16s(TableLSPInitData, &decState->a_LSP_Old[0], LP_ORDER_SIZE);
   }

   /* Initialize memories of bad frame handling */
   decState->vPrevBadFr = 0;
   decState->vPrevDegBadFr = 0;
   decState->vStateMachine = 0;

   decState->vLTPLag = 40;
   decState->vBackgroundNoise = 0;
   ippsZero_16s(decState->a_EnergyHistVector, ENERGY_HIST_SIZE);

   /* Initialize hangover handling */
   decState->vCountHangover = 0;
   decState->vVoiceHangover = 0;
   if (rate != GSMAMR_RATE_DTX) {
      ippsZero_16s(decState->a_EnergyHistSubFr, 9);
   }

   ippsZero_16s(decState->a_LTPGainHistory, 9);
   ippsZero_16s(decState->a_GainHistory, CBGAIN_HIST_SIZE);

   /* Initialize hangover handling */
   decState->vHgAverageVar = 0;
   decState->vHgAverageCount= 0;
   if (rate != GSMAMR_RATE_DTX)
     ippsCopy_16s(TableMeanLSF_5, &decState->a_LSPAveraged[0], LP_ORDER_SIZE);
   ippsZero_16s(decState->a_PastQntPredErr, LP_ORDER_SIZE);             /* Past quantized prediction error */

   /* Past dequantized lsfs */
   ippsCopy_16s(TableMeanLSF_5, &decState->a_PastLSFQnt[0], LP_ORDER_SIZE);
   ippsSet_16s(1640, decState->a_LSFBuffer, 5);
   decState->vPastGainZero = 0;
   decState->vPrevGainZero = 16384;
   ippsSet_16s(1, decState->a_GainBuffer, 5);
   decState->vPastGainCode = 0;
   decState->vPrevGainCode = 1;

   if (rate != GSMAMR_RATE_DTX) {
        ippsSet_16s(MIN_ENERGY, decState->a_PastQntEnergy, NUM_PRED_TAPS);
        ippsSet_16s(MIN_ENERGY_M122, decState->a_PastQntEnergy_M122, NUM_PRED_TAPS);
   }

   decState->vCNGen = 21845;
   ownPhDispInit_GSMAMR(&decState->stPhDispState);
  
   if (decState->EFR_flag == 1)
   {
    	amr_ef_dtx_dec_reset(&(decState->dtx.ef_rx), taf_count);    /* EFR DTX init */
   }
   else
   {
	   if (rate != GSMAMR_RATE_DTX){
	     ownDtxDecoderInit_GSMAMR(&decState->dtxDecoderState);
	     ownDecSidSyncReset_GSMAMR(&decState->dtxDecoderState);
	   }
   	}
   
   return 1;
}

/*************************************************************************
* apiGSMAMRDecoder_Init()
*   Initializes the decoder object
**************************************************************************/
GSMAMR_CODECFUN( APIGSMAMR_Status, apiGSMAMRDecoder_Init, (GSMAMRDecoder_Obj* decoderObj, Ipp32u mode, Ipp16s EFR_flag, Ipp16s taf_count ))
{
   Ipp32s fltSize;
   IPP_ALIGNED_ARRAY(16, Ipp16s, abDec, 6);
   Ipp32s obj_size = sizeof(GSMAMRDecoder_Obj);
   ippsZero_16s((Ipp16s*)decoderObj,sizeof(GSMAMRDecoder_Obj)>>1) ;   /* non-initialized data, bug in debug mode AK27.08.01 */
   decoderObj->objPrm.mode = mode;
   decoderObj->objPrm.key = DEC_KEY;
   decoderObj->postProc = (Ipp8s*)decoderObj + sizeof(GSMAMRDecoder_Obj);
   ippsHighPassFilterSize_G729(&fltSize);
   decoderObj->objPrm.objSize = obj_size+fltSize;
   decoderObj->EFR_flag = EFR_flag;

   abDec[0] = a100[0];
   abDec[1] = a100[1];
   abDec[2] = a100[2];
   abDec[3] = b100[0];
   abDec[4] = b100[1];
   abDec[5] = b100[2];
   ownPostFilterInit_GSMAMR(&decoderObj->stPFiltState);
   ippsHighPassFilterInit_G729(abDec,(char *)decoderObj->postProc);
   ownDecoderInit_GSMAMR(&decoderObj->stDecState,decoderObj->rate,decoderObj->EFR_flag,taf_count);
   return APIGSMAMR_StsNoErr;
}


GSMAMR_CODECFUN( APIGSMAMR_Status, apiGSMAMRDecoder_Mode, (GSMAMRDecoder_Obj* decoderObj, Ipp32u mode))
{
   decoderObj->objPrm.mode = mode;
   return APIGSMAMR_StsNoErr;
}

static Ipp32s ownDecode_GSMAMR(sDecoderState_GSMAMR *decState,  GSMAMR_Rate_t rate, Ipp16s *pSynthParm,
    enum enDTXStateType newDTXState, RXFrameType frameType, Ipp16s *pSynthSpeech, Ipp16s *pA_LP);

/*************************************************************************
* apiGSMAMRDecode()
*   Decodes one frame: bitstream -> pcm audio
**************************************************************************/
GSMAMR_CODECFUN( APIGSMAMR_Status, apiGSMAMRDecode,
         (GSMAMRDecoder_Obj* decoderObj,const Ipp8u* src, GSMAMR_Rate_t rate,
          RXFrameType rx_type, Ipp16s* dst))
{
   IPP_ALIGNED_ARRAY(16, Ipp16s, Az_dec, LP_ALL_FRAME);         /* Decoded Az for post-filter          */
   IPP_ALIGNED_ARRAY(16, Ipp16s, prm_buf, MAX_NUM_PRM);
   Ipp16s *prm = prm_buf;
   IPP_ALIGNED_ARRAY(16, Ipp16s, tmp_synth, 160);
   enum enDTXStateType newDTXState;

   if(NULL==decoderObj || NULL==src || NULL ==dst)
      return APIGSMAMR_StsBadArgErr;
   if(0 >= decoderObj->objPrm.objSize)
      return APIGSMAMR_StsNotInitialized;
   if(rate > GSMAMR_RATE_DTX  || rate < 0)
      return APIGSMAMR_StsBadArgErr;
   if(DEC_KEY != decoderObj->objPrm.key)
      return APIGSMAMR_StsBadCodecType;
   decoderObj->rate = rate;
   if (!decoderObj->EFR_flag)
   {
	   newDTXState = ownRX_DTX_Handler_GSMAMR(decoderObj, rx_type);
   }
   else
   {
	   newDTXState = SPEECH;
   }

   /* Synthesis */
   if (!decoderObj->EFR_flag)
   {
	   if(rx_type != RX_NO_DATA) {
		  if( rx_type == RX_SID_BAD || rx_type == RX_SID_UPDATE) {
			 ownBits2Prm_GSMAMR(src,prm,GSMAMR_RATE_DTX);
		  }else{
			 ownBits2Prm_GSMAMR(src,prm,decoderObj->rate);
		  }
   }
   }
   else
   {
	   ownBits2Prm_GSMAMR(src,prm,decoderObj->rate);
   }
   ownDecode_GSMAMR(&decoderObj->stDecState, decoderObj->rate, prm,
          newDTXState, rx_type, (Ipp16s*)dst, Az_dec);
   /* Post-filter */
   ippsPostFilter_GSMAMR_16s(Az_dec, (Ipp16s*)dst, &decoderObj->stPFiltState.vMemPrevRes,
                             &decoderObj->stPFiltState.vPastGainScale,
                             decoderObj->stPFiltState.a_SynthBuf,
                             decoderObj->stPFiltState.a_MemSynPst,
                             tmp_synth, mode2rates[rate]);
   ippsCopy_16s(tmp_synth, (Ipp16s*)dst, 160);
   /* post HP filter, and 15->16 bits */
   ippsHighPassFilter_G729_16s_ISfs((Ipp16s*)dst, FRAME_SIZE_GSMAMR, 13, (char *)decoderObj->postProc);
   return APIGSMAMR_StsNoErr;
}


/*************************************************************************
 *
 *   FUNCTION NAME: rx_dtx
 *
 *   PURPOSE: DTX handler of the speech decoder. Determines when to update
 *            the reference comfort noise parameters (LSF and gain) at the
 *            end of the speech burst. Also classifies the incoming frames
 *            according to SID flag and BFI flag
 *            and determines when the transmission is active during comfort
 *            noise insertion. This function also initializes the pseudo
 *            noise generator shift register.
 *
 *            Operation of the RX DTX handler is based on measuring the
 *            lengths of speech bursts and the lengths of the pauses between
 *            speech bursts to determine when there exists a hangover period
 *            at the end of a speech burst. The idea is to keep in sync with
 *            the TX DTX handler to be able to update the reference comfort
 *            noise parameters at the same time instances.
 *
 *   INPUTS:      *rxdtx_ctrl   Old decoder DTX control word
 *                TAF           Time alignment flag
 *                bfi           Bad frame indicator flag
 *                SID_flag      Silence descriptor flag
 *
 *   OUTPUTS:     *rxdtx_ctrl   Updated decoder DTX control word
 *                rx_dtx_state  Updated state of comfort noise interpolation
 *                              period (global variable)
 *                L_pn_seed_rx  Initialized pseudo noise generator shift
 *                              register (global variable)
 *
 *   RETURN VALUE: none
 *
 *************************************************************************/

void amr_ef_Dtx_Rx(
    AMR_ef_dtx_rx_State_t *dtx_ef_rx,
    Ipp16s frame_type
)
{
    
    /* Update of decoder state */
    /* Previous frame was classified as a speech frame */

    if ((dtx_ef_rx->rxdtx_ctrl & EF_RX_SP_FLAG) != 0)
    {
        /* previous frame was speech */
        switch (frame_type)
        {
          case RX_SPEECH_GOOD:
            dtx_ef_rx->rxdtx_ctrl = EF_RX_ACTIVE | EF_RX_SP_FLAG;
            break;
          case RX_SID_UPDATE:
            dtx_ef_rx->rxdtx_ctrl = EF_RX_ACTIVE | EF_RX_FIRST_SID_UPDATE;
            break;
          case RX_SID_BAD:
          case RX_SID_INVALID:
            dtx_ef_rx->rxdtx_ctrl = EF_RX_ACTIVE
               | EF_RX_FIRST_SID_UPDATE | EF_RX_INVALID_SID_FRAME;
            break;
          default:
          case RX_SPEECH_BAD:
          case RX_SPEECH_BAD_TAF:
            dtx_ef_rx->rxdtx_ctrl = EF_RX_ACTIVE | EF_RX_SP_FLAG;
            break;
        }
    }
    else
    {
        /* previous frame was SID */
        switch (frame_type)
        {
          case RX_SPEECH_GOOD:
            /* If the previous frame (during CNI period) was muted,
               raise the EF_RX_PREV_DTX_MUTING flag */

            if ((dtx_ef_rx->rxdtx_ctrl & EF_RX_DTX_MUTING) != 0)
            {
                dtx_ef_rx->rxdtx_ctrl = EF_RX_ACTIVE | EF_RX_SP_FLAG | EF_RX_FIRST_SP_FLAG
                            | EF_RX_PREV_DTX_MUTING;
            }
            else
            {
                dtx_ef_rx->rxdtx_ctrl = EF_RX_ACTIVE | EF_RX_SP_FLAG | EF_RX_FIRST_SP_FLAG;
            }
            break;
          case RX_SID_UPDATE:
            dtx_ef_rx->rxdtx_ctrl = EF_RX_ACTIVE | EF_RX_CONT_SID_UPDATE;
            break;
          case RX_SID_BAD:
          case RX_SID_INVALID:
            dtx_ef_rx->rxdtx_ctrl = EF_RX_ACTIVE | EF_RX_CONT_SID_UPDATE
                        | EF_RX_INVALID_SID_FRAME;
            break;
          case RX_SPEECH_BAD_TAF:
             /* If an unusable frame is received during CNI period
               when TAF == 1, the frame is classified as a lost
               SID frame */
           dtx_ef_rx->rxdtx_ctrl = EF_RX_ACTIVE | EF_RX_CNI_BFI | EF_RX_LOST_SID_FRAME;
            break;
          default:
          case RX_SPEECH_BAD:
            dtx_ef_rx->rxdtx_ctrl = EF_RX_ACTIVE | EF_RX_CNI_BFI | EF_RX_NO_TRANSMISSION;
            break;
        }
    }


    if ((dtx_ef_rx->rxdtx_ctrl & EF_RX_SP_FLAG) != 0)
    {
        dtx_ef_rx->prev_SID_frames_lost = 0;
        dtx_ef_rx->rx_dtx_state = EF_CN_INT_PERIOD - 1;
    }
    else
    {
        /* First SID frame */

        if ((dtx_ef_rx->rxdtx_ctrl & EF_RX_FIRST_SID_UPDATE) != 0)
        {
            dtx_ef_rx->prev_SID_frames_lost = 0;
            dtx_ef_rx->rx_dtx_state = EF_CN_INT_PERIOD - 1;
        }

        /* SID frame detected, but not the first SID */

        if ((dtx_ef_rx->rxdtx_ctrl & EF_RX_CONT_SID_UPDATE) != 0)
        {
            dtx_ef_rx->prev_SID_frames_lost = 0;


            if (Sub_16s(frame_type, RX_SID_UPDATE) == 0)
            {
                dtx_ef_rx->rx_dtx_state = 0;
            }
            else if ((Sub_16s(frame_type, RX_SID_BAD) == 0)
                  || (Sub_16s(frame_type, RX_SID_INVALID) == 0))
            {

                if (Sub_16s(dtx_ef_rx->rx_dtx_state, (EF_CN_INT_PERIOD - 1)) < 0)
                {
                    dtx_ef_rx->rx_dtx_state = Add_16s(dtx_ef_rx->rx_dtx_state, 1);
                }
            }
        }

        /* Bad frame received in CNI mode */

        if ((dtx_ef_rx->rxdtx_ctrl & EF_RX_CNI_BFI) != 0)
        {
            if (Sub_16s(dtx_ef_rx->rx_dtx_state, (EF_CN_INT_PERIOD - 1)) < 0)
            {
                dtx_ef_rx->rx_dtx_state = Add_16s(dtx_ef_rx->rx_dtx_state, 1);
            }

            /* If an unusable frame is received during CNI period
               when TAF == 1, the frame is classified as a lost
               SID frame */

            if ((dtx_ef_rx->rxdtx_ctrl & EF_RX_LOST_SID_FRAME) != 0)
            {
                dtx_ef_rx->prev_SID_frames_lost = Add_16s(dtx_ef_rx->prev_SID_frames_lost, 1);
            }

            if (Sub_16s(dtx_ef_rx->prev_SID_frames_lost, 1) > 0)
            {
                dtx_ef_rx->rxdtx_ctrl = dtx_ef_rx->rxdtx_ctrl | EF_RX_DTX_MUTING;
            }
        }
    }

    /* N_elapsed (frames since last SID update) is incremented. If SID
       is updated N_elapsed is cleared later in this function */

    dtx_ef_rx->rxdtx_N_elapsed = Add_16s(dtx_ef_rx->rxdtx_N_elapsed, 1);


    if ((dtx_ef_rx->rxdtx_ctrl & EF_RX_SP_FLAG) != 0)
    {
        dtx_ef_rx->rxdtx_aver_period = EF_DTX_HANGOVER;
    }
    else
    {
        if (Sub_16s(dtx_ef_rx->rxdtx_N_elapsed, EF_DTX_ELAPSED_THRESHOLD) > 0)
        {
            dtx_ef_rx->rxdtx_ctrl |= EF_RX_UPD_SID_QUANT_MEM;
            dtx_ef_rx->rxdtx_N_elapsed = 0;
            dtx_ef_rx->rxdtx_aver_period = 0;
            dtx_ef_rx->L_pn_seed_rx = EF_PN_INITIAL_SEED;
        }
        else if (dtx_ef_rx->rxdtx_aver_period == 0)
        {
            dtx_ef_rx->rxdtx_N_elapsed = 0;
        }
        else
        {
            dtx_ef_rx->rxdtx_aver_period = Sub_16s(dtx_ef_rx->rxdtx_aver_period, 1);
        }
	}
       
}



/*************************************************************************
 *
 *   Title:  ef_interpolate_CN_lsf
 *
 *   General Description: Interpolate comfort noise LSF parameter vector over the comfort
 *            noise update period.
 *
 *   Input variables:
 *                lsf_old_CN[0..9]
 *                              The older LSF parameter vector of the
 *                              interpolation (the endpoint the interpolation
 *                              is started from)
 *                lsf_new_CN[0..9]
 *                              The newer LSF parameter vector of the
 *                              interpolation (the endpoint the interpolation
 *                              is ended to)
 *                rx_dtx_state  State of the comfort noise insertion period
 *
 *   Output variables:
 *                lsf_interp_CN[0..9]
 *                              Interpolated LSF parameter vector
 *
 *   Local Variables:
 *   Global Variables:
 *
 *  Function Specifics:
 *
 *************************************************************************/


void ef_interpolate_CN_lsf(
    const Ipp16s ef_lsf_old_CN[EF_LPCORDER],
    const Ipp16s ef_lsf_new_CN[EF_LPCORDER],
    Ipp16s ef_lsf_interp_CN[EF_LPCORDER],
    Ipp16s rx_dtx_state
)
{

    Ipp16s x_l, x_h;
    Ipp32s L_temp;
    Ipp16s i;

    x_h = AG_SC_ef_interp_factor[rx_dtx_state];
    x_l = (Ipp16s)0x8000 + AG_SC_ef_interp_factor[rx_dtx_state];
    for (i = 0; i < EF_LPCORDER; i++)
    {
        L_temp = x_h * ef_lsf_new_CN[i] * 2;
        L_temp -=  x_l * ef_lsf_old_CN[i] * 2;
        ef_lsf_interp_CN[i] = (L_temp + 0x8000) >> 16;
    }    
}


#define ALPHA_5     31128
#define ONE_ALPHA_5 1639

/*
**************************************************************************
*
*  Function    : D_plsf_5
*  Purpose     : Decodes the 2 sets of LSP parameters in a frame
*                using the received quantization indices.
*
**************************************************************************
*/
void amr_ef_D_plsf_5(
    Ipp16s *past_r_q,  /* i/o: State variables */
	Ipp16s *past_lsf_q,
    Ipp16s bfi,             /* i  : bad frame indicator (set to 1 if a bad
                                    frame is received)                         */
    const Ipp16s *ef_indice,/* i  : quantization indices of 5 submatrices, Q0  */
    Ipp16s *lsp1_q,       /* o  : quantized 1st LSP vector (EF_LPCORDER),          Q15 */
    Ipp16s *lsp2_q,       /* o  : quantized 2nd LSP vector (EF_LPCORDER),          Q15 */
    AMR_ef_dtx_rx_State_t *dtx_ef_rx
)
{
/*lint -save -esym(645,ef_lsf1_q) */
    Ipp16s i;
    const Ipp16s *p_dico;
    Ipp16s temp, sign;
    Ipp16s ef_lsf1_r[EF_LPCORDER], ef_lsf2_r[EF_LPCORDER];
    Ipp16s ef_lsf1_q[EF_LPCORDER], ef_lsf2_q[EF_LPCORDER];

    
    /* Update comfort noise LSF quantizer memory */

    if ((dtx_ef_rx->rxdtx_ctrl & EF_RX_UPD_SID_QUANT_MEM) != 0)
    {
         ef_update_lsf_p_CN(dtx_ef_rx->lsf_old_rx, dtx_ef_rx->lsf_p_CN);
    }

    /*  When there is no transmission, SID frame is invalid,
     * or SID frame is lost */

    /* Handle cases of comfort noise LSF decoding in which past
     * valid SID frames are repeated */

    if ((dtx_ef_rx->rxdtx_ctrl & EF_RX_NO_TRANSMISSION) != 0)
    {
        /* DTX active: no transmission. Interpolate LSF values in memory */
        ef_interpolate_CN_lsf(dtx_ef_rx->lsf_old_CN, dtx_ef_rx->lsf_new_CN, ef_lsf2_q, dtx_ef_rx->rx_dtx_state);
    }
    else if ((dtx_ef_rx->rxdtx_ctrl & (EF_RX_INVALID_SID_FRAME|EF_RX_LOST_SID_FRAME)) != 0)
    {
        /* Invalid or lost SID frame: use LSFs from last good SID frame */
        for (i = 0; i < EF_LPCORDER; i++)
        {
            dtx_ef_rx->lsf_old_CN[i] = dtx_ef_rx->lsf_new_CN[i];
            ef_lsf2_q[i] = dtx_ef_rx->lsf_new_CN[i];
            past_r_q[i] = 0;
        }
    }
    else
    {
        if (bfi != 0)                               /* if bad frame */
        {
            /* use the past LSFs slightly shifted towards their mean */

            for (i = 0; i < EF_LPCORDER; i++)
            {
                
				ef_lsf2_q[i] = ef_lsf1_q[i] =(Ipp16s) ((past_lsf_q[i] * ALPHA_5) >> 15) +
                                                 (Ipp16s) ((TableMeanLSF_5[i] * ONE_ALPHA_5) >> 15);


                ef_lsf2_q[i] = ef_lsf1_q[i];
            }

            /* estimate past quantized residual to be used in next frame */

            for (i = 0; i < EF_LPCORDER; i++)
            {
                temp = TableMeanLSF_5[i] + (Ipp16s)((past_r_q[i] * LSP_PRED_FAC_MR122) >> 15);
                past_r_q[i] = ef_lsf2_q[i] - temp;

            }
        }
        else
        {   /* if good LSFs received */
            /* decode prediction residuals from 5 received indices */

            p_dico = &AG_SC_amr_dico1_lsf_5[(ef_indice[0] << 2)];
            ef_lsf1_r[0] = *p_dico++;
            ef_lsf1_r[1] = *p_dico++;
            ef_lsf2_r[0] = *p_dico++;
            ef_lsf2_r[1] = *p_dico++;

            p_dico = &AG_SC_amr_dico2_lsf_5[(ef_indice[1] << 2)];
            ef_lsf1_r[2] = *p_dico++;
            ef_lsf1_r[3] = *p_dico++;
            ef_lsf2_r[2] = *p_dico++;
            ef_lsf2_r[3] = *p_dico++;

            sign = ef_indice[2] & 1;
            i = ef_indice[2] >> 1;
            p_dico = &AG_SC_amr_dico3_lsf_5[(i << 2)];


            if (sign == 0)
            {
                ef_lsf1_r[4] = *p_dico++;
                ef_lsf1_r[5] = *p_dico++;
                ef_lsf2_r[4] = *p_dico++;
                ef_lsf2_r[5] = *p_dico++;
            }
            else
            {
                
				ef_lsf1_r[4] = -(*p_dico++);
                ef_lsf1_r[5] = -(*p_dico++);
                ef_lsf2_r[4] = -(*p_dico++);
                ef_lsf2_r[5] = -(*p_dico++);
            }

            
            p_dico = &AG_SC_amr_dico4_lsf_5[(ef_indice[3] << 2)];
            ef_lsf1_r[6] = *p_dico++;
            ef_lsf1_r[7] = *p_dico++;
            ef_lsf2_r[6] = *p_dico++;
            ef_lsf2_r[7] = *p_dico++;

            
			p_dico = &AG_SC_amr_dico5_lsf_5[(ef_indice[4] << 2)];
            ef_lsf1_r[8] = *p_dico++;
            ef_lsf1_r[9] = *p_dico++;
            ef_lsf2_r[8] = *p_dico++;
            ef_lsf2_r[9] = *p_dico++;


            /* Compute quantized LSFs and update the past quantized residual */
            /* Use lsf_p_CN as predicted LSF vector in case of no speech activity */


            if ((dtx_ef_rx->rxdtx_ctrl & EF_RX_SP_FLAG) != 0)
            {
                for (i = 0; i < EF_LPCORDER; i++)
                {
                    
                    temp = TableMeanLSF_5[i] + (Ipp16s)((past_r_q[i] * LSP_PRED_FAC_MR122) >> 15);
                    ef_lsf1_q[i] = ef_lsf1_r[i] + temp;
                    ef_lsf2_q[i] = ef_lsf2_r[i] + temp;
                    past_r_q[i] = ef_lsf2_r[i];
                }
            }
            else
            {
                for (i = 0; i < EF_LPCORDER; i++)     /* Valid SID frame */
                {
                    
                    ef_lsf2_q[i] = ef_lsf2_r[i] + dtx_ef_rx->lsf_p_CN[i];
                    /* Use the dequantized values of lsf2 also for lsf1 */
                    ef_lsf1_q[i] = ef_lsf2_q[i];
                    past_r_q[i] = 0;
                }
            }
        }

        /* verification that LSFs have minimum distance of AMR_LSF_GAP Hz */
		ownReorderLSFVec_GSMAMR(ef_lsf1_q,LSF_GAP,LP_ORDER_SIZE);
        ownReorderLSFVec_GSMAMR(ef_lsf2_q,LSF_GAP,LP_ORDER_SIZE);        


        if ((dtx_ef_rx->rxdtx_ctrl & EF_RX_FIRST_SID_UPDATE) != 0)
        {
            for (i = 0; i < EF_LPCORDER; i++)
            {
                dtx_ef_rx->lsf_new_CN[i] = ef_lsf2_q[i];
            }
        }
        if ((dtx_ef_rx->rxdtx_ctrl & EF_RX_CONT_SID_UPDATE) != 0)
        {
            for (i = 0; i < EF_LPCORDER; i++)
            {
                dtx_ef_rx->lsf_old_CN[i] = dtx_ef_rx->lsf_new_CN[i];
                dtx_ef_rx->lsf_new_CN[i] = ef_lsf2_q[i];
            }
        }
        if ((dtx_ef_rx->rxdtx_ctrl & EF_RX_SP_FLAG) != 0)
        {
            /*
             * Update lsf history with quantized LSFs when speech activity is
             * present
             */

            if (bfi==0)
            {
                ef_update_lsf_history(ef_lsf1_q, ef_lsf2_q, dtx_ef_rx->lsf_old_rx, &dtx_ef_rx->next_lsf_old);
            }
            else
            {
                ef_update_lsf_history(dtx_ef_rx->lsf_new_CN, dtx_ef_rx->lsf_new_CN, dtx_ef_rx->lsf_old_rx, &dtx_ef_rx->next_lsf_old);
            }

            for (i = 0; i < EF_LPCORDER; i++)
            {
                dtx_ef_rx->lsf_old_CN[i] = ef_lsf2_q[i];
            }
        }
        else
        {
            ef_interpolate_CN_lsf(dtx_ef_rx->lsf_old_CN, dtx_ef_rx->lsf_new_CN, ef_lsf2_q, dtx_ef_rx->rx_dtx_state);
        }
    }

    for (i = 0; i < EF_LPCORDER; i++)
    {
        past_lsf_q[i] = ef_lsf2_q[i];
    }

    /*  convert LSFs to the cosine domain */

    if ((dtx_ef_rx->rxdtx_ctrl & EF_RX_SP_FLAG) != 0)
        ippsLSFToLSP_GSMAMR_16s(ef_lsf1_q, lsp1_q);

    ippsLSFToLSP_GSMAMR_16s(ef_lsf2_q, lsp2_q);

    
/*lint -restore */
}

/*************************************************************************
 *
 *   Title: ef_interpolate_CN_param
 *
 *   General Description: Interpolate a comfort noise parameter value over the
 *            comfort noise update period.
 *
 *   Input variables:
 *                old_param     The older parameter of the interpolation
 *                              (the endpoint the interpolation is started from)
 *                new_param     The newer parameter of the interpolation
 *                              (the endpoint the interpolation is ended to)
 *                rx_dtx_state  State of the comfort noise insertion period
 *
 *   Output variables:
 *   Local Variables:
 *   Global Variables:
 *
 *  Function Specifics:
 *
 *************************************************************************/

Ipp16s ef_interpolate_CN_param(
    Ipp16s old_param,
    Ipp16s new_param,
    Ipp16s rx_dtx_state
)
{
    Ipp16s temp;
    Ipp32s L_temp;

    
    
    L_temp = AG_SC_ef_interp_factor[rx_dtx_state] * new_param * 2;
    temp = 0x7fff - AG_SC_ef_interp_factor[rx_dtx_state];
    temp = temp + 1;
    L_temp += temp * old_param * 2;
    temp = Round_32s16s(L_temp);
    
    
    return temp;
}



/*************************************************************************
 *
 *  FUNCTION:  d_gain_code
 *
 *  PURPOSE:  decode the fixed codebook gain using the received index.
 *
 *  DESCRIPTION:
 *      The received index gives the gain correction factor gamma.
 *      The quantized gain is given by   g_q = g0 * gamma
 *      where g0 is the predicted gain.
 *      To find g0, 4th order MA prediction is applied to the mean-removed
 *      innovation energy in dB.
 *      In case of frame erasure, downscaled past gain is used.
 *
 *************************************************************************/

void amr_ef_d_gain_code(
    Ipp16s *gbuf,
	Ipp16s *past_gain_code,
	Ipp16s *prev_gc,
    Ipp16s *past_qua_en,
	Ipp16s *past_qua_en_MR122,
    Ipp16s index,      /* input : received quantization index */
    Ipp16s *gain_code, /* output: decoded innovation gain     */
    AMR_ef_dtx_rx_State_t *dtx_ef_rx,
    Ipp16s i_subfr
)
{
    Ipp16s i;

    
    if (((dtx_ef_rx->rxdtx_ctrl & EF_RX_UPD_SID_QUANT_MEM) != 0) && (i_subfr == 0))
    {
        dtx_ef_rx->gcode0_CN = ef_update_gcode0_CN(dtx_ef_rx->gain_code_old_rx);
        dtx_ef_rx->gcode0_CN = ShiftL_16s(dtx_ef_rx->gcode0_CN, 4);
    }

    /* Handle cases of comfort noise fixed codebook gain decoding in
       which past valid SID frames are repeated */


    if ((dtx_ef_rx->rxdtx_ctrl & (EF_RX_NO_TRANSMISSION|EF_RX_INVALID_SID_FRAME|EF_RX_LOST_SID_FRAME)) != 0)
    {
        if ((dtx_ef_rx->rxdtx_ctrl & EF_RX_NO_TRANSMISSION) != 0)
        {
            /* DTX active: no transmission. Interpolate gain values
            in memory */

            if (i_subfr == 0)
            {
                *gain_code = ef_interpolate_CN_param(dtx_ef_rx->gain_code_old_CN,
                                            dtx_ef_rx->gain_code_new_CN, dtx_ef_rx->rx_dtx_state);

            }
            else
            {
                *gain_code = *prev_gc >> 1;
            }
        }
        else
        {                       /* Invalid or lost SID frame:
            use gain values from last good SID frame */
            dtx_ef_rx->gain_code_old_CN = dtx_ef_rx->gain_code_new_CN;
            *gain_code = dtx_ef_rx->gain_code_new_CN;

            /* reset table of past quantized energies */
            for (i = 0; i < 4; i++)
            {
                past_qua_en_MR122[i] = -2381;
            }
        }


        if ((dtx_ef_rx->rxdtx_ctrl & EF_RX_DTX_MUTING) != 0)
        {
            /* attenuate the gain value by 0.75 dB in each subframe */
            /* (total of 3 dB per frame) */
            dtx_ef_rx->gain_code_muting_CN = (dtx_ef_rx->gain_code_muting_CN * 30057) >> 15;
            *gain_code = dtx_ef_rx->gain_code_muting_CN;
        }
        else
        {
            /* Prepare for DTX muting by storing last good gain value */
            dtx_ef_rx->gain_code_muting_CN = dtx_ef_rx->gain_code_new_CN;
        }
    }
    else
    {
        if ((dtx_ef_rx->rxdtx_ctrl & EF_RX_SP_FLAG) == 0)
        {

            if (((dtx_ef_rx->rxdtx_ctrl & EF_RX_FIRST_SID_UPDATE) != 0) && (i_subfr == 0))
            {
                dtx_ef_rx->gain_code_new_CN = (dtx_ef_rx->gcode0_CN * AG_SC_ef_qua_gain_code[index]) >> 15;

                /*---------------------------------------------------------------*
                 *  reset table of past quantized energies                        *
                 *  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~                        *
                 *---------------------------------------------------------------*/

                for (i = 0; i < 4; i++)
                {
                    past_qua_en_MR122[i] = -2381;
                }
            }

            if (((dtx_ef_rx->rxdtx_ctrl & EF_RX_CONT_SID_UPDATE) != 0) && (i_subfr == 0))
            {
                dtx_ef_rx->gain_code_old_CN = dtx_ef_rx->gain_code_new_CN;
                dtx_ef_rx->gain_code_new_CN = Mult_16s (dtx_ef_rx->gcode0_CN, AG_SC_ef_qua_gain_code[index]);
               
            }

            if (i_subfr == 0)
            {
                *gain_code = ef_interpolate_CN_param(dtx_ef_rx->gain_code_old_CN,
                                                   dtx_ef_rx->gain_code_new_CN,
                                                   dtx_ef_rx->rx_dtx_state);
            }
            else
            {
                *gain_code = *prev_gc >> 1;
            }
        }
    }

   *past_gain_code = *gain_code << 1;

    for (i = 0; i < 4; i++)
    {
        gbuf[i] = gbuf[i + 1];
    }
    *prev_gc = gbuf[4] = *past_gain_code;

}





/***************************************************************************
*  Function: ownDecode_GSMAMR     Speech decoder routine.
***************************************************************************/
/*
*  decState  [in/out] -  State variables
*  rate          [in] - GSMAMR rate
*  pSynthParm    [in] - Vector of synthesis parameters (length of vector - PRM_SIZE)
*  newDTXState   [in] - State of DTX
*  frameType    [in] - Received frame type
*  pSynthSpeech [out] - Synthesis speech (length of vector - FRAME_SIZE_GSMAMR)
*  pA_LP        [out] - Decoded LP filter in 4 subframes (length of vector - LP_ALL_FRAME)
*/

static Ipp32s ownDecode_GSMAMR(sDecoderState_GSMAMR *decState, GSMAMR_Rate_t rate, Ipp16s *pSynthParm,
                            enum enDTXStateType newDTXState, RXFrameType frameType,
                            Ipp16s *pSynthSpeech, Ipp16s *pA_LP)
{
    /* LPC coefficients */
    Ipp16s *Az;                /* Pointer on pA_LP */
    /* LSPs */
    IPP_ALIGNED_ARRAY(16, Ipp16s, LspInter, SUBFR_SIZE_GSMAMR);
    /* LSFs */
    IPP_ALIGNED_ARRAY(16, Ipp16s, prev_lsf, LP_ORDER_SIZE);
    IPP_ALIGNED_ARRAY(16, Ipp16s, lsf_i, LP_ORDER_SIZE);
    /* Algebraic codevector */
    IPP_ALIGNED_ARRAY(16, Ipp16s, code, SUBFR_SIZE_GSMAMR);
    /* excitation */
    IPP_ALIGNED_ARRAY(16, Ipp16s, excp, SUBFR_SIZE_GSMAMR);
    IPP_ALIGNED_ARRAY(16, Ipp16s, exc_enhanced, SUBFR_SIZE_GSMAMR);
    /* Scalars */
    Ipp16s i, i_subfr;
    Ipp16s T0, index, index_mr475 = 0;
    Ipp16s gain_code, gain_code_mix, pit_sharp, pitch_fac;
    Ipp16s gain_pit = 0;
    Ipp16s tmp_shift, temp;
    Ipp16s carefulFlag, excEnergy, subfrNr;
    Ipp32s tmpRes;
    Ipp16s evenSubfr = 0;
    Ipp16s bfi = 0;   /* bad frame indication flag                          */
    Ipp16s pdfi = 0;  /* potential degraded bad frame flag                  */
    IPP_ALIGNED_ARRAY(16, Ipp16s, pDstAdptVector, SUBFR_SIZE_GSMAMR);
    sDTXDecoderSt *DTXState = &decState->dtxDecoderState;
    IppSpchBitRate irate = mode2rates[rate];
	IppStatus return_status = ippStsNoErr;

    /* find the new  DTX state  SPEECH OR DTX */
    /* function result */
    /* DTX actions */
	if ((newDTXState != SPEECH) && (decState->EFR_flag == 0))
    {
       //needs to check the taf_count setting
	   ownDecoderInit_GSMAMR(decState,GSMAMR_RATE_DTX,decState->EFR_flag,0);
	
       ownDTXDecoder_GSMAMR(&decState->dtxDecoderState, decState->a_MemorySyn, decState->a_PastQntPredErr,
                            decState->a_PastLSFQnt, decState->a_PastQntEnergy, decState->a_PastQntEnergy_M122,
                            &decState->vHgAverageVar, &decState->vHgAverageCount, newDTXState, rate,
                            pSynthParm, pSynthSpeech, pA_LP);
       /* update average lsp */
       ippsLSFToLSP_GSMAMR_16s(decState->a_PastLSFQnt, decState->a_LSP_Old);
       ippsInterpolateC_NR_G729_16s_Sfs(decState->a_PastLSFQnt,EXP_CONST_016,
                                        decState->a_LSPAveraged,EXP_CONST_084,
                                        decState->a_LSPAveraged,LP_ORDER_SIZE,15);
       DTXState->eDTXPrevState = newDTXState;
       return 0;
    }

	 if (decState->EFR_flag == 1)
	 {
		 if ((frameType == RX_SPEECH_BAD)  ||
                (frameType == RX_SPEECH_BAD_TAF) ||
                (frameType == RX_SID_BAD))
            {
                bfi = 1;
            }
	 }
	 else
	 {
		/* SPEECH action state machine  */
		if((frameType == RX_SPEECH_BAD) || (frameType == RX_NO_DATA) || (frameType == RX_ONSET))
		{
		   bfi = 1;
		   if((frameType == RX_NO_DATA) || (frameType == RX_ONSET))
			 ownBuildCNParam_GSMAMR(&decState->vCNGen, TableParamPerModes[rate],TableBitAllModes[rate],pSynthParm);
		}
		else if(frameType == RX_SPEECH_DEGRADED) pdfi = 1;
	 }

    if(bfi != 0)            decState->vStateMachine++;
    else if(decState->vStateMachine == 6) decState->vStateMachine = 5;
    else                     decState->vStateMachine = 0;
    if (decState->EFR_flag == 1)
    {   /* EFR control processing */
        amr_ef_Dtx_Rx(&decState->dtx.ef_rx, frameType);

        if ((decState->dtx.ef_rx.rxdtx_ctrl & EF_RX_FIRST_SP_FLAG) != 0)
        {
            if ((decState->dtx.ef_rx.rxdtx_ctrl & EF_RX_PREV_DTX_MUTING) != 0)
            {
                decState->vStateMachine = 5;
                decState->vPrevBadFr = 1;
            }
            else {
                decState->vStateMachine = 5;
                decState->vPrevBadFr = 0;
            }
        }
    }
    else
	{
		if(decState->vStateMachine > 6) decState->vStateMachine = 6;
		if(DTXState->eDTXPrevState == DTX) {
		   decState->vStateMachine = 5;
		   decState->vPrevBadFr = 0;
		} else if (DTXState->eDTXPrevState == DTX_MUTE) {
		   decState->vStateMachine = 5;
		   decState->vPrevBadFr = 1;
		}
	}

    /* save old LSFs for CB gain smoothing */
    ippsCopy_16s (decState->a_PastLSFQnt, prev_lsf, LP_ORDER_SIZE);
    /* decode LSF and generate interpolated lpc coefficients for the 4 subframes */
	if (!decState->EFR_flag)
	{
		ippsQuantLSPDecode_GSMAMR_16s(pSynthParm, decState->a_PastQntPredErr, decState->a_PastLSFQnt,
                                  decState->a_LSP_Old, LspInter, bfi, irate);
		ippsLSPToLPC_GSMAMR_16s(&(LspInter[0]), &(pA_LP[0]));  /* Subframe 1 */
		ippsLSPToLPC_GSMAMR_16s(&(LspInter[LP_ORDER_SIZE]), &(pA_LP[LP1_ORDER_SIZE]));  /* Subframe 2 */
		ippsLSPToLPC_GSMAMR_16s(&(LspInter[LP_ORDER_SIZE*2]), &(pA_LP[LP1_ORDER_SIZE*2]));  /* Subframe 3 */
		ippsLSPToLPC_GSMAMR_16s(&(LspInter[LP_ORDER_SIZE*3]), &(pA_LP[LP1_ORDER_SIZE*3]));  /* Subframe 4 */
	}
	else
	{
		amr_ef_D_plsf_5(decState->a_PastQntPredErr, decState->a_PastLSFQnt, bfi, pSynthParm, &LspInter[0], &LspInter[2*LP_ORDER_SIZE], &decState->dtx.ef_rx);
		if ((decState->dtx.ef_rx.rxdtx_ctrl & EF_RX_SP_FLAG) == 0)
        {
            /* EFR SID */
            ippsLSPToLPC_GSMAMR_16s(&LspInter[2*LP_ORDER_SIZE], &(pA_LP[LP1_ORDER_SIZE*2]));
			ippsCopy_16s ( &(pA_LP[LP1_ORDER_SIZE*2]),  &(pA_LP[0]), LP1_ORDER_SIZE);
			ippsCopy_16s ( &(pA_LP[LP1_ORDER_SIZE*2]),  &(pA_LP[LP1_ORDER_SIZE]), LP1_ORDER_SIZE);
			ippsCopy_16s ( &(pA_LP[LP1_ORDER_SIZE*2]),  &(pA_LP[LP1_ORDER_SIZE*3]), LP1_ORDER_SIZE);
        }
		else
		{
			amr_Int_lpc_1and3(decState->a_LSP_Old, &LspInter[0], &LspInter[2*LP_ORDER_SIZE], pA_LP);
		}
		ippsCopy_16s(&LspInter[2*LP_ORDER_SIZE], decState->a_LSP_Old, LP_ORDER_SIZE);
	}


    if(irate == IPP_SPCHBR_12200) pSynthParm += 5;
    else pSynthParm += 3;
    /*------------------------------------------------------------------------*
     *          Loop for every subframe in the analysis frame                 *
     *------------------------------------------------------------------------*/
        if (((decState->dtx.ef_rx.rxdtx_ctrl & EF_RX_ACTIVE) != 0)
         && ((decState->dtx.ef_rx.rxdtx_ctrl & EF_RX_SP_FLAG) == 0))
        {
			Ipp32s L_temp;
           /* EFR SID processing  */
           Az = pA_LP;
            for (i_subfr = 0; i_subfr < FRAME_SIZE_GSMAMR; i_subfr += SUBFR_SIZE_GSMAMR)
            {
                if (bfi==0)
                {
                   decState->vPrevGainZero = 0;
                }
                decState->vPastGainZero = 0;
                for (i = 1; i < 5; i++)
                {
                    decState->a_LSFBuffer[i - 1] = decState->a_LSFBuffer[i];
                }
                decState->a_LSFBuffer[4]=0;

                ef_build_CN_code (code, &decState->dtx.ef_rx.L_pn_seed_rx);
                pSynthParm +=12;

                index=*pSynthParm++;
                amr_ef_d_gain_code(&decState->a_GainBuffer[0],&decState->vPastGainCode, &decState->vPrevGainCode,
					&decState->a_PastQntEnergy[0], &decState->a_PastQntEnergy_M122[0], index,
                          &gain_code, &decState->dtx.ef_rx, i_subfr);

                for (i = 0; i < SUBFR_SIZE_GSMAMR; i++)
                {
                    /* exc[i] = gain_pit*exc[i] + gain_code*code[i]; */
                    L_temp = Mult_16s32s(code[i], gain_code);
                    L_temp = Shl_32s(L_temp, 3);
                    decState->a_ExcVecOld[PITCH_MAX_LAG + FLT_INTER_SIZE + i] = Round_32s16s(L_temp);
                }

                ippsSynthesisFilter_NR_16s_Sfs(Az,decState->pExcVec,&pSynthSpeech[i_subfr],SUBFR_SIZE_GSMAMR,12,decState->a_MemorySyn);
                ippsCopy_16s(&pSynthSpeech[i_subfr+SUBFR_SIZE_GSMAMR-LP_ORDER_SIZE], decState->a_MemorySyn, LP_ORDER_SIZE); /* update memory */
				ippsMove_16s (&decState->a_ExcVecOld[SUBFR_SIZE_GSMAMR], &decState->a_ExcVecOld[0], (PITCH_MAX_LAG + FLT_INTER_SIZE));
                Az += LP1_ORDER_SIZE;
            }
			            
			 /*--------------------------------------------------*
			 * Update signal for next frame.                    *
			 * -> shift to the left by SUBFR_SIZE_GSMAMR  decState->pExcVec[]       *
			 *--------------------------------------------------*/
			//ippsMove_16s (&decState->a_ExcVecOld[SUBFR_SIZE_GSMAMR], &decState->a_ExcVecOld[0], (PITCH_MAX_LAG + FLT_INTER_SIZE));

            decState->vPrevPitchLag=0;
            decState->vPrevBadFr = bfi;

            //status = AMR_SID_MASK;
			return 0;
        }
       
    Az = pA_LP;      /* pointer to interpolated LPC parameters */
    evenSubfr = 0;
    subfrNr = -1;
    for (i_subfr = 0; i_subfr < FRAME_SIZE_GSMAMR; i_subfr += SUBFR_SIZE_GSMAMR)
    {
       subfrNr++;
       evenSubfr = (Ipp16s)(1 - evenSubfr);
       /* pitch index */
       index = *pSynthParm++;

       ippsAdaptiveCodebookDecode_GSMAMR_16s(
            index, &decState->vPrevPitchLag, &decState->vLTPLag, (decState->pExcVec - MAX_OFFSET),
            &T0, pDstAdptVector, subfrNr, bfi,
            decState->vBackgroundNoise, decState->vVoiceHangover, irate);

        if (irate == IPP_SPCHBR_12200)
        { /* MR122 */
            index = *pSynthParm++;
            if (bfi != 0)
                ownConcealGainPitch_GSMAMR(decState->a_LSFBuffer,decState->vPastGainZero, decState->vStateMachine, &gain_pit);
            else
                gain_pit = (Ipp16s)((TableQuantGainPitch[index] >> 2) << 2);
            ownConcealGainPitchUpdate_GSMAMR(decState->a_LSFBuffer, &decState->vPastGainZero,&decState->vPrevGainZero, bfi,
                                  decState->vPrevBadFr, &gain_pit);
        }
        ippsFixedCodebookDecode_GSMAMR_16s(pSynthParm, code, subfrNr, irate);

        if (irate == IPP_SPCHBR_10200) { /* MR102 */
            pSynthParm+=7;
            pit_sharp = Cnvrt_32s16s(decState->vFlagSharp << 1);
        } else if (irate == IPP_SPCHBR_12200) { /* MR122 */
            pSynthParm+=10;
            pit_sharp = Cnvrt_32s16s(gain_pit << 1);
        } else {
            pSynthParm+=2;
            pit_sharp = Cnvrt_32s16s(decState->vFlagSharp << 1);
        }

        if(T0 < SUBFR_SIZE_GSMAMR)
           ippsHarmonicFilter_16s_I(pit_sharp,T0,code+T0, (SUBFR_SIZE_GSMAMR - T0));

        if (irate == IPP_SPCHBR_4750) {
           /* read and decode pitch and code gain */
           if(evenSubfr != 0)
              index_mr475 = *pSynthParm++;        /* index of gain(s) */
           if (bfi == 0) {
              ownDecodeCodebookGains_GSMAMR(decState->a_PastQntEnergy,decState->a_PastQntEnergy_M122, irate, index_mr475, code,
                       evenSubfr, &gain_pit, &gain_code);
           } else {
              ownConcealGainPitch_GSMAMR(decState->a_LSFBuffer,decState->vPastGainZero, decState->vStateMachine, &gain_pit);
              ownConcealCodebookGain_GSMAMR(decState->a_GainBuffer,decState->vPastGainCode, decState->a_PastQntEnergy,
                            decState->a_PastQntEnergy_M122, decState->vStateMachine,&gain_code);
           }
           ownConcealGainPitchUpdate_GSMAMR(decState->a_LSFBuffer, &decState->vPastGainZero,&decState->vPrevGainZero, bfi, decState->vPrevBadFr,
                                 &gain_pit);
           ownConcealCodebookGainUpdate_GSMAMR(decState->a_GainBuffer,&decState->vPastGainCode,&decState->vPrevGainCode, bfi, decState->vPrevBadFr,
                                &gain_code);
           pit_sharp = gain_pit;
           if(pit_sharp > PITCH_SHARP_MAX) pit_sharp = PITCH_SHARP_MAX;
        }
        else if ((irate <= IPP_SPCHBR_7400) || (irate == IPP_SPCHBR_10200))
        {
            /* read and decode pitch and code gain */
            index = *pSynthParm++;                /* index of gain(s) */
            if(bfi == 0) {
               ownDecodeCodebookGains_GSMAMR(decState->a_PastQntEnergy,decState->a_PastQntEnergy_M122, irate, index, code,
                        evenSubfr, &gain_pit, &gain_code);
            } else {
               ownConcealGainPitch_GSMAMR(decState->a_LSFBuffer,decState->vPastGainZero, decState->vStateMachine, &gain_pit);
               ownConcealCodebookGain_GSMAMR(decState->a_LSFBuffer,decState->vPastGainCode, decState->a_PastQntEnergy,
                             decState->a_PastQntEnergy_M122, decState->vStateMachine,&gain_code);
            }
            ownConcealGainPitchUpdate_GSMAMR(decState->a_LSFBuffer, &decState->vPastGainZero,&decState->vPrevGainZero, bfi, decState->vPrevBadFr,
                                  &gain_pit);
            ownConcealCodebookGainUpdate_GSMAMR(decState->a_GainBuffer,&decState->vPastGainCode,&decState->vPrevGainCode, bfi, decState->vPrevBadFr,
                                 &gain_code);
            pit_sharp = gain_pit;
            if (pit_sharp > PITCH_SHARP_MAX)  pit_sharp = PITCH_SHARP_MAX;
            if (irate == IPP_SPCHBR_10200) {
               if (decState->vPrevPitchLag > (SUBFR_SIZE_GSMAMR + 5)) pit_sharp >>= 2;

            }
        }
        else
        {
           /* read and decode pitch gain */
           index = *pSynthParm++;                 /* index of gain(s) */
           if (irate == IPP_SPCHBR_7950)
           {
              /* decode pitch gain */
              if (bfi != 0)
                 ownConcealGainPitch_GSMAMR(decState->a_LSFBuffer,decState->vPastGainZero, decState->vStateMachine, &gain_pit);
              else
                 gain_pit = TableQuantGainPitch[index];

              ownConcealGainPitchUpdate_GSMAMR(decState->a_LSFBuffer, &decState->vPastGainZero,&decState->vPrevGainZero, bfi, decState->vPrevBadFr,
                                    &gain_pit);
              /* read and decode code gain */
              index = *pSynthParm++;
              if (bfi == 0) {
                 ownDecodeFixedCodebookGain_GSMAMR(decState->a_PastQntEnergy,decState->a_PastQntEnergy_M122, irate, index, code, &gain_code);
              } else {
                 ownConcealCodebookGain_GSMAMR(decState->a_GainBuffer,decState->vPastGainCode, decState->a_PastQntEnergy,
                               decState->a_PastQntEnergy_M122, decState->vStateMachine,
                               &gain_code);
              }
              ownConcealCodebookGainUpdate_GSMAMR(decState->a_GainBuffer,&decState->vPastGainCode,&decState->vPrevGainCode, bfi, decState->vPrevBadFr,
                                   &gain_code);
              pit_sharp = gain_pit;
              if(pit_sharp > PITCH_SHARP_MAX) pit_sharp = PITCH_SHARP_MAX;
           }
           else
           { /* MR122 */
              if (bfi == 0) {
                 ownDecodeFixedCodebookGain_GSMAMR(decState->a_PastQntEnergy,decState->a_PastQntEnergy_M122, irate, index, code, &gain_code);
              } else {
                 ownConcealCodebookGain_GSMAMR(decState->a_GainBuffer,decState->vPastGainCode, decState->a_PastQntEnergy,
                               decState->a_PastQntEnergy_M122, decState->vStateMachine,&gain_code);
              }
              ownConcealCodebookGainUpdate_GSMAMR(decState->a_GainBuffer,&decState->vPastGainCode,&decState->vPrevGainCode, bfi, decState->vPrevBadFr,
                                   &gain_code);
                if ((decState->dtx.ef_rx.rxdtx_ctrl & EF_RX_ACTIVE) !=0)
                {
                    Ipp16s gtmp, htmp;
                    if (bfi ==0)
                    {
                        gtmp = gain_code;
                        htmp = ShiftR_16s(gain_code,1);
                    }
                    else
                    {
                        if (decState->dtx.ef_rx.gain_code_new_CN == 0)
                        {
                            htmp = ShiftR_16s(decState->vPrevGainCode,1);
                        }
                        else
                        {
                            htmp = decState->dtx.ef_rx.gain_code_new_CN;
                        }
                        gtmp = htmp;
                    }
                    if (Sub_16s(i_subfr, (3 * SUBFR_SIZE_GSMAMR)) == 0)
                    {
                        decState->dtx.ef_rx.gain_code_old_CN = gtmp;
                    }
                    ef_update_gain_code_history(htmp, decState->dtx.ef_rx.gain_code_old_rx, &decState->dtx.ef_rx.buf_p_rx);
                }

              pit_sharp = gain_pit;
           }
        }

        /* store pitch sharpening for next subframe          */
        if (irate != IPP_SPCHBR_4750 || evenSubfr == 0) {
            decState->vFlagSharp = gain_pit;
            if (decState->vFlagSharp > PITCH_SHARP_MAX)  decState->vFlagSharp = PITCH_SHARP_MAX;

        }
        pit_sharp = Cnvrt_32s16s(pit_sharp << 1);
        if (pit_sharp > 16384)
        {
           /* Bith are not bit-exact due to other rounding:
              ippsMulC_16s_Sfs(decState->pExcVec,pit_sharp,excp,SUBFR_SIZE_GSMAMR,15);
              ippsMulC_NR_16s_Sfs(decState->pExcVec,pit_sharp,excp,SUBFR_SIZE_GSMAMR,15);
           */
           for (i = 0; i < SUBFR_SIZE_GSMAMR; i++)
               excp[i] = (Ipp16s)((decState->pExcVec[i] * pit_sharp) >> 15);

           for (i = 0; i < SUBFR_SIZE_GSMAMR; i++)  {
               tmpRes = excp[i] * gain_pit;
               /* */
               if (irate != IPP_SPCHBR_12200) tmpRes = Mul2_32s(tmpRes);
               excp[i] = Cnvrt_NR_32s16s(tmpRes);
            }
        }

        if( bfi == 0 ) {
           ippsMove_16s(decState->a_LTPGainHistory+1, decState->a_LTPGainHistory, 8);
           decState->a_LTPGainHistory[8] = gain_pit;
        }

        if ( (decState->vPrevBadFr != 0 || bfi != 0) && decState->vBackgroundNoise != 0 &&
             ((irate == IPP_SPCHBR_4750) ||
              (irate == IPP_SPCHBR_5150) ||
              (irate == IPP_SPCHBR_5900))
             )
        {
           if(gain_pit > 12288)                  /* if (gain_pit > 0.75) in Q14*/
              gain_pit = (Ipp16s)(((gain_pit - 12288) >> 1) + 12288);
                                                 /* gain_pit = (gain_pit-0.75)/2.0 + 0.75; */
           if(gain_pit > 14745) gain_pit = 14745;/* if (gain_pit > 0.90) in Q14*/
        }
        switch(i_subfr){
        case 0:
           ippsInterpolate_GSMAMR_16s(decState->a_PastLSFQnt,prev_lsf,lsf_i,LP_ORDER_SIZE);
           break;
        case SUBFR_SIZE_GSMAMR:
           ippsInterpolate_G729_16s(prev_lsf,decState->a_PastLSFQnt,lsf_i,LP_ORDER_SIZE);
           break;
        case 2*SUBFR_SIZE_GSMAMR:
           ippsInterpolate_GSMAMR_16s(prev_lsf,decState->a_PastLSFQnt,lsf_i,LP_ORDER_SIZE);
           break;
        default: /* 3*SUBFR_SIZE_GSMAMR:*/
           ippsCopy_16s(decState->a_PastLSFQnt,lsf_i,LP_ORDER_SIZE);
        }

        gain_code_mix = ownCBGainAverage_GSMAMR(decState->a_GainHistory,&decState->vHgAverageVar,&decState->vHgAverageCount,
                                                irate, gain_code, lsf_i, decState->a_LSPAveraged, bfi,
                                                decState->vPrevBadFr, pdfi, decState->vPrevDegBadFr,
                                                decState->vBackgroundNoise, decState->vVoiceHangover);
        /* make sure that MR74, MR795, MR122 have original code_gain*/
        if((irate > IPP_SPCHBR_6700) && (irate != IPP_SPCHBR_10200) )  /* MR74, MR795, MR122 */
           gain_code_mix = gain_code;

        /*-------------------------------------------------------*
         * - Find the total excitation.                          *
         * - Find synthesis speech corresponding to decState->pExcVec[].   *
         *-------------------------------------------------------*/
        if (irate <= IPP_SPCHBR_10200)  { /* MR475, MR515, MR59, MR67, MR74, MR795, MR102*/
           pitch_fac = gain_pit;
           tmp_shift = 1;
        } else {      /* MR122 */
           pitch_fac = (Ipp16s)(gain_pit >> 1);
           tmp_shift = 2;
        }

        ippsCopy_16s(decState->pExcVec, exc_enhanced, SUBFR_SIZE_GSMAMR);
        ippsInterpolateC_NR_G729_16s_Sfs(code,gain_code,decState->pExcVec,pitch_fac,
                                         decState->pExcVec,SUBFR_SIZE_GSMAMR,15-tmp_shift);

        if ( ((irate == IPP_SPCHBR_4750) ||
              (irate == IPP_SPCHBR_5150) ||
              (irate == IPP_SPCHBR_5900))&&
             decState->vVoiceHangover > 3     &&
             decState->vBackgroundNoise != 0 &&
             bfi != 0 )
        {
            decState->stPhDispState.vFlagLockGain = 1; /* Always Use full Phase Disp. */
        } else { /* if error in bg noise       */
            decState->stPhDispState.vFlagLockGain = 0;    /* free phase dispersion adaption */
        }
        /* apply phase dispersion to innovation (if enabled) and
           compute total excitation for synthesis part           */
        ownPhaseDispersion_GSMAMR(&decState->stPhDispState, irate, exc_enhanced,
                                  gain_code_mix, gain_pit, code, pitch_fac, tmp_shift);

		tmpRes = 0;                                   
        for (i = 0; i < SUBFR_SIZE_GSMAMR; i++)
        {
            tmpRes = Mac_16s32s_v2(tmpRes, exc_enhanced[i], exc_enhanced[i] );
        }
		tmpRes >>= 1;
		tmpRes = ownSqrt_Exp_GSMAMR(tmpRes, &temp); /* function result */
        tmpRes >>= (temp >> 1) + 17;
        excEnergy = (Ipp16s)tmpRes;   
 
        if ( ((irate == IPP_SPCHBR_4750) ||
              (irate == IPP_SPCHBR_5150) ||
              (irate == IPP_SPCHBR_5900))  &&
             decState->vVoiceHangover > 5 &&
             decState->vBackgroundNoise != 0 &&
             decState->vStateMachine < 4 &&
             ( (pdfi != 0 && decState->vPrevDegBadFr != 0) ||
                bfi != 0 ||
                decState->vPrevBadFr != 0) )
        {
           carefulFlag = 0;
           if((pdfi != 0) && (bfi == 0)) carefulFlag = 1;

           {
			   Ipp16s *excitation, *exEnergyHist,exp;
			   Ipp16s testEnergy, scaleFactor, avgEnergy, prevEnergy, vVoiceHangover, prevBFI;
			   Ipp32s t0;

			   excitation = exc_enhanced;
			   exEnergyHist = decState->a_EnergyHistSubFr;
			   vVoiceHangover = decState->vVoiceHangover;
			   prevBFI = decState->vPrevBadFr;
			   

			   avgEnergy = ownGetMedianElements_GSMAMR(exEnergyHist, 9);
			   prevEnergy = (Ipp16s)((exEnergyHist[7] + exEnergyHist[8]) >> 1);

			   if( exEnergyHist[8] < prevEnergy) prevEnergy = exEnergyHist[8];

			   if( excEnergy < avgEnergy && excEnergy > 5) {
			      testEnergy = (Ipp16s)(prevEnergy << 2);

			      if( vVoiceHangover < 7 || prevBFI != 0 ) testEnergy = (Ipp16s)(testEnergy - prevEnergy);
			      if( avgEnergy > testEnergy) avgEnergy = testEnergy;

			      exp = 0;
			      for(; excEnergy < 0x4000; exp++) excEnergy <<= 1;
			      excEnergy = (Ipp16s)((16383<<15)/excEnergy);
			      t0 = avgEnergy * excEnergy;
			      t0 >>= 19 - exp;
			      if(t0 > 32767)  t0 = 32767;
			      scaleFactor = (Ipp16s)t0;

			      if((carefulFlag != 0) && (scaleFactor > 3072))  scaleFactor = 3072;

			      ippsMulC_16s_Sfs(excitation, scaleFactor, excitation, SUBFR_SIZE_GSMAMR, 10);
			   }		   
			}
        }
        if((decState->vBackgroundNoise != 0) &&
           ( bfi != 0 || decState->vPrevBadFr != 0 ) &&
           (decState->vStateMachine < 4)) {
           ; /* do nothing! */
        } else {
           /* Update energy history for all rates */
           ippsMove_16s(decState->a_EnergyHistSubFr+1, decState->a_EnergyHistSubFr, 8);
           decState->a_EnergyHistSubFr[8] = excEnergy;
        }

		if (pit_sharp > 16384) {		   
           ippsAdd_16s(exc_enhanced, excp, excp, SUBFR_SIZE_GSMAMR);
           ownScaleExcitation_GSMAMR(exc_enhanced, excp);
           return_status = ippsSynthesisFilter_NR_16s_Sfs(Az,excp,&pSynthSpeech[i_subfr],SUBFR_SIZE_GSMAMR,12,decState->a_MemorySyn);
        } else {
           return_status = ippsSynthesisFilter_NR_16s_Sfs(Az,exc_enhanced,&pSynthSpeech[i_subfr],SUBFR_SIZE_GSMAMR,12,decState->a_MemorySyn);
        }

		if (return_status == ippStsOverflow)    /* Test for overflow */
        {
		   int i;
           for (i = 0; i < PITCH_MAX_LAG + FLT_INTER_SIZE + SUBFR_SIZE_GSMAMR; i++)
           {
              decState->a_ExcVecOld[i] = decState->a_ExcVecOld[i] >> 2;       
           }
           for (i = 0; i < SUBFR_SIZE_GSMAMR; i++)
           {
              exc_enhanced[i] = exc_enhanced[i] >> 2;    
           }
           ippsSynthesisFilter_NR_16s_Sfs(Az,exc_enhanced,&pSynthSpeech[i_subfr],SUBFR_SIZE_GSMAMR,12,decState->a_MemorySyn);
		   //update the memory		   
			for (i = 0; i < 10; i++)
			{
				decState->a_MemorySyn[i] = pSynthSpeech[i_subfr + SUBFR_SIZE_GSMAMR - 10 + i];      
			}			
        }
		else
		{
			ippsCopy_16s(&pSynthSpeech[i_subfr+SUBFR_SIZE_GSMAMR-LP_ORDER_SIZE], decState->a_MemorySyn, LP_ORDER_SIZE);
		}
 
        /*--------------------------------------------------*
         * Update signal for next frame.                    *
         * -> shift to the left by SUBFR_SIZE_GSMAMR  decState->pExcVec[]       *
         *--------------------------------------------------*/
        ippsMove_16s (&decState->a_ExcVecOld[SUBFR_SIZE_GSMAMR], &decState->a_ExcVecOld[0], (PITCH_MAX_LAG + FLT_INTER_SIZE));
        /* interpolated LPC parameters for next subframe */
        Az += LP1_ORDER_SIZE;
        /* store T0 for next subframe */
        decState->vPrevPitchLag = T0;
    }
    /*-------------------------------------------------------*
     * Call the Source Characteristic Detector which updates *
     * decState->vBackgroundNoise and decState->vVoiceHangover.         *
     *-------------------------------------------------------*/
    decState->vBackgroundNoise = ownSourceChDetectorUpdate_GSMAMR(
                                  decState->a_EnergyHistVector, &(decState->vCountHangover),
                                  &(decState->a_LTPGainHistory[0]), &(pSynthSpeech[0]),
                                  &(decState->vVoiceHangover));
    if ((decState->dtx.ef_rx.rxdtx_ctrl & EF_RX_ACTIVE)==0)
		ippsDecDTXBuffer_GSMAMR_16s(pSynthSpeech, decState->a_PastLSFQnt, &decState->dtxDecoderState.vLogEnergyHistory,
                                decState->dtxDecoderState.a_LSFHistory, decState->dtxDecoderState.a_LogEnergyHistory);
    /* store bfi for next subframe */
    decState->vPrevBadFr = bfi;
    decState->vPrevDegBadFr = pdfi;
    ippsInterpolateC_NR_G729_16s_Sfs(decState->a_PastLSFQnt,EXP_CONST_016,
                                     decState->a_LSPAveraged,EXP_CONST_084,decState->a_LSPAveraged,LP_ORDER_SIZE,15);
    DTXState->eDTXPrevState = newDTXState;
    return 0;
}
