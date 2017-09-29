/*****************************************************************************************/
/**
@brief Function library for common MGW functions

This is a function library for functions used in more than one module and functions that
don't fit anywhere else.

*/

/*****************************************************************************************/
/****    INCLUDE GUARD    ****************************************************************/
#ifndef L_MGW_H
#define L_MGW_H

/*****************************************************************************************/
/****    1 INCLUDE FILES    **************************************************************/
#include <type_def.h>

/*****************************************************************************************/
/****    2 CONSTANTS    ******************************************************************/

/*****************************************************************************************/
/****    3 MACROS    *********************************************************************/

/** Converts a 64 bit size to addressable units */
#define SIZE_64_TO_AU(size) (size_16_to_au(size) * 4)
/** Converts a 32 bit size to addressable units */
#define SIZE_32_TO_AU(size) (size_16_to_au(size) * 2)
/** Converts a 16 bit size to addressable units */
#define SIZE_16_TO_AU(size) size_16_to_au(size)
/** Converts a 8 bit size to addressable units */
#define SIZE_8_TO_AU(size) size_8_to_au(size)
/** Converts an addressable unit size to 64 bit units */
#define SIZE_AU_TO_64(size) (SIZE_16_TO_64(SIZE_AU_TO_16(size)))
/** Converts an addressable unit size to 32 bit units */
#define SIZE_AU_TO_32(size) (SIZE_16_TO_32(SIZE_AU_TO_16(size)))
/** Converts an addressable unit size to 16 bit units */
#define SIZE_AU_TO_16(size) size_au_to_16(size)
/** Converts an addressable unit size to 8 bit units */
#define SIZE_AU_TO_8(size) size_au_to_8(size)

/** Converts a number of bits to 32 bit units */
#define SIZE_1_TO_32(size) (((size) + 31) / 32)
/** Converts a number of bits to 16 bit units */
#define SIZE_1_TO_16(size) (((size) + 15) / 16)
/** Converts a number of bits to 8 bit units */
#define SIZE_1_TO_8(size) (((size) + 7) / 8)
/** Converts a 16 bit size to number of bits */
#define SIZE_16_TO_1(size) ((size) * 16)
/** Converts a 8 bit size to number of bits */
#define SIZE_8_TO_1(size) ((size) * 8)
/** Converts a number of bits to addressable units */
#define SIZE_1_TO_AU(size) SIZE_8_TO_AU(SIZE_1_TO_8(size))
/** Converts a 16 bit size to 8 bit units */
#define SIZE_16_TO_8(size) ((size) * 2)
/** Converts a 8 bit size to 16 bit units */
#define SIZE_8_TO_16(size) (((size) + 1) / 2)
/** Converts a 8 bit size to 32 bit units */
#define SIZE_8_TO_32(size) (((size) + 3) / 4)
/** Converts a 32 bit size to 8 bit units */
#define SIZE_32_TO_8(size) ((size) * 4)
/** Converts a 32 bit size to 16 bit units */
#define SIZE_32_TO_16(size) ((size) * 2)
/** Converts a 16 bit size to 32 bit units */
#define SIZE_16_TO_32(size) (((size) + 1) / 2)
/** Converts a 64 bit size to 32 bit units */
#define SIZE_64_TO_32(size) ((size) * 2)
/** Converts a 32 bit size to 64 bit units */
#define SIZE_32_TO_64(size) (((size) + 1) / 2)

/** Converts a 16 bit size to 64 bit units */
#define SIZE_16_TO_64(size) (((size) + 3) / 4)
/** Converts a 64 bit size to 16 bit units */
#define SIZE_64_TO_16(size) ((size) * 4)

/** Calculates the maximum of two numbers */
#define L_MGW_MAX(a,b) (((a)>(b)) ? (a):(b))
/** Calculates the minimum of two numbers */
#define L_MGW_MIN(a,b) (((a)<(b)) ? (a):(b))
/** Calculates the median of three numbers */
#define L_MGW_MED(a,b,c) ((b)<(c) ? \
                           ((b)<(a)?((c)<(a)?(c):(a)):(b)) : \
                           ((c)<(a)?((b)<(a)?(b):(a)):(c)) \
                         )

/** Swap bytes in uint16 */
#define L_MGW_SWAP_UINT16(input)  *(input) = ((*(input)) << 8) | ((*(input)) >> 8)
/** Swap words in uint32 */
#define L_MGW_SWAP_UINT32(input)  *(input) = ((*(input)) << 16) | ((*(input)) >> 16)
/** Swap bytes from native to little-endian in uint16 */
#define L_MGW_SWAP_NAT_TO_LE_16(input) *(input) = L_MGW_NAT_TO_LE_16(*(input))
/** Swap bytes from native to little-endian in uint32 */
#define L_MGW_SWAP_NAT_TO_LE_32(input) *(input) = L_MGW_NAT_TO_LE_32(*(input))
/** Swap bytes from native to big-endian in uint16 */
#define L_MGW_SWAP_NAT_TO_BE_16(input) *(input) = L_MGW_NAT_TO_BE_16(*(input))
/** Swap bytes from native to big-endian in uint32 */
#define L_MGW_SWAP_NAT_TO_BE_32(input) *(input) = L_MGW_NAT_TO_BE_32(*(input))
/** Swap bytes from little-endian to native in uint16 */
#define L_MGW_SWAP_LE_TO_NAT_16(input) *(input) = L_MGW_LE_TO_NAT_16(*(input))
/** Swap bytes from little-endian to native in uint32 */
#define L_MGW_SWAP_LE_TO_NAT_32(input) *(input) = L_MGW_LE_TO_NAT_32(*(input))
/** Swap bytes from big-endian to native in uint16 */
#define L_MGW_SWAP_BE_TO_NAT_16(input) *(input) = L_MGW_BE_TO_NAT_16(*(input))
/** Swap bytes from big-endian to native in uint32 */
#define L_MGW_SWAP_BE_TO_NAT_32(input) *(input) = L_MGW_BE_TO_NAT_32(*(input))


/** Returns TRUE if value is odd, otherwise FALSE */
#define L_MGW_IS_ODD(value) (((value) % 2) != 0)

/** Returns TRUE if value is even, otherwise FALSE */
#define L_MGW_IS_EVEN(value) (((value) % 2) == 0)

/** Gets high octet of a 16-bit datatype */
#define L_MGW_HIGH_OCTET(x) (((x) & 0xFF00) >> 8)

/** Gets low octet of a 16-bit datatype */
#define L_MGW_LOW_OCTET(x) ((x) & 0x00FF)

/** Swaps high and low octets of 16-bit value. Defined because L_MGW_SWAP_UINT16 swaps its
source data, which is often uncalled for. */
#define L_MGW_DMX_SWAP_UINT16(x) (L_MGW_HIGH_OCTET(x) | (L_MGW_LOW_OCTET(x) << 8))

#if defined ARCH_BIG_ENDIAN
  /** Converts big-endian 16-bit value to native endianism */
  #define L_MGW_BE_TO_NAT_16(x) (x)
  /** Converts little-endian 16-bit value to native format */
  #define L_MGW_LE_TO_NAT_16(x) l_mgw_byte_swap_16(x)
  /** Converts native 16-bit value to big-endian endianism */
  #define L_MGW_NAT_TO_BE_16(x) (x)
  /** Converts native 16-bit value to little-endian endianism */
  #define L_MGW_NAT_TO_LE_16(x) l_mgw_byte_swap_16(x)

  /** Converts big-endian 32-bit value to native endianism */
  #define L_MGW_BE_TO_NAT_32(x) (x)
  /** Converts little-endian 32-bit value to native endianism */
  #define L_MGW_LE_TO_NAT_32(x) l_mgw_byte_swap_32(x)
  /** Converts native 32-bit value to big-endian endianism */
  #define L_MGW_NAT_TO_BE_32(x) (x)
  /** Converts native 32-bit value to little-endian endianism */
  #define L_MGW_NAT_TO_LE_32(x) l_mgw_byte_swap_32(x)

  /** Converts big-endian 64-bit value to native byte order */
  #define L_MGW_BE_TO_NAT_64(x) (x)
  /** Converts little-endian 64-bit value to native byte order */
  #define L_MGW_LE_TO_NAT_64(x) l_mgw_byte_swap_64(x)
  /** Converts native 64-bit value to big-endian byte order */
  #define L_MGW_NAT_TO_BE_64(x) (x)
  /** Converts native 64-bit value to little-endian byte order */
  #define L_MGW_NAT_TO_LE_64(x) l_mgw_byte_swap_64(x)

  /** Converts in-place big-endian 16/32/64-bit value to native endianism */
  #define L_MGW_BE_TO_NAT(x)
  /** Converts in-place little-endian 16/32/64-bit value to native endianism */
  #define L_MGW_LE_TO_NAT(x)    l_mgw_byte_swap_field(&x, sizeof_8(x))
  /** Converts in-place native 16/32/64-bit value to big-endian endianism */
  #define L_MGW_NAT_TO_BE(x)
  /** Converts in-place native 16/32/64-bit value to little-endian endianism */
  #define L_MGW_NAT_TO_LE(x)    l_mgw_byte_swap_field(&x, sizeof_8(x))
#elif defined ARCH_LITTLE_ENDIAN
  /** Converts big-endian 16-bit value to native endianism */
  #define L_MGW_BE_TO_NAT_16(x) l_mgw_byte_swap_16(x)
  /** Converts little-endian 16-bit value to native endianism */
  #define L_MGW_LE_TO_NAT_16(x) (x)
  /** Converts native 16-bit value to big-endian endianism */
  #define L_MGW_NAT_TO_BE_16(x) l_mgw_byte_swap_16(x)
  /** Converts native 16-bit value to little-endian endianism */
  #define L_MGW_NAT_TO_LE_16(x) (x)

  /** Converts big-endian 32-bit value to native endianism */
  #define L_MGW_BE_TO_NAT_32(x) l_mgw_byte_swap_32(x)
  /** Converts little-endian 32-bit value to native endianism */
  #define L_MGW_LE_TO_NAT_32(x) (x)
  /** Converts native 32-bit value to big-endian endianism */
  #define L_MGW_NAT_TO_BE_32(x) l_mgw_byte_swap_32(x)
  /** Converts native 32-bit value to little-endian endianism */
  #define L_MGW_NAT_TO_LE_32(x) (x)

  /** Converts big-endian 64-bit value to native byte order */
  #define L_MGW_BE_TO_NAT_64(x) l_mgw_byte_swap_64(x)
  /** Converts little-endian 64-bit value to native byte order */
  #define L_MGW_LE_TO_NAT_64(x) (x)
  /** Converts native 64-bit value to big-endian endianism */
  #define L_MGW_NAT_TO_BE_64(x) l_mgw_byte_swap_64(x)
  /** Converts native 64-bit value to little-endian endianism */
  #define L_MGW_NAT_TO_LE_64(x) (x)

  /** Converts in-place big-endian 16/32/64-bit value to native endianism */
  #define L_MGW_BE_TO_NAT(x)    l_mgw_byte_swap_field(&x, sizeof_8(x))
  /** Converts in-place little-endian 16/32/64-bit value to native endianism */
  #define L_MGW_LE_TO_NAT(x)
  /** Converts in-place native 16/32/64-bit value to big-endian endianism */
  #define L_MGW_NAT_TO_BE(x)    l_mgw_byte_swap_field(&x, sizeof_8(x))
  /** Converts in-place native 16/32/64-bit value to little-endian endianism */
  #define L_MGW_NAT_TO_LE(x)
#else
  #error ARCH_LITTLE_ENDIAN or ARCH_BIG_ENDIAN not defined
#endif

/** Converts little-endian 16-bit value to native endianism */
#define L_MGW_DMX_UINT16_TO_NAT(x) L_MGW_LE_TO_NAT_16(x)

/** Gets Nth. most significant bit in a 16-bit datatype */
#define L_MGW_GET_BIT(word, n) ((word) & (0x8000 >> (n)))

/** Sets Nth. most significant bit in a 16-bit datatype */
#define L_MGW_SET_BIT(word, n) ((word) = ((word) | (0x8000 >> (n))))

/** Clears Nth. most significant bit in a 16-bit datatype */
#define L_MGW_CLEAR_BIT(word, n) ((word) = ((word) & (~(0x8000 >> (n)))))

/** Gets high 16 bits of a 32-bit datatype */
#define L_MGW_GET_HIGH_16_BITS(x32) (((x32) & 0xFFFF0000) >> 16)

/** Gets low 16 bits of a 32-bit datatype */
#define L_MGW_GET_LOW_16_BITS(x32) ((x32) & 0x0000FFFF)

/** Sets mode in mode set */
#define L_MGW_SET_MODE_IN_MODE_SET(mode_set, upd_mode) \
((mode_set) |= (0x0001 << (upd_mode)))

/** Gives next random value */
#define L_MGW_RAND_16(prev_rand) ((prev_rand * 31821) + 13849)

/** Linear PCM homing pattern */
#define L_MGW_LPCM_HOMING_PATTERN 0x0008

/*****************************************************************************************/
/****    4 DATA TYPES    *****************************************************************/

/**
@brief      Mode set

Mode set defines a set of modes. Each bit in the mode set represents one mode. Bit value 1
means that mode is in the mode set and bit value 0 the opposite. Modes are divided
into bit positions in the mode set as follows. The higher the mode (e.g. in terms of
bits/s) the more significant bit mode has. LSB bit is reserved for the lowest mode. Mode
set can embody at maximum 16 different modes.
*/
typedef uint16 mode_set_t;

/****    5 FUNCTION PROTOTYPES    ********************************************************/

MGW_GLOBAL void
l_mgw_or_bit_buf(const uint16 *source_ptr, uint16 src_bit_shift, uint16 *target_ptr,
                     uint16 trg_bit_shift, uint16 buffer_size);

MGW_GLOBAL bool16
l_mgw_is_mode_in_mode_set(uint16 mode, mode_set_t mode_set);

MGW_GLOBAL uint16
l_mgw_get_num_modes_in_mode_set(mode_set_t mode_set);


/*****************************************************************************************/
/**
@brief      Memset function for 16 bit values

@param      dest_16 Destination address
@param      c_16 16 bit pattern to write
@param      count_16 Count to write
@return     The value of dest_16
@note       Added by Kalle Korhonen
*/
MGW_LOCAL C_INLINE uint16 *
l_mgw_memset_16(uint16 *dest_16, uint16 c_16, uint16 count_16)
{
  uint16 i = 0;
  for (i = 0; i < count_16; i++) {
    dest_16[i] = c_16;
  }
  return dest_16;
}

/*****************************************************************************************/
/**
@brief      Memset function for 32 bit values

@param      dest_32 Destination address
@param      c_32 32 bit pattern to write
@param      count_32 Count to write
@return     The value of dest_32
@note       Added by Juha Sarmavuori
*/
MGW_LOCAL C_INLINE uint32 *
l_mgw_memset_32(uint32 *dest_32, uint32 c_32, uint16 count_32)
{
  uint16 i = 0;
  uint32 *dest_start = dest_32;

  for (i = 0; i < count_32; i++) {
    *(dest_32++) = c_32;
  }

  return dest_start;
}

/*****************************************************************************************/
/**
@brief      memcpy() replacement for copying uint16 buffers safely also in little-endian

Standard memcpy(), uint16 buffers, and odd number of octets comes with a risk in little-
endian environment that last octet is not copied as a user would expect.

@note       Added by Kalle Pietilä
*/
MGW_LOCAL C_INLINE void
l_mgw_memcpy_16(uint16 *to, const uint16 *from, uint16 length_16)
{
  uint16 i = 0;

  for (i = 0; i < length_16; i++) {
    *(to++) = *(from++);
  }
}

/*****************************************************************************************/
/**
@brief      Fast memory copy in 32 bit words

Destination and source start addresses must be 32 bit aligned. Destination and source can
overlap, if destination start address <= source start address.

@param      to start address of destination of copy
@param      from start address of source of copy
@param      length_32 copy size
@note       Added by Juha Sarmavuori
*/
MGW_LOCAL C_INLINE void
l_mgw_memcpy_32(uint32 *to, const uint32 *from, uint16 length_32)
{
  uint16 i = 0;

  for (i = 0; i < length_32; i++) {
    *(to++) = *(from++);
  }
}

/*****************************************************************************************/
/**
@brief      Swap bytes of 16 bit word, e.g. to do endianism conversion

@note       Added by Kalle Pietilä
*/
MGW_LOCAL C_INLINE uint16
l_mgw_byte_swap_16(uint16 x)
{
  return ((x & 0x00FF) << 8) |
         ((x & 0xFF00) >> 8);
}

/*****************************************************************************************/
/**
@brief      Swap bytes of 32 bit word, e.g. to do endianism conversion

@note       Added by Kalle Pietilä
*/
MGW_LOCAL C_INLINE uint32
l_mgw_byte_swap_32(uint32 x)
{
  return ((x & 0x000000FF) << 24) |
         ((x & 0x0000FF00) <<  8) |
         ((x & 0x00FF0000) >>  8) |
         ((x & 0xFF000000) >> 24);
}

/*****************************************************************************************/
/**
@brief      Swap bytes of a 64 bit word, e.g. to do endianism conversion
*/
MGW_LOCAL C_INLINE uint64
l_mgw_byte_swap_64(uint64 x)
{
  return ((((x) & 0x00000000000000FFULL) << 56) |
          (((x) & 0x000000000000FF00ULL) << 40) |
          (((x) & 0x0000000000FF0000ULL) << 24) |
          (((x) & 0x00000000FF000000ULL) <<  8) |
          (((x) & 0x000000FF00000000ULL) >>  8) |
          (((x) & 0x0000FF0000000000ULL) >> 24) |
          (((x) & 0x00FF000000000000ULL) >> 40) |
          (((x) & 0xFF00000000000000ULL) >> 56));
}

/*****************************************************************************************/
/**
@brief      swap byte order of size (8)/16/32/64/..-bit variable

Undefined behaviour in case of bitfields or other variable sizes

@param[in,out]  field Field to be byte-swapped
@param          field_size_8 Size of the field in octets
@note           Added by Kalle Pietilä
*/
MGW_LOCAL C_INLINE void
l_mgw_byte_swap_field(void *field, uint8 field_size_8)
{
  uint8 i = 0;
  uint8 tmp = 0;
  uint8 *lo = (uint8 *) field;
  uint8 *hi = ((uint8 *) field) + field_size_8;

  for (i = 0; i < (field_size_8 / 2); i++) {
    tmp = *(--hi);
    *hi = *lo;
    *lo = tmp;
    lo++;
  }
}

/*****************************************************************************************/
#endif /* L_MGW_H */

