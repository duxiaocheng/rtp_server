/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2013 Intel Corporation. All Rights Reserved.
//
*/

#if defined(_MSC_VER)
#pragma warning(disable:4206) // warning C4206: nonstandard extension used : translation unit is empty
#endif

#if defined UNIX
#include <dlfcn.h>
#include "vm_shared_object.h"

vm_so_handle vm_so_load(vm_char* so_file_name)
{
    void *handle;

    /* check error(s) */
    if (NULL == so_file_name)
        return NULL;

    handle = dlopen(so_file_name, RTLD_LAZY);

    return (vm_so_handle)handle;
}

vm_so_func vm_so_get_addr(vm_so_handle so_handle, vm_char *so_func_name)
{
    vm_so_func addr;

    /* check error(s) */
    if (NULL == so_handle)
        return NULL;

    addr = (vm_so_func)dlsym(so_handle,so_func_name);
    if (dlerror())
        return NULL;

    return addr;
}

void vm_so_free(vm_so_handle so_handle)
{
    /* check error(s) */
    if (NULL == so_handle)
        return;

    dlclose(so_handle);
}
#endif
