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

#ifdef UNIX
#include "vm_sys_info.h"
#include "vm_file.h"
#include "vm_mutex.h"
#include <time.h>
#include <sys/utsname.h>
#include <stdlib.h>
#include <unistd.h>

#if defined OSX || defined __FreeBSD__
#include <sys/syslimits.h>
#include <sys/resource.h>
/* to access to sysctl function */
#include <sys/types.h>
#include <sys/sysctl.h>

#if defined __FreeBSD__
#define FTIMESCOPY(TO, FROM)       \
    TO.ldbuffer[0] = FROM[CP_USER];  \
    TO.ldbuffer[1] = FROM[CP_NICE];  \
    TO.ldbuffer[2] = FROM[CP_SYS];   \
    TO.ldbuffer[3] = FROM[CP_IDLE];  \
    TO.ldbuffer[4] = FROM[CP_INTR];  \
    TO.ldbuffer[5] = TO.ldbuffer[6] = TO.ldbuffer[7] = -1
#endif

/*
 * retrieve information about integer parameter
 * defined as CTL_... (ctl_class) . ENTRY (ctl_entry)
 * return value in res.
 *  status : 1  - OK
 *           0 - operation failed
 */
Ipp32u osx_sysctl_entry_32u( int ctl_class, int ctl_entry, Ipp32u *res )
{
    int dcb[2];
    size_t i;
    dcb[0] = ctl_class;  dcb[1] = ctl_entry;
    i = sizeof(res[0]);
    return (sysctl(dcb, 2, res, &i, NULL, 0) != -1) ? 1 : 0;
}
#else
#include <sys/sysinfo.h>

/* return passed time in millisecond ticks */
static vm_tick msecond_divider = 0;
float clockweight = 0.;

static vm_tick get_mseconds(void)
{
    if (0 == msecond_divider)
        msecond_divider = vm_sys_info_get_cpu_speed() / 1000;
    return vm_time_get_tick()/msecond_divider;
}

static Ipp64s get_next_tick_value(char **s)
{
    char *estr = NULL;
    Ipp64s rtv = -1;
    while(' ' == *s[0])
    {
        if (('\0' == *s[0]) || ('\n' == *s[0]))
            break;
        else
            ++s[0];
    }
    if (('\0' != *s[0]) && ('\n' != *s[0]))
    {
        rtv = strtol(s[0], &estr, 10);
        s[0] = estr;
    }
    return rtv;
}

static void parse_linux_cpu_loading_average(FILE *stat_file, struct VM_SYSINFO_CPULOAD_ENTRY *p)
{
    char lnbuffer[MAX_PATH];
    char *str = NULL;
    int i = 0;
    if ((NULL != stat_file)&&(NULL != p))
    {
        p[0].usrload = p[0].sysload = 0.;
        p[0].idleload = p[0].usrniceload = 0.;
        p[0].iowaitsload = p[0].irqsrvload = 0.;
        p[0].softirqsrvload = p[0].vmstalled = 0.;
        fgets(lnbuffer, MAX_PATH, stat_file);
        if ((str = strstr(lnbuffer, "cpu")) != NULL)
        {
                while ((' ' != *str)&&('\0' != *str)&&('\n' != *str))
                    ++str;
                if (' ' == *str)
                {
                    while(-1 != (p[0].ldbuffer[i] = get_next_tick_value(&str)))
                        ++i;
                }
        }
    }
}
#endif

/* previous CPU loading information  */
/* not initialized yet if ticks == -1 */
static struct VM_SYSINFO_CPULOAD prevCPUdata = {0, -1, NULL};
static vm_mutex prevCPULck;

static void cpu_data_cleanup(void)
{
    if(NULL != prevCPUdata.cpudes)
    {
        vm_mutex_lock(&prevCPULck);
        free(prevCPUdata.cpudes);
        prevCPUdata.cpudes = NULL;
        prevCPUdata.ncpu = 0;
        prevCPUdata.tickspassed = 0;
        vm_mutex_unlock(&prevCPULck);
        vm_mutex_set_invalid(&prevCPULck);
    }
}

static vm_status fill_cpu_loading_info(struct VM_SYSINFO_CPULOAD *dbuf)
{
    vm_status rtv = VM_OPERATION_FAILED;
    int i;

#ifdef __FreeBSD__
    long *timesb = NULL;
    long *tptr = NULL;
    long timea[CPUSTATES];
    size_t len = 0;
#endif

#if (!defined OSX) && (!defined __FreeBSD__)
    //Ipp64s tickstotal = 0;
    FILE *stfile=NULL;
    if(clockweight == 0)
    {
#ifdef _SC_CLK_TCK
        clockweight = 1./(float)sysconf(_SC_CLK_TCK);
#else
        clockweight = 0.01;
#endif
    }

    do
    {
        if(dbuf == NULL)
            break;
        stfile = fopen("/proc/stat","r");
        if (stfile == NULL)
            break;
        /* for all available CPUs + summary at position 0 */
        for (i = 0; i < dbuf[0].ncpu; ++i)
            parse_linux_cpu_loading_average(stfile, &dbuf[0].cpudes[i]);
        dbuf[0].tickspassed = get_mseconds();
        rtv = VM_OK;
    } while(0);

    if(stfile != NULL)
        fclose(stfile);
#endif

#ifdef __FreeBSD__
    /* for all available CPUs + summary at position 0 */
    if((timesb = (long *)malloc(CPUSTATES*sizeof(long)*dbuf[0].ncpu)) != NULL)
    {
        tptr = timesb;
        len = sizeof(long)*CPUSTATES;
        if(sysctlbyname("kern.cp_time", timea, &len, NULL, 0) == 0)
        {
            FTIMESCOPY(dbuf[0].cpudes[0], timea);
        }
        len = CPUSTATES*sizeof(long)*dbuf[0].ncpu;
        if (sysctlbyname("kern.cp_times", timesb, &len, NULL, 0) == 0)
        {
            for (i = 1; i < dbuf[0].ncpu; ++i)
            {
                FTIMESCOPY(dbuf[0].cpudes[i], tptr);
                tptr += CPUSTATES;
            }
        }
        free(timesb);
    }
    dbuf[0].tickspassed = get_mseconds();
    rtv = VM_OK;
#endif

    return rtv;
}

static void prepare_previous_cpu_data(void)
{
    atexit(cpu_data_cleanup);
    vm_mutex_set_invalid(&prevCPULck);
    if (VM_OK == vm_mutex_init(&prevCPULck))
    {
        vm_mutex_lock(&prevCPULck);
        prevCPUdata.ncpu = vm_sys_info_get_avail_cpu_num()+1;
        prevCPUdata.cpudes = (struct VM_SYSINFO_CPULOAD_ENTRY *)malloc(sizeof(struct VM_SYSINFO_CPULOAD_ENTRY)*prevCPUdata.ncpu);
        prevCPUdata.tickspassed = 0;
        fill_cpu_loading_info(&prevCPUdata);
        vm_mutex_unlock(&prevCPULck);
    }
}

#define LDPOINT(A) \
  (-1 != dbuf[0].cpudes[i].ldbuffer[A]) ? dbuf[0].cpudes[i].ldbuffer[A]/(float)tickstotal : 0

static void prepare_actual_cpu_loading_info(struct VM_SYSINFO_CPULOAD *dbuf)
{
    int i = 0, j  = 0;
    Ipp64s tickstotal = 0;
    for( ; i < dbuf[0].ncpu; ++i)
    {
        for( j = 0; j < MAXLOADENTRIES; ++j)
        {
            if ((-1 != dbuf[0].cpudes[i].ldbuffer[j])&&(-1 != prevCPUdata.cpudes[i].ldbuffer[j])) 
            {
                dbuf[0].cpudes[i].ldbuffer[j] = dbuf[0].cpudes[i].ldbuffer[j]-prevCPUdata.cpudes[i].ldbuffer[j];
                tickstotal += dbuf[0].cpudes[i].ldbuffer[j];
            }
        }
        dbuf[0].cpudes[i].usrload        = LDPOINT(0);
        dbuf[0].cpudes[i].usrniceload    = LDPOINT(1);
        dbuf[0].cpudes[i].sysload        = LDPOINT(2);
        dbuf[0].cpudes[i].idleload       = LDPOINT(3);
        dbuf[0].cpudes[i].iowaitsload    = LDPOINT(4);
        dbuf[0].cpudes[i].irqsrvload     = LDPOINT(5);
        dbuf[0].cpudes[i].softirqsrvload = LDPOINT(6);
        tickstotal = 0;
    }
}

vm_status vm_sys_info_cpu_loading_avg(struct VM_SYSINFO_CPULOAD *dbuf, Ipp32u timeslice)
{
    vm_status rtv = VM_OPERATION_FAILED;
    if (NULL == dbuf)
    {
        prepare_previous_cpu_data();
        rtv = VM_OK;
    }
    else
    {
        if (prevCPUdata.tickspassed == -1)
            prepare_previous_cpu_data();

        if ((NULL == dbuf[0].cpudes) && (-1 == dbuf[0].tickspassed))
        {
            dbuf[0].ncpu = vm_sys_info_get_avail_cpu_num()+1;
            dbuf[0].cpudes = (struct VM_SYSINFO_CPULOAD_ENTRY *)malloc(sizeof(struct VM_SYSINFO_CPULOAD_ENTRY)*dbuf[0].ncpu);
            dbuf[0].tickspassed = 0;
        }

        if ((NULL != dbuf[0].cpudes) &&(fill_cpu_loading_info(dbuf) == VM_OK))
        {
            /* if timslice == 0 or timeslice < tickspassed
            then return current cpu loading information
            with VM_NOT_ENOUGH_DATA sign else */
            rtv = ((0 == timeslice) || ((dbuf[0].tickspassed - prevCPUdata.tickspassed) < timeslice)) ? VM_NOT_ENOUGH_DATA : VM_OK;
            prepare_actual_cpu_loading_info(dbuf);
            rtv = VM_OK;
        }
    }
    return rtv;
}

void vm_sys_info_get_date(vm_char *m_date, DateFormat df)
{
    time_t ltime;
    struct tm *today;
    size_t len = 0;

    /* check error(s) */
    if (NULL == m_date)
        return;

    len = vm_string_strlen(m_date);

    time(&ltime);
    today = localtime(&ltime);

    switch (df)
    {
    case DDMMYY:
        strftime(m_date, len, "%d/%m/%Y", today);
        break;

    case MMDDYY:
        strftime(m_date, len, "%m/%d/%Y", today);
        break;

    case YYMMDD:
        strftime(m_date, len, "%Y/%m/%d", today);
        break;

    default:
        strftime(m_date, len, "%m/%d/%Y", today);
        break;
    }
}

void vm_sys_info_get_time(vm_char *m_time, TimeFormat tf)
{
    time_t ltime;
    struct tm *today;

    /* check error(s) */
    if (NULL == m_time)
        return;

    time(&ltime);
    today = localtime(&ltime);

    switch (tf)
    {
    case HHMMSS:
        strftime(m_time, 128, "%H:%M:%S", today);
        break;

    case HHMM:
        strftime(m_time, 128, "%H:%M", today);
        break;

    default:
        strftime(m_time, 128, "%H:%M:%S", today);
        break;
    }
}

Ipp32u vm_sys_info_get_cpu_num(void)
{
#if defined OSX || defined __FreeBSD__
    Ipp32u cpu_num;
    return (Ipp32u)((osx_sysctl_entry_32u(CTL_HW, HW_NCPU, &cpu_num)) ? cpu_num : 1);
#else
    return (Ipp32u)sysconf(_SC_NPROCESSORS_CONF);
#endif
}

Ipp32u vm_sys_info_get_avail_cpu_num(void)
{
#if defined OSX
    Ipp32u cpu_num;
    return (Ipp32u)((osx_sysctl_entry_32u(CTL_HW, HW_AVAILCPU, &cpu_num)) ? cpu_num : 1);
#elif defined __FreeBSD__
    Ipp32u cpu_num;
    return (Ipp32u)((osx_sysctl_entry_32u(CTL_HW, HW_NCPU, &cpu_num)) ? cpu_num : 1);
#else
    return (Ipp32u)sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

void vm_sys_info_get_cpu_name(vm_char *cpu_name)
{
#if defined OSX
    size_t pv;
    char s[128], k[128];
    pv = 128;
    if (sysctlbyname( VM_STRING("machdep.cpu.vendor"), s, &pv, NULL, 0 ) == -1)
        vm_string_strcpy( s, VM_STRING("Problem determine vendor"));

    pv = 128;
    if(sysctlbyname( VM_STRING("machdep.cpu.brand_string"), k, &pv, NULL, 0 ) == -1)
        vm_string_strcpy( k, (vm_char *)"Problem determine brand string" );

    vm_string_sprintf( cpu_name, VM_STRING("%s %s"), k, s );
#elif defined __FreeBSD__
    size_t pv;
    char s[128];
    pv = 128;
    if (sysctlbyname( VM_STRING("hw.model"), s, &pv, NULL, 0 ) == -1)
        vm_string_strcpy( s, VM_STRING("Problem determine vendor"));

    vm_string_sprintf( cpu_name, VM_STRING("%s "), s );
#else
    vm_file *pFile = NULL;
    vm_char buf[PATH_MAX];
    vm_char tmp_buf[PATH_MAX];
    size_t len;

    /* check error(s) */
    if (NULL == cpu_name)
        return;

    pFile = vm_file_fopen(VM_STRING("/proc/cpuinfo"), "r");
    if (!pFile)
        return;

    while((vm_file_fgets(buf, PATH_MAX, pFile)))
    {
        if (!vm_string_strncmp(buf, VM_STRING("vendor_id"), 9))
            vm_string_strncpy(tmp_buf, (vm_char*)(buf + 12), vm_string_strlen(buf) - 13);
        else if (!vm_string_strncmp(buf, VM_STRING("model name"), 10))
        {
            if ((len = vm_string_strlen(buf) - 14) > 8)
                vm_string_strncpy(cpu_name, (vm_char *)(buf + 13), len);
            else
                vm_string_sprintf(cpu_name, VM_STRING("%s"), tmp_buf);
        }
    }
    fclose(pFile);
#endif
}

void vm_sys_info_get_vga_card(vm_char *vga_card)
{
    /* check error(s) */
    if(NULL == vga_card)
        return;
    else
       vm_string_strcpy(vga_card, VM_STRING("No information about the card"));
}

void vm_sys_info_get_os_name(vm_char *os_name)
{
    struct utsname buf;

    /* check error(s) */
    if (NULL == os_name)
        return;

    uname(&buf);
    vm_string_sprintf(os_name, VM_STRING("%s %s"), buf.sysname, buf.release);
}

void vm_sys_info_get_computer_name(vm_char *computer_name)
{
    /* check error(s) */
    if (NULL == computer_name)
        return;

    gethostname(computer_name, 128);
}

void vm_sys_info_get_program_name(vm_char *program_name)
{
    /* check error(s) */
    if (NULL == program_name)
        return;

#if defined OSX || defined __FreeBSD__
    program_name = (vm_char *)getprogname();
#else
    vm_char path[PATH_MAX] = {0,};
    size_t i = 0;

    readlink("/proc/self/exe", path, sizeof(path));
    i = vm_string_strrchr(path, (vm_char)('/')) - path + 1;
    vm_string_strncpy(program_name, path + i, vm_string_strlen(path) - i);
#endif
}

void vm_sys_info_get_program_path(vm_char *program_path)
{
    vm_char path[ PATH_MAX ] = {0,};
#if defined OSX && defined __FreeBSD__
    if( getcwd( path, PATH_MAX ) != NULL )
        vm_string_strcpy( program_path, path );
#else
    size_t i = 0;
    /* check error(s) */
    if(NULL == program_path)
        return;

    readlink("/proc/self/exe", path, sizeof(path));
    i = vm_string_strrchr(path, (vm_char)('/')) - path + 1;
    vm_string_strncpy(program_path, path, i-1);
#endif
}

void vm_sys_info_get_program_description(vm_char *program_description)
{
    /* check error(s) */
    if (NULL == program_description)
        return;
}

Ipp32u vm_sys_info_get_cpu_speed(void)
{
#if defined OSX
    Ipp32u freq;
    return (osx_sysctl_entry_32u(CTL_HW, HW_CPU_FREQ, &freq)) ? (Ipp32u)(freq/1000000) : 1000;
#elif defined __FreeBSD__
    Ipp64u freq = 0;
    return (osx_sysctl_entry_32u(CTL_MACHDEP, 8, &freq)) ? (Ipp32u)(freq/1000000) : 1000;
#else
    Ipp64f ret = 0;
    vm_file *pFile = NULL;
    vm_char buf[PATH_MAX];

    pFile = vm_file_fopen(VM_STRING("/proc/cpuinfo"), "r" );
    if (!pFile)
        return 1000;

    while ((vm_file_fgets(buf, PATH_MAX, pFile)))
    {
        if (!vm_string_strncmp(buf, VM_STRING("cpu MHz"), 7))
        {
            ret = vm_string_atol((vm_char *)(buf + 10));
            break;
        }
    }
    fclose(pFile);
    return ((Ipp32u) ret);
#endif
}

#define VM_CONST_MBYTE 1048576

Ipp32u vm_sys_info_get_mem_size(void)
{
#if defined OSX || defined __FreeBSD__
    Ipp32u bts;
    return (Ipp32u)((osx_sysctl_entry_32u(CTL_HW, HW_PHYSMEM, &bts)) ? (Ipp32u)(bts/(VM_CONST_MBYTE+0.5)) : 1000);
#else
    return (Ipp32u)(sysconf(_SC_PHYS_PAGES)*sysconf(_SC_PAGESIZE)/VM_CONST_MBYTE);
#endif
}
#endif
