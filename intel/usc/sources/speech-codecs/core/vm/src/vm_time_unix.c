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
#include <unistd.h>
#include <sys/time.h>
#include <sched.h>
#include "vm_time.h"
#include "vm_sys_info.h"
#include <sys/ioctl.h>
#include <fcntl.h>

//# ifdef OSX
//#   include <sys/ioctl.h>
//# endif

/* yield the execution of current thread for msec miliseconds */
void vm_time_sleep(Ipp32u msec)
{
    if (msec)
        usleep(1000 * msec);
    else
        sched_yield();
}

#define VM_TIME_MHZ 1000000

/* obtain the clock tick of an uninterrupted master clock */
vm_tick vm_time_get_tick(void)
{
    unsigned int a,d;
    asm volatile("rdtsc" : "=a" (a), "=d" (d));
    return (vm_tick)a | (vm_tick)d << 32;
}

/* obtain the master clock resolution */
vm_tick vm_time_get_frequency(void)
{
    return vm_sys_info_get_cpu_speed()*VM_TIME_MHZ;
}

/* Create the object of time measure */
vm_status vm_time_open(vm_time_handle *handle)
{
    vm_time_handle t_handle;
    vm_status status = VM_OK;

    if (NULL == handle)
        return VM_NULL_PTR;
    t_handle = -1;

    t_handle = open("/dev/tsc", 0);
    if (t_handle > 0)
        ioctl(t_handle, ENABLE_COUNTER, 0);
    else
        status = VM_OPERATION_FAILED;
    *handle = t_handle;
    return status;
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

/* Close the object of time measure */
vm_status vm_time_close(vm_time_handle *handle)
{
   vm_time_handle t_handle;

   if (NULL == handle)
       return VM_NULL_PTR;

   t_handle = *handle;
   if (t_handle > 0)
   {
       ioctl(t_handle, DISABLE_COUNTER, 0);
       close(t_handle);
       *handle = -1;
   }
   return VM_OK;
}

/* Start the process of time measure */
vm_status vm_time_start(vm_time_handle handle, vm_time *m)
{
   if (NULL == m)
       return VM_NULL_PTR;

   if (handle > 0)
   {
       Ipp32u startHigh, startLow;
       startLow   = ioctl(handle, GET_TSC_LOW, 0);
       startHigh  = ioctl(handle, GET_TSC_HIGH, 0);
       m->start = ((Ipp64u)startHigh << 32) + (Ipp64u)startLow;
   }
   else
       m->start = vm_time_get_tick();
   return VM_OK;
}

/* Stop the process of time measure and obtain the sampling time in seconds */
Ipp64f vm_time_stop(vm_time_handle handle, vm_time *m)
{
   Ipp64f speed_sec;
   Ipp64s end;
   Ipp32s freq_mhz;

   if (handle > 0)
   {
       Ipp32u startHigh, startLow;
       startLow   = ioctl(handle, GET_TSC_LOW, 0);
       startHigh  = ioctl(handle, GET_TSC_HIGH, 0);
       end = ((Ipp64u)startHigh << 32) + (Ipp64u)startLow;
   }
   else
       end = vm_time_get_tick();

   m->diff += (end - m->start);

   if (handle > 0)
   {
      if((m->freq == 0) || (m->freq == VM_TIME_MHZ))
      {
         freq_mhz = vm_time_get_frequency();
         m->freq = (Ipp64s)freq_mhz;
      }
      speed_sec = (Ipp64f)m->diff/1000000.0/(Ipp64f)m->freq;
   }
   else
   {
      if(m->freq == 0)
          m->freq = vm_time_get_frequency();
      speed_sec = (Ipp64f)m->diff/(Ipp64f)m->freq;
   }

   return speed_sec;
}

vm_status vm_time_gettimeofday( struct vm_timeval *TP, struct vm_timezone *TZP )
{
    return (gettimeofday(TP, TZP) == 0) ? VM_OK : VM_NOT_INITIALIZED;
}
#endif
