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

#if defined VM_AFFINITY_ENABLED && defined WINDOWS
/*
 *  Windows part of VM affinity block
 */
#include <stdlib.h>
#include <vm_types.h>
#include <vm_sys_info.h>
#include <vm_affinity.h>

/* a little specific functions */
vm_affinity_mask *vm_affinity_mask_init_alloc(void)
{
    vm_affinity_mask *rtv = NULL;
    if((rtv = malloc(sizeof(vm_affinity_mask))) != NULL)
    {
        if((rtv[0].mskbits = malloc(sizeof(DWORD))) != NULL)
            rtv[0].msklen = 1;
        else
        {
            free(rtv);
            rtv = NULL;
        }
    }
    return rtv;
}

vm_thread_handle vm_affinity_thread_get_current(void)
{
    return GetCurrentThread();
}

/*
*   CPU manipulations
*
*   process affinity
*/

#define  AFOPREFIX              \
  vm_status rtv = VM_NULL_PTR;  \
  if (mptr) {                   \
    rtv = VM_OPERATION_FAILED

#define AFOEFIX     \
    }               \
    return rtv;

vm_status vm_affinity_process_set(VM_PID pid, vm_affinity_mask *mptr)
{
    if(!mptr)
        return VM_NULL_PTR;

    if(SetProcessAffinityMask(pid, mptr[0].mskbits[0]))
        return VM_OK;

    return VM_OPERATION_FAILED;
}

vm_status vm_affinity_process_get(VM_PID pid, vm_affinity_mask *mptr)
{
    DWORD threadmask = 0;

    if(!mptr)
        return VM_NULL_PTR;

    if(mptr[0].mskbits)
    {
        if(GetProcessAffinityMask(pid, (PDWORD_PTR)mptr[0].mskbits, (PDWORD_PTR)&threadmask))
            return VM_OK;
    }

    return VM_OPERATION_FAILED;
}

/* thread affinity */
#define MAX_OS_NAME_LENGTH 169
#define VM_SYSTEM_WINDOWSXP 0x51
#define VM_SYSTEM_WINVISTA  0x60
#define VM_SYSTEM_WINVISTA1 0x61
#define VM_SYSTEM_WINVISTA2 0x62
#define VM_SYSTEM_WIN200    0x50
#define VM_SYSTEM_WINNT4    0x40
#define VM_SYSTEM_WIN2003   0x52

static int windows_code = -1;

vm_status vm_affinity_thread_set(vm_thread *pthr, vm_affinity_mask *mptr)
{
    if(!mptr)
        return VM_NULL_PTR;

    if(SetThreadAffinityMask(pthr[0].handle, mptr[0].mskbits[0]))
    {
        pthr[0].preset_affinity_mask = mptr[0].mskbits[0];
        return VM_OK;
    }

    return VM_OPERATION_FAILED;
}

vm_status vm_affinity_thread_get(vm_thread *pthr, vm_affinity_mask *mptr)
{
    DWORD processmask = 0;

    if(!mptr)
        return VM_NULL_PTR;

    if(mptr[0].msklen)
    {
        /* only one way to obtain thread affinity mask - set it again */
        if (pthr[0].preset_affinity_mask)
        {
            mptr[0].mskbits[0] = (unsigned long)SetThreadAffinityMask(pthr[0].handle, pthr[0].preset_affinity_mask);
            return VM_OK;
        }
        else
        {
            if(GetProcessAffinityMask(pthr[0].handle, (PDWORD_PTR)&processmask, (PDWORD_PTR)mptr[0].mskbits))
                return VM_OK;
        }
    }

    return VM_OPERATION_FAILED;
}

#endif
