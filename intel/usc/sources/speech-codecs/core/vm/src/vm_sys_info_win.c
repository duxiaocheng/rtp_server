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
#include "vm_sys_info.h"
#include "vm_time.h"

#if _MSC_VER >= 1400
#pragma warning(disable: 4996)
/* highly specific pragma to remove warning messages for MS2005 when nameless union is defined in commctrl.h */
#pragma warning(disable: 4201)
#endif

#pragma warning(disable: 981)

void vm_sys_info_get_date(vm_char *m_date, DateFormat df)
{
    SYSTEMTIME SystemTime;

    /* check error(s) */
    if (NULL == m_date)
        return;

    GetLocalTime(&SystemTime );

    switch (df)
    {
    case DDMMYY:
        vm_string_sprintf(m_date,
                          VM_STRING("%.2d/%.2d/%d"),
                          SystemTime.wDay,
                          SystemTime.wMonth,
                          SystemTime.wYear);
        break;

    case MMDDYY:
        vm_string_sprintf(m_date,
                          VM_STRING("%.2d/%.2d/%d"),
                          SystemTime.wMonth,
                          SystemTime.wDay,
                          SystemTime.wYear);
        break;

    case YYMMDD:
        vm_string_sprintf(m_date,
                          VM_STRING("%d/%.2d/%.2d"),
                          SystemTime.wYear,
                          SystemTime.wMonth,
                          SystemTime.wDay);
        break;

    default:
        vm_string_sprintf(m_date,
                          VM_STRING("%2d/%.2d/%d"),
                          SystemTime.wMonth,
                          SystemTime.wDay,
                          SystemTime.wYear);
        break;
    }
}

void vm_sys_info_get_time(vm_char *m_time, TimeFormat tf)
{
    SYSTEMTIME SystemTime;

    /* check error(s) */
    if (NULL == m_time)
        return;

    GetLocalTime(&SystemTime);

    switch (tf)
    {
    case HHMMSS:
        vm_string_sprintf(m_time,
                          VM_STRING("%.2d:%.2d:%.2d"),
                          SystemTime.wHour,
                          SystemTime.wMinute,
                          SystemTime.wSecond);
        break;

    case HHMM:
        vm_string_sprintf(m_time,
                          VM_STRING("%.2d:%.2d"),
                          SystemTime.wHour,
                          SystemTime.wMinute);
        break;

    case HHMMSSMS1:
        vm_string_sprintf(m_time,
                          VM_STRING("%.2d:%.2d:%.2d.%d"),
                          SystemTime.wHour,
                          SystemTime.wMinute,
                          SystemTime.wSecond,
                          SystemTime.wMilliseconds / 100);
        break;

    case HHMMSSMS2:
        vm_string_sprintf(m_time,
                          VM_STRING("%.2d:%.2d:%.2d.%d"),
                          SystemTime.wHour,
                          SystemTime.wMinute,
                          SystemTime.wSecond,
                          SystemTime.wMilliseconds / 10);
        break;

    case HHMMSSMS3:
        vm_string_sprintf(m_time,
                          VM_STRING("%.2d:%.2d:%.2d.%d"),
                          SystemTime.wHour,
                          SystemTime.wMinute,
                          SystemTime.wSecond,
                          SystemTime.wMilliseconds);
        break;

    default:
        vm_string_sprintf(m_time,
                          VM_STRING("%.2d:%.2d:%.2d"),
                          SystemTime.wHour,
                          SystemTime.wMinute,
                          SystemTime.wSecond);
        break;
    }
}

void vm_sys_info_get_os_name(vm_char *os_name)
{
    OSVERSIONINFOEX osvi;
    BOOL bOsVersionInfo;

    /* check error(s) */
    if (NULL == os_name)
        return;

    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    bOsVersionInfo = GetVersionEx((OSVERSIONINFO *) &osvi);
    if (!bOsVersionInfo)
    {
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        if (!GetVersionEx((OSVERSIONINFO *) &osvi))
        {
            vm_string_sprintf(os_name, VM_STRING("Unknown"));
            return;
        }
    }

    switch (osvi.dwPlatformId)
    {
    case 2:
        /* test for the specific product family. */

        if ((6 == osvi.dwMajorVersion) && (0 == osvi.dwMinorVersion) && (osvi.wProductType == VER_NT_WORKSTATION))
            vm_string_sprintf(os_name, VM_STRING("WinVista"));
        if ((6 == osvi.dwMajorVersion) && (0 == osvi.dwMinorVersion) && (osvi.wProductType != VER_NT_WORKSTATION))
            vm_string_sprintf(os_name, VM_STRING("Windows Server 2008"));
        if ((6 == osvi.dwMajorVersion) && (1 == osvi.dwMinorVersion) && (osvi.wProductType == VER_NT_WORKSTATION))
            vm_string_sprintf(os_name, VM_STRING("Windows7"));
        if ((6 == osvi.dwMajorVersion) && (1 == osvi.dwMinorVersion) && (osvi.wProductType != VER_NT_WORKSTATION))
            vm_string_sprintf(os_name, VM_STRING("Windows Server 2008 R2"));
#if(_WIN32_WINNT >= 0x0501)
        if ((5 == osvi.dwMajorVersion) && (2 == osvi.dwMinorVersion) && (GetSystemMetrics(SM_SERVERR2) != 0))
            vm_string_sprintf(os_name, VM_STRING("Windows Server 2003"));
        if ((5 == osvi.dwMajorVersion) && (2 == osvi.dwMinorVersion) && (GetSystemMetrics(SM_SERVERR2) == 0))
            vm_string_sprintf(os_name, VM_STRING("Windows Server 2003 R2"));
#endif

        if ((5 == osvi.dwMajorVersion) && (1 == osvi.dwMinorVersion))
            vm_string_sprintf(os_name, VM_STRING("WinXP"));

        if ((5 == osvi.dwMajorVersion) && (0 == osvi.dwMinorVersion))
            vm_string_sprintf(os_name, VM_STRING("Win2000"));

        if (4 >= osvi.dwMajorVersion)
            vm_string_sprintf(os_name, VM_STRING("WinNT"));
        break;

        /* test for the Windows 95 product family. */
    case 1:
        if ((4 == osvi.dwMajorVersion) && (0 == osvi.dwMinorVersion))
        {
            vm_string_sprintf(os_name, VM_STRING("Win95"));
            if (('C' == osvi.szCSDVersion[1]) || ('B' == osvi.szCSDVersion[1]))
                vm_string_strcat(os_name, VM_STRING("OSR2" ));
        }

        if ((4 == osvi.dwMajorVersion) && (10 == osvi.dwMinorVersion))
        {
            vm_string_sprintf(os_name, VM_STRING("Win98"));
            if ('A' == osvi.szCSDVersion[1])
                vm_string_strcat(os_name, VM_STRING("SE"));
        }

        if ((4 == osvi.dwMajorVersion) && (90 == osvi.dwMinorVersion))
        {
            vm_string_sprintf(os_name,VM_STRING("WinME"));
        }
        break;

    case 3:
        /* get platform string */
        /* SystemParametersInfo(257, MAX_PATH, os_name, 0); */
        if ((4 == osvi.dwMajorVersion) && (20 == osvi.dwMinorVersion))
        {
            vm_string_sprintf(os_name, VM_STRING("PocketPC 2003"));
        }

        if ((4 == osvi.dwMajorVersion) && (21 == osvi.dwMinorVersion))
        {
            vm_string_sprintf(os_name, VM_STRING("WinMobile2003SE"));
        }

        if ((5 == osvi.dwMajorVersion) && (0 == osvi.dwMinorVersion))
        {
            vm_string_sprintf(os_name, VM_STRING("WinCE 5.0"));
        }
        break;

        /* Something else */
    default:
        vm_string_sprintf(os_name, VM_STRING("Win..."));
        break;
    }
}

void vm_sys_info_get_program_name(vm_char *program_name)
{
    vm_char tmp[_MAX_LEN] = {0,};
    Ipp32s i = 0;

    /* check error(s) */
    if (NULL == program_name)
        return;

    GetModuleFileName(NULL, tmp, _MAX_LEN);
    i = (Ipp32s) (vm_string_strrchr(tmp, (vm_char)('\\')) - tmp + 1);
    vm_string_strncpy(program_name,tmp + i, vm_string_strlen(tmp) - i);
}

void vm_sys_info_get_program_path(vm_char *program_path)
{
    vm_char tmp[_MAX_LEN] = {0,};
    Ipp32s i = 0;

    /* check error(s) */
    if (NULL == program_path)
        return;

    GetModuleFileName(NULL, tmp, _MAX_LEN);
    i = (Ipp32s) (vm_string_strrchr(tmp, (vm_char)('\\')) - tmp + 1);
    vm_string_strncpy(program_path, tmp, i - 1);
}

#ifdef VM_RESOURCE_USAGE_FUNCTIONS
Ipp32u vm_sys_info_get_process_memory_usage(VM_PID process_ID)
{
    PROCESS_MEMORY_COUNTERS mc;
    GetProcessMemoryInfo(process_ID, &mc, sizeof(PROCESS_MEMORY_COUNTERS));
    return (Ipp32u)(mc.WorkingSetSize/1024);
}
#endif

Ipp32u vm_sys_info_get_cpu_num(void)
{
    SYSTEM_INFO siSysInfo;

    ZeroMemory(&siSysInfo, sizeof(SYSTEM_INFO));
    GetSystemInfo(&siSysInfo);

    return siSysInfo.dwNumberOfProcessors;
}

Ipp32u vm_sys_info_get_avail_cpu_num(void)
{
    Ipp32u rtval = 0, i = 0;
    SYSTEM_INFO siSysInfo;

    ZeroMemory(&siSysInfo, sizeof(SYSTEM_INFO));
    GetSystemInfo(&siSysInfo);
    for( ; i < 32; ++i )
    {
        if (siSysInfo.dwActiveProcessorMask & 1)
            ++rtval;
        siSysInfo.dwActiveProcessorMask >>= 1;
    }
    return rtval;
}

Ipp32u vm_sys_info_get_mem_size(void)
{
    MEMORYSTATUS m_memstat;

    ZeroMemory(&m_memstat, sizeof(MEMORYSTATUS));
    GlobalMemoryStatus(&m_memstat);

    return (Ipp32u)((Ipp64f)m_memstat.dwTotalPhys / (1024 * 1024) + 0.5);
}

#if defined _WIN32_WCE
#include <winioctl.h>

BOOL KernelIoControl(Ipp32u dwIoControlCode,
                     LPVOID lpInBuf,
                     Ipp32u nInBufSize,
                     LPVOID lpOutBuf,
                     Ipp32u nOutBufSize,
                     LPDWORD lpBytesReturned);

#define IOCTL_PROCESSOR_INFORMATION CTL_CODE(FILE_DEVICE_HAL, 25, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _PROCESSOR_INFO
{
    Ipp16u        wVersion;
    vm_char       szProcessCore[40];
    Ipp16u        wCoreRevision;
    vm_char       szProcessorName[40];
    Ipp16u        wProcessorRevision;
    vm_char       szCatalogNumber[100];
    vm_char       szVendor[100];
    Ipp32u        dwInstructionSet;
    Ipp32u        dwClockSpeed;
} PROCESSOR_INFO;

void vm_sys_info_get_vga_card(vm_char *vga_card)
{
    /* check error(s) */
    if (NULL == vga_card)
        return;

    vm_string_sprintf(vga_card, VM_STRING("Unknown"));
}

void vm_sys_info_get_cpu_name(vm_char *cpu_name)
{
    PROCESSOR_INFO pi;
    Ipp32u dwBytesReturned;
    Ipp32u dwSize = sizeof(PROCESSOR_INFO);
    BOOL bResult;

    /* check error(s) */
    if (NULL == cpu_name)
        return;

    ZeroMemory(&pi, sizeof(PROCESSOR_INFO));
    bResult = KernelIoControl(IOCTL_PROCESSOR_INFORMATION,
                              NULL,
                              0,
                              &pi,
                              sizeof(PROCESSOR_INFO),
                              &dwBytesReturned);

    vm_string_sprintf(cpu_name,
                      VM_STRING("%s %s"),
                      pi.szProcessCore,
                      pi.szProcessorName);
}

void vm_sys_info_get_computer_name(vm_char *computer_name)
{
    vm_char data[_MAX_LEN] = {0,};
    /* it can be wchar variable */
    Ipp32u dwSize = sizeof(data) / sizeof(data[0]);

    /* check error(s) */
    if (NULL == computer_name)
        return;

    SystemParametersInfo(SPI_GETOEMINFO, dwSize, data, 0);
    vm_string_sprintf(computer_name, VM_STRING("%s"), data);
}

Ipp32u vm_sys_info_get_cpu_speed(void)
{
    PROCESSOR_INFO pi;
    Ipp32u res = 400;
    Ipp32u dwBytesReturned;
    Ipp32u dwSize = sizeof(PROCESSOR_INFO);
    BOOL bResult;

    ZeroMemory(&pi,sizeof(PROCESSOR_INFO));
    bResult = KernelIoControl(IOCTL_PROCESSOR_INFORMATION,
                              NULL,
                              0,
                              &pi,
                              sizeof(PROCESSOR_INFO),
                              &dwBytesReturned);
    if (pi.dwClockSpeed)
        res = pi.dwClockSpeed;
    return res;
}
#else

#include <setupapi.h>

void vm_sys_info_get_vga_card(vm_char *vga_card)
{
    SP_DEVINFO_DATA sp_dev_info;
    HDEVINFO DeviceInfoSet;
    vm_char string1[] = VM_STRING("Display");
    Ipp32u dwIndex = 0;
    vm_char data[_MAX_LEN] = {0,};

    /* check error(s) */
    if (NULL == vga_card)
        return;

    ZeroMemory(&sp_dev_info, sizeof(SP_DEVINFO_DATA));
    ZeroMemory(&DeviceInfoSet, sizeof(HDEVINFO));

    DeviceInfoSet = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);
    sp_dev_info.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    while (SetupDiEnumDeviceInfo(DeviceInfoSet, dwIndex, &sp_dev_info))
    {
        SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                         &sp_dev_info,
                                         SPDRP_CLASS,
                                         NULL,
                                         (Ipp8u *) data,
                                         _MAX_LEN,
                                         NULL);
        if (!vm_string_strcmp((vm_char*)data, string1))
        {
            SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                             &sp_dev_info,
                                             SPDRP_DEVICEDESC,
                                             NULL,
                                             (PBYTE) vga_card,
                                             _MAX_LEN,
                                             NULL);
            break;
        }
        dwIndex++;
    }
    SetupDiDestroyDeviceInfoList(DeviceInfoSet);
}

void vm_sys_info_get_cpu_name(vm_char *cpu_name)
{
    HKEY hKey;
    vm_char data[_MAX_LEN] = {0,};
    /* it can be wchar variable */
    DWORD dwSize = sizeof(data) / sizeof(data[0]);
    Ipp32s i = 0;
    LONG lErr;

    /* check error(s) */
    if (NULL == cpu_name)
        return;

    lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        VM_STRING("Hardware\\Description\\System\\CentralProcessor\\0"),
                        0,
                        KEY_QUERY_VALUE,
                        &hKey);
    if (ERROR_SUCCESS == lErr)
    {
        RegQueryValueEx(hKey,
                        _T("ProcessorNameString"),
                        NULL,
                        NULL,
                        (LPBYTE)&data,
                        &dwSize);
        /* error protection */
        data[sizeof(data) / sizeof(data[0]) - 1] = 0;
        while (data[i++] == ' ');
        vm_string_strcpy(cpu_name, (vm_char *)(data + i - 1));
        RegCloseKey (hKey);
    }
    else
        vm_string_sprintf(cpu_name, VM_STRING("Unknown"));
}

Ipp32u vm_sys_info_get_cpu_speed(void)
{
    HKEY hKey;
    Ipp32u data = 0;
    DWORD dwSize = sizeof(Ipp32u);
    LONG lErr;

    lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        VM_STRING("Hardware\\Description\\System\\CentralProcessor\\0"),
                        0,
                        KEY_QUERY_VALUE,
                        &hKey);
    if (ERROR_SUCCESS == lErr)
    {
        RegQueryValueEx (hKey, _T("~MHz"), NULL, NULL, (LPBYTE)&data, &dwSize);
        RegCloseKey(hKey);
        return data;
    }
    else
        return 0;
}

void vm_sys_info_get_computer_name(vm_char *computer_name)
{
    vm_char data[_MAX_LEN] = {0,};
    /* it can be wchar variable */
    DWORD dwSize = sizeof(data) / sizeof(data[0]);

    /* check error(s) */
    if (NULL == computer_name)
        return;

    GetComputerName(data, &dwSize);
    vm_string_sprintf(computer_name, VM_STRING("%s"), data);
}

void vm_sys_info_get_program_description(vm_char *program_description)
{
    /* check error(s) */
    if (NULL == program_description)
    return;
    /* may we'll need to do something here later */
}
#endif

VM_PID vm_sys_info_getpid(void)
{
    return (VM_PID)GetCurrentProcess();
}

/*
 * if information pointer will be NULL then function tries to create it on heap
 * if cpudes pointer is not NULL then function tries to fill it. Buffer size
 * must be enough for all cpu data else fail may occur. Destroying all buffers
 * is always caller program responsibility.
 *
 *
 * return values:
 *     VM_NULL_PTR -- cannot create data structure or internal entries buffer
 *     VM_NOT_INITIALIZED -- illegal number of cpus was returning.
 *     VM_NOT_ENOUGH_DATA -- data collect interval is too small
 *     VM_OPERATION_FAILED -- any PDH request problem
 */
#define AVGFLOAT(A) ((sts == ERROR_SUCCESS) ? ((float)A/(float)100.) : 0)
#define ONEMILLISECOND 1
#define MAX_COUNTER_NAME PDH_MAX_COUNTER_NAME * sizeof(TCHAR)

struct VM_SYSINFO_CPULOAD *vm_sys_info_create_cpu_data(void)
{
    int i = 0;
    HQUERY tmpq = 0;
    struct VM_SYSINFO_CPULOAD *dbuf = NULL;
    dbuf = (struct VM_SYSINFO_CPULOAD *)malloc(sizeof(struct VM_SYSINFO_CPULOAD));

    if (dbuf != NULL)
    {
        dbuf[0].cpudes = NULL;
        dbuf[0].ncpu = vm_sys_info_get_cpu_num();
        dbuf[0].cpudes = (struct VM_SYSINFO_CPULOAD_ENTRY *)(malloc(sizeof(struct VM_SYSINFO_CPULOAD_ENTRY)*(dbuf[0].ncpu+1)));
        if (dbuf[0].cpudes == NULL)
        {
            free(dbuf);
            dbuf = NULL;
        }
        else
        {
            dbuf[0].tickspassed = 0;
            // prepare system specific fields
            dbuf[0].CpuLoadQuery = 0;

            for( i = 0; i < MAX_PERF_PARAMETERS; ++i )
                dbuf[0].CpuPerfCounters[i] = NULL;

            tmpq = dbuf[0].CpuLoadQuery;
            PdhOpenQuery(NULL, 0, &tmpq);
            if (tmpq == 0)
            {
                free(dbuf[0].cpudes);
                free(dbuf);
                dbuf = NULL;
            }
            else
                dbuf[0].CpuLoadQuery = tmpq;
        }
    }
    return dbuf;
}

static vm_status vm_sys_info_remove_cpu_data(struct VM_SYSINFO_CPULOAD *dbuf)
{
    vm_status rtval = VM_NULL_PTR;
    if (dbuf != NULL)
    {
        if (dbuf[0].CpuLoadQuery != 0)
            PdhCloseQuery(dbuf[0].CpuLoadQuery);
        if (dbuf[0].cpudes != NULL)
            free(dbuf[0].cpudes);
        free(dbuf);
    }
    return rtval;
}

static vm_tick ticks_on_avg_call = 0;
/* to use while(0) without warnings */
# pragma warning(disable:4127)

vm_status vm_sys_info_cpu_loading_avg(struct VM_SYSINFO_CPULOAD *dbuf, Ipp32u timeslice/* in msec */)
{
    PDH_STATUS sts = ERROR_SUCCESS;
    HQUERY tmpq = 0;
    vm_status rtval = VM_NULL_PTR;
    vm_char scountername[MAX_COUNTER_NAME];
    PDH_HCOUNTER cptr = NULL;
    BOOL isprepared = FALSE;
    DWORD t;
    PDH_FMT_COUNTERVALUE v;
    int i;
    /* we need 1 ms period to hold timeslice time correct */
    timeBeginPeriod(ONEMILLISECOND);

    do
    {
#ifdef VM_CHECK_CPU_CHECK_TIMESLICE
        if (timeslice < 10)
        {
            rtval = VM_NOT_ENOUGH_DATA;
            break;
        }
#endif
        if (dbuf == NULL)
        {
            dbuf = (struct VM_SYSINFO_CPULOAD *)malloc(sizeof(struct VM_SYSINFO_CPULOAD));
            if (dbuf != NULL)
                dbuf[0].cpudes = NULL;
        }
        if(dbuf == NULL)
            break;
        if(dbuf[0].ncpu == 0)
            break;
        if(dbuf[0].cpudes == NULL)
        {
            dbuf[0].ncpu = vm_sys_info_get_cpu_num();
            dbuf[0].cpudes = (struct VM_SYSINFO_CPULOAD_ENTRY *)(malloc(sizeof(struct VM_SYSINFO_CPULOAD_ENTRY)*(dbuf[0].ncpu+1)));
        }
        if(dbuf[0].cpudes == NULL)
            break;

        /* if cpu description buffer was not NULL I hope it will be enough to hold all information about all the CPUs */
        tmpq = dbuf[0].CpuLoadQuery;
        if ( tmpq == 0) /* no more performance handle created yet */
        {
            PdhOpenQuery(NULL, 0, &tmpq);
            if (tmpq == 0)
                break;
            dbuf[0].CpuLoadQuery = tmpq;
        }
        if(dbuf[0].CpuPerfCounters[0] != 0)
            isprepared = TRUE; /* performance monitor already prepared */
        if(dbuf[0].CpuLoadQuery == 0)
            break;

        if(!isprepared)
        {
            /* create buffers for counters */
            rtval = VM_OK;
            for( i = 0; i < MAX_PERF_PARAMETERS; ++i )
            {
                dbuf[0].CpuPerfCounters[i] = NULL;
                dbuf[0].CpuPerfCounters[i] = (PDH_HCOUNTER *)malloc((dbuf[0].ncpu+1/*for overall counter*/)*sizeof(PDH_HCOUNTER));
                if (dbuf[0].CpuPerfCounters[i] == NULL)
                {
                    rtval = VM_OPERATION_FAILED;
                    break;
                }
            }
            if (rtval != VM_OK)
            {
                for( i = 0; i < MAX_PERF_PARAMETERS; ++i )
                {
                    if (dbuf[0].CpuPerfCounters[i] != NULL)
                    {
                        free(dbuf[0].CpuPerfCounters[i]);
                        dbuf[0].CpuPerfCounters[i] = NULL;
                    }
                }
                break;
            }

            /* add performance counters */
            /* user time                */
            vm_string_strcpy(scountername, VM_STRING("\\Processor     "));
            for( i = 0; i < dbuf[0].ncpu; ++i )
            {
                vm_string_sprintf(&scountername[10], VM_STRING("(%d)\\%% User Time"), i);
                cptr = dbuf[0].CpuPerfCounters[0] + i*sizeof(PDH_HCOUNTER);
                PdhAddCounter(dbuf[0].CpuLoadQuery, scountername, 0, cptr);    
                vm_string_sprintf(&scountername[10], VM_STRING("(%d)\\%% Privileged Time"), i);
                cptr = dbuf[0].CpuPerfCounters[1] + i*sizeof(PDH_HCOUNTER);
                sts = PdhAddCounter(dbuf[0].CpuLoadQuery, scountername, 0, cptr);

                vm_string_sprintf(&scountername[10], VM_STRING("(%d)\\%% Interrupt Time"), i);
                cptr = dbuf[0].CpuPerfCounters[2] + i*sizeof(PDH_HCOUNTER);
                sts = PdhAddCounter(dbuf[0].CpuLoadQuery, scountername, 0, cptr);
            }
        }

        vm_time_sleep(timeslice);
        if ((sts = PdhCollectQueryData(dbuf[0].CpuLoadQuery)) != ERROR_SUCCESS)
            break;

        /* fill in output data structure */
        for(i = 0; i < dbuf[0].ncpu; ++i)
        {
            sts = PdhGetFormattedCounterValue(dbuf[0].CpuPerfCounters[0][i], PDH_FMT_LONG, &t, &v);
            dbuf[0].cpudes[i].usrload = AVGFLOAT(v.longValue);
            sts = PdhGetFormattedCounterValue(dbuf[0].CpuPerfCounters[1][i], PDH_FMT_LONG, &t, &v); 
            dbuf[0].cpudes[i].sysload = AVGFLOAT(v.longValue);           
            dbuf[0].cpudes[i].idleload = (float)1. - dbuf[0].cpudes[i].usrload - dbuf[0].cpudes[i].sysload;
            dbuf[0].cpudes[i].idleload = (dbuf[0].cpudes[i].idleload < 0) ? 0 : dbuf[0].cpudes[i].idleload;
            sts = PdhGetFormattedCounterValue(dbuf[0].CpuPerfCounters[2][i], PDH_FMT_LONG, &t, &v);
            dbuf[0].cpudes[i].irqsrvload = AVGFLOAT(v.longValue);
            /* clear other parameters for each processor */
            dbuf[0].cpudes[i].iowaitsload = dbuf[0].cpudes[i].softirqsrvload = 0.;
            dbuf[0].cpudes[i].usrniceload = dbuf[0].cpudes[i].vmstalled = 0.;
        }
        rtval = VM_OK;
    } while (0);

    if ((NULL != dbuf) && (ticks_on_avg_call != 0))
        dbuf[0].tickspassed = (vm_tick)((vm_time_get_tick() - ticks_on_avg_call)*1000/vm_time_get_frequency());
    ticks_on_avg_call = vm_time_get_tick();
    timeEndPeriod(ONEMILLISECOND);

    return rtval;
}
#endif
