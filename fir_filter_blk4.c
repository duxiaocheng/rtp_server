/*****************************************************************************************/
/**
@brief      FIR filter blk4 C-implementation sourcefile
*/

/*****************************************************************************************/
/****    1 INCLUDE FILES    **************************************************************/

#include "fir_filter_blk4.h"

#include <limits.h> // for SHRT_MAX
#include <xmmintrin.h> // for __m64

/*****************************************************************************************/
/****    2 LOCAL CONSTANTS    ************************************************************/

/*****************************************************************************************/
/****    3 LOCAL MACROS    ***************************************************************/

#define _sat_m(x) (x)

/*****************************************************************************************/
/****    4 LOCAL DATA TYPES    ***********************************************************/

/*****************************************************************************************/
/****    5 LOCAL FUNCTION PROTOTYPES    **************************************************/

MGW_LOCAL int16
saturate16(int32 x);

/*****************************************************************************************/
/****    6  FUNCTIONS    *****************************************************************/

/**
@brief      FIR filter blk4 implementation

The sixth input parameter must be maximum 14 and positive so that the shift does not get
negative.

@param      out
@param      dLine64
@param      nNew
@param      nCoeffs
@param      coeffs64
@param      scale, must be 14 at maximum
@return     None
*/
#if 0
MGW_GLOBAL void
fir_blk4(int16 *out, int64 *dLine64, int32 nNew,
         int32 nCoeffs, int64 *coeffs64, int32 scale)
{
  int16 *dLine = (int16 *) dLine64;
  int16 *coeffs = (int16 *) coeffs64;
  uint16 i = 1;
  uint16 j = 1;
  int40 Y[2] = { 0, 0 };

  int16 dinxa = 0;
  int16 cinxa = 0;
  int16 dinxb = 0;
  int16 cinxb = 0;

  int40 Y00a = 0;
  int40 Y10a = 0;
  int40 Y00b = 0;
  int40 Y10b = 0;

  int40 Y01a = 0;
  int40 Y11a = 0;
  int40 Y01b = 0;
  int40 Y11b = 0;

  for (i = 1; i <= (nNew / 2); i++)
  {
    /* Zero 2 output accumulators */
    Y[0] = 0;
    Y[1] = 0;

    /* Compute next 2 outputs */
    for (j = 1; j <= (nCoeffs / 8); j++)
    {
      dinxa = 2 * (i - 1) + 4 * (j - 1) + 0 - 2;
      cinxa = 4 * (j - 1) + 0;
      dinxb = dinxa + (nCoeffs / 2);
      cinxb = cinxa + (nCoeffs / 2);

      Y00a = _sat_m((int32) dLine[dinxa + 2 + 0] * coeffs[cinxa + 0] +
             (int32) dLine[dinxa + 2 + 1] * coeffs[cinxa + 1]);
      Y10a = _sat_m((int32) dLine[dinxa + 2 + 1] * coeffs[cinxa + 0] +
             (int32) dLine[dinxa + 2 + 2] * coeffs[cinxa + 1]);
      Y00b = _sat_m((int32) dLine[dinxb + 2 + 0] * coeffs[cinxb + 0] +
             (int32) dLine[dinxb + 2 + 1] * coeffs[cinxb + 1]);
      Y10b = _sat_m((int32) dLine[dinxb + 2 + 1] * coeffs[cinxb + 0] +
             (int32) dLine[dinxb + 2 + 2] * coeffs[cinxb + 1]);

      Y01a = _sat_m((int32) dLine[dinxa + 2 + 2] * coeffs[cinxa + 2] +
             (int32) dLine[dinxa + 2 + 3] * coeffs[cinxa + 3]);
      Y11a = _sat_m((int32) dLine[dinxa + 2 + 3] * coeffs[cinxa + 2] +
             (int32) dLine[dinxa + 2 + 4] * coeffs[cinxa + 3]);
      Y01b = _sat_m((int32) dLine[dinxb + 2 + 2] * coeffs[cinxb + 2] +
             (int32) dLine[dinxb + 2 + 3] * coeffs[cinxb + 3]);
      Y11b = _sat_m((int32) dLine[dinxb + 2 + 3] * coeffs[cinxb + 2] +
             (int32) dLine[dinxb + 2 + 4] * coeffs[cinxb + 3]);

      Y01a += Y00a;
      Y11b += Y10b;
      Y11a += Y10a;
      Y01b += Y00b;

      Y[0] += Y01a;
      Y[1] += Y11b;

      Y[0] += Y01b;
      Y[1] += Y11a;
    }

    /*lint -save -e647 "Suspicious truncation", scale must be limited between [0, 14]*/
    Y[0] += 1 << (14 - scale); /* Beware negative shift! */
    Y[0] >>= 15 - scale; /* Beware negative shift! */

    Y[1] += 1 << (14 - scale); /* Beware negative shift! */
    Y[1] >>= 15 - scale; /* Beware negative shift! */
    /*lint -restore*/

    out[(2 * i) - 2] = saturate16((int32) Y[0]);
    out[(2 * i) - 1] = saturate16((int32) Y[1]);
  }
}
#endif
MGW_GLOBAL void
fir_blk4(int16 *outVec, int16 *dLine, int16 nNew, int16 nCoeffs,
         const int16 *coeffs, int16 scale)
{
  __m64 c10, c11, c20, c30, c31, c40 ;
  __m64 res1, res2, res3, res4 ;

  uint16 i = 1;
  uint16 j = 1;
  int40 Y[2] = { 0, 0 };

  int16 dinxa = 0;
  int16 cinxa = 0;
  int16 dinxb = 0;
  int16 cinxb = 0;

#if 0
  int40 Y00a = 0;
  int40 Y10a = 0;
  int40 Y00b = 0;
  int40 Y10b = 0;

  int40 Y01a = 0;
  int40 Y11a = 0;
  int40 Y01b = 0;
  int40 Y11b = 0;
#endif

  for (i = 1; i <= (nNew / 2); i++)
  {
    /* Zero 2 output accumulators */
    Y[0] = 0;
    Y[1] = 0;

    dinxa = 2 * (i - 1) + 4 * (1 - 1) + 0 - 2;
    cinxa = 4 * (1 - 1) + 0;
    dinxb = dinxa + (nCoeffs / 2);
    cinxb = cinxa + (nCoeffs / 2);

    /* Compute next 2 outputs */
    for (j = 1; j <= (nCoeffs / 8); j++)
    {

      //c1 =  __builtin_ia32_loadups128(dLine + dinxa + 2 + 0);
      //c2 =  __builtin_ia32_loadups128(coeffs + cinxa + 0);
      c10 =  *(((__m64 *)(&(dLine[dinxa + 2 + 0]))+j-1));
      c11 =  *(((__m64 *)(&(coeffs[cinxa + 0]))+j-1));
      c20 =  *(((__m64 *)(&(dLine[dinxa + 2 + 1]))+j-1));
      c30 =  *(((__m64 *)(&(dLine[dinxb + 2 + 0]))+j-1));
      c31 =  *(((__m64 *)(&(coeffs[cinxb + 0]))+j-1));
      c40 =  *(((__m64 *)(&(dLine[dinxb + 2 + 1]))+j-1));

      res1 = _mm_madd_pi16(c10, c11);

      res2 = _mm_madd_pi16(c20, c11);

      res3 = _mm_madd_pi16(c30, c31);

      res4 = _mm_madd_pi16(c40, c31);

#if 0
      Y00a = _sat_m((int32) dLine[dinxa + 2 + 0] * coeffs[cinxa + 0] +
             (int32) dLine[dinxa + 2 + 1] * coeffs[cinxa + 1]);
      Y10a = _sat_m((int32) dLine[dinxa + 2 + 1] * coeffs[cinxa + 0] +
             (int32) dLine[dinxa + 2 + 2] * coeffs[cinxa + 1]);
      Y00b = _sat_m((int32) dLine[dinxb + 2 + 0] * coeffs[cinxb + 0] +
             (int32) dLine[dinxb + 2 + 1] * coeffs[cinxb + 1]);
      Y10b = _sat_m((int32) dLine[dinxb + 2 + 1] * coeffs[cinxb + 0] +
             (int32) dLine[dinxb + 2 + 2] * coeffs[cinxb + 1]);

      Y01a = _sat_m((int32) dLine[dinxa + 2 + 2] * coeffs[cinxa + 2] +
             (int32) dLine[dinxa + 2 + 3] * coeffs[cinxa + 3]);
      Y11a = _sat_m((int32) dLine[dinxa + 2 + 3] * coeffs[cinxa + 2] +
             (int32) dLine[dinxa + 2 + 4] * coeffs[cinxa + 3]);
      Y01b = _sat_m((int32) dLine[dinxb + 2 + 2] * coeffs[cinxb + 2] +
             (int32) dLine[dinxb + 2 + 3] * coeffs[cinxb + 3]);
      Y11b = _sat_m((int32) dLine[dinxb + 2 + 3] * coeffs[cinxb + 2] +
             (int32) dLine[dinxb + 2 + 4] * coeffs[cinxb + 3]);

      Y01a += Y00a;
      Y11b += Y10b;
      Y11a += Y10a;
      Y01b += Y00b;

      Y[0] += Y01a;
      Y[1] += Y11b;

      Y[0] += Y01b;
      Y[1] += Y11a;
#endif

      res1 = _mm_add_pi32(res1, res3);
      res4 = _mm_add_pi32(res2, res4);
      Y[0] += _mm_cvtsi64_si32(res1) + _mm_cvtsi64_si32(_mm_srai_pi32(res1, 32));
      Y[1] += _mm_cvtsi64_si32(res4) + _mm_cvtsi64_si32(_mm_srai_pi32(res4, 32));

#if 0
      Y[0] += _mm_cvtsi64_si32(res3) + _mm_cvtsi64_si32(_mm_srai_pi32(res3, 32));
      Y[1] += _mm_cvtsi64_si32(res2) + _mm_cvtsi64_si32(_mm_srai_pi32(res2, 32));
#endif
    }

    /*lint -save -e647 "Suspicious truncation", scale must be limited between [0, 14]*/
    Y[0] += 1 << (14 - scale); /* Beware negative shift! */
    Y[0] >>= 15 - scale; /* Beware negative shift! */

    Y[1] += 1 << (14 - scale); /* Beware negative shift! */
    Y[1] >>= 15 - scale; /* Beware negative shift! */
    /*lint -restore*/

    outVec[(2 * i) - 2] = saturate16((int32) Y[0]);
    outVec[(2 * i) - 1] = saturate16((int32) Y[1]);
  }
}

/**
@brief      Saturate 32 bit to 16 bit

@param      x, 32 bit integer to be saturated
@return     saturated 16 bit integer
*/
MGW_LOCAL int16
saturate16(int32 x)
{
  if (x > SHRT_MAX) {
    return SHRT_MAX;
  } else if (x < SHRT_MIN) {
    return SHRT_MIN;
  }

  return (int16) x;
}

