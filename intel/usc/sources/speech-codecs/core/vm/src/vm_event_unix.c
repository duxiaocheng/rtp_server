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
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>

#include "vm_event.h"

/* Invalidate an event */
void vm_event_set_invalid(vm_event *event)
{
    /* check error(s) */
    if (NULL == event)
        return;

    event->state= -1;
}

/* Verify if an event is valid */
Ipp32s vm_event_is_valid(vm_event *event)
{
    /* check error(s) */
    if (NULL == event)
        return 0;

    return event->state >= 0;
}

/* Init an event. Event is created unset. return 1 if success */
vm_status vm_event_init(vm_event *event, Ipp32s manual, Ipp32s state)
{
    /* check error(s) */
    if(NULL == event)
        return VM_NULL_PTR;

    vm_event_destroy(event);

    event->manual = manual;
    event->state = state ? 1 : 0;
    pthread_cond_init(&event->cond, 0);
    pthread_mutex_init(&event->mutex, 0);

    return VM_OK;
}

/* Set the event to either HIGH (1) or LOW (0) state */
vm_status vm_event_signal(vm_event *event)
{
    vm_status umc_status = VM_NOT_INITIALIZED;

    /* check error(s) */
    if (NULL == event)
        return VM_NULL_PTR;

    if (0 <= event->state)
    {
        pthread_mutex_lock(&event->mutex);
        if (0 == event->state)
        {
            event->state = 1;
            if (event->manual)
                pthread_cond_broadcast(&event->cond);
            else
                pthread_cond_signal(&event->cond);
        }
        umc_status = VM_OK;
        pthread_mutex_unlock(&event->mutex);
    }
    return umc_status;
}

vm_status vm_event_reset(vm_event *event)
{
    vm_status umc_status = VM_NOT_INITIALIZED;

    /* check error(s) */
    if (NULL == event)
        return VM_NULL_PTR;

    if (0 <= event->state)
    {
        pthread_mutex_lock(&event->mutex);

        if (1 == event->state)
            event->state = 0;

        pthread_mutex_unlock(&event->mutex);
        umc_status = VM_OK;
    }
    return umc_status;
}

/* Pulse the event 0 -> 1 -> 0 */
vm_status vm_event_pulse(vm_event *event)
{
    vm_status umc_status = VM_NOT_INITIALIZED;

    /* check error(s) */
    if (NULL == event)
        return VM_NULL_PTR;

    if (0 <= event->state)
    {
        pthread_mutex_lock(&event->mutex);

        if (event->manual)
            pthread_cond_broadcast(&event->cond);
        else
            pthread_cond_signal(&event->cond);

        event->state = 0;
        pthread_mutex_unlock(&event->mutex);
        umc_status = VM_OK;
    }
    return umc_status;
}

/* Wait for event to be high with blocking */
vm_status vm_event_wait(vm_event *event)
{
    vm_status umc_status = VM_NOT_INITIALIZED;

    /* check error(s) */
    if (NULL == event)
        return VM_NULL_PTR;

    if (0 <= event->state)
    {
        pthread_mutex_lock(&event->mutex);

        if (!event->state)
            pthread_cond_wait(&event->cond,&event->mutex);

        if (!event->manual)
            event->state = 0;

        pthread_mutex_unlock(&event->mutex);
        umc_status = VM_OK;
    }
    return umc_status;
}

/* Wait for event to be high without blocking, return 1 if successful */
vm_status vm_event_timed_wait(vm_event *event, Ipp32u msec)
{
    vm_status umc_status = VM_NOT_INITIALIZED;

    /* check error(s) */
    if (NULL == event)
        return VM_NULL_PTR;

    if (0 <= event->state)
    {
        pthread_mutex_lock(&event->mutex);

        if (0 == event->state)
        {
            struct timeval tval;
            struct timespec tspec;
            Ipp32s i_res;

            gettimeofday(&tval, NULL);
            msec = 1000 * msec + tval.tv_usec;
            tspec.tv_sec = tval.tv_sec + msec / 1000000;
            tspec.tv_nsec = (msec % 1000000) * 1000;
            i_res = pthread_cond_timedwait(&event->cond,
                                           &event->mutex,
                                           &tspec);
            if (0 == i_res)
                umc_status = VM_OK;
            else if (ETIMEDOUT == i_res)
                umc_status = VM_TIMEOUT;
            else
                umc_status = VM_OPERATION_FAILED;
        }
        else
            umc_status = VM_OK;

        if (!event->manual)
            event->state = 0;

        pthread_mutex_unlock(&event->mutex);
    }
    return umc_status;
}

/* Destory the event */
void vm_event_destroy(vm_event *event)
{
    /* check error(s) */
    if (NULL == event)
        return;

    if (event->state >= 0)
    {
        pthread_cond_destroy(&event->cond);
        pthread_mutex_destroy(&event->mutex);
        event->state= -1;
    }
}
#endif
