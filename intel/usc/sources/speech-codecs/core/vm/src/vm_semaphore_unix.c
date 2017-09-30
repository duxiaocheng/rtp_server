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
#include <semaphore.h>
#include <sys/time.h>
#include <errno.h>
#include "vm_semaphore.h"
#include <stdio.h>

/* Invalidate a semaphore */
void vm_semaphore_set_invalid(vm_semaphore *sem)
{
    /* check error(s) */
    if (NULL == sem)
        return;

    sem->count = -1;
}

/* Verify if a semaphore is valid */
Ipp32s vm_semaphore_is_valid(vm_semaphore *sem)
{
    /* check error(s) */
    if (NULL == sem)
        return 0;

    return (-1 < sem->count);
}

/* Init a semaphore with value */
vm_status vm_semaphore_init(vm_semaphore *sem, Ipp32s init_count)
{
    /* check error(s) */
    if (NULL == sem)
        return VM_NULL_PTR;

    sem->count = init_count;
    pthread_cond_init(&sem->cond, 0);
    pthread_mutex_init(&sem->mutex,0);

    return VM_OK;
}

/* Decrease the semaphore value with blocking. */
vm_status vm_semaphore_timedwait(vm_semaphore *sem, Ipp32u msec)
{
    vm_status umc_status = VM_NOT_INITIALIZED;

    /* check error(s) */
    if (NULL == sem)
        return VM_NULL_PTR;

    if (0 <= sem->count)
    {
        umc_status = VM_OK;
        pthread_mutex_lock(&sem->mutex);

        if (0 == sem->count)
        {
            struct timeval tval;
            struct timespec tspec;
            Ipp32s i_res;

            gettimeofday(&tval, NULL);
            msec = 1000 * msec + tval.tv_usec;
            tspec.tv_sec = tval.tv_sec + msec / 1000000;
            tspec.tv_nsec = (msec % 1000000) * 1000;
            i_res = pthread_cond_timedwait(&sem->cond, &sem->mutex, &tspec);

            if (ETIMEDOUT == i_res)
                umc_status = VM_TIMEOUT;
            else if (0 != i_res)
                umc_status = VM_OPERATION_FAILED;
        }

        if (VM_OK == umc_status)
            sem->count--;

        pthread_mutex_unlock(&sem->mutex);
    }
    return umc_status;
}

/* Decrease the semaphore value with blocking. */
vm_status vm_semaphore_wait(vm_semaphore *sem)
{
    vm_status umc_status = VM_NOT_INITIALIZED;

    /* check error(s) */
    if (NULL == sem)
        return VM_NULL_PTR;

    if (0 <= sem->count)
    {
        umc_status = VM_OK;
        pthread_mutex_lock(&sem->mutex);

        if (0 == sem->count && 0 != pthread_cond_wait(&sem->cond, &sem->mutex))
            umc_status = VM_OPERATION_FAILED;

        if (VM_OK == umc_status)
            sem->count--;

        pthread_mutex_unlock(&sem->mutex);
    }
    return umc_status;
}

/* Decrease the semaphore value without blocking, return 1 if success */
vm_status vm_semaphore_try_wait(vm_semaphore *sem)
{
    vm_status umc_status = VM_NOT_INITIALIZED;

    /* check error(s) */
    if (NULL == sem)
        return VM_NULL_PTR;

    if (0 <= sem->count)
    {
        pthread_mutex_lock(&sem->mutex);
        if (0 == sem->count)
            umc_status = VM_TIMEOUT;
        else
        {
            sem->count--;
            umc_status = VM_OK;
        }
        pthread_mutex_unlock(&sem->mutex);
    }
    return umc_status;
}

/* Increase the semaphore value */
vm_status vm_semaphore_post(vm_semaphore *sem)
{
    vm_status umc_status = VM_NOT_INITIALIZED;

    /* check error(s) */
    if (NULL == sem)
        return VM_NULL_PTR;

    if (0 <= sem->count)
    {
        pthread_mutex_lock(&sem->mutex);

        if (0 == sem->count++)
            pthread_cond_signal(&sem->cond);

        pthread_mutex_unlock(&sem->mutex);
        umc_status = VM_OK;
    }
    return umc_status;
}

/* Destory a semaphore */
void vm_semaphore_destroy(vm_semaphore *sem)
{
    /* check error(s) */
    if (NULL == sem)
        return;

    if (0 <= sem->count)
    {
        pthread_cond_destroy(&sem->cond);
        pthread_mutex_destroy(&sem->mutex);
        sem->count = -1;
    }
}
#endif
