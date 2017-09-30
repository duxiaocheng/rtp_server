/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2013 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __VM_SHARED_OBJECT_H__
#define __VM_SHARED_OBJECT_H__

#include "vm_types.h"
#include "vm_strings.h"

typedef void *vm_so_handle;
typedef void (*vm_so_func)(void);

#ifdef __cplusplus
extern "C" {
#endif

vm_so_handle vm_so_load(vm_char* so_file_name);
vm_so_func   vm_so_get_addr(vm_so_handle so_handle, vm_char* so_func_name);
void         vm_so_free(vm_so_handle so_handle);

#ifdef __cplusplus
}
#endif

#ifndef NULL
#define NULL (void*)0L
#endif

#endif
