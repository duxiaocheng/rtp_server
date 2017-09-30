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
#include "vm_mutex.h"

/* Invalidate a mutex */
void vm_mutex_set_invalid(vm_mutex *mutex)
{
    /* check error(s) */
    if (NULL == mutex)
        return;

    memset(&mutex->sCritSection, 0, sizeof(CRITICAL_SECTION));
    mutex->iInited = 0;
}

/* Verify if a mutex is valid */
Ipp32s vm_mutex_is_valid(vm_mutex *mutex)
{
    /* check error(s) */
    if (NULL == mutex)
        return 0;

    return mutex->iInited;
}

/* Init a mutex, return 1 if successful */
vm_status vm_mutex_init(vm_mutex *mutex)
{
    /* check error(s) */
    if (NULL == mutex)
        return VM_NULL_PTR;

    vm_mutex_destroy(mutex);
    InitializeCriticalSection(&mutex->sCritSection);
    mutex->iInited = 1;
    return VM_OK;
}

/* Lock the mutex with blocking. */
vm_status vm_mutex_lock(vm_mutex *mutex)
{
    vm_status umcRes = VM_OK;

    /* check error(s) */
    if (NULL == mutex)
        return VM_NULL_PTR;

    if (mutex->iInited)
    {
        __try
        {
            EnterCriticalSection(&mutex->sCritSection);
        }
        __except ((GetExceptionCode() == STATUS_INVALID_HANDLE) ?  EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
        {
            umcRes = VM_OPERATION_FAILED;
        }
    }
    else
        umcRes = VM_NOT_INITIALIZED;

    return umcRes;
}

/* Unlock the mutex. */
vm_status vm_mutex_unlock(vm_mutex *mutex)
{
    /* check error(s) */
    if (NULL == mutex)
        return VM_NULL_PTR;

    if (mutex->iInited)
    {
        LeaveCriticalSection(&mutex->sCritSection);
        return VM_OK;
    }
    else
        return VM_NOT_INITIALIZED;
}

/* Lock the mutex without blocking, return 1 if success */
vm_status vm_mutex_try_lock(vm_mutex *mutex)
{
    /* check error(s) */
    if (NULL == mutex)
        return VM_NULL_PTR;

    if (mutex->iInited)
    {
        if (TryEnterCriticalSection(&mutex->sCritSection))
            return VM_OK;
        else
            return VM_OPERATION_FAILED;
    }
    else
        return VM_NOT_INITIALIZED;
}

/* Destroy the mutex */
void vm_mutex_destroy(vm_mutex *mutex)
{
    /* check error(s) */
    if (NULL == mutex)
        return;

    if (mutex->iInited)
    {
        mutex->iInited = 0;
        DeleteCriticalSection(&mutex->sCritSection);
    }
}
#endif
