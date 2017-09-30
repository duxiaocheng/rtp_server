/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2013 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __VM_TYPES_H__
#define __VM_TYPES_H__

#if defined WINDOWS
#include <io.h>
#include "vm_types_win.h"
#else
#include "vm_types_unix.h"
#endif

#include "vm_strings.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VM_ALIGN16_DECL(X) VM_ALIGN_DECL(16,X)
#define VM_ALIGN32_DECL(X) VM_ALIGN_DECL(32,X)
#define VM_ALIGN64_DECL(X) VM_ALIGN_DECL(64,X)

#define _MAX_LEN 256

typedef enum e_vm_Status
{
    VM_OK                           = 0,    // no error
    VM_OPERATION_FAILED             =-999,
    VM_NOT_INITIALIZED              =-998,
    VM_TIMEOUT                      =-987,
    VM_NOT_ENOUGH_DATA              =-996,  // not enough input data

    VM_NULL_PTR                     =-995,
    VM_SO_CANT_LOAD                 =-994,
    VM_SO_INVALID_HANDLE            =-993,
    VM_SO_CANT_GET_ADDR             =-992
} vm_status;

/* Resource examination definitions */

#ifdef WINDOWS
typedef HANDLE VM_PID;
#else
typedef pid_t VM_PID;
#endif

#include "vm_file.h"

#ifdef __cplusplus
}
#endif

#endif
