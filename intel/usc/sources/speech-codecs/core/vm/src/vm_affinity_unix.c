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

/* valid only for Linux */
#if defined VM_AFFINITY_ENABLED && defined LINUX
#include <vm_types.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#ifdef VM_AFFINITY_OLD_NPTL
#include <nptl/pthread.h>
#else
#include <pthread.h>
#endif

#ifdef __FreeBSD__
#include <sys/param.h>
#include <sys/cpuset.h>
#include <pthread_np.h>
#define cpu_set_t cpuset_t
#endif

#include <unistd.h>
#include <sys/types.h>

#include <vm_affinity.h>

/*
 *  Unix part of VM affinity block
 */

/* Set the CPU affinity for a task */
vm_affinity_mask *vm_affinity_mask_init_alloc(void)
{
    vm_affinity_mask *rtv = NULL;
    Ipp32s nwords = (sizeof(cpu_set_t)/sizeof(unsigned long));
    if ((rtv = (vm_affinity_mask *)malloc(sizeof(vm_affinity_mask))) != NULL)
    {
        rtv[0].msklen = nwords;
        if ((rtv[0].mskbits = (unsigned long *)malloc(nwords*sizeof(unsigned long))) == NULL)
        {
            free(rtv);
            rtv = NULL;
        }
    }
    return rtv;
}

#define VM_MAX_PATH 1024
#define VM_MAX_STAT_LINE_LENGTH 1024

vm_thread_handle vm_affinity_thread_get_current(void)
{
    return pthread_self();
}

/* process affinity */
vm_status vm_affinity_process_set(VM_PID pid, vm_affinity_mask *mptr)
{
#ifndef __FreeBSD__
    cpu_set_t bst;
    memcpy( bst.__bits, mptr[0].mskbits, sizeof(cpu_set_t));
    return (sched_setaffinity(pid, sizeof(cpu_set_t), &bst) == 0) ? VM_OK : VM_OPERATION_FAILED;
#else
    return VM_OK;
#endif
}

vm_status vm_affinity_process_get(VM_PID pid, vm_affinity_mask *mptr)
{
#ifndef __FreeBSD__
    cpu_set_t bst;
    vm_status rtv = VM_OPERATION_FAILED;
    if (sched_getaffinity(pid, sizeof(cpu_set_t), &bst) == 0)
    {
        memcpy(mptr[0].mskbits, bst.__bits, sizeof(cpu_set_t));
        rtv = VM_OK;
    }
    return rtv;
#else
    return VM_OK;
#endif
}

vm_status vm_affinity_thread_set(vm_thread *pthr, vm_affinity_mask *mptr)
{
#ifdef VM_NPTL_WITH_AFFINITY
    vm_status rtv = VM_OPERATION_FAILED;
    cpu_set_t bst;
    if(pthr == NULL)
        rtv = VM_NULL_PTR;
    else
    {
        memcpy( bst.__bits, mptr[0].mskbits, sizeof(cpu_set_t));
        if(pthread_setaffinity_np(pthr[0].handle, sizeof(cpu_set_t), &bst) == 0)
            rtv = VM_OK;
    }
    return rtv;
#else
    return VM_OK;
#endif
}

vm_status vm_affinity_thread_get(vm_thread *pthr, vm_affinity_mask *mptr)
{
    vm_status rtv = VM_OPERATION_FAILED;

#ifdef VM_NPTL_WITH_AFFINITY
    cpu_set_t bst;
    /* only for NPTL 2.4 and more */
    if(pthr == NULL)
        rtv = VM_NULL_PTR;
    else
    {
        if (pthread_getaffinity_np(pthr[0].handle, sizeof(cpu_set_t), &bst) == 0)
        {
            memcpy(mptr[0].mskbits, bst.__bits, sizeof(cpu_set_t));
            rtv = VM_OK;
        }
    }
#endif
    return rtv;
}

#endif
