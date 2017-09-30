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
// Purpose: GSMAMR speech codec: encoder API.
//
*/

#include "owngsmamr.h"

/* filter coefficients (fc = 140 Hz, coeff. b[] is divided by 2) */
static __ALIGN32 CONST Ipp16s pTblCoeff_b140[3] = {1899, -3798, 1899};      /* 1/2 in Q12 */
static __ALIGN32 CONST Ipp16s pTblCoeff_a140[3] = {4096, 7807, -3733};      /* Q12 */

/* Spectral expansion factors */


static __ALIGN32 CONST Ipp8s efr_sid_pad[12*4] =
{
    0x3,0x7,0xf,0xf,0xf,0xf,0xc,0x0,0x0,0x0,0x0,0x0, /*  5-16 */
    0x7,0x7,0xf,0xf,0xf,0xf,0xc,0x0,0x0,0x0,0x0,0x0, /* 18-29 */
    0x3,0xf,0xf,0xf,0xf,0xf,0xc,0x0,0x0,0x0,0x0,0x0, /* 31-42 */
    0xf,0xf,0xf,0xc,0xf,0xf,0xc,0x0,0x0,0x0,0x0,0x0  /* 44-55 */
};


static __ALIGN32 CONST Ipp16s pTableGamma1[4*LP_ORDER_SIZE+4] =
{
   32767, 30802, 28954, 27217, 25584, 24049, 22606, 21250, 19975, 18777, 17650,
   32767, 30802, 28954, 27217, 25584, 24049, 22606, 21250, 19975, 18777, 17650,
   32767, 30802, 28954, 27217, 25584, 24049, 22606, 21250, 19975, 18777, 17650,
   32767, 30802, 28954, 27217, 25584, 24049, 22606, 21250, 19975, 18777, 17650
};

/* pTableGamma1 differs for the 12k2 coder */
static __ALIGN32 CONST Ipp16s pTableGamma1_M122[4*LP_ORDER_SIZE+4] =
{
    32767, 29491, 26542, 23888, 21499, 19349, 17414, 15672, 14105, 12694, 11425,
    32767, 29491, 26542, 23888, 21499, 19349, 17414, 15672, 14105, 12694, 11425,
    32767, 29491, 26542, 23888, 21499, 19349, 17414, 15672, 14105, 12694, 11425,
    32767, 29491, 26542, 23888, 21499, 19349, 17414, 15672, 14105, 12694, 11425
};

static __ALIGN32 CONST Ipp16s pTableGamma2[4*LP_ORDER_SIZE+4] =
{
   32767, 19661, 11797, 7078, 4247, 2548, 1529, 917, 550, 330, 198,
   32767, 19661, 11797, 7078, 4247, 2548, 1529, 917, 550, 330, 198,
   32767, 19661, 11797, 7078, 4247, 2548, 1529, 917, 550, 330, 198,
   32767, 19661, 11797, 7078, 4247, 2548, 1529, 917, 550, 330, 198
};


#define AMR_PRMNO_MR122_EFR 57
static __ALIGN32 CONST Ipp16s AG_SC_amr_ef_dhf_mask[AMR_PRMNO_MR122_EFR] =
{
        0x0004,                 /* LPC 1 */
        0x002f,                 /* LPC 2 */
        0x00b4,                 /* LPC 3 */
        0x0090,                 /* LPC 4 */
        0x003e,                 /* LPC 5 */

        0x0156,                 /* LTP-LAG 1 */
        0x000b,                 /* LTP-GAIN 1 */
        0x0000,                 /* PULSE 1_1 */
        0x0001,                 /* PULSE 1_2 */
        0x000f,                 /* PULSE 1_3 */
        0x0001,                 /* PULSE 1_4 */
        0x000d,                 /* PULSE 1_5 */
        0x0000,                 /* PULSE 1_6 */
        0x0003,                 /* PULSE 1_7 */
        0x0000,                 /* PULSE 1_8 */
        0x0003,                 /* PULSE 1_9 */
        0x0000,                 /* PULSE 1_10 */
        0x0003,                 /* FCB-GAIN 1 */

        0x0036,                 /* LTP-LAG 2 */
        0x0001,                 /* LTP-GAIN 2 */
        0x0008,                 /* PULSE 2_1 */
        0x0008,                 /* PULSE 2_2 */
        0x0005,                 /* PULSE 2_3 */
        0x0008,                 /* PULSE 2_4 */
        0x0001,                 /* PULSE 2_5 */
        0x0000,                 /* PULSE 2_6 */
        0x0000,                 /* PULSE 2_7 */
        0x0001,                 /* PULSE 2_8 */
        0x0001,                 /* PULSE 2_9 */
        0x0000,                 /* PULSE 2_10 */
        0x0000,                 /* FCB-GAIN 2 */

        0x0156,                 /* LTP-LAG 3 */
        0x0000,                 /* LTP-GAIN 3 */
        0x0000,                 /* PULSE 3_1 */
        0x0000,                 /* PULSE 3_2 */
        0x0000,                 /* PULSE 3_3 */
        0x0000,                 /* PULSE 3_4 */
        0x0000,                 /* PULSE 3_5 */
        0x0000,                 /* PULSE 3_6 */
        0x0000,                 /* PULSE 3_7 */
        0x0000,                 /* PULSE 3_8 */
        0x0000,                 /* PULSE 3_9 */
        0x0000,                 /* PULSE 3_10 */
        0x0000,                 /* FCB-GAIN 3 */

        0x0036,                 /* LTP-LAG 4 */
        0x000b,                 /* LTP-GAIN 4 */
        0x0000,                 /* PULSE 4_1 */
        0x0000,                 /* PULSE 4_2 */
        0x0000,                 /* PULSE 4_3 */
        0x0000,                 /* PULSE 4_4 */
        0x0000,                 /* PULSE 4_5 */
        0x0000,                 /* PULSE 4_6 */
        0x0000,                 /* PULSE 4_7 */
        0x0000,                 /* PULSE 4_8 */
        0x0000,                 /* PULSE 4_9 */
        0x0000,                 /* PULSE 4_10 */
        0x0000                  /* FCB-GAIN 4 */
};


GSMAMR_CODECFUN( APIGSMAMR_Status, apiGSMAMREncoder_GetSize,
         (GSMAMREncoder_Obj* encoderObj, Ipp32u *pCodecSize))
{
   if(NULL == encoderObj)
      return APIGSMAMR_StsBadArgErr;
   if(NULL == pCodecSize)
      return APIGSMAMR_StsBadArgErr;
   if(encoderObj->objPrm.key != ENC_KEY)
      return APIGSMAMR_StsNotInitialized;

   *pCodecSize = encoderObj->objPrm.objSize;
   return APIGSMAMR_StsNoErr;
}


/*************************************************************************
*  Function:   apiGSMAMREncoder_Alloc
*     Enquire a size of the coder state memory
**************************************************************************/
GSMAMR_CODECFUN( APIGSMAMR_Status, apiGSMAMREncoder_Alloc,
         (const GSMAMREnc_Params *gsm_Params, Ipp32u *pSizeTab))
{
   Ipp32s fltSize;
   Ipp32s allocSize=sizeof(GSMAMREncoder_Obj);
   ippsHighPassFilterSize_G729(&fltSize);
   allocSize += fltSize;
   if(GSMAMR_CODEC != gsm_Params->codecType)
      return APIGSMAMR_StsBadCodecType;
   //ownEncDetectSize_GSMAMR(gsm_Params->mode,&allocSize);
   ownEncDetectSize_GSMAMR(&allocSize);
   pSizeTab[0] =  allocSize;
   return APIGSMAMR_StsNoErr;
}

Ipp32s ownDtxEncoderInit_GSMAMR(sDTXEncoderSt* dtxSt)
{
   Ipp16s i;

   dtxSt->vHistoryPtr = 0;
   dtxSt->vLogEnergyIndex = 0;
   dtxSt->vLSFQntIndex = 0;
   dtxSt->a_LSPIndex[0] = 0;
   dtxSt->a_LSPIndex[1] = 0;
   dtxSt->a_LSPIndex[2] = 0;

   for(i = 0; i < DTX_HIST_SIZE; i++)
     ippsCopy_16s(TableLSPInitData, &dtxSt->a_LSPHistory[i * LP_ORDER_SIZE], LP_ORDER_SIZE);

   ippsZero_16s(dtxSt->a_LogEnergyHistory, DTX_HIST_SIZE);
   dtxSt->vDTXHangoverCt = DTX_HANG_PERIOD;
   dtxSt->vDecExpireCt = 32767;

   return 1;
}


/*************************************************************************
 *
 *   FUNCTION NAME: amr_ef_dtx_enc_reset
 *
 *   PURPOSE:  Resets the static variables of the TX DTX handler to their
 *             initial values
 *
 *************************************************************************/

void amr_ef_dtx_enc_reset(
    AMR_ef_dtx_tx_State_t *dtx_ef_tx,
    Ipp16s taf_count
)
{
    Ipp16s i, j;
    /* suppose infinitely long speech period before start */

    
    for (i = 0; i < 6; i++)
    {
        dtx_ef_tx->old_CN_mem_tx[i] = 0;
    }

    for (i = 0; i < 4 * EF_DTX_HANGOVER; i++)
    {
        dtx_ef_tx->gain_code_old_tx[i] = 0;
    }

    dtx_ef_tx->CN_excitation_gain = 0;

    dtx_ef_tx->next_lsf_old = 0;
    dtx_ef_tx->buf_p_tx = 0;

    dtx_ef_tx->gcode0_CN = 0;              /* CNI */
    dtx_ef_tx->dummy = 0;

    dtx_ef_tx->taf_count = taf_count;

    dtx_ef_tx->txdtx_ctrl = EF_TX_SP_FLAG | EF_TX_VAD_FLAG | EF_TX_ACTIVE;
    dtx_ef_tx->txdtx_hangover = EF_DTX_HANGOVER;
    dtx_ef_tx->txdtx_N_elapsed = 0x7fff;

    dtx_ef_tx->prev_ft = TX_SPEECH_GOOD;

    for (i = 0; i < EF_DTX_HANGOVER; i++)
        for (j = 0; j < EF_LPCORDER; j++)
            dtx_ef_tx->lsf_old_tx[i][j] = TableMeanLSF_5[j];

    dtx_ef_tx->L_pn_seed_tx = EF_PN_INITIAL_SEED;

    /* reset all the encoder state variables */
    /* ------------------------------------- */

    for (i = 0; i < EF_LPCORDER; i++)
        dtx_ef_tx->lsf_p_CN[i] = TableMeanLSF_5[i];      /* CNI */

    
}


Ipp32s ownGainQuantInit_GSMAMR(sGainQuantSt *state)
{

   state->vExpPredCBGain = 0;
   state->vFracPredCBGain = 0;
   state->vExpTargetEnergy = 0;
   state->vFracTargetEnergy = 0;

   ippsZero_16s(state->a_ExpEnCoeff, 5);
   ippsZero_16s(state->a_FracEnCoeff, 5);
   state->pGainPtr = NULL;
   ippsSet_16s(MIN_ENERGY, state->a_PastQntEnergy, NUM_PRED_TAPS);
   ippsSet_16s(MIN_ENERGY_M122, state->a_PastQntEnergy_M122, NUM_PRED_TAPS);
   ippsSet_16s(MIN_ENERGY, state->a_PastUnQntEnergy, NUM_PRED_TAPS);
   ippsSet_16s(MIN_ENERGY_M122, state->a_PastUnQntEnergy_M122, NUM_PRED_TAPS);
   state->vOnSetQntGain = 0;
   state->vPrevAdaptOut = 0;
   state->vPrevGainCode = 0;
   ippsZero_16s(state->a_LTPHistoryGain,NUM_MEM_LTPG);

   return 1;
}

Ipp32s ownVAD1Init_GSMAMR(IppGSMAMRVad1State *state)
{

   state->pitchFlag = 0;
   state->complexHigh = 0;
   state->complexLow = 0;
   state->complexHangTimer = 0;
   state->vadReg = 0;
   state->statCount = 0;
   state->burstCount = 0;
   state->hangCount = 0;
   state->complexHangCount = 0;

   ippsZero_16s( state->pFifthFltState, 6 );
   ippsZero_16s( state->pThirdFltState, 5 );

   ippsSet_16s(INIT_BACKGROUND_NOISE, state->pBkgNoiseEstimate, NUM_SUBBANDS_VAD);
   ippsSet_16s(INIT_BACKGROUND_NOISE, state->pPrevSignalLevel, NUM_SUBBANDS_VAD);
   ippsSet_16s(INIT_BACKGROUND_NOISE, state->pPrevAverageLevel, NUM_SUBBANDS_VAD);
   ippsZero_16s( state->pPrevSignalSublevel, NUM_SUBBANDS_VAD );

   state->corrHp = INIT_LOWPOW_SEGMENT;
   state->complexWarning = 0;

   return 1;
}

Ipp32s ownVAD2Init_GSMAMR(IppGSMAMRVad2State *state)
{
    ippsZero_16s((Ipp16s *)state, sizeof(IppGSMAMRVad2State)/2);
    return 1;
}

//Ipp32s ownEncDetectSize_GSMAMR(Ipp32s mode, Ipp32s* pEncSize)
Ipp32s ownEncDetectSize_GSMAMR(Ipp32s* pEncSize)
{
   //if (mode == GSMAMREncode_VAD1_Enabled)
      *pEncSize += sizeof(IppGSMAMRVad1State);
   //if (mode == GSMAMREncode_VAD2_Enabled)
      *pEncSize += sizeof(IppGSMAMRVad2State);

   return 1;
}

Ipp32s ownEncoderInit_GSMAMR(GSMAMREncoder_Obj* pEnc)
{
   sEncoderState_GSMAMR *stEnc = &pEnc->stEncState;
   stEnc->vFlagDTX = pEnc->objPrm.mode;

   /*Further passing down the EFR flag */
   stEnc->EFR_flag = pEnc->EFR_flag;

   stEnc->pSpeechPtrNew = stEnc->a_SpeechVecOld + SPEECH_BUF_SIZE - FRAME_SIZE_GSMAMR;   /* New speech     */
   stEnc->pSpeechPtr = stEnc->pSpeechPtrNew - SUBFR_SIZE_GSMAMR;                  /* Present frame  */
   stEnc->pWindowPtr = stEnc->a_SpeechVecOld + SPEECH_BUF_SIZE - LP_WINDOW_SIZE;    /* For LPC window */
   stEnc->pWindowPtr_M122 = stEnc->pWindowPtr - SUBFR_SIZE_GSMAMR; /* EFR LPC window: no lookahead */

   stEnc->pWeightSpeechVec = stEnc->a_WeightSpeechVecOld + PITCH_MAX_LAG;
   stEnc->pExcVec = stEnc->a_ExcVecOld + PITCH_MAX_LAG + FLT_INTER_SIZE;
   stEnc->pZeroVec = stEnc->a_ZeroVec + LP1_ORDER_SIZE;
   stEnc->pErrorPtr = stEnc->a_MemoryErr + LP_ORDER_SIZE;
   stEnc->pImpResVec = &stEnc->a_ImpResVec[0];

   ippsZero_16s(stEnc->a_SpeechVecOld, SPEECH_BUF_SIZE);
   ippsZero_16s(stEnc->a_ExcVecOld,PITCH_MAX_LAG + FLT_INTER_SIZE);
   ippsZero_16s(stEnc->a_WeightSpeechVecOld,PITCH_MAX_LAG);
   ippsZero_16s(stEnc->a_MemorySyn,LP_ORDER_SIZE);
   //ippsZero_16s(stEnc->a_Memory_W1,LP_ORDER_SIZE);
   ippsZero_16s(stEnc->a_Memory_W0,LP_ORDER_SIZE);
   ippsZero_16s(stEnc->a_MemoryErr,LP_ORDER_SIZE);
   ippsZero_16s(stEnc->pZeroVec,SUBFR_SIZE_GSMAMR);

   ippsSet_16s(40, stEnc->a_LTPStateOld, 5);

   ippsZero_16s(stEnc->a_SubState, (LP_ORDER_SIZE + 1));
   stEnc->a_SubState[0] = 4096;

   ippsCopy_16s(TableLSPInitData, &stEnc->a_LSP_Old[0], LP_ORDER_SIZE);
   ippsCopy_16s(stEnc->a_LSP_Old, stEnc->a_LSPQnt_Old, LP_ORDER_SIZE);
   ippsZero_16s(stEnc->a_PastQntPredErr,LP_ORDER_SIZE);

   stEnc->vTimePrevSubframe = 0;

   ownGainQuantInit_GSMAMR(&stEnc->stGainQntSt);

   stEnc->vTimeMedOld = 40;
   stEnc->vFlagVADState = 0;
   //stEnc->wght_flg = 0;

   stEnc->vCount = 0;
   ippsZero_16s(stEnc->a_GainHistory, PG_NUM_FRAME);    /* Init Gp_Clipping */

  // if (stEnc->vFlagDTX == GSMAMREncode_VAD1_Enabled)
      ownVAD1Init_GSMAMR((IppGSMAMRVad1State*)stEnc->pVAD1St);
  // if (stEnc->vFlagDTX == GSMAMREncode_VAD2_Enabled)
      ownVAD2Init_GSMAMR((IppGSMAMRVad2State*)stEnc->pVAD2St);
  
   if (pEnc->EFR_flag)
        amr_ef_dtx_enc_reset(&stEnc->dtx.ef_tx, stEnc->dtx.ef_tx.taf_count);  /* reset all the transmit DTX and CN variables */
    else
        ownDtxEncoderInit_GSMAMR(&stEnc->stDTXEncState);
  
   stEnc->vFlagSharp = PITCH_SHARP_MIN;
   stEnc->vLagCountOld = 0;
   stEnc->vLagOld = 0;
   stEnc->vFlagTone = 0;
   stEnc->vBestHpCorr = INIT_LOWPOW_SEGMENT;

   return 1;
}
GSMAMR_CODECFUN( APIGSMAMR_Status, apiGSMAMREncoder_Mode,
                (GSMAMREncoder_Obj* encoderObj, Ipp32u mode))
{
   encoderObj->objPrm.mode = mode;
   encoderObj->stEncState.vFlagDTX = mode;
   return APIGSMAMR_StsNoErr;
}

/*************************************************************************
*  Function:   apiGSMAMREncoder_Init
*     Initializes coder state memory
**************************************************************************/
GSMAMR_CODECFUN( APIGSMAMR_Status, apiGSMAMREncoder_Init,
                (GSMAMREncoder_Obj* encoderObj, Ipp32u mode, Ipp16s EFR_flag, Ipp16s taf_count))
{
   Ipp32s fltSize;
   IPP_ALIGNED_ARRAY(16, Ipp16s, abEnc, 6);
   Ipp32s obj_size = sizeof(GSMAMREncoder_Obj);
   ippsZero_16s((Ipp16s*)encoderObj,sizeof(GSMAMREncoder_Obj)>>1) ;
   encoderObj->objPrm.mode = mode;
   encoderObj->objPrm.key = ENC_KEY;
   encoderObj->preProc = (Ipp8s*)encoderObj + sizeof(GSMAMREncoder_Obj);
   ippsHighPassFilterSize_G729(&fltSize);
   obj_size += fltSize;
   //ownEncDetectSize_GSMAMR(mode,&obj_size);
   ownEncDetectSize_GSMAMR(&obj_size);
   encoderObj->stEncState.pVAD1St = (Ipp8s*)encoderObj->preProc + fltSize;
   fltSize = sizeof(IppGSMAMRVad1State);
   encoderObj->stEncState.pVAD2St = (Ipp8s*)encoderObj->stEncState.pVAD1St + fltSize;
   obj_size += fltSize;
   encoderObj->objPrm.objSize = obj_size;
   /*passing the EFR flag */
   encoderObj->EFR_flag = EFR_flag;
   encoderObj->reset_flag = 0;
   encoderObj->reset_flag_old = 1;

   abEnc[0] = pTblCoeff_a140[0];
   abEnc[1] = pTblCoeff_a140[1];
   abEnc[2] = pTblCoeff_a140[2];
   abEnc[3] = pTblCoeff_b140[0];
   abEnc[4] = pTblCoeff_b140[1];
   abEnc[5] = pTblCoeff_b140[2];

   encoderObj->stEncState.dtx.ef_tx.taf_count = taf_count;
   ownEncoderInit_GSMAMR(encoderObj);
   ippsHighPassFilterInit_G729(abEnc, (char *)encoderObj->preProc);

   return APIGSMAMR_StsNoErr;
}


/*************************************************************************
*  Function:   apiGSMAMREncode
**************************************************************************/

GSMAMR_CODECFUN(  APIGSMAMR_Status, apiGSMAMREncode,
         (GSMAMREncoder_Obj* encoderObj,const Ipp16s* src,  GSMAMR_Rate_t rate,
          Ipp8u* dst, Ipp32s *pVad ))
{
   IPP_ALIGNED_ARRAY(16, Ipp16s, pSynthVec, FRAME_SIZE_GSMAMR);        /* Buffer for synthesis speech           */
   IPP_ALIGNED_ARRAY(16, Ipp16s, pParamVec, MAX_NUM_PRM + 1);
   Ipp16s *pParamPtr = pParamVec;
   Ipp16s *pNewSpeech;
   TXFrameType frame_type;
   
   if(NULL==encoderObj || NULL==src || NULL ==dst )
      return APIGSMAMR_StsBadArgErr;

   pNewSpeech = encoderObj->stEncState.pSpeechPtrNew;

   if(0 >= encoderObj->objPrm.objSize)
      return APIGSMAMR_StsNotInitialized;
   if(rate > GSMAMR_RATE_DTX  || rate < 0)
      return APIGSMAMR_StsBadArgErr;
   if(ENC_KEY != encoderObj->objPrm.key)
      return APIGSMAMR_StsBadCodecType;
   encoderObj->rate = rate;

   /* filter + downscaling */
   ippsCopy_16s(src, pNewSpeech, FRAME_SIZE_GSMAMR);
   ippsHighPassFilter_G729_16s_ISfs(pNewSpeech, FRAME_SIZE_GSMAMR, 12, (char *)encoderObj->preProc);

   /* Call the speech encoder */
   ownEncode_GSMAMR(&encoderObj->stEncState, encoderObj->rate, pParamPtr, pVad, pSynthVec);

    if ((encoderObj->stEncState.dtx.ef_tx.txdtx_ctrl & EF_TX_ACTIVE) != 0)
    { /* EFR */
        pParamPtr[AMR_PRMNO_MR122_EFR] = encoderObj->stEncState.dtx.ef_tx.txdtx_ctrl; /* save flags from reset */

        encoderObj->stEncState.dtx.ef_tx.taf_count = (encoderObj->stEncState.dtx.ef_tx.taf_count < (EF_CN_INT_PERIOD-1))
                                              ? encoderObj->stEncState.dtx.ef_tx.taf_count+1 : 0;

        if ((encoderObj->stEncState.dtx.ef_tx.txdtx_ctrl & EF_TX_SP_FLAG) == 0)  /* if not speech */
        {
            rate = GSMAMR_RATE_DTE;
            frame_type = TX_SID_UPDATE;
            if (encoderObj->stEncState.dtx.ef_tx.prev_ft != TX_SPEECH_GOOD)
            {
                Ipp16s sid_count;
                sid_count = (encoderObj->dtx8_flag)? encoderObj->stEncState.dtx.ef_tx.taf_count & 7 /* if every 8th */
                                                                          : encoderObj->stEncState.dtx.ef_tx.taf_count;
                if (sid_count != 0)
                    frame_type = TX_NO_DATA;
            }
	    }
        else
        {
            if ((encoderObj->reset_flag != 0) && (encoderObj->reset_flag_old != 0))
                /* send homing pattern */
                ippsCopy_16s(AG_SC_amr_ef_dhf_mask, pParamPtr, AMR_PRMNO_MR122_EFR);
            	frame_type = TX_SPEECH_GOOD;
        }
	        encoderObj->stEncState.dtx.ef_tx.prev_ft = frame_type;
			encoderObj->frame_type = frame_type;
    }
	else    
   	{	
   		if(!*pVad) rate = GSMAMR_RATE_DTX;
    }
   ownPrm2Bits_GSMAMR(pParamVec, dst, rate);

   return APIGSMAMR_StsNoErr;
}

/*************************************************************************
 *
 *   FUNCTION NAME: amr_ef_Dtx_Tx
 *
 *   PURPOSE: DTX handler of the speech encoder. Determines when to add
 *            the hangover period to the end of the speech burst, and
 *            also determines when to use old SID parameters, and when
 *            to update the SID parameters. This function also initializes
 *            the pseudo noise generator shift register.
 *
 *            Operation of the TX DTX handler is based on the VAD flag
 *            given as input from the speech encoder.
 *
 *   INPUTS:      VAD_flag      Voice activity decision
 *                *txdtx_ctrl   Old encoder DTX control word
 *
 *   OUTPUTS:     *txdtx_ctrl   Updated encoder DTX control word
 *                L_pn_seed_tx  Initialized pseudo noise generator shift
 *                              register (global variable)
 *
 *   RETURN VALUE: none
 *
 *************************************************************************/

void amr_ef_Dtx_Tx(
    Word16 VAD_flag,
    AMR_ef_dtx_tx_State_t *dtx_ef_tx
)
{   

    /* N_elapsed (frames since last SID update) is incremented. If SID
       is updated N_elapsed is cleared later in this function */

    dtx_ef_tx->txdtx_N_elapsed = Add_16s(dtx_ef_tx->txdtx_N_elapsed, 1);

    /* If voice activity was detected, reset hangover counter */


    if (VAD_flag == 1)
    {
        dtx_ef_tx->txdtx_hangover = EF_DTX_HANGOVER;
        dtx_ef_tx->txdtx_ctrl = EF_TX_ACTIVE
              | EF_TX_SP_FLAG | EF_TX_VAD_FLAG;
    }
    else
    {

        if (dtx_ef_tx->txdtx_hangover == 0)
        {
            /* Hangover period is over, SID should be updated */

            dtx_ef_tx->txdtx_N_elapsed = 0;

            /* Check if this is the first frame after hangover period */

            if ((dtx_ef_tx->txdtx_ctrl & EF_TX_HANGOVER_ACTIVE) != 0)
            {
                dtx_ef_tx->txdtx_ctrl = EF_TX_ACTIVE
                   | EF_TX_PREV_HANGOVER_ACTIVE | EF_TX_SID_UPDATE;
                dtx_ef_tx->L_pn_seed_tx = EF_PN_INITIAL_SEED;
            }
            else
            {
                dtx_ef_tx->txdtx_ctrl = EF_TX_ACTIVE | EF_TX_SID_UPDATE;
            }
        }
        else
        {
            /* Hangover period is not over, update hangover counter */
            dtx_ef_tx->txdtx_hangover = Sub_16s(dtx_ef_tx->txdtx_hangover, 1);

            /* Check if elapsed time from last SID update is greater than
               threshold. If not, set SP=0 (although hangover period is not
               over) and use old SID parameters for new SID frame.
               N_elapsed counter must be summed with hangover counter in order
               to avoid erroneus SP=1 decision in case when N_elapsed is grown
               bigger than threshold and hangover period is still active */


            if (Sub_16s(Add_16s(dtx_ef_tx->txdtx_N_elapsed, dtx_ef_tx->txdtx_hangover),
                     EF_DTX_ELAPSED_THRESHOLD) < 0)
            {
                /* old SID frame should be used */
                dtx_ef_tx->txdtx_ctrl = EF_TX_ACTIVE | EF_TX_USE_OLD_SID;
            }
            else
            {

                if ((dtx_ef_tx->txdtx_ctrl & EF_TX_HANGOVER_ACTIVE) != 0)
                {
                    dtx_ef_tx->txdtx_ctrl = EF_TX_ACTIVE
                        | EF_TX_PREV_HANGOVER_ACTIVE
                        | EF_TX_HANGOVER_ACTIVE
                        | EF_TX_SP_FLAG;
                }
                else
                {
                    dtx_ef_tx->txdtx_ctrl = EF_TX_ACTIVE
                        | EF_TX_HANGOVER_ACTIVE
                        | EF_TX_SP_FLAG;
                }
            }
        }
    }
	    
}



#define  INV_SQRT10  10362   /*  1/sqroot(10) */
#define  INV_SQRT2   23170   /*  1/sqroot(2) */
#define  LONG_3OVER8   805306368L  /* 3/8 = 0.375 */
#define  SHORT_3OVER4  24576        /* 3/4=0.75   */

/*************************************************************************
*  Title: ef_compute_CN_excitation_gain
*  General Description: Compute the unquantized fixed codebook gain.
*                       Computation is based on the energy of the Linear
*                       Prediction residual signal.
*  Input variables: res2[0..39]   Linear Prediction residual signal
*  Output variables: Unquantized fixed codebook gain
*  Local Variables:
*  Global Variables:
*
*  Function Specifics (Functions, Algorithms used in file)
*           gain=sqrt(residual energy)/sqrt(10)
**************************************************************************/

Ipp16s ef_compute_CN_excitation_gain(
    const Ipp16s res2[EF_L_SUBFR]
)
{
    Ipp16s i, norm, norm1, temp, overfl;
    Ipp32s L_temp;

    /* Compute the energy of the LP residual signal */
    norm = 0;
    do
    {
        overfl = 0;

        L_temp = 0L;
        for (i = 0; i < EF_L_SUBFR; i++)
        {
            temp = res2[i] >> norm;
            L_temp += temp * temp * 2;
        }


        if ((L_temp - IPP_MAX_32S) == 0)
        {
            norm = norm + 1;
            overfl = 1;         /* Set the overflow flag */
        }

    } while (overfl != 0);

    
    L_temp = L_temp + 1; /* Avoid the case of all zeros */

    /**********************************************************************
    *
    *   Gain = sqrt(energy)/sqrt(10);
    *
    * Take the square root of the obtained energy value (sqroot is a 2nd
    * order Taylor series approximation)
    *  sqroot(1-x) = 1 - x/2 -x*x/8
    *              = 3/8 +3*(1-x)/4 - (1-x)*(1-x)/8
    **********************************************************************/

    norm1 = Normalization_32s(L_temp);
    temp = Extract_h_32s16s(ShiftL_32s(L_temp, norm1));
    L_temp = temp * temp * 2;
    L_temp =LONG_3OVER8 - (L_temp >> 3);
    L_temp = L_temp + SHORT_3OVER4 * temp * 2;

    temp = Extract_h_32s16s(L_temp);


    if ((norm1 & 0x0001) != 0)
    {
        temp = Mult_r_16s(temp, INV_SQRT2);  /* 1/sqroot(2) */      
        norm1 = norm1 - 1;
    }

    /* Divide the result of sqroot operation by sqroot(10) */

    temp = Mult_r_16s(temp, INV_SQRT10);  /* 1/sqroot(2) */
    

    /* Re-scale to get the final value */

    norm1 = norm1 >> 1;
    norm1 = norm1 - norm;


    if (norm1 >= 0)
    {
        temp = temp >> norm1;
    }
    else
    {
        temp = temp << (-norm1);
    }
    
    return temp;
}


/*************************************************************************
*  Title: ef_aver_gain_code_history
*  General Description: Compute the averaged fixed codebook gain parameter
*                       value.
*  Input variables: CN_excitation_gain
*                              Unquantized fixed codebook gain value
*                              of the current subframe
*                   gain_code_old[0..4*EF_DTX_HANGOVER-1]
*                              fixed codebook gain parameter history
*  Output variables: Averaged fixed codebook gain value
*  Local Variables:
*  Global Variables:
*
*  Function Specifics
*            Computation is performed by averaging the fixed codebook
*            gain parameter values which exist in the fixed codebook
*            gain parameter history, together with the fixed codebook
*            gain parameter value of the current subframe.
**************************************************************************/

Ipp16s ef_aver_gain_code_history(
    Ipp16s CN_excitation_gain,
    const Ipp16s gain_code_old[4 * EF_DTX_HANGOVER]
)
{
    Ipp16s i;
    Ipp32s L_ret;

    L_ret = 0x470 * CN_excitation_gain * 2;
    
    for (i = 0; i < (4 * EF_DTX_HANGOVER); i++)
    {
        L_ret = Mac_16s32s_v2(L_ret, 0x470, gain_code_old[i]);
        
    }

    return Extract_h_32s16s(L_ret);
}


Ipp16s amr_ef_q_gain_code(   /* Return quantization index       */
    const Ipp16s code[],  /* (i)      : fixed codebook excitation       */
    Ipp16s exc[],         /* (i)      : adaptive codebook excitation    */
    AMR_ef_dtx_tx_State_t *dtx_ef_tx,
    Ipp16s i_subfr
)
{
    Ipp16s i, index = 0;
    Ipp16s err, err_min;
    Ipp16s aver_gain;
    Ipp16s gain_code;       /* quantized fixed codebook gain */
    Ipp32s L_temp;

     if ((dtx_ef_tx->txdtx_ctrl & EF_TX_PREV_HANGOVER_ACTIVE) != 0 && (i_subfr == 0))
    {
        dtx_ef_tx->gcode0_CN = ef_update_gcode0_CN(dtx_ef_tx->gain_code_old_tx);
        dtx_ef_tx->gcode0_CN = ShiftL_16s(dtx_ef_tx->gcode0_CN, 4);
     }
    gain_code = dtx_ef_tx->CN_excitation_gain;

    if ((dtx_ef_tx->txdtx_ctrl & EF_TX_SID_UPDATE) != 0)
    {
        aver_gain = ef_aver_gain_code_history(dtx_ef_tx->CN_excitation_gain,
                                                          dtx_ef_tx->gain_code_old_tx);

       /*---------------------------------------------------------------*
        *                   Search for best quantizer                   *
        *---------------------------------------------------------------*/

        err_min = Abs_16s(Sub_16s(aver_gain, Mult_16s(dtx_ef_tx->gcode0_CN, AG_SC_ef_qua_gain_code[0])));
        index = 0;

        for (i = 1; i < EF_NB_QUA_CODE; i++)
        {
            err = Abs_16s(Sub_16s(aver_gain, Mult_16s(dtx_ef_tx->gcode0_CN, AG_SC_ef_qua_gain_code[i])));
            
            if (Sub_16s(err, err_min) < 0)
            {
                err_min = err;
                index = i;
            }
        }

    }
    ef_update_gain_code_history(dtx_ef_tx->CN_excitation_gain, dtx_ef_tx->gain_code_old_tx, &dtx_ef_tx->buf_p_tx);

    /*------------------------------------------------------*
     * - Find the total excitation                          *
     *------------------------------------------------------*/

    for (i = 0; i < EF_L_SUBFR; i++)
    {
        /* exc[i] = gain_pit*exc[i] + gain_code*code[i]; */
        L_temp = code[i] * gain_code * 2;
        L_temp = L_temp << 3;
        exc[i] = Round_32s16s(L_temp);
    }
   
    return index;
}


/*************************************************************************
 *
 *   FUNCTION NAME: amr_ef_CN_encoding
 *
 *   PURPOSE:  Encoding of the comfort noise parameters into a SID frame.
 *             Use old SID parameters if necessary. Set the parameter
 *             indices not used by comfort noise parameters to zero.
 *
 *   INPUTS:      params[0..56]  Comfort noise parameter frame from the
 *                               speech encoder
 *                dtx_ef_tx->txdtx_ctrl     TX DTX handler control word
 *
 *   OUTPUTS:     params[0..56]  Comfort noise encoded parameter frame
 *
 *   RETURN VALUE: none
 *
 *************************************************************************/

void amr_ef_CN_encoding(
    Ipp16s params[],
    AMR_ef_dtx_tx_State_t *dtx_ef_tx
)
{
    Ipp16s *p;
    const Ipp8s *f;
    Ipp32s i, j;

    
    if ((dtx_ef_tx->txdtx_ctrl & EF_TX_SID_UPDATE) != 0)
    {
        /* Store new CN parameters in memory to be used later as old
           CN parameters */

        /* LPC parameter indices */
        for (i = 0; i < 5; i++)
        {
            dtx_ef_tx->old_CN_mem_tx[i] = params[i];
        }
        /* Codebook index computed in last subframe */
        dtx_ef_tx->old_CN_mem_tx[5] = params[56];
    }


    if ((dtx_ef_tx->txdtx_ctrl & EF_TX_USE_OLD_SID) != 0)
    {
        /* Use old CN parameters previously stored in memory */
        for (i = 0; i < 5; i++)
        {
            params[i] = dtx_ef_tx->old_CN_mem_tx[i];
        }
        params[17] = dtx_ef_tx->old_CN_mem_tx[5];
        params[30] = dtx_ef_tx->old_CN_mem_tx[5];
        params[43] = dtx_ef_tx->old_CN_mem_tx[5];
        params[56] = dtx_ef_tx->old_CN_mem_tx[5];
    }

    /* Set all the rest of the SID codewords
     *  INPUTS:      params[0..4]  Comfort noise parameters
     *               params[17,30,32,56] Comfort noise parameters
     */

    p = &params[5];
    f = efr_sid_pad;

    for (j = 0; j < 4; ++j)
    {
        for (i = 0; i < 12; ++i)
            *p++ = *f++;    /* 5-16, 18-29, 31-42, 44-55 */
        p++;        /*   17,    30,    43,    56 */
    }

/*
   Assignment    param[n] Bits (MSB LSB)    Description
     CN[0]           0       s1 - s7     index of 1st LSF submatrix               * LPC1 *
     CN[1]           1       s8 - s15    index of 2nd LSF submatrix               * LPC2 *
     CN[2]           2      s16 - s23    index of 3rd LSF submatrix               * LPC3 *
                            s24          sign of 3rd LSF submatrix
     CN[3]           3      s25 - s32    index of 4th LSF submatrix               * LPC4 *
     CN[4]           4      s33 - s38    index of 5th LSF submatrix               * LPC5 *
                          subframe 1
      0x3            5      s39 - s47    adaptive codebook index
      0x7            6      s48 - s51    adaptive codebook gain
      0xf            7      s52          sign information for 1st and 6th pulses
                            s53 - s55    position of 1st pulse
      0xf            8      s56          sign information for 2nd and 7th pulses
                            s57 - s59    position of 2nd pulse
      0xf            9      s60          sign information for 3rd and 8th pulses
                            s61 - s63    position of 3rd pulse
      0xf           10      s64          sign information for 4th and 9th pulses
                            s65 - s67    position of 4th pulse
      0xc           11      s68          sign information for 5th and 10th pulses
                            s69 - s71    position of 5th pulse
      0x0           12      s72 - s74    position of 6th pulse
      0x0           13      s75 - s77    position of 7th pulse
      0x0           14      s78 - s80    position of 8th pulse
      0x0           15      s81 - s83    position of 9th pulse
      0x0           16      s84 - s86    position of 10th pulse
     CN[5]*         17      s87 - s91    fixed codebook gain                      * FCB-GAIN 1 *
                          subframe 2
      0x7           18      s92 - s97    adaptive codebook index (relative)
      0x7           19      s98 - s101   adaptive codebook gain
      0xf           20     s102          sign information for 1st and 6th pulses
                           s103 - s105   position of 1st pulse
      0xf           21     s106          sign information for 2nd and 7th pulses
                           s107 - s109   position of 2nd pulse
      0xf           22     s110          sign information for 3rd and 8th pulses
                           s111 - s113   position of 3rd pulse
      0xf           23     s114          sign information for 4th and 9th pulses
                           s115 - s117   position of 4th pulse
      0xc           24     s118          sign information for 5th and 10th pulses
                           s119 - s121   position of 5th pulse
      0x0           25     s122 - s124   position of 6th pulse
      0x0           26     s125 - s127   position of 7th pulse
      0x0           27     s128 - s130   position of 8th pulse
      0x0           28     s131 - s133   position of 9th pulse
      0x0           29     s134 - s136   position of 10th pulse
     CN[5]*         30     s137 - s141   fixed codebook gain
                          subframe 3
      0x3           31     s142 - s150   adaptive codebook index
      0xf           32     s151 - s154   adaptive codebook gain
      0xf           33     s155          sign information for 1st and 6th pulses
                           s156 - s158   position of 1st pulse
      0xf           34     s159          sign information for 2nd and 7th pulses
                           s160 - s162   position of 2nd pulse
      0xf           35     s163          sign information for 3rd and 8th pulses
                           s164 - s166   position of 3rd pulse
      0xf           36     s167          sign information for 4th and 9th pulses
                           s168 - s170   position of 4th pulse
      0xc           37     s171          sign information for 5th and 10th pulses
                           s172 - s174   position of 5th pulse
      0x0           38     s175 - s177   position of 6th pulse
      0x0           39     s178 - s180   position of 7th pulse
      0x0           40     s181 - s183   position of 8th pulse
      0x0           41     s184 - s186   position of 9th pulse
      0x0           42     s187 - s189   position of 10th pulse
     CN[5]*         43     s190 - s194   fixed codebook gain
                          subframe 4
      0xf           44     s195 - s200   adaptive codebook index (relative)
      0xf           45     s201 - s204   adaptive codebook gain
      0xf           46     s205          sign information for 1st and 6th pulses
                           s206 - s208   position of 1st pulse
      0xc           47     s209          sign information for 2nd and 7th pulses
                           s210 - s212   position of 2nd pulse
      0xf           48     s213          sign information for 3rd and 8th pulses
                           s214 - s216   position of 3rd pulse
      0xf           49     s217          sign information for 4th and 9th pulses
                           s218 - s220   position of 4th pulse
      0xc           50     s221          sign information for 5th and 10th pulses
                           s222 - s224   position of 5th pulse
      0x0           51     s225 - s227   position of 6th pulse
      0x0           52     s228 - s230   position of 7th pulse
      0x0           53     s231 - s233   position of 8th pulse
      0x0           54     s234 - s236   position of 9th pulse
      0x0           55     s237 - s239   position of 10th pulse
     CN[5]          56     s240 - s244   fixed codebook gain
*/    
}


/***************************************************************************
 * Function: ownEncode_GSMAMR
 ***************************************************************************/
Ipp32s ownEncode_GSMAMR(sEncoderState_GSMAMR *encSt,GSMAMR_Rate_t rate, Ipp16s *pAnaParam,
                     Ipp32s *pVad, Ipp16s *pSynthVec)
{
   /* LPC coefficients */
   IPP_ALIGNED_ARRAY(16, Ipp16s, A_t, (LP1_ORDER_SIZE) * 4);
   IPP_ALIGNED_ARRAY(16, Ipp16s, Aq_t, (LP1_ORDER_SIZE) * 4);
   Ipp16s *A, *Aq;
   IPP_ALIGNED_ARRAY(16, Ipp16s, pNewLSP, 2*LP_ORDER_SIZE);

   IPP_ALIGNED_ARRAY(16, Ipp16s, pPitchSrch, SUBFR_SIZE_GSMAMR);
   IPP_ALIGNED_ARRAY(32, Ipp16s, pCodebookSrch, SUBFR_SIZE_GSMAMR);
   IPP_ALIGNED_ARRAY(16, Ipp16s, pFixCodebookExc, SUBFR_SIZE_GSMAMR);
   IPP_ALIGNED_ARRAY(16, Ipp16s, pFltAdaptExc, SUBFR_SIZE_GSMAMR);
   IPP_ALIGNED_ARRAY(16, Ipp16s, pFltFixExc, SUBFR_SIZE_GSMAMR);
   IPP_ALIGNED_ARRAY(16, Ipp16s, pLPCPredRes, SUBFR_SIZE_GSMAMR);
   IPP_ALIGNED_ARRAY(16, Ipp16s, pLPCPredRes2, SUBFR_SIZE_GSMAMR);

   IPP_ALIGNED_ARRAY(16, Ipp16s, pPitchSrchM475, SUBFR_SIZE_GSMAMR);
   IPP_ALIGNED_ARRAY(16, Ipp16s, pFltCodebookM475, SUBFR_SIZE_GSMAMR);
   IPP_ALIGNED_ARRAY(16, Ipp16s, pFixCodebookExcM475, SUBFR_SIZE_GSMAMR);
   IPP_ALIGNED_ARRAY(16, Ipp16s, pImpResM475, SUBFR_SIZE_GSMAMR);
   IPP_ALIGNED_ARRAY(16, Ipp16s, pMemSynSave, LP_ORDER_SIZE);
   IPP_ALIGNED_ARRAY(16, Ipp16s, pMemW0Save, LP_ORDER_SIZE);
   IPP_ALIGNED_ARRAY(16, Ipp16s, pMemErrSave, LP_ORDER_SIZE);
   IPP_ALIGNED_ARRAY(16, Ipp16s, pSrcWgtLpc1, 44);
   IPP_ALIGNED_ARRAY(16, Ipp16s, pSrcWgtLpc2, 44);
   IPP_ALIGNED_ARRAY(16, Ipp16s, pNewLSP1, 2*LP_ORDER_SIZE);
   IPP_ALIGNED_ARRAY(16, Ipp16s, pNewQntLSP1, 2*LP_ORDER_SIZE);
   Ipp16s sharp_save = 0;
   Ipp16s evenSubfr;
   Ipp16s T0_sf0 = 0;
   Ipp16s T0_frac_sf0 = 0;
   Ipp16s i_subfr_sf0 = 0;
   Ipp16s gain_pit_sf0;
   Ipp16s gain_code_sf0;

   Ipp16s i_subfr, subfrNr, i;
   Ipp16s T_op[2];
   Ipp16s T0, T0_frac;
   Ipp16s gain_pit, gain_code;
   IppSpchBitRate irate = mode2rates[rate];
   IppSpchBitRate usedRate = irate;

   /* Flags */
   Ipp16s lsp_flag = 0;
   Ipp16s gainPitchLimit;
   Ipp16s vad_flag=0;
   Ipp16s compute_sid_flag=0;

   const Ipp16s *g1;
   Ipp32s L_Rmax=0;
   Ipp32s L_R0=0;
   Ipp16s *pAnaParamOld = pAnaParam;
   

   {
      IPP_ALIGNED_ARRAY(16, Ipp32s, r_auto, LP1_ORDER_SIZE*2);
      if ( IPP_SPCHBR_12200 == irate ) {
        ippsAutoCorr_GSMAMR_16s32s(encSt->pWindowPtr_M122, r_auto, irate);
        ippsLevinsonDurbin_GSMAMR_32s16s(&r_auto[0], &A_t[LP1_ORDER_SIZE]);
        ippsLevinsonDurbin_GSMAMR_32s16s(&r_auto[LP1_ORDER_SIZE], &A_t[LP1_ORDER_SIZE*3]);
      } else {
        ippsAutoCorr_GSMAMR_16s32s(encSt->pWindowPtr, r_auto,  irate);
        ippsLevinsonDurbin_GSMAMR_32s16s(r_auto, &A_t[LP1_ORDER_SIZE * 3]);
      }
   }
   *pVad = 1;

   if (encSt->vFlagDTX) {
      Ipp16s srate = -1;

      if (encSt->vFlagDTX == GSMAMREncode_VAD2_Enabled)
         ippsVAD2_GSMAMR_16s(encSt->pSpeechPtrNew, (IppGSMAMRVad2State*)encSt->pVAD2St,
                             &vad_flag, encSt->vFlagLTP);
      else
         ippsVAD1_GSMAMR_16s(encSt->pSpeechPtr, (IppGSMAMRVad1State*)encSt->pVAD1St,
                             &vad_flag, encSt->vBestHpCorr, encSt->vFlagTone);
        if ((encSt->dtx.ef_tx.txdtx_ctrl & EF_TX_ACTIVE) != 0)
        {
            amr_ef_Dtx_Tx (vad_flag, &(encSt->dtx.ef_tx));
        }
        else
		{
			ippsEncDTXHandler_GSMAMR_16s(&encSt->stDTXEncState.vDTXHangoverCt,
                                   &encSt->stDTXEncState.vDecExpireCt,
                                   &srate, &compute_sid_flag, vad_flag);
			if(srate == IPP_SPCHBR_DTX) {
				usedRate = IPP_SPCHBR_DTX;
				*pVad = 0;  /* SID frame */
			}
		}
   }
   else
   {
	   if ((encSt->dtx.ef_tx.txdtx_ctrl & EF_TX_ACTIVE) != 0)
        {
            encSt->dtx.ef_tx.txdtx_ctrl = EF_TX_ACTIVE | EF_TX_VAD_FLAG | EF_TX_SP_FLAG;
        }

   }

   /* From A(z) to lsp. LSP quantization and interpolation */
   {
      IPP_ALIGNED_ARRAY(16, Ipp16s, lsp_new_q, LP_ORDER_SIZE*2);

	  /* in case it is DTX */
	   ippsZero_16s(lsp_new_q,    LP_ORDER_SIZE*2);

      if( IPP_SPCHBR_12200 == irate) {
         IPP_ALIGNED_ARRAY(16, Ipp16s, lsp, LP_ORDER_SIZE);

         ippsLPCToLSP_GSMAMR_16s(&A_t[LP1_ORDER_SIZE], encSt->a_LSP_Old, &(pNewLSP[LP_ORDER_SIZE]));
         ippsLPCToLSP_GSMAMR_16s(&A_t[LP1_ORDER_SIZE * 3], &(pNewLSP[LP_ORDER_SIZE]), &(pNewLSP[0]));

         ippsInterpolate_G729_16s(&pNewLSP[LP_ORDER_SIZE],encSt->a_LSP_Old,lsp,LP_ORDER_SIZE);
         ippsLSPToLPC_GSMAMR_16s(lsp, A_t);
         ippsInterpolate_G729_16s(&pNewLSP[LP_ORDER_SIZE],pNewLSP,lsp,LP_ORDER_SIZE);
         ippsLSPToLPC_GSMAMR_16s(lsp, &A_t[2*LP1_ORDER_SIZE]);

        if ((encSt->dtx.ef_tx.txdtx_ctrl & EF_TX_ACTIVE) != 0)
        {
            Ipp16s *tmpEFRP[3];
			ippsCopy_16s(&pNewLSP[LP_ORDER_SIZE], pNewLSP1, LP_ORDER_SIZE);
            ippsCopy_16s(pNewLSP, &pNewLSP1[LP_ORDER_SIZE], LP_ORDER_SIZE);
            /* LSP quantization (lsp_mid[] and lsp_new[] jointly quantized) */
            tmpEFRP[0] = &(encSt->dtx.ef_tx.lsf_old_tx[0][0]);
			tmpEFRP[1] = encSt->dtx.ef_tx.lsf_p_CN;
			tmpEFRP[2] = &(encSt->dtx.ef_tx.next_lsf_old);
            ipps_Q_plsf_5_GSMEFR_16s(&encSt->a_PastQntPredErr[0], &pNewLSP[LP_ORDER_SIZE], &pNewLSP[0], &lsp_new_q[LP_ORDER_SIZE], &lsp_new_q[0], pAnaParam, encSt->dtx.ef_tx.txdtx_ctrl,
                                    &tmpEFRP[0]);

            /* Advance analysis parameters pointer */
            pAnaParam += 5;

            if ((encSt->dtx.ef_tx.txdtx_ctrl & EF_TX_SP_FLAG) != 0){
                amr_Int_lpc_1and3(encSt->a_LSPQnt_Old, &lsp_new_q[LP_ORDER_SIZE], &lsp_new_q[0], Aq_t);
            }
        }
        else
         if ( IPP_SPCHBR_DTX != usedRate) {

             ippsCopy_16s(&pNewLSP[LP_ORDER_SIZE], pNewLSP1, LP_ORDER_SIZE);
             ippsCopy_16s(pNewLSP, &pNewLSP1[LP_ORDER_SIZE], LP_ORDER_SIZE);
             ippsLSPQuant_GSMAMR_16s(pNewLSP1, encSt->a_PastQntPredErr, pNewQntLSP1, pAnaParam, irate);
             ippsCopy_16s(&pNewQntLSP1[LP_ORDER_SIZE], lsp_new_q, LP_ORDER_SIZE);
             ippsCopy_16s(pNewQntLSP1, &lsp_new_q[LP_ORDER_SIZE], LP_ORDER_SIZE);

             ippsInterpolate_G729_16s(&lsp_new_q[LP_ORDER_SIZE],encSt->a_LSPQnt_Old,lsp,LP_ORDER_SIZE);
             ippsLSPToLPC_GSMAMR_16s(lsp, Aq_t);
             ippsLSPToLPC_GSMAMR_16s(&lsp_new_q[LP_ORDER_SIZE], &Aq_t[LP1_ORDER_SIZE]);
             ippsInterpolate_G729_16s(&lsp_new_q[LP_ORDER_SIZE],lsp_new_q,lsp,LP_ORDER_SIZE);
             ippsLSPToLPC_GSMAMR_16s(lsp, &Aq_t[2*LP1_ORDER_SIZE]);
             ippsLSPToLPC_GSMAMR_16s(lsp_new_q, &Aq_t[3*LP1_ORDER_SIZE]);
             pAnaParam += 5;
         }
      }
      else
      {
         IPP_ALIGNED_ARRAY(16, Ipp16s, lsp, LP_ORDER_SIZE);
         ippsLPCToLSP_GSMAMR_16s(&A_t[LP1_ORDER_SIZE * 3], encSt->a_LSP_Old, pNewLSP);

         ippsInterpolate_GSMAMR_16s(pNewLSP,encSt->a_LSP_Old,lsp,LP_ORDER_SIZE);
         ippsLSPToLPC_GSMAMR_16s(lsp, A_t);
         ippsInterpolate_G729_16s(encSt->a_LSP_Old,pNewLSP,lsp,LP_ORDER_SIZE);
         ippsLSPToLPC_GSMAMR_16s(lsp, &A_t[LP1_ORDER_SIZE]);
         ippsInterpolate_GSMAMR_16s(encSt->a_LSP_Old,pNewLSP,lsp,LP_ORDER_SIZE);
         ippsLSPToLPC_GSMAMR_16s(lsp, &A_t[2*LP1_ORDER_SIZE]);
         /*  subframe 4 is not processed (available) */

         if ( IPP_SPCHBR_DTX != usedRate)
         {

             ippsLSPQuant_GSMAMR_16s(pNewLSP, encSt->a_PastQntPredErr, lsp_new_q, pAnaParam, irate);

             ippsInterpolate_GSMAMR_16s(lsp_new_q,encSt->a_LSPQnt_Old,lsp,LP_ORDER_SIZE);
             ippsLSPToLPC_GSMAMR_16s(lsp, Aq_t);
             ippsInterpolate_G729_16s(encSt->a_LSPQnt_Old,lsp_new_q,lsp,LP_ORDER_SIZE);
             ippsLSPToLPC_GSMAMR_16s(lsp, &Aq_t[LP1_ORDER_SIZE]);
             ippsInterpolate_GSMAMR_16s(encSt->a_LSPQnt_Old,lsp_new_q,lsp,LP_ORDER_SIZE);
             ippsLSPToLPC_GSMAMR_16s(lsp, &Aq_t[2*LP1_ORDER_SIZE]);

             ippsLSPToLPC_GSMAMR_16s(lsp_new_q, &Aq_t[3*LP1_ORDER_SIZE]);
             pAnaParam += 3;
         }
      }

      ippsCopy_16s(&(pNewLSP[0]), encSt->a_LSP_Old, LP_ORDER_SIZE);
      ippsCopy_16s(&(lsp_new_q[0]), encSt->a_LSPQnt_Old, LP_ORDER_SIZE);
   }

    if ((encSt->dtx.ef_tx.txdtx_ctrl & EF_TX_ACTIVE) == 0)
	{
		ippsEncDTXBuffer_GSMAMR_16s(encSt->pSpeechPtrNew, pNewLSP, &encSt->stDTXEncState.vHistoryPtr,
                               encSt->stDTXEncState.a_LSPHistory, encSt->stDTXEncState.a_LogEnergyHistory);
	}


   if (((encSt->dtx.ef_tx.txdtx_ctrl & EF_TX_ACTIVE) == 0) && (IPP_SPCHBR_DTX == usedRate))
   {
      ippsEncDTXSID_GSMAMR_16s(encSt->stDTXEncState.a_LSPHistory, encSt->stDTXEncState.a_LogEnergyHistory,
                               &encSt->stDTXEncState.vLogEnergyIndex,
                               &encSt->stDTXEncState.vLSFQntIndex, encSt->stDTXEncState.a_LSPIndex,
                               encSt->stGainQntSt.a_PastQntEnergy,
                               encSt->stGainQntSt.a_PastQntEnergy_M122, compute_sid_flag);
      *(pAnaParam)++ = encSt->stDTXEncState.vLSFQntIndex;
      *(pAnaParam)++ = encSt->stDTXEncState.a_LSPIndex[0];
      *(pAnaParam)++ = encSt->stDTXEncState.a_LSPIndex[1];
      *(pAnaParam)++ = encSt->stDTXEncState.a_LSPIndex[2];
      *(pAnaParam)++ = encSt->stDTXEncState.vLogEnergyIndex;

      ippsZero_16s(encSt->a_ExcVecOld,    PITCH_MAX_LAG + FLT_INTER_SIZE);
      ippsZero_16s(encSt->a_Memory_W0,     LP_ORDER_SIZE);
      ippsZero_16s(encSt->a_MemoryErr,    LP_ORDER_SIZE);
      ippsZero_16s(encSt->pZeroVec,       SUBFR_SIZE_GSMAMR);

      ippsCopy_16s(TableLSPInitData, &encSt->a_LSP_Old[0], LP_ORDER_SIZE);
      ippsCopy_16s(encSt->a_LSP_Old, encSt->a_LSPQnt_Old, LP_ORDER_SIZE);
      ippsZero_16s(encSt->a_PastQntPredErr,LP_ORDER_SIZE);
      ippsCopy_16s(pNewLSP, encSt->a_LSP_Old, LP_ORDER_SIZE);
      ippsCopy_16s(pNewLSP, encSt->a_LSPQnt_Old, LP_ORDER_SIZE);
      encSt->vTimePrevSubframe = 0;
      encSt->vFlagSharp = PITCH_SHARP_MIN;

   } else lsp_flag = ownCheckLSPVec_GSMAMR(&encSt->vCount, encSt->a_LSP_Old);


   if(irate <= IPP_SPCHBR_7950) g1 = pTableGamma1;
   else                         g1 = pTableGamma1_M122;

   ippsMul_NR_16s_Sfs(A_t,g1,pSrcWgtLpc1,4*LP_ORDER_SIZE+4,15); /* four frames at a time*/
   ippsMul_NR_16s_Sfs(A_t,pTableGamma2,pSrcWgtLpc2,4*LP_ORDER_SIZE+4,15);

   if(encSt->vFlagDTX == GSMAMREncode_DefaultMode) {

       ippsOpenLoopPitchSearchNonDTX_GSMAMR_16s(pSrcWgtLpc1,pSrcWgtLpc2,(encSt->pSpeechPtr-LP_ORDER_SIZE),
                                                &encSt->vTimeMedOld,&encSt->vFlagVADState,encSt->a_LTPStateOld,
                                                encSt->a_WeightSpeechVecOld,T_op,encSt->a_GainFlg,irate);
   }
   else if(encSt->vFlagDTX == GSMAMREncode_VAD1_Enabled){

         ippsOpenLoopPitchSearchDTXVAD1_GSMAMR_16s(pSrcWgtLpc1, pSrcWgtLpc2,(encSt->pSpeechPtr-LP_ORDER_SIZE),
             &encSt->vFlagTone, &encSt->vTimeMedOld,&encSt->vFlagVADState,encSt->a_LTPStateOld,encSt->a_WeightSpeechVecOld,
             &encSt->vBestHpCorr,T_op,encSt->a_GainFlg,irate);

         ownVADPitchDetection_GSMAMR((IppGSMAMRVad1State*)encSt->pVAD1St, T_op, &encSt->vLagCountOld, &encSt->vLagOld);
   } else if(encSt->vFlagDTX == GSMAMREncode_VAD2_Enabled){
         ippsOpenLoopPitchSearchDTXVAD2_GSMAMR_16s32s(pSrcWgtLpc1,
         pSrcWgtLpc2,(encSt->pSpeechPtr-LP_ORDER_SIZE),&encSt->vTimeMedOld,&encSt->vFlagVADState,encSt->a_LTPStateOld,encSt->a_WeightSpeechVecOld,
         &L_Rmax, &L_R0, T_op,encSt->a_GainFlg,irate);

         ownUpdateLTPFlag_GSMAMR(irate, L_Rmax, L_R0, &encSt->vFlagLTP);
      }

   if (((encSt->dtx.ef_tx.txdtx_ctrl & EF_TX_ACTIVE) != 0) && ((encSt->dtx.ef_tx.txdtx_ctrl & EF_TX_SP_FLAG) == 0))
   {
	   /* EFR SID */

	   //ippsCopy_16s(&encSt->a_ExcVecOld[0], encSt->pExcVec, PITCH_MAX_LAG + FLT_INTER_SIZE);

	   A = A_t;	/* pointer to interpolated quantized LPC parameters */
	   for (i_subfr = 0; i_subfr < FRAME_SIZE_GSMAMR; i_subfr += SUBFR_SIZE_GSMAMR)
	   {

		   ippsResidualFilter_G729_16s_amr_sat(&encSt->pSpeechPtr[i_subfr],A, pLPCPredRes2);
		   encSt->dtx.ef_tx.CN_excitation_gain = ef_compute_CN_excitation_gain (pLPCPredRes2);
		   ef_build_CN_code(pFixCodebookExc, &encSt->dtx.ef_tx.L_pn_seed_tx);
		   pAnaParam += 12;

		   *pAnaParam = amr_ef_q_gain_code(pFixCodebookExc, &(encSt->a_ExcVecOld[PITCH_MAX_LAG + FLT_INTER_SIZE + i_subfr]), &encSt->dtx.ef_tx, i_subfr);
		    pAnaParam += 1;

		   ippsSynthesisFilter_NR_16s_Sfs(A, &(encSt->a_ExcVecOld[PITCH_MAX_LAG + FLT_INTER_SIZE + i_subfr]), &pSynthVec[i_subfr], 40, 12, encSt->a_MemorySyn);
		   ippsCopy_16s(&pSynthVec[SUBFR_SIZE_GSMAMR - LP_ORDER_SIZE + i_subfr], encSt->a_MemorySyn, LP_ORDER_SIZE); /* update memory */

		   /* update_gp_clipping(&st->tonStabSt, 0); */
		   ippsMove_16s(&encSt->a_GainHistory[1], &encSt->a_GainHistory[0], PG_NUM_FRAME-1);
		   encSt->a_GainHistory[PG_NUM_FRAME-1] = 0;

		   A += LP1_ORDER_SIZE;
	   }
	   amr_ef_CN_encoding(pAnaParamOld, &encSt->dtx.ef_tx);

	   for (i = 0; i < 4; i++)
		   encSt->stGainQntSt.a_PastQntEnergy_M122[i] = -2381;

	   ippsMove_16s(&encSt->a_ExcVecOld[FRAME_SIZE_GSMAMR], &encSt->a_ExcVecOld[0], PITCH_MAX_LAG + FLT_INTER_SIZE);

	   ippsZero_16s(encSt->a_Memory_W0, LP_ORDER_SIZE);
	   ippsZero_16s(encSt->a_MemoryErr,	LP_ORDER_SIZE);

	   /* Reset lsp states */	      
       ippsZero_16s(encSt->a_PastQntPredErr,LP_ORDER_SIZE);
       ippsCopy_16s(pNewLSP, encSt->a_LSP_Old, LP_ORDER_SIZE);
       ippsCopy_16s(pNewLSP, encSt->a_LSPQnt_Old, LP_ORDER_SIZE);

	   /* Reset clLtp states */
	   encSt->vTimePrevSubframe = 0;
	   encSt->vFlagSharp = PITCH_SHARP_MIN;
	   ippsCopy_16s(&encSt->a_ExcVecOld[FRAME_SIZE_GSMAMR], &encSt->a_ExcVecOld[0], PITCH_MAX_LAG + FLT_INTER_SIZE);
       ippsCopy_16s(&encSt->a_SpeechVecOld[FRAME_SIZE_GSMAMR], &encSt->a_SpeechVecOld[0], (SPEECH_BUF_SIZE - FRAME_SIZE_GSMAMR));

       return 0;
   }	   

   if (usedRate == IPP_SPCHBR_DTX)  {
      ippsCopy_16s(&encSt->a_SpeechVecOld[FRAME_SIZE_GSMAMR], &encSt->a_SpeechVecOld[0], (SPEECH_BUF_SIZE - FRAME_SIZE_GSMAMR));
      return 0;
   }

   A = A_t;
   Aq = Aq_t;
   if (irate == IPP_SPCHBR_12200 || irate == IPP_SPCHBR_10200) g1 = pTableGamma1_M122;
   else                                                        g1 = pTableGamma1;

   evenSubfr = 0;
   subfrNr = -1;
   for(i_subfr = 0; i_subfr < FRAME_SIZE_GSMAMR; i_subfr += SUBFR_SIZE_GSMAMR) {
      subfrNr++;
      evenSubfr = (Ipp16s)(1 - evenSubfr);

      if ((evenSubfr != 0) && (usedRate == IPP_SPCHBR_4750)) {
         ippsCopy_16s(encSt->a_MemorySyn, pMemSynSave, LP_ORDER_SIZE);
         ippsCopy_16s(encSt->a_Memory_W0, pMemW0Save, LP_ORDER_SIZE);
         ippsCopy_16s(encSt->a_MemoryErr, pMemErrSave, LP_ORDER_SIZE);
         sharp_save = encSt->vFlagSharp;
      }

      ippsMul_NR_16s_Sfs(A,g1,pSrcWgtLpc1,LP_ORDER_SIZE+1,15);
      ippsMul_NR_16s_Sfs(A,pTableGamma2,pSrcWgtLpc2,LP_ORDER_SIZE+1,15);

      if (usedRate != IPP_SPCHBR_4750) {
         ippsImpulseResponseTarget_GSMAMR_16s(&encSt->pSpeechPtr[i_subfr-LP_ORDER_SIZE],pSrcWgtLpc1,pSrcWgtLpc2,
            Aq,encSt->a_MemoryErr,encSt->a_Memory_W0,encSt->pImpResVec, pLPCPredRes, pPitchSrch);

      } else { /* MR475 */
         ippsImpulseResponseTarget_GSMAMR_16s(&encSt->pSpeechPtr[i_subfr-LP_ORDER_SIZE],pSrcWgtLpc1,pSrcWgtLpc2,
            Aq,encSt->a_MemoryErr,pMemW0Save,encSt->pImpResVec, pLPCPredRes, pPitchSrch);

         if (evenSubfr != 0)
             ippsCopy_16s (encSt->pImpResVec, pImpResM475, SUBFR_SIZE_GSMAMR);

      }

      ippsCopy_16s(pLPCPredRes, &encSt->pExcVec[i_subfr], SUBFR_SIZE_GSMAMR);
      ippsCopy_16s (pLPCPredRes, pLPCPredRes2, SUBFR_SIZE_GSMAMR);

      ownCloseLoopFracPitchSearch_GSMAMR(&encSt->vTimePrevSubframe, encSt->a_GainHistory, usedRate, i_subfr,
                                         T_op, encSt->pImpResVec, &encSt->pExcVec[i_subfr], pLPCPredRes2,
                                         pPitchSrch, lsp_flag, pCodebookSrch, pFltAdaptExc, &T0, &T0_frac,
                                         &gain_pit, &pAnaParam, &gainPitchLimit);

      if ((subfrNr == 0) && (encSt->a_GainFlg[0] > 0))
         encSt->a_LTPStateOld[1] = T0;
      if ((subfrNr == 3) && (encSt->a_GainFlg[1] > 0))
         encSt->a_LTPStateOld[0] = T0;

      if (usedRate == IPP_SPCHBR_12200)
        ippsAlgebraicCodebookSearch_GSMAMR_16s(T0, gain_pit,
            pCodebookSrch, pLPCPredRes2, encSt->pImpResVec, pFixCodebookExc, pFltFixExc, pAnaParam, subfrNr, IPP_SPCHBR_12200);
      else
        ippsAlgebraicCodebookSearch_GSMAMR_16s(T0, encSt->vFlagSharp,
            pCodebookSrch, pLPCPredRes2, encSt->pImpResVec, pFixCodebookExc, pFltFixExc, pAnaParam, subfrNr, usedRate);

      if(usedRate == IPP_SPCHBR_10200)       pAnaParam+=7;
      else if (usedRate == IPP_SPCHBR_12200) pAnaParam+=10;
      else                                   pAnaParam+=2;

      ownGainQuant_GSMAMR(&encSt->stGainQntSt, usedRate, pLPCPredRes, &encSt->pExcVec[i_subfr], pFixCodebookExc,
                          pPitchSrch, pCodebookSrch,  pFltAdaptExc, pFltFixExc, evenSubfr, gainPitchLimit,
                          &gain_pit_sf0, &gain_code_sf0, &gain_pit, &gain_code, &pAnaParam);
        if ((encSt->dtx.ef_tx.txdtx_ctrl & EF_TX_ACTIVE) != 0)
        {
            ef_update_gain_code_history(ShiftR_16s(gain_code,1), encSt->dtx.ef_tx.gain_code_old_tx, &encSt->dtx.ef_tx.buf_p_tx);
        }

      ippsMove_16s(&encSt->a_GainHistory[1], &encSt->a_GainHistory[0], PG_NUM_FRAME-1);
      encSt->a_GainHistory[PG_NUM_FRAME-1] = (Ipp16s)(gain_pit >> 3);

      if (usedRate != IPP_SPCHBR_4750)  {
         ownSubframePostProc_GSMAMR(encSt->pSpeechPtr, usedRate, i_subfr, gain_pit,
                                    gain_code, Aq, pSynthVec, pPitchSrch, pFixCodebookExc,
                                    pFltAdaptExc, pFltFixExc, encSt->a_MemorySyn,
                                    encSt->a_MemoryErr, encSt->a_Memory_W0, encSt->pExcVec,
                                    &encSt->vFlagSharp);
      } else {

         if (evenSubfr != 0) {
            i_subfr_sf0 = i_subfr;
            ippsCopy_16s(pPitchSrch, pPitchSrchM475, SUBFR_SIZE_GSMAMR);
            ippsCopy_16s(pFltFixExc, pFltCodebookM475, SUBFR_SIZE_GSMAMR);
            ippsCopy_16s(pFixCodebookExc, pFixCodebookExcM475, SUBFR_SIZE_GSMAMR);
            T0_sf0 = T0;
            T0_frac_sf0 = T0_frac;

            ownSubframePostProc_GSMAMR(encSt->pSpeechPtr, usedRate, i_subfr, gain_pit,
                                       gain_code, Aq, pSynthVec, pPitchSrch, pFixCodebookExc,
                                       pFltAdaptExc, pFltFixExc, pMemSynSave, encSt->a_MemoryErr,
                                       pMemW0Save, encSt->pExcVec, &encSt->vFlagSharp);
            encSt->vFlagSharp = sharp_save;
         } else {

            ippsCopy_16s(pMemErrSave, encSt->a_MemoryErr, LP_ORDER_SIZE);
            ownPredExcMode3_6_GSMAMR(&encSt->pExcVec[i_subfr_sf0], T0_sf0, T0_frac_sf0,
                         SUBFR_SIZE_GSMAMR, 1);

			ippsConvPartial_16s_Sfs_amr_sat(&encSt->pExcVec[i_subfr_sf0], pImpResM475,  pFltAdaptExc, SUBFR_SIZE_GSMAMR, 12);

            Aq -= LP1_ORDER_SIZE;
            ownSubframePostProc_GSMAMR(encSt->pSpeechPtr, usedRate, i_subfr_sf0, gain_pit_sf0, gain_code_sf0,
                                       Aq, pSynthVec, pPitchSrchM475, pFixCodebookExcM475, pFltAdaptExc,
                                       pFltCodebookM475, encSt->a_MemorySyn, encSt->a_MemoryErr, encSt->a_Memory_W0,
                                       encSt->pExcVec, &sharp_save);
            Aq += LP1_ORDER_SIZE;
            ippsImpulseResponseTarget_GSMAMR_16s(&encSt->pSpeechPtr[i_subfr-LP_ORDER_SIZE],pSrcWgtLpc1,pSrcWgtLpc2,
            Aq,encSt->a_MemoryErr,encSt->a_Memory_W0,encSt->pImpResVec, pLPCPredRes, pPitchSrch);

            ownPredExcMode3_6_GSMAMR(&encSt->pExcVec[i_subfr], T0, T0_frac, SUBFR_SIZE_GSMAMR, 1);

            ippsConvPartial_16s_Sfs_amr_sat(&encSt->pExcVec[i_subfr], encSt->pImpResVec,  pFltAdaptExc, SUBFR_SIZE_GSMAMR, 12);

            ownSubframePostProc_GSMAMR(encSt->pSpeechPtr, usedRate, i_subfr, gain_pit, gain_code, Aq, pSynthVec,
                                       pPitchSrch, pFixCodebookExc, pFltAdaptExc, pFltFixExc, encSt->a_MemorySyn,
                                       encSt->a_MemoryErr, encSt->a_Memory_W0, encSt->pExcVec, &encSt->vFlagSharp);
         }
      }

      A += LP1_ORDER_SIZE;
      Aq += LP1_ORDER_SIZE;
   }

   ippsCopy_16s(&encSt->a_ExcVecOld[FRAME_SIZE_GSMAMR], &encSt->a_ExcVecOld[0], PITCH_MAX_LAG + FLT_INTER_SIZE);
   ippsCopy_16s(&encSt->a_SpeechVecOld[FRAME_SIZE_GSMAMR], &encSt->a_SpeechVecOld[0], (SPEECH_BUF_SIZE - FRAME_SIZE_GSMAMR));

   return 0;
   /* End of ownEncode_GSMAMR() */
}

