/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2013 Intel Corporation. All Rights Reserved.
//
*/

#if defined WINDOWS
#include "vm_types.h"
#include "vm_shared_object.h"

vm_so_handle vm_so_load(vm_char *so_file_name)
{
      /* check error(s) */
    if (NULL == so_file_name)
        return NULL;

    return ((vm_so_handle) LoadLibrary(so_file_name));
}

vm_so_func vm_so_get_addr(vm_so_handle so_handle, vm_char *so_func_name)
{
    /* check error(s) */
    if (NULL == so_handle)
        return NULL;

    return (vm_so_func)GetProcAddress((HMODULE)so_handle, (LPCSTR)so_func_name);
}

void vm_so_free(vm_so_handle so_handle)
{
    /* check error(s) */
    if (NULL == so_handle)
        return;

    FreeLibrary((HMODULE)so_handle);
}
#endif
