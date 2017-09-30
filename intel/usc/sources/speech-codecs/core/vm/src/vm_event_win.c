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
#include "vm_event.h"

/* Invalidate an event */
void vm_event_set_invalid(vm_event *event)
{
    /* check error(s) */
    if (NULL == event)
        return;

    event->handle = NULL;
}

/* Verify if the event handle is valid */
Ipp32s vm_event_is_valid(vm_event *event)
{
    /* check error(s) */
    if (NULL == event)
        return 0;

    return (Ipp32s)(NULL != event->handle);
}

/* Init an event. Event is created unset. return 1 if success */
vm_status vm_event_init(vm_event *event, Ipp32s manual, Ipp32s state)
{
    /* check error(s) */
    if (NULL == event)
        return VM_NULL_PTR;

    vm_event_destroy(event);
    event->handle = CreateEvent(NULL, manual, state, NULL);

    if (NULL == event->handle)
        return VM_OPERATION_FAILED;
    else
        return VM_OK;
}

/* Set the event to either HIGH (1) or LOW (0) state */
vm_status vm_event_signal(vm_event *event)
{
    /* check error(s) */
    if (NULL == event)
        return VM_NULL_PTR;

    if (NULL == event->handle)
        return VM_NOT_INITIALIZED;
    else if (SetEvent(event->handle))
        return VM_OK;
    else
        return VM_OPERATION_FAILED;
}

/* Set the event to either HIGH (1) or LOW (0) state */
vm_status vm_event_reset(vm_event *event)
{
    /* check error(s) */
    if (NULL == event)
        return VM_NULL_PTR;

    if (NULL == event->handle)
        return VM_NOT_INITIALIZED;
    else if (ResetEvent(event->handle))
        return VM_OK;
    else
        return VM_OPERATION_FAILED;
}

/* Pulse the event 0 -> 1 -> 0 */
vm_status vm_event_pulse(vm_event *event)
{
    /* check error(s) */
    if (NULL == event)
        return VM_NULL_PTR;

    if (NULL == event->handle)
        return VM_NOT_INITIALIZED;
    else if (PulseEvent(event->handle))
        return VM_OK;
    else
        return VM_OPERATION_FAILED;
}

/* Wait for event to be high with blocking */
vm_status vm_event_wait(vm_event *event)
{
    /* check error(s) */
    if (NULL == event)
        return VM_NULL_PTR;

    if (NULL == event->handle)
        return VM_NOT_INITIALIZED;
    else if (WAIT_OBJECT_0 == WaitForSingleObject(event->handle, INFINITE))
        return VM_OK;
    else
        return VM_OPERATION_FAILED;
}

/* Wait for event to be high without blocking, return 1 if signaled */
vm_status vm_event_timed_wait(vm_event *event, Ipp32u msec)
{
    vm_status umcRes = VM_NOT_INITIALIZED;

    /* check error(s) */
    if (NULL == event)
        return VM_NULL_PTR;

    if (NULL != event->handle)
    {
        Ipp32u dwRes = WaitForSingleObject(event->handle, msec);

        if (WAIT_OBJECT_0 == dwRes)
            umcRes = VM_OK;
        else if (WAIT_TIMEOUT == dwRes)
            umcRes = VM_TIMEOUT;
        else
            umcRes = VM_OPERATION_FAILED;
    }
    return umcRes;
}

/* Destroy the event */
void vm_event_destroy(vm_event *event)
{
    /* check error(s) */
    if (NULL == event)
        return;

    if (event->handle)
    {
        CloseHandle(event->handle);
        event->handle = NULL;
    }
}

#endif
