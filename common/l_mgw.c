/*****************************************************************************************/
/**
@brief Function library for common MGW functions

Remember to name the author of the function when adding one.

*/

/*****************************************************************************************/
/****    1 INCLUDE FILES    **************************************************************/

#include "l_mgw.h"

#include <string.h>
#include <stdint.h>

/*****************************************************************************************/
/****    X PRAGMAS    ********************************************************************/

/*****************************************************************************************/
/****    2 LOCAL CONSTANTS    ************************************************************/

/*****************************************************************************************/
/****    3 LOCAL MACROS    ***************************************************************/

/*****************************************************************************************/
/****    4 LOCAL DATA TYPES    ***********************************************************/

/*****************************************************************************************/
/****    5 LOCAL FUNCTION PROTOTYPES    **************************************************/

/*****************************************************************************************/
/****    6 FUNCTIONS    ******************************************************************/

/*****************************************************************************************/
/**
@brief      Bitwise boolean OR operation between two bit buffers

The bits in target buffer are set to the result of boolean OR operation between the source
and target buffers. The main purpose of this function is to enable fast copying of short
buffers: it is faster to clear a large buffer and use boolean or instead of full copy for
the short buffers.

@param      source_ptr Pointer to word containing first bit of source buffer
@param      src_bit_shift The number of target bit in the word
@param      target_ptr Pointer to word containing first bit of target buffer
@param      trg_bit_shift The number of target bit in the word
@param      buffer_size Number of bits in buffer
@note       Added by Juhani Simola
*/
MGW_GLOBAL void
l_mgw_or_bit_buf(const uint16 *source_ptr, uint16 src_bit_shift,
                 uint16 *target_ptr, uint16 trg_bit_shift, uint16 buffer_size)
{
  uint16 copy_count_words;
  uint16 copy_count_bits;
  uint16 temp_word;

  source_ptr += (src_bit_shift >> 4);
  src_bit_shift &= 0x000F;
  target_ptr += (trg_bit_shift >> 4);
  trg_bit_shift &= 0x000F;

  copy_count_words = (buffer_size >> 4);
  copy_count_bits = (buffer_size & 0x000F);

  while (copy_count_words > 0) {
    /* copy full word to temp */
    temp_word = (*source_ptr++ << src_bit_shift);
    temp_word |= (*source_ptr >> (16 - src_bit_shift));

    /* copy to target */
    *target_ptr++ |= (temp_word >> trg_bit_shift);
    *target_ptr |= (temp_word << (16 - trg_bit_shift));

    copy_count_words--;
  }

  if (copy_count_bits > 0) {
    /* copy full word to temp */
    temp_word = (*source_ptr << src_bit_shift);
    temp_word |= (*(source_ptr + 1) >> (16 - src_bit_shift));

    /* set excess bits to zero */
    temp_word &= (0xFFFF << (16 - copy_count_bits));

    /* copy to target */
    *target_ptr |= (temp_word >> trg_bit_shift);
    *(target_ptr + 1) |= (temp_word << (16 - trg_bit_shift));
  }
  return;
}

/*****************************************************************************************/
/**
@brief      Checks whether mode is a member of mode set

Checks whether given mode is a member of given mode set.

@param      mode Mode whose membership is checked
@param      mode_set Set of modes, LSB bit is the lowest mode etc.
@return     TRUE if given mode is in the given mode set, FALSE otherwise
@note       Added by Mikko Ruotsalainen
*/
MGW_GLOBAL bool16
l_mgw_is_mode_in_mode_set(uint16 mode, mode_set_t mode_set)
{
  if (((mode_set >> mode) & 0x0001) == 1) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/*****************************************************************************************/
/**
@brief      Gets the number of modes in the given mode set

Gets the number of modes in the given mode set.

@param      mode_set Set of modes, LSB bit is the lowest mode etc.
@return     Number of modes in the given mode set
@note       Added by Mikko Ruotsalainen
*/
MGW_GLOBAL uint16
l_mgw_get_num_modes_in_mode_set(mode_set_t mode_set)
{
  uint16 num_modes = 0;
  uint16 i = 0;

  for (i = 0; i < 16; i++) {
    num_modes += ((mode_set >> i) & 0x0001);
  }

  return num_modes;
}

