/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2013 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __VM_SYS_INFO_H__
#define __VM_SYS_INFO_H__

#include "vm_types.h"
#include "vm_time.h"

#if defined WINDOWS
#define OLDWINNT 0

#if _MSC_VER >= 1400
#pragma warning(disable: 4996)
#endif

/* for performance monitoring */
#if _WIN32_WINNT < 0x501
#undef OLDWINNT
#define OLDWINNT _WIN32_WINNT
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x501
#endif
#include <pdh.h>
#include <pdhmsg.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Processor's feature bits */
enum
{
    CMOV_FEATURE                = 0x00001,
    MMX_FEATURE                 = 0x00002,
    MMX_EXT_FEATURE             = 0x00004,
    SSE_FEATURE                 = 0x00008,
    SSE2_FEATURE                = 0x00010,
    SSE3_FEATURE                = 0x00020,

    UNK_MANUFACTURE             = 0x00000,
    INTEL_MANUFACTURE           = 0x10000
};

/* Processors ID list */
enum
{
    UNKNOWN_CPU                 = 0,

    PENTIUM                     = INTEL_MANUFACTURE,
    PENTIUM_PRO                 = PENTIUM | CMOV_FEATURE,
    PENTIUM_MMX                 = PENTIUM | MMX_FEATURE,
    PENTIUM_2                   = PENTIUM_MMX | CMOV_FEATURE,
    PENTIUM_3                   = PENTIUM_2 | MMX_EXT_FEATURE | SSE_FEATURE,
    PENTIUM_4                   = PENTIUM_3 | SSE2_FEATURE,
    PENTIUM_4_PRESCOTT          = PENTIUM_4 | SSE3_FEATURE
};

typedef enum
{
    DDMMYY = 0,
    MMDDYY = 1,
    YYMMDD = 2
} DateFormat;

typedef enum
{
    HHMMSS      = 0,
    HHMM        = 1,
    HHMMSSMS1   = 2,
    HHMMSSMS2   = 3,
    HHMMSSMS3   = 4
} TimeFormat;

/* structures to obtain processor loading info */
#if defined UNIX
#define MAXLOADENTRIES 8
#endif

struct VM_SYSINFO_CPULOAD_ENTRY
{
    /* all are floats 0. - 1. */
    float usrload;
    float sysload;
    float idleload;
    /* unixes specific info - may be useful */
    float usrniceload;
    float iowaitsload;
    float irqsrvload;
    float softirqsrvload;
    /* time to sleep due to processor "occupation" by another OS under Xen */
    float vmstalled;
#if defined UNIX
    Ipp64s ldbuffer[MAXLOADENTRIES];
#endif
};

#if defined WINDOWS
#define MAX_PERF_PARAMETERS 3
#endif

struct VM_SYSINFO_CPULOAD
{
    Ipp32s    ncpu;
    vm_tick   tickspassed;
    struct VM_SYSINFO_CPULOAD_ENTRY *cpudes;

#if defined WINDOWS
    /* additional fields to hold system specific info for Windows */
    HQUERY        CpuLoadQuery;
    PDH_HCOUNTER *CpuPerfCounters[MAX_PERF_PARAMETERS];   /* we will receive only 3 parameters: user time, system time and interrupt time */
#endif
};

/* Functions to obtain processor's specific information */
void vm_sys_info_get_cpu_name(vm_char *cpu_name);
void vm_sys_info_get_date(vm_char *m_date, DateFormat df);
void vm_sys_info_get_time(vm_char *m_time, TimeFormat tf);
void vm_sys_info_get_vga_card(vm_char *vga_card);
void vm_sys_info_get_os_name(vm_char *os_name);
void vm_sys_info_get_computer_name(vm_char *computer_name);
void vm_sys_info_get_program_name(vm_char *program_name);
void vm_sys_info_get_program_path(vm_char *program_path);
void vm_sys_info_get_program_description(vm_char *program_description);
Ipp32u vm_sys_info_get_cpu_speed(void);
Ipp32u vm_sys_info_get_mem_size(void);
Ipp32u vm_sys_info_get_avail_cpu_num(void); 
Ipp32u vm_sys_info_get_cpu_num(void);
VM_PID vm_sys_info_getpid(void);

#ifdef VM_RESOURCE_USAGE_FUNCTIONS
#include <psapi.h>
/* functions to obtain process and thread specific information */
/*
 *  Return process memory size in kibibytes */
Ipp32u vm_sys_info_get_process_memory_usage(VM_PID process_ID);
#endif

/*
 * CPU loading information functions are not available for Linux and 
 * other OSes yet
 */

/*
 * CPU loading information obtaining functions
 */
struct VM_SYSINFO_CPULOAD *vm_sys_info_create_cpu_data(void);

/*
 * For Unices dbf == NULL is used to start new timing interval for loading evaluation
 */
vm_status vm_sys_info_cpu_loading_avg(struct VM_SYSINFO_CPULOAD *dbuf, Ipp32u timeslice);

#if defined WINDOWS

#if OLDWINNT > 0
#undef _WIN32_WINNT
#define _WIN32_WINNT OLDWINNT
#undef OLDWINNT
#define OLDWINNT 0
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif
