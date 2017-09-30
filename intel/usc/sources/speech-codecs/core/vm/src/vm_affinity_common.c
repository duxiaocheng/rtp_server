/*
*
*                  INTEL CORPORATION PROPRIETARY INFORMATION
*     This software is supplied under the terms of a license agreement or
*     nondisclosure agreement with Intel Corporation and may not be copied
*     or disclosed except in accordance with the terms of that agreement.
*       Copyright(c) 2003-2013 Intel Corporation. All Rights Reserved.
*
*/

#if defined(_MSC_VER)
#pragma warning(disable:4206) // warning C4206: nonstandard extension used : translation unit is empty
#endif

#ifdef VM_AFFINITY_ENABLED
/*
//  Common functions for affinity API
//
//    CPU numbers started from 0
*/

#include <stdlib.h>
#include "vm_types.h"
#include "vm_sys_info.h"
#include "vm_thread.h"
#include "vm_affinity.h"

#include "ippcore.h"

/*
// prepare processors topology
*/

typedef struct
{
    Ipp32u EAX;
    Ipp32u EBX;
    Ipp32u ECX;
    Ipp32u EDX;
} CPUIDi;


#define MAXCACHELEVELS 4

static Ipp32s unitary_to_bin(Ipp32s val)
{
    Ipp32s i;
    Ipp32s rtv = 0;

    for (i = 0; i < 32; ++i)
        rtv += (val >> i) & 1;

    return rtv;
}

/*
//  move process to selected logical processor
//    - no corenumber check performed
//    - for internal needs only
*/

vm_status vm_affinity_thread_move_thread_to_logical_cpu(vm_thread* thr, Ipp32u corenumber)
{
    vm_status rtv = VM_OPERATION_FAILED;
    vm_affinity_mask* paffmask = NULL;

    if ((paffmask = vm_affinity_mask_init_alloc()) != NULL)
    {
        vm_affinity_mask_reset(paffmask);
        vm_affinity_mask_set_cpu(corenumber, paffmask);
        rtv = vm_affinity_thread_set(thr, paffmask);
        vm_affinity_mask_free(paffmask);
    }

    return rtv;
}

#ifdef NONALIGNMENT_PROBLEM
static void CCPU_Copy(VM_AFFINITY_CCPU* dst, VM_AFFINITY_CCPU* src)
{
    Ipp32s i = 0;

    dst[0].APICid  = src[0].APICid;
    dst[0].nshared = src[0].nshared;

    for( ; i < src[0].nshared; ++i)
        dst[0].CACHEsharedwith[i] = src[0].CACHEsharedwith[i];

    dst[0].isHTT       = src[0].isHTT;
    dst[0].ordernumber = src[0].ordernumber;
    dst[0].PackageId   = src[0].PackageId;
}
#endif

/*
//  return current topology description buffer or NULL in case of problems
*/

void ippGetReg(int* bfr, int eaxval, int ecxval)
{
    Ipp64u featuresmask = (Ipp64u)(ippCPUID_GETINFO_A);

    bfr[0] = eaxval;
    bfr[1] = bfr[3] = 0;
    bfr[2] = ecxval;

    ippGetCpuFeatures(&featuresmask, (Ipp32u*)bfr);
}

static vm_thread* work_thread = NULL;

static void vm_affinity_topology_thread(VM_AFFINITY_MACHINE_TOPOLOGY** bpt)
{
    Ipp32s logcores = 0, physcores = 0, distantcache = 0, distantthreads = 0;
    Ipp32s lcsize = 0; /* far cache size */
    Ipp32s i = 0, j = 0, k = 0, m = 0;
    Ipp32s current_package_Id = 0;
    CPUIDi binfo;
    Ipp8u count = 0;
    vm_affinity_mask *tmask;
    VM_AFFINITY_CCPU tmpC;
    //  VM_PID wpid;
    VM_AFFINITY_MACHINE_TOPOLOGY *rtv = NULL;
    tmask = vm_affinity_mask_init_alloc();
    tmpC.CACHEsharedwith = NULL;   /* temporary position - used for sorting */
    ippGetReg((int *)&binfo, 1, 0);
    logcores = vm_sys_info_get_avail_cpu_num(); /* only for OS enabled logical processors */

    if ((rtv = (VM_AFFINITY_MACHINE_TOPOLOGY *)malloc(sizeof(VM_AFFINITY_MACHINE_TOPOLOGY))) != NULL)
    {
        if ((rtv[0].cpuinfo = (VM_AFFINITY_CCPU *)malloc(sizeof(VM_AFFINITY_CCPU)*logcores)) != NULL)
        {
            rtv[0].ncpus = logcores;
            vm_sys_info_getpid();
            for( i = 0; i < rtv[0].ncpus; ++i)
            {
                if (vm_affinity_thread_move_thread_to_logical_cpu(work_thread, i) == VM_OK)
                {
                    rtv[0].cpuinfo[i].ordernumber = i;
                    rtv[0].cpuinfo[i].APICid = 0;
                    rtv[0].cpuinfo[i].CACHEsharedwith = NULL;
                    rtv[0].cpuinfo[i].isHTT = 0;
                    rtv[0].cpuinfo[i].nshared = 0;
                    rtv[0].cpuinfo[i].totalcaches = 0;

                    for(m = 0; m < VM_AFFINITY_MAX_CACHE_LEVELS; ++m)
                        rtv[0].cpuinfo[i].ccinfo[m].ctype = 0; /* preset no more caches */

                    ippGetReg((int *)&binfo, 1, 0);
                    /* check for HTT */
                    rtv[0].cpuinfo[i].isHTT = 0;
                    count = (Ipp8u)((binfo.EBX >> 16) & 0xFF); /* amount of cores/packages ? */
                    vm_affinity_thread_get(work_thread, tmask);
                    rtv[0].cpuinfo[i].APICid = (int)((binfo.EBX >> 24) & 0xFF);
                    ippGetReg((int *)&binfo, 4, 0);
                    physcores = (int)(((binfo.EAX >> 26) & 0x3F)+1);
                    rtv[0].cpuinfo[i].isHTT = (Ipp8u)(((count/physcores) >= 2) ? 1 : 0);

                    /* and now cache info and find the longest-distant shared cache */
                    j = 0;
                    do
                    {
                        ippGetReg((int *)&binfo,4, j);
                        rtv[0].cpuinfo[i].ccinfo[j].ctype = (Ipp8u)((binfo.EAX & 0x1F));
                        rtv[0].cpuinfo[i].ccinfo[j].level = (Ipp8u)((binfo.EAX >> 5) & 7);
                        rtv[0].cpuinfo[i].ccinfo[j].threadsserved = (Ipp32s)((binfo.EAX >> 14) & 0x3FF) + 1;
                        rtv[0].cpuinfo[i].ccinfo[j].csize = (((binfo.EBX >> 22)+1)*(((binfo.EBX >> 12) & 0x3FF)+1)*((binfo.EBX & 0xFFF)+1)*binfo.ECX)/1024+1;
                        if ((rtv[0].cpuinfo[i].ccinfo[j].threadsserved > 1) && (j >= distantcache))
                        {
                            distantcache = j;
                            distantthreads = rtv[0].cpuinfo[i].ccinfo[j].threadsserved;
                            lcsize = rtv[0].cpuinfo[i].ccinfo[j].csize;
                        }
                        ++j;
                    } while(rtv[0].cpuinfo[i].ccinfo[j-1].ctype != 0);

                    rtv[0].cpuinfo[i].totalcaches = (Ipp8u)(j-1);
                    rtv[0].cpuinfo[i].nshared = (logcores >= distantthreads) ? distantthreads : logcores;
                    rtv[0].cpuinfo[i].cachesize = lcsize;
                    rtv[0].cpuinfo[i].PackageId = rtv[0].cpuinfo[i].APICid >> unitary_to_bin(rtv[0].ncpus-1); /* rtv[0].cpuinfo[i].nshared; */
                } /* get single cpu info */
                else
                {
                    free(rtv[0].cpuinfo);
                    free(rtv);
                    rtv = NULL;
                    break;
                }
            } /* all cpus for loop */

            if (rtv != NULL)
            {
                /* sort according to package Id */
                current_package_Id = 0;
                if (rtv[0].cpuinfo[0].nshared > 1)
                {
                    for (i = 0; i < rtv[0].ncpus; i += rtv[0].cpuinfo[i].nshared)
                    {
                        for (j = 0; j < rtv[0].cpuinfo[i].nshared; ++j)
                        {
                            if (rtv[0].cpuinfo[i+j].PackageId != current_package_Id)
                            {
                            for (k = i+rtv[0].cpuinfo[i].nshared; k < rtv[0].ncpus; ++k)
                                if (rtv[0].cpuinfo[k].PackageId == current_package_Id)
                                {
                                memcpy(&tmpC, &rtv[0].cpuinfo[k], sizeof(VM_AFFINITY_CCPU));
                                memcpy(&rtv[0].cpuinfo[k], &rtv[0].cpuinfo[i+j], sizeof(VM_AFFINITY_CCPU));
                                memcpy(&rtv[0].cpuinfo[i+j], &tmpC, sizeof(VM_AFFINITY_CCPU));
                                }
                            }
                        }
                        ++current_package_Id;
                    }
                }

                /* all required info is collected at this point */
                if (rtv[0].cpuinfo[0].nshared > 1)
                {
                    if (rtv[0].cpuinfo[0].nshared > rtv[0].ncpus)
                    {
                        for( i = 0; i < rtv[0].ncpus; ++i)
                            rtv[0].cpuinfo[i].nshared = rtv[0].ncpus;
                    }

                    for (i = 0; i < rtv[0].ncpus; i += rtv[0].cpuinfo[i].nshared)
                    {
                        if ((j = rtv[0].cpuinfo[i].nshared) > 1)
                        {
                            while(j)
                            {
                                if ((rtv[0].cpuinfo[i+j-1].CACHEsharedwith = (Ipp32s *)malloc(sizeof(Ipp32s)*(rtv[0].cpuinfo[i].nshared))) == NULL)
                                {
                                    vm_affinity_topology_free(rtv);
                                    break;
                                }

                                for (k = i; k < rtv[0].cpuinfo[i+j-1].nshared+i; ++k)
                                    rtv[0].cpuinfo[i+j-1].CACHEsharedwith[k-i] = rtv[0].cpuinfo[k].ordernumber;

                                --j;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            free(rtv);
            rtv = NULL;
            vm_affinity_mask_free(tmask);
            bpt[0] = NULL;
            return;
        }
    }
    vm_affinity_mask_free(tmask);
    bpt[0] = rtv;
}


static VM_AFFINITY_MACHINE_TOPOLOGY *curcpu = NULL;

static void freetopo(void) { vm_affinity_topology_free(curcpu); }

VM_AFFINITY_MACHINE_TOPOLOGY* vm_affinity_topology_init_alloc(void)
{
    vm_thread wthr;
    Ipp32s i = 0;
    VM_AFFINITY_MACHINE_TOPOLOGY *bf = NULL;
    vm_thread_set_invalid(&wthr);
    work_thread = &wthr;

    if (vm_thread_create(&wthr, (vm_thread_callback)vm_affinity_topology_thread, &bf))
    {
        vm_thread_wait(&wthr);
        vm_thread_close(&wthr);
        // copy current cpu topology for internal needs - to obtain current CPU number for example
        if ((bf != NULL) && ((curcpu = (VM_AFFINITY_MACHINE_TOPOLOGY *)malloc(sizeof(VM_AFFINITY_MACHINE_TOPOLOGY))) != NULL))
        {
            if ((curcpu[0].cpuinfo = (VM_AFFINITY_CCPU *)malloc(sizeof(VM_AFFINITY_CCPU)*bf[0].ncpus)) != NULL)
            {
                curcpu[0].ncpus = bf[0].ncpus;
                for( ; i < curcpu[0].ncpus; ++i)
                {
                    curcpu[0].cpuinfo[i].APICid = bf[0].cpuinfo[i].APICid;
                    curcpu[0].cpuinfo[i].isHTT = bf[0].cpuinfo[i].isHTT;
                    curcpu[0].cpuinfo[i].nshared = bf[0].cpuinfo[i].nshared;
                    curcpu[0].cpuinfo[i].ordernumber = bf[0].cpuinfo[i].ordernumber;
                    curcpu[0].cpuinfo[i].PackageId = bf[0].cpuinfo[i].PackageId;

                    if((curcpu[0].cpuinfo[i].CACHEsharedwith = (Ipp32s *)malloc(sizeof(Ipp32s)*(curcpu[0].cpuinfo[i].nshared))) != NULL)
                        memcpy(curcpu[0].cpuinfo[i].CACHEsharedwith, bf[0].cpuinfo[i].CACHEsharedwith, sizeof(Ipp32s)*(curcpu[0].cpuinfo[i].nshared));
                    else
                    {
                        vm_affinity_topology_free(curcpu);
                        curcpu=NULL;
                        break;
                    }
                }
                atexit(freetopo);
            }
            else
            {
                vm_affinity_topology_free(curcpu);
                curcpu=NULL;
            }
        }
    }
    return bf;
}


void vm_affinity_topology_free(VM_AFFINITY_MACHINE_TOPOLOGY* bf)
{
    Ipp32s i;

    if (bf != NULL)
    {
        for(i = 0; i < bf[0].ncpus; ++i)
        {
            if (bf[0].cpuinfo[i].CACHEsharedwith != NULL)
            {
                free(bf[0].cpuinfo[i].CACHEsharedwith);
                bf[0].cpuinfo[i].CACHEsharedwith = NULL;
            }
        }

        free(bf[0].cpuinfo);
        free(bf);
    }
}

//
// return current CPU number for process or thread
//
Ipp32s vm_affinity_get_current_cpu_number(void)
{
    CPUIDi binfo;
    int i = 0;
    int apicid = 0;
    Ipp32s rtv = -1;

    if (curcpu == NULL)
        curcpu = vm_affinity_topology_init_alloc();

    if (curcpu != NULL)
    {
    ippGetReg((int *)&binfo, 1, 0);
    apicid = (int)((binfo.EBX >> 24) & 0xFF);

        for( ; i < curcpu[0].ncpus; ++i)
        {
            if (apicid == curcpu[0].cpuinfo[i].APICid)
            {
                rtv = curcpu[0].cpuinfo[i].ordernumber;
                break;
            }
        }
    }

    return rtv;
}

static vm_status vm_affinity_bit_action(unsigned int cpunumber, vm_affinity_mask *mptr, char action)
{
    vm_status rtv = VM_NULL_PTR;
    unsigned int whatword = 0;
    int whatcpu = 0;
    int len = 0;

    if (mptr)
    {
        len      = sizeof(mptr[0].mskbits[0]) << 3;
        whatword = cpunumber/len;
        whatcpu  = 1 << (cpunumber % len);

        if (mptr[0].msklen > whatword)
        {
            switch(action)
            {
            case '&' : mptr[0].mskbits[whatword] &= ~whatcpu;
                break;

            case '|' : mptr[0].mskbits[whatword] |= whatcpu;
                break;

            case '#' : vm_affinity_mask_reset(mptr);
                mptr[0].mskbits[whatword] |= whatcpu;
                break;
            }
            rtv = VM_OK;
        }
        else
            rtv = VM_OPERATION_FAILED;
    }

    return rtv;
}

vm_status vm_affinity_mask_free(vm_affinity_mask* mptr)
{
    vm_status rtv = VM_NULL_PTR;

    if (mptr)
    {
        if(mptr[0].mskbits)
            free(mptr[0].mskbits);

        free(mptr);
        rtv = VM_OK;
    }

    return rtv;
}

vm_status vm_affinity_mask_clear_cpu(unsigned int cpunumber, vm_affinity_mask* mptr)
{
    return vm_affinity_bit_action(cpunumber, mptr, '&');
}

vm_status vm_affinity_mask_set_cpu(unsigned int cpunumber, vm_affinity_mask* mptr)
{
    return vm_affinity_bit_action(cpunumber, mptr, '|');
}

vm_status vm_affinity_mask_reset(vm_affinity_mask* mptr)
{
    vm_status rtv = VM_NULL_PTR;
    unsigned i = 0;

    if (mptr)
    {
        for( ; i < mptr[0].msklen; ++i )
            mptr[0].mskbits[i] = 0;

        rtv = VM_OK;
    }

    return rtv;
}

#endif
