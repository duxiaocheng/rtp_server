/*****************************************************************************************/
/**
@brief Byte order independent definitions for bitfields

Usage example

struct data_structure {
  BITFIELDS(
    bf16 a : 3,
    bf16 b : 5,
    bf16 c : 2,
    bf16 d : 6
  )
};
*/

/*****************************************************************************************/
/****    INCLUDE GUARD    ****************************************************************/

#ifndef BITFIELDS_H
#define BITFIELDS_H

/*****************************************************************************************/
/****    1 INCLUDE FILES    **************************************************************/

/*****************************************************************************************/
/****    2 CONSTANTS    ******************************************************************/

/*****************************************************************************************/
/****    3 MACROS    *********************************************************************/

#define NUM_ARGS_COUNTER(_1, _2, _3, _4, _5, _6, _7, _8, _9, \
                         _10, _11, _12, _13, _14, _15_, _16, _17, _18, _19, \
                         _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, \
                         _30, _31, _32, N, ...) N

#define NUM_ARGS(...) NUM_ARGS_COUNTER(__VA_ARGS__, \
                                       32, 31, 30, \
                                       29, 28, 27, 26, 25, 24, 23, 22, 21, 20, \
                                       19, 18, 17, 16, 15, 14, 13, 12, 11, 10, \
                                       9, 8, 7, 6, 5, 4, 3, 2, 1)

#ifdef ARCH_BIG_ENDIAN
#define ORDER_ARGS(a, b) a; b
#elif defined ARCH_LITTLE_ENDIAN
#define ORDER_ARGS(a, b) b a;
#else
#error Neither ARCH_BIG_ENDIAN nor ARCH_LITTLE_ENDIAN defined
#endif

#define BITFIELDS_1(bf0) bf0;
#define BITFIELDS_2(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_1(__VA_ARGS__))
#define BITFIELDS_3(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_2(__VA_ARGS__))
#define BITFIELDS_4(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_3(__VA_ARGS__))
#define BITFIELDS_5(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_4(__VA_ARGS__))
#define BITFIELDS_6(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_5(__VA_ARGS__))
#define BITFIELDS_7(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_6(__VA_ARGS__))
#define BITFIELDS_8(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_7(__VA_ARGS__))
#define BITFIELDS_9(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_8(__VA_ARGS__))
#define BITFIELDS_10(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_9(__VA_ARGS__))
#define BITFIELDS_11(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_10(__VA_ARGS__))
#define BITFIELDS_12(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_11(__VA_ARGS__))
#define BITFIELDS_13(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_12(__VA_ARGS__))
#define BITFIELDS_14(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_13(__VA_ARGS__))
#define BITFIELDS_15(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_14(__VA_ARGS__))
#define BITFIELDS_16(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_15(__VA_ARGS__))
#define BITFIELDS_17(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_16(__VA_ARGS__))
#define BITFIELDS_18(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_17(__VA_ARGS__))
#define BITFIELDS_19(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_18(__VA_ARGS__))
#define BITFIELDS_20(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_19(__VA_ARGS__))
#define BITFIELDS_21(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_20(__VA_ARGS__))
#define BITFIELDS_22(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_21(__VA_ARGS__))
#define BITFIELDS_23(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_22(__VA_ARGS__))
#define BITFIELDS_24(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_23(__VA_ARGS__))
#define BITFIELDS_25(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_24(__VA_ARGS__))
#define BITFIELDS_26(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_25(__VA_ARGS__))
#define BITFIELDS_27(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_26(__VA_ARGS__))
#define BITFIELDS_28(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_27(__VA_ARGS__))
#define BITFIELDS_29(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_28(__VA_ARGS__))
#define BITFIELDS_30(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_29(__VA_ARGS__))
#define BITFIELDS_31(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_30(__VA_ARGS__))
#define BITFIELDS_32(bf0, ...) ORDER_ARGS(bf0, BITFIELDS_31(__VA_ARGS__))

#define BITFIELDS_IMPL(count, ...) BITFIELDS_##count(__VA_ARGS__)
#define BITFIELDS_TMP(count, ...) BITFIELDS_IMPL(count, __VA_ARGS__)
#define BITFIELDS(...) BITFIELDS_TMP(NUM_ARGS(__VA_ARGS__), __VA_ARGS__)

/*****************************************************************************************/
/****    4 DATA TYPES    *****************************************************************/

/*****************************************************************************************/
/****    5 FUNCTION PROTOTYPES    ********************************************************/

/*****************************************************************************************/
#endif /* BITFIELDS_H */


