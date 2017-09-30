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
#include "vm_mmap.h"

/* Set the mmap handle an invalid value */
void vm_mmap_set_invalid(vm_mmap *handle)
{
    /* check error(s) */
    if (NULL == handle)
        return;

    handle->fd_file     = NULL;
    handle->address     = NULL;
    handle->fd_map      = NULL;
    handle->fAccessAttr = 0;
}

/* Verify if the mmap handle is valid */
Ipp32s vm_mmap_is_valid(vm_mmap *handle)
{
    /* check error(s) */
    if (NULL == handle)
        return 0;

    return (Ipp32s)(NULL != handle->fd_map);
}

/* Map a file into system meory, return the file size */
Ipp64u vm_mmap_create(vm_mmap *handle, vm_char *file, Ipp32s fileAccessAttr)
{
    Ipp32u error;
    LARGE_INTEGER sizet;

    /* check error(s) */
    if (NULL == handle)
        return 0;

    handle->fd_map = NULL;
    handle->address = NULL;

    if (FLAG_ATTRIBUTE_READ & fileAccessAttr)
    {
#ifdef _WIN32_WCE
        handle->fd_file = CreateFileForMapping(file,
                                               GENERIC_READ,
                                               FILE_SHARE_READ,
                                               NULL,
                                               OPEN_EXISTING,
                                               FILE_ATTRIBUTE_NORMAL,
                                               NULL);
#else
        handle->fd_file = CreateFile(file,
                                     GENERIC_READ,
                                     FILE_SHARE_READ,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_ATTRIBUTE_READONLY|FILE_FLAG_RANDOM_ACCESS,
                                     0);
#endif

        if (INVALID_HANDLE_VALUE == handle->fd_file)
            return 0;

        sizet.LowPart = GetFileSize(handle->fd_file, (LPDWORD) &sizet.HighPart);
        handle->fd_map = CreateFileMapping(handle->fd_file,
                                           NULL,
                                           PAGE_READONLY,
                                           sizet.HighPart,
                                           sizet.LowPart,
                                           0);
    }
    else
    {
#ifdef _WIN32_WCE
        handle->fd_file = CreateFileForMapping(file,
                                               GENERIC_WRITE|GENERIC_READ,
                                               FILE_SHARE_WRITE,
                                               NULL,
                                               CREATE_ALWAYS,
                                               FILE_ATTRIBUTE_NORMAL,
                                               NULL);
#else
        handle->fd_file = CreateFile(file,
                                     GENERIC_WRITE|GENERIC_READ,
                                     FILE_SHARE_WRITE,
                                     NULL,
                                     CREATE_ALWAYS,
                                     FILE_ATTRIBUTE_NORMAL,
                                     0);
#endif

        if (INVALID_HANDLE_VALUE == handle->fd_file)
            return 0;

        sizet.LowPart  = 0;
        sizet.HighPart = 1;
        handle->fd_map = CreateFileMapping(handle->fd_file,
                                           NULL,
                                           PAGE_READWRITE,
                                           0,
                                           sizet.LowPart,
                                           0);
        sizet.LowPart  = 1024 * 1024 * 1024;
        sizet.HighPart = 0;

        while (!handle->fd_map)
        {
            handle->fd_map = CreateFileMapping(handle->fd_file,
                                               NULL,
                                               PAGE_READWRITE,
                                               0,
                                               sizet.LowPart,
                                               0);
            error = GetLastError();

            if (ERROR_NOT_ENOUGH_MEMORY == error || ERROR_DISK_FULL == error)
                sizet.LowPart /= 2;
            else
                break;
        }
    }

    handle->fAccessAttr = fileAccessAttr;

    if (FLAG_ATTRIBUTE_READ & fileAccessAttr || 16*1024*1024 <= sizet.LowPart)
    {
        if (handle->fd_map)
            return (Ipp64u) sizet.QuadPart;
    }

    vm_mmap_close(handle);
    return 0;
}

/* Obtain the a view of the mapped file */
void *vm_mmap_set_view(vm_mmap *handle, Ipp64u *offset, size_t *sizet)
{
    LARGE_INTEGER t_offset;
    LARGE_INTEGER t_sizet;
    SYSTEM_INFO info;
    Ipp64u pagesize;
    Ipp64u edge;

    /* check error(s) */
    if (NULL == handle)
        return NULL;

    if (handle->address)
        UnmapViewOfFile(handle->address);

    GetSystemInfo(&info);
    pagesize = info.dwAllocationGranularity;

    edge = (*sizet) + (*offset);
    t_offset.QuadPart =
    (*offset) = (Ipp64u)((*offset) / pagesize) * pagesize;

    t_sizet.QuadPart = (*sizet) = (size_t)(edge - (*offset));

    if (FLAG_ATTRIBUTE_READ & handle->fAccessAttr)
    {
        handle->address = MapViewOfFile(handle->fd_map,
                                        FILE_MAP_READ,
                                        t_offset.HighPart,
                                        t_offset.LowPart,
                                        t_sizet.LowPart);
    }
    else
    {
        handle->address = MapViewOfFile(handle->fd_map,
                                        FILE_MAP_WRITE,
                                        t_offset.HighPart,
                                        t_offset.LowPart,
                                        t_sizet.LowPart);
    }
    return handle->address;
}

/* Remove the mmap */
void vm_mmap_close(vm_mmap *handle)
{
    /* check error(s) */
    if (NULL == handle)
        return;

    if (handle->address)
    {
        UnmapViewOfFile(handle->address);
        handle->address = NULL;
    }

    if (handle->fd_map)
    {
        CloseHandle(handle->fd_map);
        handle->fd_map = NULL;
    }

    if (INVALID_HANDLE_VALUE != handle->fd_file)
    {
        CloseHandle(handle->fd_file);
        handle->fd_file = INVALID_HANDLE_VALUE;
    }
}

/*  Return page size*/
Ipp32u vm_mmap_get_page_size(void)
{
    SYSTEM_INFO info;

    GetSystemInfo(&info);
    return info.dwPageSize;
}

Ipp32u vm_mmap_get_alloc_granularity(void)
{
    SYSTEM_INFO info;

    GetSystemInfo(&info);
    return info.dwAllocationGranularity;
}

void vm_mmap_unmap(vm_mmap *handle)
{
    /* check error(s) */
    if (NULL == handle)
        return;

    if (handle->address)
    {
        UnmapViewOfFile(handle->address);
        handle->address = NULL;
    }
}
#endif
