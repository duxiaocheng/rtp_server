/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2013 Intel Corporation. All Rights Reserved.
//
*/

#ifndef VM_TYPES_WIN_H
#define VM_TYPES_WIN_H

#if _MSC_VER <= 1400 && !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x0500
#endif

#include <windows.h>
#include <stdio.h>
#include <ippdefs.h>

#ifdef _UNICODE
# define vm_main wmain
#else
# define vm_main _tmain
#endif

#define vm_thread_handle HANDLE

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __ICL
#define VM_ALIGN_DECL(X,Y) __declspec(align(X)) Y
#else
#define VM_ALIGN_DECL(X,Y) Y
#endif

#define CONST_LL(X) X
#define CONST_ULL(X) X##ULL

/* vm_event.h */
typedef struct vm_event
{
    HANDLE handle;
} vm_event;

/* vm_mmap.h */
typedef struct vm_mmap
{
    HANDLE fd_file, fd_map;
    void  *address;
    Ipp32s fAccessAttr;
} vm_mmap;

/* vm_mutex.h */
typedef struct vm_mutex
{
    CRITICAL_SECTION sCritSection;
    Ipp32s iInited;
} vm_mutex;

/* vm_semaphore.h */
typedef struct vm_semaphore
{
    HANDLE handle;
} vm_semaphore;

/* vm_thread.h */
typedef struct vm_thread
{
    HANDLE handle;
    vm_mutex access_mut;
    Ipp32s i_wait_count;

    Ipp32u (__stdcall * protected_func)(void *);
    void *protected_arg;
    DWORD preset_affinity_mask;
//    Ipp32s selected_cpu;
} vm_thread;


typedef struct vm_time
{
    Ipp64s start;
    Ipp64s diff;
    Ipp64s freq;
} vm_time;

struct vm_timeval
{
    Ipp32u tv_sec;
    long int tv_usec;
};

struct vm_timezone
{
    int tz_minuteswest; // This is the number of minutes west of UTC.
    int tz_dsttime;    /* If nonzero, Daylight Saving Time applies during some part of the year. */
};

#ifdef __cplusplus
}
#endif

#pragma warning(disable: 4995) // "Warning: name was marked as #pragma deprecated"

#endif
