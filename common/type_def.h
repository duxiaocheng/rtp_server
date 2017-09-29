/*****************************************************************************************/
/**
@brief standard types
*/

/*****************************************************************************************/
/****    INCLUDE GUARD    ****************************************************************/

#ifndef TYPE_DEF
#define TYPE_DEF

/*****************************************************************************************/
/****    1 INCLUDE FILES    **************************************************************/

/*****************************************************************************************/
/****    2 CONSTANTS    ******************************************************************/

#ifndef NULL
#define NULL  ((void *)0)
#endif

#ifndef FALSE
#define FALSE (0)
#endif
#ifndef TRUE
/** C99 standard defines TRUE as 1 */
#define TRUE  (1)
#endif

#define PID_NOT_SET  ((PROCESS) (-1))

#ifndef far
#define far
#endif

#define C_INLINE inline
#if defined UNIT_TEST
/** Set visibility to extern for unit test usage */
#define MGW_GLOBAL extern
/** Set visibility for unit test usage */
#define MGW_LOCAL
#else
/** Visibility (scope) attribute for global functions and variables */
#define MGW_GLOBAL
/** Visibility (scope) attribute for local functions and varibles */
#define MGW_LOCAL static
#endif

/*****************************************************************************************/
/****    3 MACROS    *********************************************************************/

#define ARRAY_SIZE( ARRAY ) (sizeof (ARRAY) / sizeof (ARRAY[0])) 

#define AU_TO_8    (2 / sizeof(uint16))

#define AU_TO_BITS (AU_TO_8 * 8)

/** Returns the size of a data type in bits */
#define sizeof_bits(x)  (sizeof(x) * AU_TO_BITS)

/** Returns size of a struct in 16 bit units. */
#define sizeof_16(str) ((sizeof(str) + 1) / 2)

/** Returns size of a struct in 8 bit units. */
#define sizeof_8(str) (sizeof(str))

/** Returns size of a struct in addressable units. This is the sizeof(). */
#define sizeof_au(str) (sizeof(str))

/** Converts a 32 bit size to addressable units. */
#define size_32_to_au(size) (4 * (size))

/** Converts a 16 bit size to addressable units. */
#define size_16_to_au(size) (2 * (size))

/** Converts a 8 bit size to addressable units. */
#define size_8_to_au(size) (size)

/** Converts an addressable unit size to 32 bit units. */
#define size_au_to_32(size) (((size) + 3) / 4)

/** Converts an addressable unit size to 16 bit units. */
#define size_au_to_16(size) (((size) + 1) / 2)

/** Converts an addressable unit size to 8 bit units. */
#define size_au_to_8(size) (size)

/** Returns the offset of a field ina struct in 16 bit units. Rounds UP if needed. */
#define offsetof_16(x,y) ((offsetof(x,y) + sizeof(uint16) - 1)/sizeof(uint16))

/** Returns the offset of a field ina struct in 8 bit units. */
#define offsetof_8(x,y) (offsetof(x,y) * AU_TO_8)

/** 16-bit memset: fills str[0...size_16] = c_16 */
#define memset_16(str, c_16, size_16) \
  size_16;\
  {short memset_16_i = 0; \
  while (memset_16_i<(short)(size_16)) { \
    (*((short *)(str) + memset_16_i) = (short)(c_16)); \
    memset_16_i++; \
  }}

/** Converts a 16 bit variable from big-endian to native */
#define P_BE_TO_NAT_16(x) (((x & 0x00FF) << 8) | ((x & 0xFF00) >> 8))

/** Converts a 16 bit variable from native to big-endian */
#define P_NAT_TO_BE_16(x) (((x & 0x00FF) << 8) | ((x & 0xFF00) >> 8))

/** Converts a 32 bit variable from big-endian to native */
#define P_BE_TO_NAT_32(x) (((x & 0xFF000000) >> 24) | ((x & 0x00FF0000) >> 8) | \
                           ((x & 0x0000FF00) << 8) | ((x & 0x000000FF) << 24))

/** Converts a 32 bit variable from native to big-endian */
#define P_NAT_TO_BE_32(x) (((x & 0xFF000000) >> 24) | ((x & 0x00FF0000) >> 8) | \
                           ((x & 0x0000FF00) << 8) | ((x & 0x000000FF) << 24))

/** Converts a 64 bit variable from big-endian to native */
#define P_BE_TO_NAT_64(x) ((((x) & 0x00000000000000FFULL) << 56) | \
                           (((x) & 0x000000000000FF00ULL) << 40) | \
                           (((x) & 0x0000000000FF0000ULL) << 24) | \
                           (((x) & 0x00000000FF000000ULL) <<  8) | \
                           (((x) & 0x000000FF00000000ULL) >>  8) | \
                           (((x) & 0x0000FF0000000000ULL) >> 24) | \
                           (((x) & 0x00FF000000000000ULL) >> 40) | \
                           (((x) & 0xFF00000000000000ULL) >> 56))

/** Converts a 64 bit variable from native to big-endian */
#define P_NAT_TO_BE_64(x) ((((x) & 0x00000000000000FFULL) << 56) | \
                           (((x) & 0x000000000000FF00ULL) << 40) | \
                           (((x) & 0x0000000000FF0000ULL) << 24) | \
                           (((x) & 0x00000000FF000000ULL) <<  8) | \
                           (((x) & 0x000000FF00000000ULL) >>  8) | \
                           (((x) & 0x0000FF0000000000ULL) >> 24) | \
                           (((x) & 0x00FF000000000000ULL) >> 40) | \
                           (((x) & 0xFF00000000000000ULL) >> 56))

/** Bitfield definition macros */
#define P_BITFIELDS_2(b1, b2) b2; b1;
#define P_BITFIELDS_3(b1, b2, b3) b3; b2; b1;
#define P_BITFIELDS_4(b1, b2, b3, b4) b4; b3; b2; b1;
#define P_BITFIELDS_5(b1, b2, b3, b4, b5) b5; b4; b3; b2; b1;
#define P_BITFIELDS_6(b1, b2, b3, b4, b5, b6) b6; b5; b4; b3; b2; b1;
#define P_BITFIELDS_7(b1, b2, b3, b4, b5, b6, b7) b7; b6; b5; b4; b3; b2; b1;
#define P_BITFIELDS_8(b1, b2, b3, b4, b5, b6, b7, b8) b8; b7; b6; b5; b4; b3; b2; b1;

/*****************************************************************************************/
/****    4 DATA TYPES    *****************************************************************/

typedef unsigned char      uint8;
typedef short              int16;
typedef unsigned short     uint16;
typedef int                int32;
typedef unsigned int       uint32;
typedef unsigned short     bits16;
typedef unsigned int       bits32;
typedef char               bool8;
typedef short              bool16;
typedef char               char_au;
typedef char               char8;
typedef short              char16;

typedef uint8              bf8;  /* Bit Field, 8 bits - nonstandard... */
typedef uint16             bf16; /* Bit Field, 16 bits - nonstandard... */
typedef uint32             bf32; /* Bit Field, 32 bits */

typedef long long          int64;
typedef unsigned long long uint64;

/*****************************************************************************************/
/****    5 FUNCTION PROTOTYPES    ********************************************************/

/*****************************************************************************************/
#endif  /* TYPE_DEF */


