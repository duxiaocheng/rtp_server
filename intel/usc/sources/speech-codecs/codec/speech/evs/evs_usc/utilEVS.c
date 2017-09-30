/*/////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2012 Intel Corporation. All Rights Reserved.
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
// Purpose: EVS speech codec: USC functions.
//
*/
//lint -save -e10 -e2
#include <stddef.h> //for size_t
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//lint -restore
#include "typedefs.h"
#include "usc.h"
#include "ownevs.h"
#include "evsapi.h"
//#include "cnst_fx.h"
#include "ippsc.h"


Word32 evs_get_delay_fx(            /* o  : delay value in ms                         */
	const Word16 what_delay,    /* i  : what delay? (ENC or DEC)                  */
	const Word32 io_fs          /* i  : input/output sampling frequency           */
	);

void evs_indices_to_serial_generic(
	const Indice_fx *ind_list,              /* i: indices list */
	const Word16  num_indices,              /* i: number of indices to write */
	UWord8 *pFrame,                   /* o: byte array with bit packet and byte aligned coded speech data */
	Word16 *pFrame_size               /* i/o: number of bits in the binary encoded access unit [bits] */
	);
Word16 evs_rate2EVSmode(
	Word32 rate                    /* i: bit rate */
	);
/*---------------------------------------------------------------------*
* to_upper()
*
* Capitalize all letters of a string.
* (normally, _strupr() function would be used but it does not work in Unix)
*---------------------------------------------------------------------*/

static char *to_upper(char *str)
{
	short i;

	i = 0;
	while (str[i] != 0)
	{
		if (str[i] >= 'a' && str[i] <= 'z') str[i] += 'A' - 'a';
		i++;
	}

	return str;
}
/*-------------------------------------------------------------------*
* pack_bit()
*
* insert a bit into packed octet
*-------------------------------------------------------------------*/
static void pack_bit(
	const Word16 bit,    /* i:   bit to be packed */
	UWord8 **pt,         /* i/o: pointer to octet array into which bit will be placed */
	UWord8 *omask        /* i/o: output mask to indicate where in the octet the bit is to be written */
	)
{
	if (*omask == 0x80)
	{
		**pt = 0;
	}
	if (bit != 0)
	{
		**pt = **pt | *omask;
	}
	*omask >>= 1;
	if (*omask == 0)
	{
		*omask = 0x80;
		(*pt)++;
	}

	return;
}
/*--------------------------------------------------------------------------
*  evs_get_delay_fx()
*
*  Function returns various types of delays in the codec in ms.
*--------------------------------------------------------------------------*/

Word32 evs_get_delay_fx(            /* o  : delay value in ms                         */
	const Word16 what_delay,    /* i  : what delay? (ENC or DEC)                  */
	const Word32 io_fs          /* i  : input/output sampling frequency           */
	)
{
	Word32 delay = 0;

	if((what_delay - ENC) == 0)
	{
		delay = (DELAY_FIR_RESAMPL_NS + ACELP_LOOK_NS);
	}
	else
	{
		if((io_fs -  8000) == 0)
		{
			delay = DELAY_CLDFB_NS;
		}
		else
		{
			delay = DELAY_BWE_TOTAL_NS;
		}
	}

	return delay;
}

/*-------------------------------------------------------------------*
* evs_indices_to_serial_generic()
*
* pack indices into serialized payload format
*-------------------------------------------------------------------*/

void evs_indices_to_serial_generic(
	const Indice_fx *ind_list,      /* i: indices list */
	const Word16  num_indices,      /* i: number of indices to write */
	UWord8 *pFrame,           /* o: byte array with bit packet and byte aligned coded speech data */
	Word16 *pFrame_size       /* i/o: number of bits in the binary encoded access unit [bits] */
	)
{
	Word16 i, k, j;
	Word32 mask;
	UWord8 omask;
	UWord8 *pt_pFrame = pFrame;
	Word16 nb_bits_tot;

	nb_bits_tot = 0;
	omask = (0x80 >> (*pFrame_size & 0x7));
	pt_pFrame += (*pFrame_size) >> 3;

	/*----------------------------------------------------------------*
	* Bitstream packing (conversion of individual indices into a serial stream)
	*----------------------------------------------------------------*/
	j = 0;
	for (i = 0; i<num_indices; i++)
	{
		if (ind_list[i].nb_bits != -1)
		{
			/* mask from MSB to LSB */
			mask = 1 << (ind_list[i].nb_bits - 1);

			/* write bit by bit */
			for (k = 0; k < ind_list[i].nb_bits; k++)
			{
				pack_bit(ind_list[i].value & mask, &pt_pFrame, &omask);
				j++;
				mask >>= 1;
			}
			nb_bits_tot = nb_bits_tot + ind_list[i].nb_bits;
		}
	}

	*pFrame_size = *pFrame_size + nb_bits_tot;

	return;
}
/*-------------------------------------------------------------------*
* rate2AMRWB_IOmode()
*
* lookup AMRWB IO mode
*-------------------------------------------------------------------*/
static Word16 rate2AMRWB_IOmode(
	Word32 rate                    /* i: bit rate */
	)
{
	switch (rate)
	{
		/* EVS AMR-WB IO modes */
	case SID_1k75:
		return AMRWB_IO_SID;
	case ACELP_6k60:
		return AMRWB_IO_6600;
	case ACELP_8k85:
		return AMRWB_IO_8850;
	case ACELP_12k65:
		return AMRWB_IO_1265;
	case ACELP_14k25:
		return AMRWB_IO_1425;
	case ACELP_15k85:
		return AMRWB_IO_1585;
	case ACELP_18k25:
		return AMRWB_IO_1825;
	case ACELP_19k85:
		return AMRWB_IO_1985;
	case ACELP_23k05:
		return AMRWB_IO_2305;
	case ACELP_23k85:
		return AMRWB_IO_2385;
	default:
		return -1;
	}
}

/*-------------------------------------------------------------------*
* evs_rate2EVSmode()
*
* lookup EVS mode
*-------------------------------------------------------------------*/
Word16 evs_rate2EVSmode(
	Word32 rate                    /* i: bit rate */
	)
{
	switch (rate)
	{
		/* EVS Primary modes */
	case FRAME_NO_DATA:
		return NO_DATA_TYPE;
	case SID_2k40:
		return PRIMARY_SID;
	case PPP_NELP_2k80:
		return PRIMARY_2800;
	case ACELP_7k20:
		return PRIMARY_7200;
	case ACELP_8k00:
		return PRIMARY_8000;
	case ACELP_9k60:
		return PRIMARY_9600;
	case ACELP_13k20:
		return PRIMARY_13200;
	case ACELP_16k40:
		return PRIMARY_16400;
	case ACELP_24k40:
		return PRIMARY_24400;
	case ACELP_32k:
		return PRIMARY_32000;
	case ACELP_48k:
		return PRIMARY_48000;
	case ACELP_64k:
		return PRIMARY_64000;
	case HQ_96k:
		return PRIMARY_96000;
	case HQ_128k:
		return PRIMARY_128000;
	default:
		return rate2AMRWB_IOmode(rate);
	}
}

/*---------------------------------------------------------------------*
* evs_read_next_rfparam_fx()
*
* Read next channel aware configuration parameters
*      p: RF FEC indicator HI or LO, and
*      o: RF FEC offset in number of frame (2, 3, 5, or 7)
*---------------------------------------------------------------------*/

void evs_read_next_rfparam_fx(
	Word16 *rf_fec_offset,    /* o: rf offset                         */
	Word16 *rf_fec_indicator, /* o: rf FEC indicator                  */
	FILE* f_rf                /* i: file pointer to read parameters   */
	)
{
	char rline[10], str[4];
	Word32 tmp;

	/* initialize */
	*rf_fec_offset = 0;
	*rf_fec_indicator = 1;


	if (f_rf != NULL)
	{

		while (fgets(rline, 10, f_rf) == NULL && feof(f_rf))
		{
			rewind(f_rf);
		}
	}
	else
	{
		return;
	}

	if (sscanf(rline, "%3s %d", str, &tmp) != 2)
	{
		fprintf(stderr, "Error in the RF configuration file. There is no proper configuration line.\n");
		exit(-1);
	}

	/* Read RF FEC indicator */
	if (strcmp(to_upper(str), "HI") == 0)
	{
		*rf_fec_indicator = 1;
	}
	else if (strcmp(to_upper(str), "LO") == 0)
	{
		*rf_fec_indicator = 0;
	}
	else
	{
		fprintf(stderr, " Incorrect FEC indicator string. Exiting the encoder.\n");
		exit(-1);
	}

	/* Read RF FEC offset */
	if (tmp == 0 || tmp == 2 || tmp == 3 || tmp == 5 || tmp == 7)
	{
		*rf_fec_offset = (Word16)tmp;
	}
	else
	{
		fprintf(stderr, "Error: incorrect FEC offset specified in the RF configration file; RF offset can be 2, 3, 5, or 7. \n");
		exit(-1);
	}
}

/*---------------------------------------------------------------------*
* evs_read_next_bwidth()
*
* Read next bandwidth from profile file (only if invoked on cmd line)
*---------------------------------------------------------------------*/

void evs_read_next_bwidth_fx(
	Word16  *max_bwidth,            /* i/o: maximum encoded bandwidth                 */
	FILE    *f_bwidth,              /* i  : bandwidth switching profile (0 if N/A)    */
	Word32  *bwidth_profile_cnt    /* i/o: counter of frames for bandwidth switching profile file */
	)
{
	int res;
	char stmp[4];

	if (*bwidth_profile_cnt == 0)
	{
		/* read next bandwidth value and number of frames from the profile file */
		while ((res = fscanf(f_bwidth, "%d %3s", bwidth_profile_cnt, stmp)) != 2 && feof(f_bwidth))
		{
			rewind(f_bwidth);
		}

		(*bwidth_profile_cnt)--;

		if (strcmp(to_upper(stmp), "NB") == 0)
		{
			*max_bwidth = NB;
		}
		else if (strcmp(to_upper(stmp), "WB") == 0)
		{
			*max_bwidth = WB;
		}
		else if (strcmp(to_upper(stmp), "SWB") == 0)
		{
			*max_bwidth = SWB;
		}
		else if (strcmp(to_upper(stmp), "FB") == 0)
		{
			*max_bwidth = FB;
		}
	}
	else
	{
		/* current profile still active, only decrease the counter */
		(*bwidth_profile_cnt)--;
	}

	return;
}

/*---------------------------------------------------------------------*
* read_next_brate()
*
* Read next bitrate from profile file (only if invoked on cmd line)
*---------------------------------------------------------------------*/

void evs_read_next_brate_fx(
	Word32  *total_brate,             /* i/o: total bitrate                             */
	FILE   *f_rate                   /* i  : bitrate switching profile (0 if N/A)      */
	)
{
	/*The complexity don't need to be taken into account given that is only simulation */
	/* read next bitrate value from the profile file */
	if (f_rate != NULL)
	{
		while (fread(total_brate, 4, 1, f_rate) != 1 && feof(f_rate))
		{
			rewind(f_rate);
		}
	}
}