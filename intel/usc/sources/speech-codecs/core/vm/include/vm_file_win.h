/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2013 Intel Corporation. All Rights Reserved.
//
*/
/*
 * VM 64-bits buffered file operations library
 *    Windows definitions
 */

#ifndef VM_FILE_WIN_H
#define VM_FILE_WIN_H

#include <windows.h>
#include <Winbase.h>
#include <stdarg.h>
#include <string.h>
#include "vm_mutex.h"

#if _MSC_VER >= 1400
#pragma warning( disable: 4996 )
#endif

#define VM_TEMPORARY_PREFIX 0x55

#ifndef USE_SYSTEM_FILE_FUNCTIONS
#define vm_stdin  (vm_file *)3
#define vm_stderr (vm_file *)1
#define vm_stdout (vm_file *)2
#else
#define vm_stdin  stdin
#define vm_stderr stderr
#define vm_stdout stdout
#endif

#ifndef USE_SYSTEM_FILE_FUNCTIONS
typedef struct
{
    HANDLE      fd;
    vm_mutex    lck;
    vm_char    *tbuf;    /* temporary buffer for character io operations */
    Ipp64u      fsize;
    Ipp32u      fattributes;
    Ipp8u       ftemp; /* temporary flag sign - added to support temporary files VM_TEMPORARY_PREFIX - temporary file */
    Ipp8u       fbin;  /* binary file sign */

    /* to make fscanf pseudo reentrant */
    vm_char    *lnptr;
    Ipp32u      lnpos;
    vm_char     lnchar;
} vm_file;
#else
typedef FILE vm_file;
#endif

typedef struct
{
    HANDLE          handle;
    WIN32_FIND_DATA ent;
} vm_dir;

#endif
