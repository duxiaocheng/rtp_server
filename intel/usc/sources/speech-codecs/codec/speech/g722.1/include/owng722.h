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
// Purpose: G.722.1 speech codec: internal definitions.
//
*/

#ifndef __OWNG722_H__
#define __OWNG722_H__

#if defined( _WIN32_WCE)
#pragma warning( disable : 4505 )
#endif

#include <ipps.h>
#include <ippsc.h>
#include "g722.h"
#include "scratchmem.h"

#define   G722_CODECFUN(type,name,arg)                extern type name arg

typedef struct {
   Ipp16s *pBitstream;
   Ipp16s curWord;
   Ipp16s bitCount;
   Ipp16s curBitsNumber;
}SBitObj;


#define NUM_CATEGORIES              8
#define CAT_CONTROL_BITS            4
#define CAT_CONTROL_BITS_14kHz      5
#define CAT_CONTROL_NUM             16
#define CAT_CONTROL_NUM_14kHz       32
#define CAT_CONTROL_NUM_MAX         CAT_CONTROL_NUM_14kHz
#define REG_NUM                     14
#define REG_NUM_14kHz               28
#define REG_NUM_MAX                 REG_NUM_14kHz
#define REG_SIZE                    20
#define NUMBER_OF_VALID_COEFS       (REG_NUM * REG_SIZE)
#define NUMBER_OF_VALID_COEFS_14kHz (REG_NUM_14kHz * REG_SIZE)
#define REG_POW_TABLE_SIZE 64                              /*   */
#define REG_POW_TABLE_NUM_NEGATIVES 24                     /* C */
#define ESF_ADJUSTMENT_TO_RMS_INDEX (9-2)                  /* N */
#define REG_POW_STEPSIZE_DB 3.010299957                    /* S */
#define ABS_REG_POW_LEVELS  32                             /* T */
#define DIFF_REG_POW_LEVELS 24                             /* A */
#define DRP_DIFF_MIN -12                                   /* N */
#define DRP_DIFF_MAX 11                                    /* T */
#define MAX_NUM_BINS 16
#define MAX_VECTOR_DIMENSION 5                             /* . */

extern CONST Ipp16s cnstDiffRegionPowerBits_G722[REG_NUM_MAX][DIFF_REG_POW_LEVELS];
extern CONST Ipp16s cnstVectorDimentions_G722[NUM_CATEGORIES];
extern CONST Ipp16s cnstNumberOfVectors_G722[NUM_CATEGORIES];
extern CONST Ipp16s cnstMaxBin_G722[NUM_CATEGORIES];
extern CONST Ipp16s cnstMaxBinInverse_G722[NUM_CATEGORIES];

typedef struct {
   Ipp32s              objSize;
   Ipp32s              key;
   Ipp32s              mode;          /* mode's */
   Ipp32s              res;           /* reserved*/
}own_G722_Obj_t;

struct _G722Encoder_Obj {
   own_G722_Obj_t    *objPrm;
   Ipp16s  history[G722_FRAMESIZE_MAX];
};

struct _G722Decoder_Obj {
   own_G722_Obj_t* objPrm;
   Ipp16s prevScale;
   Ipp16s vecOldMlt[G722_DCT_LENGTH_MAX];
   Ipp16s vecOldSmpls[G722_DCT_LENGTH_MAX>>1];
   Ipp16s randVec[4];
   SBitObj bitObj;
};

Ipp16s GetFrameRegionsPowers(Ipp16s* pMlt, Ipp16s scale, Ipp16s* pDrpNumBits,
         Ipp16u *pDrpCodeBits, Ipp16s *pRegPowerIndices, Ipp32s nRegs);

void MltQquantization(Ipp16s nBitsAvailable, Ipp32s nRegs, Ipp32s nCatCtrlPossibs, Ipp16s* pMlt,
         Ipp16s *pRegPowerIndices, Ipp16s *pPowerCtgs, Ipp16s *pCtgBalances, Ipp16s *pCtgCtrl,
         Ipp32s *pRegMltBitCounts, Ipp32u *pRegMltBits);

void ExpandBitsToWords(Ipp32u *pRegMltBits,Ipp32s *pRegMltBitCounts, Ipp16s *pDrpNumBits,
         Ipp16u *pDrpCodeBits, Ipp16s *pDst, Ipp16s ctgCtrl,
         Ipp32s nRegs, Ipp32s nCatCtrlPossibs, Ipp16s bitFrameSize);

void NormalizeWndData(Ipp16s* pWndData, Ipp16s* pScale, Ipp32s dctSize);

void EncodeFrame(Ipp32s bitFrameSize, Ipp32s nRegs, Ipp16s* pMlt, Ipp16s sacle, Ipp16s* pDst);

void DecodeFrame(SBitObj* bitObj, Ipp16s* randVec, Ipp32s nRegs, Ipp16s* pMlt, Ipp16s* pScale,
         Ipp16s* pOldScale, Ipp16s *pOldMlt, Ipp16s errFlag);

Ipp16s ExpandIndexToVector(Ipp16s index, Ipp16s* pVector, Ipp16s ctgNumber);

void CategorizeFrame(Ipp16s nBitsAvailable, Ipp32s nRegs, Ipp32s nCatCtrlPossibs,
         Ipp16s *pRmsIndices, Ipp16s *pPowerCtgs, Ipp16s *pCtgBalances);

void GetPowerCategories(Ipp16s* pRmsIndices, Ipp16s nAccessibleBits, Ipp32s nRegs,
         Ipp16s *pPowerCategs, Ipp16s* pOffset);

void GetCtgPoweresBalances(Ipp16s *pPowerCtgs, Ipp16s *pCtgBalances, Ipp16s *pRmsIndices,
         Ipp16s nBitsAvailable, Ipp32s nRegs, Ipp32s nCatCtrlPossibs, Ipp16s offset);

void RecoveStdDeviations(SBitObj *bitObj, Ipp32s nRegs, Ipp16s *pStdDeviations,
         Ipp16s *pRegPowerIndices, Ipp16s *pScale);

void DecodeBitsToMlt(SBitObj* bitObj, Ipp16s* randVec, Ipp32s nRegs, Ipp16s* pStdDeviations,
         Ipp16s* pPowerCtgs, Ipp16s* pMlt);

void ArrangePowerCategories(Ipp16s ctgCtrl, Ipp16s* pPowerCtgs, Ipp16s* pCtgBalances);

void TestFrame(SBitObj* bitObj, Ipp32s nRegs, Ipp32s nCatCtrlPossibs, Ipp16s *errFlag,
         Ipp16s ctgCtrl, Ipp16s *pRegPowerIndices);

void ProcessErrors(Ipp32s dctLen, Ipp32s nValidCoeffs, Ipp16s* pErrFlag, Ipp16s* pMlt,
         Ipp16s* pOldMlt, Ipp16s* pScale, Ipp16s* pOldScale);

static Ipp16u GetNextBit(SBitObj *bitObj){
   Ipp16u temp;

   if (bitObj->bitCount == 0){
      bitObj->curWord = *bitObj->pBitstream++;
      bitObj->bitCount = 16;
   }
   bitObj->bitCount--;
   temp = bitObj->curWord >> bitObj->bitCount;
   return (temp & 1);
}

static Ipp32u GetNextBits(SBitObj* bitObj, Ipp32s n){
   int i;
   Ipp32u word=0;

   for (i=0; i<n; i++){
      word <<= 1;
      word |= GetNextBit(bitObj);
   }
   bitObj->curBitsNumber = (Ipp16s)(bitObj->curBitsNumber - n);
   return word;
}

static Ipp16s GetRand(Ipp16s *randVec){
   Ipp16s rndWord;

   rndWord = (Ipp16s) (randVec[0] + randVec[3]);
   if (rndWord < 0){
      rndWord++;
   }
   randVec[3] = randVec[2];
   randVec[2] = randVec[1];
   randVec[1] = randVec[0];
   randVec[0] = rndWord;
   return(rndWord);
}

#include "aux_fnxs.h"

#endif /* __OWNG722_H__ */
