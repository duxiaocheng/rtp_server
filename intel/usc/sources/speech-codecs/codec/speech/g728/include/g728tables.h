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
// Purpose: G.728 speech codec: tables.
//
*/

#ifndef __G728TABLES_H__
#define __G728TABLES_H__

extern CONST Ipp16s wnrw[64];
extern CONST Ipp16s wnrlg[40];
extern CONST Ipp16s wnr[112];
extern CONST Ipp16s cnstSynthesisFilterBroadenVector[56];/*facv*/
extern CONST Ipp16s cnstGainPreditorBroadenVector[16]; /*facgpv*/
extern CONST Ipp16s cnstPoleCtrlFactor[16]; /*wpcfv*/
extern CONST Ipp16s cnstZeroCtrlFactor[16]; /*wzcfv*/
extern CONST Ipp16s cnstSTPostFltrPoleVector[16];/*spfpcfv*/
extern CONST Ipp16s cnstSTPostFltrZeroVector[16];/*spfzcfv*/
extern CONST Ipp16s gq[8];
extern CONST Ipp16s cnstShapeCodebookVectors[128];
extern CONST Ipp16s shape_all_norm[128*5];
extern CONST Ipp16s shape_all_nls[128];
extern CONST Ipp16s nngq[8];
extern CONST Ipp16s cnstCodebookVectorsGain[8];/*gcb*/

/* Rate 12.8 spicific*/

extern CONST Ipp16s    gq_128[4];
extern CONST Ipp16s  nngq_128[4];
extern CONST Ipp16s cnstCodebookVectorsGain_128[4];/*gcblg_128*/

/* Rate 9.6 specific*/

extern CONST Ipp16s gq_96[4];
extern CONST Ipp16s nngq_96[4];
extern CONST Ipp16s cnstCodebookVectorsGain_96[4];/*gcblg_96*/

extern CONST Ipp16s cnstSynthesisFilterBroadenVectorFE[56];/*facvfe*/

#endif /* __G728TABLES_H__ */

