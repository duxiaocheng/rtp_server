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
// Purpose: USC: auxiliary functions.
//
*/

#ifndef __AUX_FNXS__
#define __AUX_FNXS__

#if defined( _WIN32_WCE)
#pragma warning( disable : 4505 )
#endif

#include <ipps.h>
#include "scratchmem.h"


extern void InvSqrt_32s16s_I(Ipp32s *valFrac, Ipp16s *valExp);

/********************************************************
*      auxiliary inline functions declarations
*********************************************************/
__INLINE Ipp16s Cnvrt_32s16s(Ipp32s x){
   if (IPP_MAX_16S < x) return IPP_MAX_16S;
   else if (IPP_MIN_16S > x) return IPP_MIN_16S;
   return (Ipp16s)(x);
}

__INLINE Ipp16s Cnvrt_NR_32s16s(Ipp32s x) {
   Ipp16s s = IPP_MAX_16S;
   if(x<(Ipp32s)0x7fff8000) s = (Ipp16s)((x+0x8000)>>16);
   return s;
}

__INLINE Ipp32s Cnvrt_64s32s(__INT64 z) {
   if(IPP_MAX_32S < z) return IPP_MAX_32S;
   else if(IPP_MIN_32S > z) return IPP_MIN_32S;
   return (Ipp32s)z;
}

/* Random generator  */
__INLINE Ipp16s Rand_16s(Ipp16s *seed)
{
  *seed = (Ipp16s)(*seed * 31821 + 13849);

  return(*seed);
}

#ifdef _USC_G723 

__INLINE Ipp16s  Rand2_16s( Ipp16s *pSeed )
{
    *pSeed = (Ipp16s)(*pSeed * 521 + 259);
    return *pSeed ;
}

__INLINE Ipp16s NormRand_16s(Ipp16s N, Ipp16s *nRandom)
{
    Ipp16s sTmp;

    sTmp = (Ipp16s)(Rand2_16s(nRandom) & 0x7FFF);
    sTmp = (Ipp16s)((sTmp * N)>>15);
    return sTmp;
}
__INLINE Ipp16s Mul2_16s(Ipp16s x) {
	if (x > IPP_MAX_16S / 2) return IPP_MAX_16S;
	else if (x < IPP_MIN_16S / 2) return IPP_MIN_16S;
	x = (Ipp16s)(x << 1);
	return x;
}
#endif

__INLINE Ipp16s Abs_16s(Ipp16s x){
   if(x<0){
      if(IPP_MIN_16S == x) return IPP_MAX_16S;
      x = (Ipp16s)-x;
   }
   return x;
}
__INLINE Ipp32s Abs_32s(Ipp32s x){
   if(x<0){
      if(IPP_MIN_32S == x) return IPP_MAX_32S;
      x = -x;
   }
   return x;
}

__INLINE int Add_32s(int x, int y) {
   return Cnvrt_64s32s((__INT64)x + y);
}

extern CONST Ipp16s ExpPosNormTbl[256];
extern CONST Ipp16s ExpPosNormTbl2[256];

__INLINE Ipp16s Exp_16s_Pos(Ipp16u x)
{
   if((x>>8)==0)
      return ExpPosNormTbl2[x];
   else {
      return ExpPosNormTbl[(x>>8)];
   }
}

__INLINE Ipp16s Exp_16s(Ipp16s x){
   if (x == -1) return 15;
   if (x == 0) return 0;
   if (x < 0) x = (Ipp16s)(~x);
   return Exp_16s_Pos(x);
}

//__INLINE Ipp16s Exp_32s_Pos(Ipp32s x){
//   Ipp16s i;
//   if (x == 0) return 0;
//   for(i = 0; x < (Ipp32s)0x40000000; i++) x <<= 1;
//   return i;
//}
__INLINE short Exp_32s_Pos(unsigned int x){
   if (x == 0) return 0;
   if((x>>16)==0) return (short)(16+Exp_16s_Pos((unsigned short) x));
   return Exp_16s_Pos((unsigned short)(x>>16));
}

__INLINE Ipp16s Exp_32s(Ipp32s x){
   if (x == 0) return 0;
   if (x == -1) return 31;
   if (x < 0) x = ~x;
   return Exp_32s_Pos(x);
}

__INLINE Ipp16s Norm_32s_I(Ipp32s *x){
   Ipp16s i;
   if (*x == 0) return 0;
   if (*x < 0){
      for(i = 0; *x >= (Ipp32s)0xC0000000; i++) *x <<= 1;
   }else
      for(i = 0; *x < (Ipp32s)0x40000000; i++) *x <<= 1;
   return i;
}

#ifdef _USC_G7291
__INLINE short Norm_16s_I(short *x){
   short i;
   short y = *x;
   if (y == 0) return 0;
   if (y < 0) y = (short)~y;
   i = Exp_16s_Pos(y);
   *x <<= i;
   return i;
}
#endif

__INLINE Ipp16s Norm_32s_Pos_I(Ipp32s *x){
   Ipp16s i;
   if (*x == 0) return 0;
   for(i = 0; *x < (Ipp32s)0x40000000; i++) *x <<= 1;
   return i;
}

#ifdef FUTURE_USE
__INLINE Ipp16s MulHR_16s(Ipp16s x, Ipp16s y) {
   return (Ipp16s)( ((((Ipp32s)x * (Ipp32s)y)) + 0x4000) >> 15 );   
}
#endif

#ifdef _USC_G7291
__INLINE Ipp16s Negate_16s(Ipp16s x) {
   if(IPP_MIN_16S == x)
      return IPP_MAX_16S;
   return (Ipp16s)-x;
}
#endif
__INLINE Ipp32s Negate_32s(Ipp32s x) {
   if(IPP_MIN_32S == x)
      return IPP_MAX_32S;
   return (Ipp32s)-x;
}

__INLINE Ipp32s Mul2_32s(Ipp32s x) {
    if(x > IPP_MAX_32S/2) return IPP_MAX_32S;
    else if( x < IPP_MIN_32S/2) return IPP_MIN_32S;
    return x <<= 1;
}

__INLINE Ipp32s Mul16_32s(Ipp32s x) {
    if(x > (Ipp32s) 0x07ffffffL) return IPP_MAX_32S;
    else if( x < (Ipp32s) 0xf8000000L) return IPP_MIN_32S;
    return (x <<= 4);
}

#ifdef _USC_G723
__INLINE Ipp32s MulC_32s(Ipp16s val, Ipp32s x) {
   Ipp32s z ;
   Ipp32s xh, xl;
   xh  = x >> 16;
   xl  = x & 0xffff;
   z = 2*val*xh;
   z = Add_32s(z,(xl*val)>>15);

  return( z );
}
#endif

__INLINE Ipp32s ShiftL_32s(Ipp32s x, Ipp16u n)
{
   Ipp32s max = IPP_MAX_32S >> n;
   Ipp32s min = IPP_MIN_32S >> n;
   if(x > max) return IPP_MAX_32S;
   else if(x < min) return IPP_MIN_32S;
   return (x<<n);
}

__INLINE Ipp16s ShiftL_16s(Ipp16s n, Ipp16u x)
{
   Ipp16s max = (Ipp16s)(IPP_MAX_16S >> x);
   Ipp16s min = (Ipp16s)(IPP_MIN_16S >> x);
   if(n > max) return IPP_MAX_16S;
   else if(n < min) return IPP_MIN_16S;
   return (Ipp16s)(n<<x);
}

__INLINE Ipp16s ShiftR_NR_16s(Ipp16s x, Ipp16u n){
   return (Ipp16s)((x + (1<<(n-1)))>>n);
}

#ifdef FUTURE_USE
__INLINE Ipp16s ExtractHigh(Ipp32s x){
   return ((Ipp16s)(x >> 16));
}
#endif

__INLINE void Unpack_32s (Ipp32s number, Ipp16s *hi, Ipp16s *lo)
{
    *hi = (Ipp16s)(number >> 16);
    *lo = (Ipp16s)((number>>1)&0x7fff);	}


#ifdef FUTURE_USE
__INLINE Ipp32s Pack_16s32s(Ipp16s hi, Ipp16s lo)
{
    return ( ((Ipp32s)hi<<16) + ((Ipp32s)lo<<1) );
}
#endif

__INLINE Ipp32s Mpy_32_16 (Ipp16s hi, Ipp16s lo, Ipp16s n)
{
    return (2 * (hi * n) +  2 * ((lo * n) >> 15));
}

__INLINE Ipp16s Mul_16s_Sfs(Ipp16s x, Ipp16s y, Ipp32s scaleFactor) {
   return (Ipp16s)((x * y) >> scaleFactor);
}
__INLINE Ipp32s Mul_32s(Ipp32s x,Ipp32s y) {
   Ipp32s z,z1,z2;
   Ipp16s xh, xl, yh, yl;
   xh  = (Ipp16s)(x >> 15);
   yh  = (Ipp16s)(y >> 15);
   xl  = (Ipp16s)(x & 0x7fff);
   yl  = (Ipp16s)(y & 0x7fff);
   z1  = Mul_16s_Sfs(xh,yl,15);
   z2  = Mul_16s_Sfs(xl,yh,15);
   z   = xh * yh;
   z   += z1;
   z   += z2;
   return (z<<1);
}

#ifdef _USC_G7291
__INLINE Ipp32s Mul4_32s(Ipp32s x) {
    if(x > IPP_MAX_32S/4) return IPP_MAX_32S;
    else if( x < IPP_MIN_32S/4) return IPP_MIN_32S;
    return x <<= 2;	
}
#endif

#ifdef _USC_G7291
__INLINE Ipp32s Mul_32s16s(Ipp32s x, Ipp16s y) {
   short xh, xl;
   xh  = (short)(x >> 15);
   xl  = (short)(x & 0x7fff);
   return ((xh*y)+((xl*y)>>15));
}
#endif

__INLINE Ipp16s Add_16s(Ipp16s x, Ipp16s y) {
   return Cnvrt_32s16s((Ipp32s)x + (Ipp32s)y);
}
__INLINE Ipp16s Sub_16s(Ipp16s x, Ipp16s y) {
   return Cnvrt_32s16s((Ipp32s)x - (Ipp32s)y);
}

__INLINE Ipp32s Sub_32s(Ipp32s x,Ipp32s y){
   return Cnvrt_64s32s((__INT64)x - y);
}

#if (defined _USC_G723 || defined _USC_G729)
__INLINE Ipp32s ownSqrt_32s( Ipp32s n )
{
    Ipp32s   i  ;

    Ipp16s  x =  0 ;
    Ipp16s  y =  0x4000 ;

    Ipp32s   z ;

    for ( i = 0 ; i < 14 ; i ++ ) {
        z = (x + y) * (x + y ) ;
        if ( n >= z )
            x = (Ipp16s)(x + y);
        y >>= 1 ;
    }
    return x;
}
#endif

#ifdef FUTURE_USE

__INLINE Ipp32s Exp_64s_Pos(Ipp64u x)
{
   if (x == 0) return 0;
   if((x>>32)==0) return (Ipp32s)(32+Exp_32s_Pos((Ipp32u) x));
   return (Ipp32s)Exp_32s_Pos((Ipp32u)(x>>32));
}


__INLINE Ipp32s Norm_64s(Ipp64s x){
   Ipp32s i;
   if (x == 0) return 0;
   if (x == -1) return 63;
   if (x < 0) x = ~x;
   i = (Ipp32s)Exp_64s_Pos((Ipp64u)x);
   return i;
}

__INLINE Ipp32s Norm_32s(Ipp32s x){
   Ipp32s i;
   if (x == 0) return 0;
   if (x == -1) return 31;
   if (x < 0) x = ~x;
   i = (Ipp32s)Exp_32s_Pos((Ipp32u)x);
   return i;
}
#endif

__INLINE Ipp16s Div_16s(Ipp16s num, Ipp16s den)
{
   if ((num < den) && (num > 0) && (den > 0)) {
      return (Ipp16s)( (((Ipp32s)num)<<15) / den);
   } else if ((den != 0) && (num == den)) {
      return IPP_MAX_16S;
   }
   return 0;
}

#ifdef _USC_G7291
__INLINE short Div_32s16s(int num32, short den)
{
  return (den>0) ? (short)((num32>>1)/den) : IPP_MAX_16S; /* div: den # 0*/
}

#endif

extern CONST short LostFrame[320];



__INLINE void L_Extract_hl (int L_32, short *hi, short *lo)
{
    *hi = (short)(L_32 >> 16);
    *lo = (short)((L_32>>1)&0x7fff);
    return;
}


__INLINE short ShiftR_16s (short var1, unsigned short var2)
{
    short var_out;
    
    if (var2 >= 15)
    {
        var_out = (var1 < 0) ? -1 : 0;
    }
    else
    {
        if (var1 < 0)
        {
            var_out = ~((~var1) >> var2);
        }
        else
        {
            var_out = var1 >> var2;
        }
    }
    
    return (var_out);
}


__INLINE short Mult_16s (short var1, short var2)
{
    short var_out;
    int L_product;

    L_product = (int) var1 *(int) var2;

    L_product = (L_product & (int) 0xffff8000L) >> 15;

    if (L_product & (int) 0x00010000L)
        L_product = L_product | (int) 0xffff0000L;

    var_out = Cnvrt_32s16s(L_product);
    return (var_out);
}

__INLINE int Mult_16s32s (short var1, short var2)
{
    int L_var_out;

    L_var_out = (int) var1 *(int) var2;

    if (L_var_out != (int) 0x40000000L)
    {
        L_var_out *= 2;
    }
    else
    {
        L_var_out = IPP_MAX_32S;
    }

    return (L_var_out);
}

__INLINE short Round_32s16s (int L_var1)
{
    short var_out,var_dis;
    int L_rounded;

    L_rounded = Add_32s (L_var1, (int) 0x00008000L);
    L_Extract_hl(L_rounded, &var_out, &var_dis);
    return (var_out);
}

__INLINE int Mac_16s32s (int L_var3, short var1, short var2)
{
    int L_var_out;
    int L_product;

    L_product = Mult_16s32s (var1, var2);
    L_var_out = Add_32s(L_var3, L_product);
    return (L_var_out);
}



__INLINE int Mac_16s32s_v2 (int L_var3, short var1, short var2)
{
    int L_var_out;
    __INT64 L_product;

    {
       L_product = (__INT64)(2 * var1 * var2) + (__INT64)(L_var3);
       L_var_out = Cnvrt_64s32s(L_product);
    }
    return(L_var_out);  
}



#ifdef FUTURE_USE
__INLINE int Msu_16s32s (int L_var3, short var1, short var2)
{
    int L_var_out;
    int L_product;

    L_product = Mult_16s32s (var1, var2);
    L_var_out = Sub_32s(L_var3, L_product);

    return (L_var_out);
}
#endif

__INLINE short Mult_r_16s (short var1, short var2)
{
    short var_out;
    int L_product_arr;

    L_product_arr = (int) var1 *(int) var2;       /* product */
    L_product_arr += (int) 0x00004000L;      /* round */
    L_product_arr &= (int) 0xffff8000L;
    L_product_arr >>= 15;       /* shift */

    if (L_product_arr & (int) 0x00010000L)   /* sign extend when necessary */
    {
        L_product_arr |= (int) 0xffff0000L;
    }
    var_out = Cnvrt_32s16s(L_product_arr);

    return (var_out);
}

__INLINE int ShiftR_32s(int L_var1, unsigned short var2)
{
	int L_var_out;
	
	if (var2 >= 31)
	{
		L_var_out = (L_var1 < 0L) ? -1 : 0;
	}
	else
	{
		if (L_var1 < 0)
		{
			L_var_out = ~((~L_var1) >> var2);
		}
		else
		{
			L_var_out = L_var1 >> var2;
		}
	}
	
	return (L_var_out);
}

#ifdef FUTURE_USE
__INLINE short Shl_16s (short var1, short var2)
{
    short var_out;
    
    if (var2 < 0)
    {
        if (var2 < -16)
            var2 = -16;
        var_out = ShiftR_16s(var1, -var2);
    }
    else
    {
        var_out = ShiftL_16s(var1, var2);
    }
    return (var_out);
}
#endif
__INLINE short Shr_16s (short var1, short var2)
{
    short var_out;

    if (var2 < 0)
    {
        if (var2 < -16)
            var2 = -16;
        var_out = ShiftL_16s(var1, -var2);
    }
    else
    {
        var_out = ShiftR_16s(var1, var2);
    }
    return (var_out);
}

__INLINE int Shl_32s (int L_var1, short var2)
{
    int L_var_out;

    if (var2 <= 0)
    {
        if (var2 < -32)
            var2 = -32;
        L_var_out = ShiftR_32s(L_var1, -var2);
    }
    else
    {
        L_var_out = ShiftL_32s(L_var1, var2);
    }
    return (L_var_out);
}

__INLINE int Shr_32s (int L_var1, short var2)
{
    int L_var_out;

    if (var2 <= 0)
    {
        if (var2 < -32)
            var2 = -32;
        L_var_out = ShiftL_32s(L_var1, -var2);
    }
    else
    {
        L_var_out = ShiftR_32s(L_var1, var2);
    }
    return (L_var_out);
}

#ifdef FUTURE_USE
__INLINE short Shr_r_16s (short var1, short var2)
{
    short var_out;

    if (var2 > 15)
    {
        var_out = 0;
    }
    else
    {
        var_out = Shr_16s(var1, var2);

        if (var2 > 0)
        {
            if ((var1 & ((short) 1 << (var2 - 1))) != 0)
            {
                var_out++;
            }
        }
    }
    return (var_out);
}


__INLINE int Deposit_h (short var1)
{
    int L_var_out;

    L_var_out = (int) var1 << 16;
    return (L_var_out);
}

__INLINE int Deposit_l (short var1)
{
    int L_var_out;

    L_var_out = (int) var1;
    return (L_var_out);
}


__INLINE int Shr_r_32s (int L_var1, short var2)
{
    int L_var_out;

    if (var2 > 31)
    {
        L_var_out = 0;
    }
    else
    {
        L_var_out = Shr_32s(L_var1, var2);
        if (var2 > 0)
        {
            if ((L_var1 & ((int) 1 << (var2 - 1))) != 0)
            {
                L_var_out++;
            }
        }
    }
    return (L_var_out);
}

__INLINE short Normalization_16s (short var1)
{
    short var_out;

    if (var1 == 0)
    {
        var_out = 0;
    }
    else
    {
        if (var1 == (short) 0xffff)
        {
            var_out = 15;
        }
        else
        {
            if (var1 < 0)
            {
                var1 = ~var1;
            }
            for (var_out = 0; var1 < 0x4000; var_out++)
            {
                var1 <<= 1;
            }
        }
    }
    return (var_out);
}

__INLINE short Division_16s(short var1, short var2)
{
    short var_out = 0;
    short iteration;
    int L_num;
    int L_denom;

    
    if (var1 == 0)
    {
        var_out = 0;
    }
    else
    {
        if (var1 == var2)
        {
            var_out = IPP_MAX_16S;
        }
        else
        {
            L_num = Deposit_l (var1);
            L_denom = Deposit_l (var2);

            for (iteration = 0; iteration < 15; iteration++)
            {
                var_out <<= 1;
                L_num <<= 1;

                if (L_num >= L_denom)
                {
                    L_num = Sub_32s(L_num, L_denom);
                    var_out = Add_16s(var_out, 1);
                }
            }
        }
    }
    return (var_out);
}
#endif

__INLINE short Normalization_32s (int L_var1)
{
    short var_out;

    if (L_var1 == 0)
    {
        var_out = 0;
    }
    else
    {
        if (L_var1 == (int) 0xffffffffL)
        {
            var_out = 31;
        }
        else
        {
            if (L_var1 < 0)
            {
                L_var1 = ~L_var1;
            }
            for (var_out = 0; L_var1 < (int) 0x40000000L; var_out++)
            {
                L_var1 <<= 1;
            }
        }
    }
    return (var_out);
}

__INLINE short Extract_h_32s16s (int L_var1)
{
    short var_out;

    var_out = (short) (L_var1 >> 16);
    return (var_out);
}

__INLINE short Extract_l_32s16s (int L_var1)
{
    short var_out;

    var_out = (short) L_var1;
    return (var_out);
}


#ifdef FUTURE_USE
__INLINE void Extract_hl_32s16s (int L_32, short *hi, short *lo)
{
    *hi = Extract_h_32s16s (L_32);
    *lo = Extract_l_32s16s (Msu_16s32s (Shr_32s (L_32, 1), *hi, 16384));
    return;
}

__INLINE int Mpy_hl_32s (short hi1, short lo1, short hi2, short lo2)
{
    int L_32;

    L_32 = Mult_16s32s (hi1, hi2);
    L_32 = Mac_16s32s (L_32, Mult_16s (hi1, lo2), 1);
    L_32 = Mac_16s32s (L_32, Mult_16s (lo1, hi2), 1);

    return (L_32);
}


__INLINE int Comp_32s (short hi, short lo)
{
    int L_32;
    L_32 = Deposit_h (hi);
    return (Mac_16s32s (L_32, lo, 1));       /* = hi<<16 + lo<<1 */
}


__INLINE int Mac_32s_16s (int L_32, short hi, short lo, short n)
{
    L_32 = Mac_16s32s (L_32, hi, n);
    L_32 = Mac_16s32s (L_32, Mult_16s(lo, n), 1);

    return (L_32);
}

__INLINE int Mac_32s (int L_32, short hi1, short lo1, short hi2, short lo2)
{
    L_32 = Mac_16s32s (L_32, hi1, hi2);
    L_32 = Mac_16s32s (L_32, Mult_16s(hi1, lo2), 1);
    L_32 = Mac_16s32s (L_32, Mult_16s(lo1, hi2), 1);

    return (L_32);
}
#endif





#endif /* __AUX_FNXS__ */
