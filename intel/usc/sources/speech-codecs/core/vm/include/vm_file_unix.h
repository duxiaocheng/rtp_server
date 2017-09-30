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
 *    Linux and OSX definitions
 */

#ifndef VM_FILE_UNIX_H
#define VM_FILE_UNIX_H

#ifndef __USE_LARGEFILE64
#define __USE_LARGEFILE64
#endif

#ifndef __USE_LARGEFILE
#define __USE_LARGEFILE
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#if !defined ANDROID
#include <sys/statvfs.h>
#endif

#define MAX_PATH 260

typedef FILE vm_file;
typedef DIR  vm_dir;

#define vm_stderr  stderr
#define vm_stdout  stdout
#define vm_stdin   stdin

/* file access functions */
#if defined(OSX) || defined(INTEL64) || defined(__USE_LARGEFILE64)
/* native fopen is 64-bits */
#define vm_file_fopen    fopen
#else
#define vm_file_fopen    fopen64
#endif

#define vm_file_fclose  fclose
#define vm_file_feof    feof
#define vm_file_remove  remove

/* binary file IO */
#define vm_file_fread    fread
#define vm_file_fwrite   fwrite

/* character (string) file IO */
#define vm_file_fgets      fgets
#define vm_file_fputs      fputs
#define vm_file_fscanf     fscanf
#define vm_file_fprintf    fprintf
#define vm_file_vfprintf   vfprintf

/* temporary file support */
#define vm_file_tmpfile      tmpfile
#define vm_file_tmpnam       tmpnam
#define vm_file_tmpnam_r     tmpnam_r
#define vm_file_tempnam      tempnam

#endif
