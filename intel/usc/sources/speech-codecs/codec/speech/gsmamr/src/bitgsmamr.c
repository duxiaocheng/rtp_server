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
// Purpose: GSMAMR speech codec: parameters to/from bitstream pack/unpack functions.
//
*/

#include "owngsmamr.h"
static Ipp8s* ownPar2Ser_GSMAMR( Ipp32s Inp, Ipp8s *Pnt, Ipp32s BitNum );
static Ipp32s  ownExtractBits_GSMAMR( const Ipp8u **pBits, Ipp32s *nBit, Ipp32s Count );

void ownPrm2Bits_GSMAMR(const Ipp16s* prm, Ipp8u *Vout, GSMAMR_Rate_t rate )
{
    Ipp32s  i ;
    Ipp8s *Bsp;
    Ipp32s  Temp ;
    Ipp32s BitCount = 0;
    IPP_ALIGNED_ARRAY(16, Ipp8s, BitStream, MAX_SERIAL_SIZE); /* max number of bits (rate 12.2 )*/

    Bsp = BitStream ;

    for( i = 0; i < TableParamPerModes[rate]; i++) {
       BitCount += TableBitAllModes[rate][i];
       Temp = prm[i];
       Bsp = ownPar2Ser_GSMAMR( Temp, Bsp, TableBitAllModes[rate][i] ) ;
    }

    /* Clear the output vector */
    ippsZero_8u((Ipp8u*)Vout, (BitCount+7)/8);
    for ( i = 0 ; i < BitCount ; i ++ )
        Vout[i>>3] = (Ipp8u)(Vout[i>>3] ^ (BitStream[i] << (~i & 0x7)));

    return;
}

static Ipp8s* ownPar2Ser_GSMAMR( Ipp32s Inp, Ipp8s *Pnt, Ipp32s BitNum )
{
    Ipp32s i   ;
    Ipp16s Temp ;
    Ipp8s *TempPnt = Pnt + BitNum -1;

    for ( i = 0 ; i < BitNum ; i ++ ) {
        Temp = (Ipp16s) (Inp & 0x1);
        Inp >>= 1;
        *TempPnt-- = (Ipp8s)Temp;
    }

    return (Pnt + BitNum) ;
    /* End of ownPar2Ser_GSMAMR() */
}

void  ownBits2Prm_GSMAMR( const Ipp8u *bitstream, Ipp16s* prm , GSMAMR_Rate_t rate )
{
    Ipp32s   i  ;
    const Ipp8u **pBits = &bitstream;
    Ipp32s nBit = 0;

    for( i = 0; i < TableParamPerModes[rate]; i++) {
       prm[i] = (Ipp16s)ownExtractBits_GSMAMR( pBits, &nBit, TableBitAllModes[rate][i] ) ;
    }
    /* End of ownBits2Prm_GSMAMR() */
}


static Ipp32s  ownExtractBits_GSMAMR( const Ipp8u **pBits, Ipp32s *nBit, Ipp32s Count )
{
    Ipp32s  i ;
    Ipp32s  ResSum = 0 ;

    for ( i = 0 ; i < Count ; i ++ ){
        Ipp32s  ExtVal, idx ;
        idx = i + *nBit;
        ExtVal = ((*pBits)[idx>>3] >> (~idx & 0x7)) & 1;
        ResSum +=  ExtVal << (Count-i-1) ;
    }

    *pBits += (Count + *nBit)>>3;
    *nBit = (Count + *nBit) & 0x7;

    return ResSum ;
    /* End of ownExtractBits_GSMAMR() */
}

