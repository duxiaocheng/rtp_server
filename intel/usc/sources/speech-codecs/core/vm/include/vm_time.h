/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2013 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __VM_TIME_H__
#define __VM_TIME_H__

#include "vm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef Ipp64s vm_tick;
typedef Ipp32s vm_time_handle;

/* Yield the execution of current thread for msec miliseconds */
void vm_time_sleep(Ipp32u msec);

/* Obtain the clock tick of an uninterrupted master clock */
vm_tick vm_time_get_tick(void);

/* Obtain the clock resolution */
vm_tick vm_time_get_frequency(void);

/* Create the object of time measure */
vm_status vm_time_open(vm_time_handle *handle);

/* Initialize the object of time measure */
vm_status vm_time_init(vm_time *m);

/* Start the process of time measure */
vm_status vm_time_start(vm_time_handle handle, vm_time *m);

/* Stop the process of time measure and obtain the sampling time in seconds */
Ipp64f vm_time_stop(vm_time_handle handle, vm_time *m);

/* Close the object of time measure */
vm_status vm_time_close(vm_time_handle *handle);

/* Unix style time related functions */
/* TZP must be NULL or VM_NOT_INITIALIZED status will returned */
/* !!! Time zone not supported, !!! vm_status instead of int will returned */
vm_status vm_time_gettimeofday( struct vm_timeval *TP, struct vm_timezone *TZP );

void vm_time_timeradd(struct vm_timeval* destination,  struct vm_timeval* src1, struct vm_timeval* src2);
void vm_time_timersub(struct vm_timeval* destination,  struct vm_timeval* src1, struct vm_timeval* src2);
int vm_time_timercmp(struct vm_timeval* src1, struct vm_timeval* src2, struct vm_timeval* threshold);

#ifdef __cplusplus
}
#endif

#endif
