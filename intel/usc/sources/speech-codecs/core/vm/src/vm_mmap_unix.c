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
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "vm_mmap.h"

/* Set the mmap handle an invalid value */
void vm_mmap_set_invalid(vm_mmap *handle)
{
    /* check error(s) */
    if(NULL == handle)
        return;

    handle->fd= -1;
    handle->address = NULL;
    handle->fAccessAttr = 0;
}

/* Verify if the mmap handle is valid */
Ipp32s vm_mmap_is_valid(vm_mmap *handle)
{
    /* check error(s) */
    if (NULL == handle)
        return 0;

    return (-1 != handle->fd);
}

/* Map a file into system meory, return size of the mapped file */
#define VM_DEFAULT_MAP_CREATE_MODE 0666
Ipp64u vm_mmap_create(vm_mmap *handle, vm_char *file, Ipp32s fileAccessAttr)
{
    size_t sizet;

    /* check error(s) */
    if (NULL == handle)
        return 0;

    handle->address = NULL;
    handle->sizet = 0;

    if(FLAG_ATTRIBUTE_READ & fileAccessAttr)
        handle->fd = open(file, O_RDONLY);
    else
      handle->fd = open(file, O_RDWR | O_CREAT, VM_DEFAULT_MAP_CREATE_MODE);

    if (-1 == handle->fd)
        return 0;

    sizet = lseek(handle->fd, 0, SEEK_END);
    lseek(handle->fd, 0, SEEK_SET);

    return sizet;
}

/* Obtain a view of the mapped file, return the adjusted offset & size */
void *vm_mmap_set_view(vm_mmap *handle, Ipp64u *offset, size_t *sizet)
{
    Ipp64u pagesize = getpagesize();
    Ipp64u edge;

    /* check error(s) */
    if (NULL == handle)
        return NULL;

    if (handle->address)
        munmap(handle->address,handle->sizet);

    edge = (*sizet) + (*offset);
    (*offset) = ((Ipp64u)((*offset) / pagesize)) * pagesize;
    handle->sizet = (*sizet) = edge - (*offset);
    handle->address = mmap(0, *sizet, PROT_READ, MAP_SHARED, handle->fd, *offset);

    return (handle->address == (void *)-1) ? NULL : handle[0].address;
}

/* Remove the mmap */
void vm_mmap_close(vm_mmap *handle)
{
    /* check error(s) */
    if (NULL == handle)
        return;

    if (handle->address)
    {
        munmap(handle->address, handle->sizet);
        handle->address = NULL;
    }

    if (-1 != handle->fd)
    {
        close(handle->fd);
        handle->fd= -1;
    }
}

Ipp32u vm_mmap_get_page_size(void)
{
    return getpagesize();
}

Ipp32u vm_mmap_get_alloc_granularity(void)
{
    return 16 * getpagesize();
}

void vm_mmap_unmap(vm_mmap *handle)
{
    /* check error(s) */
    if (NULL == handle)
        return;

    if (handle->address)
    {
        munmap(handle->address, handle->sizet);
        handle->address = NULL;
    }
}
#endif
