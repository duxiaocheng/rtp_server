/*/////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2011 Intel Corporation. All Rights Reserved.
//
//     Intel(R) Integrated Performance Primitives
//     USC - Unified Speech Codec interface library
//
//  Purpose: Dual Tones Multi Friquency: USC functions.
//
//  Matrix formation of a voice-frequency set:
//
//   fl/fh Hz 1209  1336  1477  1633
//   697        1    2     3      A
//   770        4    5     6      B
//   852        7    8     9      C
//   941        *    0     #      D

*/
#if defined( _WIN32_WCE)
#pragma warning( disable : 4505 )
#endif

#include <math.h>
#include "usc_td.h"
#include <ipps.h>
#include <ippsc.h>


static USC_Status GetInfoSize(int *pSize);
static USC_Status GetInfo(USC_Handle handle, USC_TD_Info *pInfo);
static USC_Status NumAlloc(const USC_TD_Option *options, int *nbanks);
static USC_Status MemAlloc(const USC_TD_Option *options, USC_MemBank *pBanks);
static USC_Status Init(const USC_TD_Option *options, const USC_MemBank *pBanks, USC_Handle *handle);
static USC_Status Reinit(const USC_TD_Modes *modes, USC_Handle handle );
static USC_Status Control(const USC_TD_Modes *modes, USC_Handle handle );

static USC_Status DetectTone(USC_Handle handle, USC_PCMStream *pIn, USC_ToneID *pDetectedToneID);
static USC_Status GenerateTone(USC_Handle handle, USC_ToneID ToneID, int volume, int durationMS, USC_PCMStream *pOut);

#define FRAME_SIZE                      77      //by default
#define DTMF_SHIFT_FRAME_SIZE           205
#define DTMF_NUM_TONE_IDS               16
#define MAX_FRAME_SIZE                  600     //80

typedef struct{
    int hlen;
    int flgFirst;
    int num_det;
    int dtmf_fs;
    Ipp16s buf[MAX_FRAME_SIZE];
    Ipp16s dfreq;
} DTMFState_16s;

static USC_ToneID DetCandidate(Ipp32s freq_1, Ipp32s freq_2);
static int DTMF_16s(const Ipp16s *pBuffer, int nbytes, USC_ToneID *toneRes, DTMFState_16s *pIppTDParams);

#define dbellTocount(x) pow(10., x/10.);

typedef struct {
    int bitPerSample;
    int sample_frequency;
    int reserved3;
    int reserved4;
} DTMF_Handle_Header;

/* global usc vector table */
USCFUN USC_TD_Fxns USC_DTMF_Fxns=
{
    {
        USC_TD,
        (GetInfoSize_func) GetInfoSize,
        (GetInfo_func)     GetInfo,
        (NumAlloc_func)    NumAlloc,
        (MemAlloc_func)    MemAlloc,
        (Init_func)        Init,
        (Reinit_func)      Reinit,
        (Control_func)     Control
    },
    DetectTone,
    GenerateTone
};

static const USC_ToneID pToneIdsTbl_DTMF[DTMF_NUM_TONE_IDS]={
    USC_Tone_1, USC_Tone_2, USC_Tone_3, USC_Tone_A,
    USC_Tone_4, USC_Tone_5, USC_Tone_6, USC_Tone_B,
    USC_Tone_7, USC_Tone_8, USC_Tone_9, USC_Tone_C,
    USC_Tone_ASTERISK, USC_Tone_0, USC_Tone_HASH, USC_Tone_D
};
static const Ipp32s pFreqTbl_DTMF[2][4] = {
   {697, 770, 852, 941},
   {1209, 1336, 1477, 1633}
};
static USC_Status GetInfoSize(int *pSize)
{
   USC_CHECK_PTR(pSize);

   *pSize = sizeof(USC_TD_Info) + sizeof(USC_TD_Option);
   return USC_NoError;
}

static USC_Status GetInfo(USC_Handle handle, USC_TD_Info *pInfo)
{
   USC_CHECK_PTR(pInfo);
   pInfo->name = "IPP_TDDTMF";

   if(handle==NULL) {
      pInfo->nOptions = 2;
      if (pInfo->params[0].framesize == 0)
          pInfo->params[0].framesize = 2*FRAME_SIZE;   //default
      else
          pInfo->params[0].framesize *= 2;
      pInfo->params[0].minframesize = pInfo->params[0].framesize;
      pInfo->params[0].maxframesize = pInfo->params[0].framesize;
      pInfo->params[0].pcmType.bitPerSample = 16;
      pInfo->params[0].pcmType.sample_frequency = 8000;
      pInfo->params[0].pcmType.nChannels = 1;
      pInfo->params[0].modes.reserved1 = 0;
      pInfo->params[0].modes.reserved2 = 0;
      pInfo->params[0].modes.reserved3 = 0;
      pInfo->params[0].modes.reserved4 = 0;

      if (pInfo->params[1].framesize == 0)
          pInfo->params[1].framesize = 2*FRAME_SIZE;   //default
      else
          pInfo->params[1].framesize *= 2;

      pInfo->params[1].minframesize = pInfo->params[0].framesize;
      pInfo->params[1].maxframesize = pInfo->params[0].framesize;

      pInfo->params[1].pcmType.bitPerSample = 16;
      pInfo->params[1].pcmType.sample_frequency = 16000;
      pInfo->params[1].modes.reserved1 = 0;
      pInfo->params[1].modes.reserved2 = 0;
      pInfo->params[1].modes.reserved3 = 0;
      pInfo->params[1].modes.reserved4 = 0;
   } else {
      DTMF_Handle_Header *DTMF_header;
      DTMF_header = (DTMF_Handle_Header*)handle;

      pInfo->nOptions = 1;
      if (pInfo->params[0].framesize == 0)
          pInfo->params[0].framesize = 2*FRAME_SIZE;

      pInfo->params[0].minframesize = pInfo->params[0].framesize;
      pInfo->params[0].maxframesize = pInfo->params[0].framesize;

      pInfo->params[0].pcmType.bitPerSample = DTMF_header->bitPerSample;
      pInfo->params[0].pcmType.sample_frequency = DTMF_header->sample_frequency;
      pInfo->params[0].pcmType.nChannels = 1;
      pInfo->params[0].modes.reserved1 = 0;
      pInfo->params[0].modes.reserved2 = 0;
      pInfo->params[0].modes.reserved3 = 0;
      pInfo->params[0].modes.reserved4 = 0;
   }

   pInfo->nToneIDs = DTMF_NUM_TONE_IDS;
   pInfo->pToneIDsTbl = (const USC_ToneID*)pToneIdsTbl_DTMF;

   return USC_NoError;
}

static IppPCMFrequency USC2IPP_Freq(int frequency)
{
   if(frequency==8000) return IPP_PCM_FREQ_8000;
   return IPP_PCM_FREQ_16000;
}
static USC_Status NumAlloc(const USC_TD_Option *options, int *nbanks)
{
   USC_CHECK_PTR(options);
   USC_CHECK_PTR(nbanks);
   *nbanks = 1;
   return USC_NoError;
}

static USC_Status MemAlloc(const USC_TD_Option *options, USC_MemBank *pBanks)
{
   int size = 0;
   USC_CHECK_PTR(options);
   USC_CHECK_PTR(pBanks);

   pBanks->pMem = NULL;
   pBanks->align = 32;
   pBanks->memType = USC_OBJECT;
   pBanks->memSpaceType = USC_NORMAL;

   size = sizeof(DTMFState_16s) + 16;

   pBanks->nbytes = sizeof(DTMF_Handle_Header) + size;
   return USC_NoError;
}

static int DTMFInit_16s (DTMFState_16s *state, IppPCMFrequency sampleFreq)
{
    if (sampleFreq == IPP_PCM_FREQ_8000) {
        state->dfreq = 8000;
    } else {
        state->dfreq = 16000;
    }

    state->hlen = 0;
    state->num_det = 0;
    state->flgFirst = 0;
    state->dtmf_fs = 0;

    return 0;
}
static USC_Status Init(const USC_TD_Option *options, const USC_MemBank *pBanks, USC_Handle *handle)
{
   DTMF_Handle_Header *DTMF_header;
   DTMFState_16s *pIppTDParams;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(pBanks);
   USC_CHECK_PTR(pBanks->pMem);
   USC_BADARG(pBanks->nbytes<=0,USC_NotInitialized);
   USC_CHECK_HANDLE(handle);

   *handle = (USC_Handle*)pBanks->pMem;
   DTMF_header = (DTMF_Handle_Header*)*handle;

   pIppTDParams = (DTMFState_16s *)((char*)*handle + sizeof(DTMF_Handle_Header));

   DTMF_header->bitPerSample = options->pcmType.bitPerSample;
   DTMF_header->sample_frequency = options->pcmType.sample_frequency;
   DTMF_header->reserved4 = 0;

   DTMFInit_16s((DTMFState_16s *)pIppTDParams, USC2IPP_Freq(DTMF_header->sample_frequency));

   return USC_NoError;
}

static USC_Status Reinit(const USC_TD_Modes *modes, USC_Handle handle )
{
   DTMF_Handle_Header *DTMF_header;
   DTMFState_16s *pIppTDParams;

   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);

   DTMF_header = (DTMF_Handle_Header*)handle;

   pIppTDParams = (DTMFState_16s *)((char*)handle + sizeof(DTMF_Handle_Header));

   DTMFInit_16s((DTMFState_16s *)pIppTDParams, USC2IPP_Freq(DTMF_header->sample_frequency));

   DTMF_header->reserved4 = 0;

   return USC_NoError;
}

static USC_Status Control(const USC_TD_Modes *modes, USC_Handle handle )
{
   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);

   return USC_NoError;
}

static USC_ToneID DetCandidate(Ipp32s freq_l, Ipp32s freq_h){
   Ipp32s n, m;
   for (n = 0; n < 4; n++)
      for (m = 0; m < 4; m++){
         if ((pFreqTbl_DTMF[0][n] == freq_l) && (pFreqTbl_DTMF[1][m] == freq_h)){
            return pToneIdsTbl_DTMF[n*4+m];
         }
      }
   return 0;
}

static int DTMF_16s(const Ipp16s *pBuffer, int nbytes, USC_ToneID *toneRes, DTMFState_16s *pIppTDParams)
{
    USC_ToneID toneCandidate = USC_NoTone;
    Ipp32fc pSrc[MAX_FRAME_SIZE], pVal[2], pValGarm[2];
    Ipp32f pMagnitude[8], pMagnitudeGarm[8];
    Ipp32f rFreq[2];
    Ipp32s freq_l = 0, freq_h = 0;
    Ipp32s lIndex = 0, hIndex = 0;
    Ipp32s cntFreq = 0, i, j, signFlag = 1, dtmf_frame_size = 0;
    Ipp32f lowLimit, highLimit, maxLmagn = 0.0, maxHmagn = 0.0;

    Ipp16s tmpBuf[MAX_FRAME_SIZE];

    dtmf_frame_size = nbytes;
    ////if (pIppTDParams->flgFirst == 1){
    ////    for (i = DTMF_FRAME_SIZE, j = 0; i < DTMF_SHIFT_FRAME_SIZE, j < DTMF_FRAME_SIZE * 3; i++, j++)
    ////        tmpBuf[j] = pIppTDParams->buf[i];
    ////    for (i = 0, j = DTMF_FRAME_SIZE * 3; i < DTMF_FRAME_SIZE, j < DTMF_SHIFT_FRAME_SIZE; i++, j++)
    ////        tmpBuf[j] = pBuffer[i];
    ////    for (i = 0; i < DTMF_SHIFT_FRAME_SIZE; i++)
    ////        pIppTDParams->buf[i] = tmpBuf[i];
    ////    pIppTDParams->la += nbytes;
    ////}

    if (pIppTDParams->flgFirst == 1){
        for (i = dtmf_frame_size, j = 0; i < pIppTDParams->dtmf_fs, j < pIppTDParams->dtmf_fs - dtmf_frame_size; i++, j++)
            tmpBuf[j] = pIppTDParams->buf[i];
        for (i = 0, j = pIppTDParams->dtmf_fs - dtmf_frame_size; i < dtmf_frame_size, j < pIppTDParams->dtmf_fs; i++, j++)
            tmpBuf[j] = pBuffer[i];
        for (i = 0; i < pIppTDParams->dtmf_fs; i++)
            pIppTDParams->buf[i] = tmpBuf[i];
    }

    if ((pIppTDParams->flgFirst == 0) && ((pIppTDParams->dtmf_fs) <= DTMF_SHIFT_FRAME_SIZE)){
        for (i = pIppTDParams->dtmf_fs, j = 0; i < dtmf_frame_size+pIppTDParams->dtmf_fs, j < nbytes; i++, j++)
            pIppTDParams->buf[i] = pBuffer[j];
        pIppTDParams->dtmf_fs += nbytes;
    }

    if (pIppTDParams->dtmf_fs >= DTMF_SHIFT_FRAME_SIZE) {
        lowLimit = IPP_MAX_16S*(Ipp32f)dbellTocount(7.);
        highLimit = IPP_MAX_16S*(Ipp32f)dbellTocount(30.);

        for (i = 0; i < pIppTDParams->dtmf_fs; i++){
            pSrc[i].re = (Ipp32f)pIppTDParams->buf[i];
            pSrc[i].im = 0.0;
        }

        for (i = 0; i < 2; i++){
            for (j = 0; j < 4; j+=2){
                rFreq[0] = (Ipp32f)((0.5 + (Ipp32f)(pFreqTbl_DTMF[i][j]*pIppTDParams->dtmf_fs/pIppTDParams->dfreq))/pIppTDParams->dtmf_fs);
                rFreq[1] = (Ipp32f)((0.5 + (Ipp32f)(pFreqTbl_DTMF[i][j+1]*pIppTDParams->dtmf_fs/pIppTDParams->dfreq))/pIppTDParams->dtmf_fs);
                ippsGoertzTwo_32fc(pSrc, pIppTDParams->dtmf_fs, pVal, rFreq);

                rFreq[0] = (Ipp32f)((0.5 + (Ipp32f)(2*pFreqTbl_DTMF[i][j]*pIppTDParams->dtmf_fs/pIppTDParams->dfreq))/pIppTDParams->dtmf_fs);
                rFreq[1] = (Ipp32f)((0.5 + (Ipp32f)(2*pFreqTbl_DTMF[i][j+1]*pIppTDParams->dtmf_fs/pIppTDParams->dfreq))/pIppTDParams->dtmf_fs);
                ippsGoertzTwo_32fc(pSrc, pIppTDParams->dtmf_fs, pValGarm, rFreq);

                pMagnitude[cntFreq] = (Ipp32f)sqrt(pow(pVal[0].re,2) + pow(pVal[0].im,2));
                pMagnitude[cntFreq+1] = (Ipp32f)sqrt(pow(pVal[1].re,2) + pow(pVal[1].im,2));
                pMagnitudeGarm[cntFreq] = (Ipp32f)sqrt(pow(pValGarm[0].re,2) + pow(pValGarm[0].im,2));
                pMagnitudeGarm[cntFreq+1] = (Ipp32f)sqrt(pow(pValGarm[1].re,2) + pow(pValGarm[1].im,2));
                cntFreq += 2;
            }
        }

        /* Signal frequencies */
        for (i = 0; i < cntFreq; i++){
            if ((i < 4) && (pMagnitude[i] > maxLmagn)){
                maxLmagn = pMagnitude[i];
                lIndex = i;
            }
            else if ((i >= 4) && (pMagnitude[i] > maxHmagn)){
                maxHmagn = pMagnitude[i];
                hIndex = i;
            }
            if (i == 3){
                maxLmagn = 0;
                maxHmagn = 0;
            }
        }

        /* Check-up: Power level difference between frequencies and Power levels per frequency */
        if ((abs((int)(10.0*log10(pMagnitude[hIndex]/pMagnitude[lIndex]))) > 4.0) ||
            (abs((int)(10.0*log10(pMagnitude[hIndex]/pMagnitude[lIndex]))) < -8.0) ||
            (pMagnitude[hIndex] < lowLimit || pMagnitude[hIndex] > highLimit) ||
            (pMagnitude[lIndex] < lowLimit || pMagnitude[lIndex] > highLimit)){
                freq_h = 0;
        }
        /* Check-up: voice constituent*/
        else {
            for (i = 0; i < cntFreq; i++){
                if ((i >= 4) && (hIndex != i) && (pMagnitude[hIndex] < 2*pMagnitude[i]))
                    signFlag = 0;
                if ((i < 4) && (lIndex != i) && (pMagnitude[lIndex] < 2*pMagnitude[i]))
                    signFlag = 0;
            }
            if (signFlag){
                if (pMagnitude[hIndex] > 2*pMagnitudeGarm[hIndex])
                    freq_h = pFreqTbl_DTMF[1][hIndex-4];
                if (pMagnitude[lIndex] > 2*pMagnitudeGarm[lIndex])
                    freq_l = pFreqTbl_DTMF[0][lIndex];
                if (freq_l != 0 && freq_h != 0){
                    toneCandidate = DetCandidate((Ipp32s)freq_l, (Ipp32s)freq_h);
                }
            }
        }

        if (pIppTDParams->flgFirst == 0){     //for first step
            pIppTDParams->num_det = -1;
            pIppTDParams->hlen = pIppTDParams->dtmf_fs;
            pIppTDParams->flgFirst = 1;
        }
        /* Check-up: Signal reception timing */
        if (pIppTDParams->num_det == -1 && toneCandidate !=-1){
            pIppTDParams->num_det = toneCandidate;
            pIppTDParams->hlen += nbytes;
        }
        else if ((pIppTDParams->num_det != -1 && toneCandidate == -1) || (toneCandidate !=-1 && pIppTDParams->num_det != toneCandidate)){
            if(pIppTDParams->hlen >= 320){
                *toneRes = pIppTDParams->num_det;
            }
            if (toneCandidate != -1)
                pIppTDParams->hlen = nbytes;
            else
                pIppTDParams->hlen = 0;
            pIppTDParams->num_det = toneCandidate;
        }
        else{
            if (pIppTDParams->num_det != -1 && toneCandidate != -1)
                pIppTDParams->hlen += nbytes;
            pIppTDParams->num_det = toneCandidate;
        }
    }

return 0;
}

static USC_Status DetectTone(USC_Handle handle, USC_PCMStream *pIn, USC_ToneID *pDetectedToneID)
{
   DTMF_Handle_Header *DTMF_header;
   USC_ToneID toneRes = USC_NoTone;
   DTMFState_16s *pIppTDParams;

   USC_CHECK_PTR(pIn);
   USC_CHECK_PTR(pIn->pBuffer);
   USC_CHECK_PTR(pDetectedToneID);
   USC_CHECK_HANDLE(handle);

   if(pIn->nbytes <= 0) {
      pIn->nbytes = 0;
      *pDetectedToneID = USC_NoTone;
      return USC_NoError;
   }

   DTMF_header = (DTMF_Handle_Header*)handle;

   USC_BADARG(DTMF_header->bitPerSample != pIn->pcmType.bitPerSample,USC_UnsupportedPCMType);
   USC_BADARG(DTMF_header->sample_frequency != pIn->pcmType.sample_frequency,USC_UnsupportedPCMType);

   pIppTDParams = (DTMFState_16s *)((char*)handle + sizeof(DTMF_Handle_Header));

   DTMF_16s((const Ipp16s *)pIn->pBuffer, pIn->nbytes>>1, &toneRes, pIppTDParams);

   if(toneRes != USC_NoTone) {
      *pDetectedToneID = toneRes;
   } else {
      *pDetectedToneID = USC_NoTone;
   }

   return USC_NoError;
}

static USC_Status GenerateTone(USC_Handle handle, USC_ToneID ToneID, int volume, int durationMS, USC_PCMStream *pOut)
{
    DTMF_Handle_Header *DTMF_header;
    Ipp16s *tone;
    int numSamples;
    double nAmplitude=0;
    Ipp32s i, n, m;
    USC_CHECK_PTR(pOut);
    USC_CHECK_HANDLE(handle);
    USC_BADARG((volume < 1) || (volume > 63),USC_BadArgument);

    DTMF_header = (DTMF_Handle_Header*)handle;

    numSamples = durationMS*DTMF_header->sample_frequency/1000;
    nAmplitude = IPP_MAX_16S/(double)dbellTocount(volume);

    tone = (Ipp16s *)pOut->pBuffer;

    for (n = 0; n < 4; n++)
        for (m = 0; m < 4; m++){
            if (ToneID == pToneIdsTbl_DTMF[4*n+m]){
                for (i = 0; i < numSamples; i++){
                    double dToneSample;
                    dToneSample = (nAmplitude/2) * (sin((double)IPP_2PI*i*pFreqTbl_DTMF[0][n]/DTMF_header->sample_frequency) +
                        sin((double)IPP_2PI*i*pFreqTbl_DTMF[1][m]/DTMF_header->sample_frequency));
                    if (dToneSample > IPP_MAX_16S)
                        tone[i] = IPP_MAX_16S;
                    else if (dToneSample < IPP_MIN_16S)
                        tone[i] = IPP_MIN_16S;
                    else
                        tone[i] = (Ipp16s)dToneSample;
                }
                pOut->nbytes = numSamples*sizeof(short);
                return 0;
            }
        }
    return USC_NoOperation;
}
