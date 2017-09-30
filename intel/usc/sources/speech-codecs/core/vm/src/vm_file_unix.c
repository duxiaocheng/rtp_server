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
 * VM 64-bits buffered file operations library
 *       UNIX implementation
 */

#if defined(_MSC_VER)
#pragma warning(disable:4206) // warning C4206: nonstandard extension used : translation unit is empty
#endif

#if defined UNIX
#include "vm_file.h"

/* obtain file info. return 0 if file is not accessible, file_size or file_attr can be NULL if either is not interested */
Ipp32s vm_file_getinfo(const vm_char *filename, Ipp64u *file_size, Ipp32u *file_attr)
{
#if !defined ANDROID
#if defined OSX || defined INTEL64
    struct stat buf;
    if(stat(filename,&buf) != 0)
        return 0;
#else
    struct stat64 buf;
    if(stat64(filename,&buf) != 0)
        return 0;
#endif

    if(file_size)
        *file_size = buf.st_size;
    if(file_attr)
    {
        *file_attr=0;
        if (buf.st_mode & S_IFREG)
            *file_attr|=VM_FILE_ATTR_FILE;
        if (buf.st_mode & S_IFDIR)
            *file_attr|=VM_FILE_ATTR_DIRECTORY;
        if (buf.st_mode & S_IFLNK)
            *file_attr|=VM_FILE_ATTR_LINK;
    }
#endif
    return 1;
}

Ipp64u vm_file_fseek(vm_file *fd, Ipp64s position, VM_FILE_SEEK_MODE mode)
{
#if defined ANDROID
    return fseek(fd, (size_t)position, mode);
#else
#if defined OSX || defined INTEL64
    return fseeko(fd, (off_t)position, mode);
#else
    return fseeko64(fd, (__off64_t)position, mode);
#endif
#endif
}

Ipp64u vm_file_ftell(vm_file *fd)
{
#if defined ANDROID
    return (Ipp64u) ftell(fd);
#else
#if defined OSX || defined INTEL64
    return (Ipp64u)ftello(fd);
#else
    return (Ipp64u)ftello64(fd);
#endif
#endif
}

/* Directory manipulations */
Ipp32s vm_dir_remove(vm_char *path)
{
   return !remove(path);
}

Ipp32s vm_dir_mkdir(vm_char *path)
{
   return !mkdir(path,0777);
}

static vm_char *d_name = NULL;
Ipp32s vm_dir_open(vm_dir **dd, vm_char *path)
{
    if ((dd[0]=opendir(path)) != NULL)
    {
        d_name = NULL;
        getcwd(d_name, 0);
        chdir(path);
    }
    return (dd[0] != NULL) ? 1 : 0;
}

/* directory traverse */
Ipp32s vm_dir_read(vm_dir *dd, vm_char *filename,int nchars)
{
    Ipp32s rtv = 0;
    if (dd != NULL)
    {
        struct dirent *ent=readdir(dd);
        if (ent)
        {
            vm_string_strncpy(filename,ent->d_name,nchars);
            rtv = 1;
        }
    }
    return rtv;
}

void vm_dir_close(vm_dir *dd)
{
    if (dd != NULL)
    {
        if (d_name != NULL)
        {
            chdir(d_name);
            free(d_name);
            d_name = NULL;
        }
        closedir(dd);
    }
}

/*
 * findfirst, findnext, findclose direct emulation
 * for old ala Windows applications
 */
Ipp32s vm_string_findnext(vm_findptr handle, vm_finddata_t *fileinfo)
{
    Ipp32s rtv = 1;
    Ipp64u sz;
    Ipp32u atr;

    if (vm_dir_read(handle, fileinfo[0].name, MAX_PATH))
    {
        if (vm_file_getinfo(fileinfo[0].name, &sz, &atr))
        {
            fileinfo[0].size = sz;
            fileinfo[0].attrib = atr;
            rtv = 0;
        }
    }
    return rtv;
}

vm_findptr vm_string_findfirst(vm_char *filespec, vm_finddata_t *fileinfo)
{
    vm_findptr dd;
    vm_dir_open(&dd, filespec);

    if (dd != NULL)
        vm_string_findnext(dd, fileinfo);
    return dd;
}

Ipp32s vm_string_findclose(vm_findptr handle)
{
    return closedir(handle);
}

Ipp64u vm_dir_get_free_disk_space( void )
{
    Ipp64u rtv = 0;
#if !defined(ANDROID)
    struct statvfs fst;
    if (statvfs(".", &fst) >= 0)
        rtv = fst.f_bsize*fst.f_bavail;
#endif
    return rtv;
}

// CVT functions
Ipp32s vm_file_fputs_cvt(vm_char *str, vm_file *fd, CVT_DESTINATION dst)
{
    return vm_file_fputs(str, fd);
}

vm_char* vm_file_fgets_cvt(vm_char *str, int nchar, vm_file *fd, CVT_DESTINATION dst)
{
    return vm_file_fgets(str, nchar, fd);
}

Ipp32s vm_file_fscanf_cvt(CVT_DESTINATION dst, vm_file *fd, vm_char *format, ...)
{
    return 0; // no vm_file_vfscanf
}

Ipp32s vm_file_fprintf_cvt(CVT_DESTINATION dst, vm_file *fd, vm_char *format, ...)
{
    Ipp32s res;
    va_list argptr;
    va_start(argptr, format);
    res = vm_file_vfprintf(fd, format, argptr);
    va_end(argptr);
    return res;
}

Ipp32s vm_file_vfprintf_cvt(CVT_DESTINATION dst, vm_file* fd, vm_char* format, va_list argptr)
{
    return vm_file_vfprintf(fd, format, argptr);
}
#endif
