/*****************************************************************************************/
/**
@brief RTP process head
*/

/*****************************************************************************************/
/****    INCLUDE GUARD    ****************************************************************/

#ifndef __RTP_PROCESS_H__
#define __RTP_PROCESS_H__

/*****************************************************************************************/
/****    1 INCLUDE FILES    **************************************************************/

/*****************************************************************************************/
/****    2 CONSTANTS    ******************************************************************/

/*****************************************************************************************/
/****    3 MACROS    *********************************************************************/

/*****************************************************************************************/
/****    4 DATA TYPES    *****************************************************************/

/*****************************************************************************************/
/****    5 FUNCTION PROTOTYPES    ********************************************************/

int rtp_process(void *buff, int len, void *buff_out, int *len_out_8);

/*****************************************************************************************/
#endif  /* __RTP_PROCESS_H__ */


