/* /////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the tems of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the tems of that agreement.
//          Copyright(c) 2006-2011 Intel Corporation. All Rights Reserved.
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
//  Purpose: NLP
//
*/

#include <ipps.h>
#include <ippsc.h>
#include "math.h"
#include "nlp_api.h"
#define E_COEFF (((1<<14) * 981 / 1000))
#define AE_COEFF ((1<<14)-E_COEFF)

void initNLP(nlpState *state,int c1,int c2,int c3,int c4,int c5,int coef){
    state->si_pwr=0;
    state->sil_pwr=0;
    state->srin_pwr=state->sin_pwr2=0;
    state->suppStop=0;
    state->cnvgd=0;
    state->nAttn=0;
    state->c1=c1;
    state->c2=c2;
    state->c3=c3; 
    state->c4=c4;
    state->c5=c5;
    state->coef=coef;
    state->sstart=-1;
}
void soutProc(nlpState *state,Ipp16s *h,Ipp16s *inout,Ipp16s *rin,Ipp16s *sin,Ipp32s noisePwr){
    Ipp64s   sum,sumi,sumr;
    Ipp32s   s_out_pwr=0,i,l,s_in_pwr,s_rin_pwr,attn;
    Ipp16s  *psout,*psin,*psh,*psr;
    Ipp32f   tmp;
    for (i=0;i<(1<<(NLP_FRAME_ORDER-3));i++) {
        psr=&rin[i*(NLP_FRAME_SIZE>>1)];
        psh=&h[i*(NLP_FRAME_SIZE>>1)];
        psout=&inout[i*(NLP_FRAME_SIZE>>1)];
        psin=&sin[i*(NLP_FRAME_SIZE>>1)];
        for (sumi=sumr=sum=0,l=0;l<(NLP_FRAME_SIZE>>1);l++) {
            sum+=psh[l]*psh[l];
            sumi+=psin[l]*psin[l];
            sumr+=psr[l]*psr[l];
        }
        s_out_pwr=(Ipp32s)(sum>>(NLP_FRAME_ORDER-1));
        s_in_pwr=(Ipp32s)(sumi>>(NLP_FRAME_ORDER-1));
        s_rin_pwr=(Ipp32s)(sumr>>(NLP_FRAME_ORDER-1));
        state->si_pwr = (Ipp32s)(((Ipp64s)16302* (Ipp64s)state->si_pwr +
            (Ipp64s)82 * (Ipp64s)s_in_pwr)>>14);
        state->sil_pwr = (Ipp32s)(((Ipp64s)16302* (Ipp64s)state->sil_pwr +
            (Ipp64s)82 * (Ipp64s)s_out_pwr)>>14);
        state->srin_pwr = (Ipp32s)(((Ipp64s)E_COEFF* (Ipp64s)state->srin_pwr +
            (Ipp64s)AE_COEFF * (Ipp64s)s_rin_pwr)>>14);
        state->sin_pwr2 = (Ipp32s)(((Ipp64s)E_COEFF* (Ipp64s)state->sin_pwr2 +
            (Ipp64s)AE_COEFF * (Ipp64s)s_in_pwr)>>14);

        if (!state->cnvgd && (state->sstart!=0)){
            tmp=(float)(10*log10(((float)state->si_pwr+1)/((float)state->sil_pwr+1)));
            if (tmp>0)
            {
                if (state->sstart>0)
                {
                    --state->sstart;
                }
                state->nAttn=(float)(0.9*state->nAttn+0.1*tmp);
                if ((state->nAttn>12)||(state->sstart==0))
                {
                    state->cnvgd=1;
                    state->c1=15;
//                    state->c2=30;
                    state->c2=12;
                }
            }
        }
        if (
            ((Ipp64s)state->sin_pwr2*10 > (Ipp64s)state->srin_pwr*state->c3) &&
            ((Ipp64s)state->sin_pwr2*10>(Ipp64s)noisePwr*state->c4)
            ) {
                state->suppStop=(1<<11);
            }
        if (state->suppStop) {
            --state->suppStop;
            if(((Ipp64s)state->srin_pwr*10>(Ipp64s)state->sin_pwr2*state->c2)&&
                ((Ipp64s)state->sin_pwr2*10>(Ipp64s)noisePwr*state->c4)){
                    if (state->sstart<0)
                    {
                        state->sstart=6000;
                    }
                    attn=(Ipp32s)(((Ipp64s)state->sin_pwr2*(1<<14)+1)/(state->srin_pwr+1))
                        +state->suppStop;
                    attn*=state->cnvgd;
                    if(!attn) attn=1;
                    if(attn<(1<<14))
                        ippsMulC_NR_16s_ISfs((Ipp16s)attn,psout,NLP_FRAME_SIZE>>1,14);
                }
        }else
            if(((Ipp64s)state->srin_pwr*10>(Ipp64s)state->sin_pwr2*state->c5)&&
                ((Ipp64s)state->sin_pwr2*10>(Ipp64s)noisePwr*state->c4)){
                    attn=(Ipp32s)(((Ipp64s)state->sin_pwr2*(1<<14)+1)/(state->srin_pwr+1));
                    if(attn<(1<<14))
                        ippsMulC_NR_16s_ISfs((Ipp16s)attn,psout,NLP_FRAME_SIZE>>1,14);
                }

        else{
            if((Ipp64s)state->si_pwr*10>(Ipp64s)state->sil_pwr*state->coef){
                ippsMulC_NR_16s_ISfs(((Ipp16s)((Ipp64s)state->sil_pwr*(1<<8)+1)/(state->si_pwr+1)),
                    psout,NLP_FRAME_SIZE>>1,8);
            }
        }
    }
}
