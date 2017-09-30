/* /////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the tems of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the tems of that agreement.
//          Copyright(c) 2005-2011 Intel Corporation. All Rights Reserved.
//
//     Intel(R) Integrated Performance Primitives EC Sample
//
//  By downloading and installing this sample, you hereby agree that the
//  accompanying Materials are being provided to you under the terms and
//  conditions of the End User License Agreement for the Intel(R) Integrated
//  Performance Primitives product previously accepted by you. Please refer
//  to the file ippEULA.rtf or ippEULA.txt located in the root directory of your Intel(R) IPP
//  product installation for more information.
//
//
//  Purpose: anti-howler, tested for subband algorithm
//
*/

#include <stdio.h>
#include <stdlib.h>
#include <ipps.h>
#include <ippsc.h>
#include "ec_td.h"
/* Tone Disabler */
int toneDisabler_32f(Ipp32f *r_in, Ipp32f *s_in,Ipp16s samplingRate,int frameSize,
                _td *td)
{
    int resr, ress;
    Ipp32f lr_in_pwr = 0.f, ls_in_pwr = 0.f;
    /* Detect 2100 Hz with periodical phase reversal */
    ippsToneDetect_EC_32f(r_in, frameSize, &resr, td->tdr);
    ippsToneDetect_EC_32f(s_in, frameSize, &ress, td->tds);

    /* Update receive-in signal and send-in signal powers */
    ippsDotProd_32f(r_in,r_in,frameSize,&lr_in_pwr);
    ippsDotProd_32f(s_in,s_in,frameSize,&ls_in_pwr);

    if ((td->td_ress) || (td->td_resr)) {
        if ((lr_in_pwr < td->td_thres) &&
            (ls_in_pwr < td->td_thres))
        {
            /* Restore previous mode if 250+-100ms signal of<=36dbm0*/
            (td->td_sil_time)=(Ipp16s)((td->td_sil_time)+(frameSize/(samplingRate/1000)));
            if ((td->td_sil_time)>=td->td_sil_thresh) {
                (td->td_resr) = (td->td_ress) = 0;
                return 0;
            }
        }
        return 1;
    } else{
        (td->td_sil_time)=0;
        if (resr || ress) {
            /* Zero coeffs, disable adaptation and NLP */
            if (resr) (td->td_resr) = 1;
            if (ress) (td->td_ress) = 1;
            return 1;
        }
    }

    return 0;
}

