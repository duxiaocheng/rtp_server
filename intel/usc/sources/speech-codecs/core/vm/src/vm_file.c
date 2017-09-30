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
 *       common implementation
 */

#include "vm_file.h"

#if _MSC_VER >= 1400
#pragma warning(disable:4996)
#endif

#if defined WINDOWS
#define SLASH '\\'
#else
#define SLASH '/'
#endif

/* file name manipulations */

/* return only path of file name */
void vm_file_getpath(vm_char *filename, vm_char *path, int nchars)
{
    /* go to end of line and then move up until first SLASH will be found */
    size_t len;
    path[0] = '\0';

    len = vm_string_strlen(filename);
    while(len && (filename[len--] != SLASH));

    if (len)
    {
        memcpy((void *)path, (const void *)filename, (len <= (size_t)nchars) ? len+1 : nchars);
        path[len+1] = '\0';
    }
}

/* return base file name free of path and all suffixes */
void vm_file_getbasename(vm_char *filename, vm_char *base, int nchars)
{
    Ipp32s chrs = 0;
    vm_char *p, *q0 = NULL, *q1 = NULL, *s;
    base[0] = '\0';
    p  = vm_string_strchr(filename, '.'); // first invocation of .
    s  = filename;

    do
    {
        q0 = vm_string_strchr(s, SLASH);
        if (q0 != NULL)
        {
            q1 = q0;
            s = q0+1;
        }
    } while( q0 != NULL );

    if(p == NULL)
        p = &filename[vm_string_strlen(filename)];

    if( q1 == NULL )
        q1 = filename;

    chrs = (Ipp32s)(p - q1);
    if (chrs)
    {
        if(q1[0] == SLASH)
        {
            ++q1;
            --chrs;
        }
        if (chrs > nchars)
            chrs = nchars-1;

        memcpy((void *)base, (const void *)q1, chrs);
        base[chrs] = '\0';
    }
}

/*
 * return full file suffix or nchars of suffix if nchars is to small to fit the suffix
 * !!! if more then one suffix applied then only first from the end of filename will be found
 */
void vm_file_getsuffix(vm_char *filename, vm_char *suffix, int nchars)
{
    /* go to end of line and then go up until we will meet the suffix sign . or
    * to beginning of line if no suffix found */
    size_t len, i = 0;
    len = vm_string_strlen(filename);
    suffix[0] = '\0';

    while(len && (filename[len--] != '.'));

    if(len)
    {
        len += 2;
        for( ; filename[len]; ++len)
        {
            suffix[i] = filename[len];
            if (++i >= (size_t)nchars)
                break;
        }
        suffix[i] = '\0';
    }
}

#define ADDPARM(A)                    \
  if((Ipp32u)nchars > (Ipp32u)vm_string_strlen(A)) {   \
    vm_string_strcat(filename, A);              \
    offs = (Ipp32u)vm_string_strlen(filename);          \
    nchars -= offs;                   \
    if (nchars)                       \
      filename[offs] = SLASH;         \
    ++offs;                           \
    --nchars;                         \
    filename[offs] = '\0';            \
    }

/*
 * prepare complex file name according with OS rules:
 *    / delimiter for unix and \ delimiter for Windows */
void vm_file_makefilename(vm_char *path, vm_char *base, vm_char *suffix, vm_char *filename, int nchars)
{
    Ipp32u offs = 0;
    filename[0] = '\0';
    if((path != NULL) && (vm_string_strlen(path) < (size_t)nchars))
    {
        ADDPARM(path)
    }

    if (nchars && (base != NULL))
    {
        ADDPARM(base)
    }

    if (nchars && (suffix != NULL))
    {
        if(offs == 0)
        {
            filename[offs++] = '.';
            filename[offs] = '\0';
            --nchars;
        }
        else
        {
            if (filename[offs-1] == SLASH)
                filename[offs-1] = '.';
        }

        ADDPARM(suffix)
    }

    /* remove SLASH if exist */
    if (filename[offs-1] == SLASH)
        filename[offs-1] = '\0';
}
