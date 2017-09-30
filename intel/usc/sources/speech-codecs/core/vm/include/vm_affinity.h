/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2013 Intel Corporation. All Rights Reserved.
//
*/

/*
//  VM library changes :
//       
//    - cpu loading functions will be removed from vm_sysinfo... a bit later
//    - compilation condition VM_AFFINITY_ENABLED will be added
//    - compilation condition VM_NPTL_WITH_AFFINITY required to correct work
//      under various NPTL versions  (may be checked by <<getconf GNU_LIBPTHREAD_VERSION>>) on Linux
//    - special VM_AFFINITY_OLD_NPTL thread affinity for Linux NPTL up to 2.4
//    - vm_affinity.h, vm_affinity_windows.c, vm_affinity_unix.c and vm_affinity_common.c is going to be added.
//    
//    all functions in affinity block (except cpu loading evaluation) will have the vm_affinity prefix
*/

/*
//  Two types of affinity are supported -- process affinity and thread affinity
*/

/*
//     A little usage illustration - move existing thread to selected core if we have one it will be thr.
//     0. create vm_thread as vm_thread thr
//     1. prepare working mask
//          vm_affinity_mask *paffmask = NULL;
//          if ((paffmask = vm_affinity_mask_create()) != NULL) {
//     2. obtain amount of available CPUs and select the required CPU for this thread
//            if (vm_affinity_thread_get(pthr, paffmask) == VM_OK) {
//     3. imagine you want core 2
//                if (paffmask.mskbits[0] & 2) {
//                  vm_affinity_mask_reset(paffmask);
//                  vm_affinity_mask_set_cpu(1, paffmask);
//                  vm_affinity_thread_set(thr, paffmask);  -- returning status may be checked -- VM_OK required
//                  }
//              }            
//            }
//     E. destroy affinity mask if it isn't used.
//          vm_status vm_affinity_mask_destroy(paffmask);
//
*/

/*
//  Affinity internals:
*/

#ifdef VM_AFFINITY_ENABLED

#ifndef __VM_AFFINITY_H__
#define __VM_AFFINITY_H__

#ifdef __cplusplus
extern "C" {
#endif

/* unified affinity mask */
typedef struct
{
    unsigned long *mskbits; /* unitary code (bitset) */
    unsigned int msklen;
} vm_affinity_mask;

/* logical CPU description */
#define VM_AFFINITY_MAX_CACHE_LEVELS 8

typedef struct
{
    Ipp8u  level;
    Ipp8u  ctype;
    Ipp32s csize;
    Ipp32s threadsserved;

} CacheInfo;

typedef struct
{
    Ipp8u isHTT;              /* HyperThread activity flag */
    Ipp32s ordernumber;       /* CPU number to use in affinity functions */
    Ipp32s APICid;
    Ipp32s PackageId;
    CacheInfo ccinfo[VM_AFFINITY_MAX_CACHE_LEVELS];
    Ipp32s cachesize;         /* long distance cache size */
    Ipp32s nshared;           /* amount of neighbors to share cache nearest cache */
    Ipp8u  totalcaches;
    Ipp32s *CACHEsharedwith;  /* neighbors numbers */

} VM_AFFINITY_CCPU;

typedef struct VM_AFFINITY_MACHINE_TOPOLOGY
{
    Ipp32s ncpus;
    VM_AFFINITY_CCPU* cpuinfo;
} VM_AFFINITY_MACHINE_TOPOLOGY;

/* affinity mask manipulations */

VM_AFFINITY_MACHINE_TOPOLOGY* vm_affinity_topology_init_alloc(void);
void vm_affinity_topology_free(VM_AFFINITY_MACHINE_TOPOLOGY* bf);

/* 
//  if whole mask will be created  on the heap 
*/
vm_status vm_affinity_mask_free(vm_affinity_mask* mptr);
vm_affinity_mask* vm_affinity_mask_init_alloc(void);

/* 
//  bit manipulations 
*/
vm_status vm_affinity_mask_clear_cpu(unsigned int cpunumber, vm_affinity_mask* mptr);
vm_status vm_affinity_mask_set_cpu(unsigned int cpunumber, vm_affinity_mask* mptr);
vm_status vm_affinity_mask_reset(vm_affinity_mask* mptr);

vm_thread_handle vm_affinity_thread_get_current(void);

/*
//  CPU manipulations 
//  process affinity
*/

/* cpu number will be returned or -1 if it is impossible to obtain cpu number */
Ipp32s vm_affinity_get_current_cpu_number(void);

/* process affinity functions */
vm_status vm_affinity_process_set(VM_PID pid, vm_affinity_mask* mptr);
vm_status vm_affinity_process_get(VM_PID pid, vm_affinity_mask* mptr);

/* thread affinity */
vm_status vm_affinity_thread_set(vm_thread* pthr, vm_affinity_mask* mptr);
vm_status vm_affinity_thread_get(vm_thread* pthr, vm_affinity_mask* mptr);

#ifdef __cplusplus
}
#endif

#endif
#endif
