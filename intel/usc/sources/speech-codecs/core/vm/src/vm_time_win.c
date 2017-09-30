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
#include "vm_time.h"
#include "time.h"
#include "winbase.h"


/* yield the execution of current thread for msec milliseconds */
void vm_time_sleep(Ipp32u msec)
{
#if defined _WIN32_WCE
    Sleep(msec);  /* always Sleep for WinCE */
#else
    if (msec)
        Sleep(msec);
    else
        SwitchToThread();
#endif
}

/* obtain the clock tick of an uninterrupted master clock */
vm_tick vm_time_get_tick(void)
{
    LARGE_INTEGER t1;

    QueryPerformanceCounter(&t1);
    return t1.QuadPart;
}

/* obtain the clock resolution */
vm_tick vm_time_get_frequency(void)
{
    LARGE_INTEGER t1;

    QueryPerformanceFrequency(&t1);
    return t1.QuadPart;
}

/* Create the object of time measure */
vm_status vm_time_open(vm_time_handle *handle)
{
   if (NULL == handle)
       return VM_NULL_PTR;
   *handle = -1;
   return VM_OK;
}

/* Close the object of time measure */
vm_status vm_time_close(vm_time_handle *handle)
{
   if (NULL == handle)
       return VM_NULL_PTR;
   *handle = -1;
   return VM_OK;
}

/* Initialize the object of time measure */
vm_status vm_time_init(vm_time *m)
{
   if (NULL == m)
       return VM_NULL_PTR;
   m->start = 0;
   m->diff = 0;
   m->freq = vm_time_get_frequency();
   return VM_OK;
}

/* Start the process of time measure */
vm_status vm_time_start(vm_time_handle handle, vm_time *m)
{
   if (NULL == m)
       return VM_NULL_PTR;
   m->start = vm_time_get_tick();
   return VM_OK;
}

/* Stop the process of time measure and obtain the sampling time in seconds */
Ipp64f vm_time_stop(vm_time_handle handle, vm_time *m)
{
   Ipp64f speed_sec;
   vm_tick end;

   end = vm_time_get_tick();
   m->diff += end - m->start;

   if(m->freq == 0) m->freq = vm_time_get_frequency();
   speed_sec = (Ipp64f)m->diff / (Ipp64f)m->freq;
   return speed_sec;
}

static Ipp64u offset_from_1601_to_1970 = 0;

vm_status vm_time_gettimeofday( struct vm_timeval *TP, struct vm_timezone *TZP )
{
    /* FILETIME data structure is a 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601 */
    Ipp64u tmb;
    SYSTEMTIME bp;
    if( offset_from_1601_to_1970 == 0 )
    {
        /* prepare 1970 "epoch" offset */
        bp.wDay = 1; bp.wDayOfWeek = 4; bp.wHour = 0;
        bp.wMinute = 0; bp.wMilliseconds = 0;
        bp.wMonth = 1; bp.wSecond = 0;
        bp.wYear = 1970;
        SystemTimeToFileTime(&bp, (FILETIME *)&offset_from_1601_to_1970);
    }

#if defined _WIN32_WCE
    GetSystemTime(&bp);
    SystemTimeToFileTime(&bp, (FILETIME *)&tmb);
#else
    GetSystemTimeAsFileTime((FILETIME *)&tmb);
#endif

    tmb -= offset_from_1601_to_1970; /* 100 nsec ticks since 1 jan 1970 */
    TP[0].tv_sec = (Ipp32u)(tmb / 10000000); /* */
    TP[0].tv_usec = (long)((tmb % 10000000) / 10); /* microseconds OK? */
    return  (TZP != NULL) ? VM_NOT_INITIALIZED : VM_OK;
}
#endif
