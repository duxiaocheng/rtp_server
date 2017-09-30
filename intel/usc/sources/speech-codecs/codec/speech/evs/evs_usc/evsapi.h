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
// Purpose: EVS speech codec API header.
//
*/

#ifndef __EVS_API_H__
#define __EVS_API_H__

#include <ipps.h>
#include <ippsc.h>
#include "evs_types.h"

#ifdef __cplusplus
extern "C" {
#endif

Word32 evs_get_delay_fx(					/* o  : delay value in ms                         */
		const Word16 what_delay,		/* i  : what delay? (ENC or DEC)                  */
		const Word32 io_fs				/* i  : input/output sampling frequency           */
		);

void evs_indices_to_serial_generic(
		const Indice_fx *ind_list,		/* i: indices list */
		const Word16  num_indices,      /* i: number of indices to write */
		UWord8 *pFrame,                 /* o: byte array with bit packet and byte aligned coded speech data */
		Word16 *pFrame_size             /* i/o: number of bits in the binary encoded access unit [bits] */
		);
Word16 evs_rate2EVSmode(
		Word32 rate						/* i: bit rate */
		);


#define   EVS_CODECAPI(type,name,arg)                extern type name arg;

#if(0)
/*
                   Functions declarations
*/
EVS_CODECAPI( APIEVS_Status, apiEVSEncoder_Alloc,
         (const EVSEnc_Params *amrwb_Params, Ipp32u *pCodecSize))
EVS_CODECAPI( APIEVS_Status, apiEVSDecoder_Alloc,
         (const EVSDec_Params *amrwb_Params, Ipp32u *pCodecSize))
EVS_CODECAPI( APIEVS_Status, apiEVSEncoder_Init,
         (EVSEncoder_Obj* encoderObj, Ipp32s mode))
EVS_CODECAPI( APIEVS_Status, apiEVSEncoderProceedFirst,
            (EVSEncoder_Obj* pEncoderObj, const Ipp16s* pSrc))
EVS_CODECAPI( APIEVS_Status, apiEVSDecoder_Init,
         (EVSDecoder_Obj* decoderObj))
EVS_CODECAPI( APIEVS_Status, apiEVSDecoder_GetSize,
         (EVSDecoder_Obj* decoderObj, Ipp32u *pCodecSize))
EVS_CODECAPI( APIEVS_Status, apiEVSEncoder_GetSize,
         (EVSEncoder_Obj* encoderObj, Ipp32u *pCodecSize))
EVS_CODECAPI( APIEVS_Status, apiEVSEncode,
         (EVSEncoder_Obj* encoderObj, const Ipp16u* src, Ipp8u* dst,
         EVS_Rate_t *rate, Ipp32s Vad, Ipp32s offsetWBE ))
EVS_CODECAPI( APIEVS_Status, apiEVSDecode,
         (EVSDecoder_Obj* decoderObj,const Ipp8u* src, EVS_Rate_t rate,
          RXFrameType rx_type, Ipp16u* dst, Ipp32s offsetWBE))
EVS_CODECAPI( APIEVS_Status, apiEVSEncoder_Mode,
               (EVSEncoder_Obj* encoderObj, Ipp32s mode))
#endif

#ifdef __cplusplus
}
#endif

#endif /* __EVS_API_H__ */
