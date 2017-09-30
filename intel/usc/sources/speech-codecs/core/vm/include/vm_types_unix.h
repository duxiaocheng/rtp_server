/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2013 Intel Corporation. All Rights Reserved.
//
*/

#ifndef VM_TYPES_UNIX_H
#define VM_TYPES_UNIX_H

#include <pthread.h>
#include <sys/types.h>
#include <semaphore.h>
#include <sys/time.h>

#ifdef __INTEL_COMPILER
/* ICC and Fedora Core 3 incompatibility */
#define __interface xxinterface
#include <netinet/in.h>
#undef __interface
#else
#include <netinet/in.h>
#endif

#include <ippcore.h>
#include <sys/socket.h>
#include <sys/select.h>

#define vm_main main
#define vm_sys_info_getpid getpid

#ifdef __cplusplus
extern "C" {
#endif

#define ENABLE_COUNTER          100
#define DISABLE_COUNTER         101
#define GET_TSC_LOW             102
#define GET_TSC_HIGH            103

#define VM_ALIGN_DECL(X,Y) __attribute__ ((aligned(X))) Y

#define CONST_LL(X) X##LL
#define CONST_ULL(X) X##ULL

#define vm_timeval timeval
#define vm_timezone timezone

#define vm_thread_handle pthread_t

/* vm_event.h */
typedef struct vm_event
{
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    Ipp32s manual;
    Ipp32s state;
} vm_event;

/* vm_mmap.h */
typedef struct vm_mmap
{
    Ipp32s fd;
    void *address;
    size_t sizet;
    Ipp32s fAccessAttr;
} vm_mmap;

/* vm_mutex.h */
typedef struct vm_mutex
{
    pthread_mutex_t handle;
    Ipp32s is_valid;
} vm_mutex;

/* vm_semaphore.h */
typedef struct vm_semaphore
{
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    Ipp32s count;
} vm_semaphore;

/* vm_thread.h */
typedef struct vm_thread
{
    pthread_t handle;
    Ipp32s is_valid;
    Ipp32u (*p_thread_func)(void *);
    void *p_arg;
    vm_event exit_event;
    vm_mutex access_mut;
    Ipp32s i_wait_count;
//    Ipp32s selected_cpu;
} vm_thread;

/* vm_socket.h */
#define VM_SOCKET_QUEUE 20
typedef struct vm_socket
{
   fd_set r_set, w_set;
   Ipp32s chns[VM_SOCKET_QUEUE+1];
    struct sockaddr_in sal;
   struct sockaddr_in sar;
   struct sockaddr_in peers[VM_SOCKET_QUEUE+1];
   Ipp32s flags;
} vm_socket;

typedef struct vm_time
{
   Ipp64s start;
   Ipp64s diff;
   Ipp64s freq;
} vm_time;

#ifdef __cplusplus
}
#endif

#endif
