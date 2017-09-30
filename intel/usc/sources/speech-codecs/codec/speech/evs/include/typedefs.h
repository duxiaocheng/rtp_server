/******************************************************************************
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>    NOTIFICATION    <<<<<<<<<<<<<<<<<<<<<<<<<<<<
 *
 * Copyright (©) 2015-2016 Intel Corporation
 * All Rights Reserved
 *
 * This is unpublished proprietary information of Intel Corporation.  This
 * copyright notice does not evidence publication.
 *
 * The use of the software, documentation, methodologies, and other information
 * contained herein is governed solely by the associated license agreements.
 * Any inconsistent use shall be deemed to be a misappropriation of the
 * intellectual property of Intel Corporation and treated accordingly.
 * ----------------------------------------------------------------------------
 *
 * Content:  Basic Data Types
 *
 * $History: $
 *
 ******************************************************************************/
#ifndef _TYPEDEFS_H_
#define _TYPEDEFS_H_

#if defined(__sun__) && defined( __sparc__)
#if 1
#include <sys/int_types.h>      /* /usr/include/sys/int_types.h */
#else
typedef char int8_t;                /* 8 bit "register"  (c_*) */
typedef unsigned char uint8_t;      /* 8 bit "register"  (b_*) */
typedef short int16_t;              /* 16 bit "register"  (sw*) */
typedef unsigned short uint16_t;    /* 16 bit "register"  (usw*) */
typedef long int32_t;               /* 32 bit "accumulator" (L_*) */
typedef unsigned long uint32_t;     /* 32 bit "accumulator" (uL_*) */
typedef long long int64_t;
typedef unsigned long long uint64_t;
#endif
#define BIG_ENDIAN_TYPES
#define __restrict

#elif defined (__GNUC__)

#if (__GNUC__ < 3)
typedef char int8_t;                /* 8 bit "register"  (c_*) */
typedef unsigned char uint8_t;      /* 8 bit "register"  (b_*) */
typedef short int16_t;              /* 16 bit "register"  (sw*) */
typedef unsigned short uint16_t;    /* 16 bit "register"  (usw*) */
typedef long int32_t;               /* 32 bit "accumulator" (L_*) */
typedef unsigned long uint32_t;     /* 32 bit "accumulator" (uL_*) */
typedef long long int64_t;
typedef unsigned long long uint64_t;
#define __restrict
#else
#include <stdint.h>
#include <limits.h>
/* __restrict supported by gcc */
#endif
#if defined (__arm__)
#if defined (__ARMEB__)
#define BIG_ENDIAN_TYPES
#else
#define LSBFIRST
#endif
#elif defined (__APPLE__)
#if defined (__BIG_ENDIAN__)
#define BIG_ENDIAN_TYPES
#else
#define LSBFIRST
#endif
#elif defined(__INTEL_COMPILER)
#define LSBFIRST
#elif defined(__linux__) || defined (__CYGWIN__)
#include <endian.h>
#if (__BYTE_ORDER==__BIG_ENDIAN)
#define BIG_ENDIAN_TYPES
#else
#define LSBFIRST
#endif
#endif

#elif defined (__CC_ARM) || defined (__arm__) || defined (__arm)
#include <stdint.h>
#include <limits.h>
#if defined (__BIG_ENDIAN)
#define BIG_ENDIAN_TYPES
#else
#define LSBFIRST
#endif
/* __restrict supported by ARM */

#elif defined (__MWERKS__)
#include <stdint.h>
#include <limits.h>
#if (!defined (__INTEL__) && (defined(__POWERPC__) || defined(__MC68K__)))
#define BIG_ENDIAN_TYPES
#else
#define LSBFIRST
#endif
#if __option (c99)
#define __restrict restrict
#else
#define __restrict
#endif

#elif defined(_MSC_VER) || defined(_WIN32) /* Visual C */
#if ( _MSC_VER < 1600 )
#if (_MSC_VER < 1300)
typedef signed char       int8_t;
typedef signed short      int16_t;
typedef signed int        int32_t;
typedef unsigned char     uint8_t;
typedef unsigned short    uint16_t;
typedef unsigned int      uint32_t;
#else
typedef signed __int8     int8_t;
typedef signed __int16    int16_t;
typedef signed __int32    int32_t;
typedef unsigned __int8   uint8_t;
typedef unsigned __int16  uint16_t;
typedef unsigned __int32  uint32_t;
#endif
typedef signed __int64       int64_t;
typedef unsigned __int64     uint64_t;
#else
#include <stdint.h>
#endif
#include <limits.h>
#define LSBFIRST
/* __restrict supported by Visual Studio 2005 */
#if !defined(_MSC_VER) || (_MSC_VER < 1400) /* before Visual Studio 2005 */
#define __restrict
#endif

#elif defined (lint) || defined (_lint)
typedef char int8_t;                /* 8 bit "register"  (c_*) */
typedef unsigned char uint8_t;      /* 8 bit "register"  (b_*) */
typedef short int int16_t;          /* 16 bit "register"  (s_*) */
typedef unsigned short int uint16_t;/* 16 bit "register"  (us_*) */
typedef long int int32_t;           /* 32 bit "accumulator" (L_*) */
typedef unsigned long int uint32_t; /* 32 bit "accumulator" (uL_*) */
typedef long long int64_t;
typedef unsigned long long uint64_t;
#define __restrict

#else
#error "can't determine architecture; adapt typedefs.h to your platform"
#endif

/* limits definitions (usually defined in stdint.h) *
 ****************************************************/

#ifndef INT8_MIN
#define INT8_MIN    ((int8_t)-0x80)
#endif
#ifndef INT8_MAX
#define INT8_MAX    ((int8_t)0x7f)
#endif
#ifndef UINT8_MAX
#define UINT8_MAX   ((uint8_t)0xff)
#endif
#ifndef INT16_MIN
#define INT16_MIN   ((int16_t)-0x8000)
#endif
#ifndef INT16_MAX
#define INT16_MAX   ((int16_t)0x7fff)
#endif
#ifndef UINT16_MAX
#define UINT16_MAX  ((uint16_t)0xffff)
#endif
#ifndef INT32_MIN
#define INT32_MIN   ((int32_t)-0x80000000)
#endif
#ifndef INT32_MAX
#define INT32_MAX   ((int32_t)0x7fffffff)
#endif
#ifndef UINT32_MAX
#define UINT32_MAX  ((uint32_t)0xffffffff)
#endif
#ifndef INT64_MIN
#define INT64_MIN   ((int64_t)-0x8000000000000000LL)
#endif
#ifndef INT64_MAX
#define INT64_MAX   ((int64_t)0x7fffffffffffffffLL)
#endif
#ifndef UINT64_MAX
#define UINT64_MAX  ((uint64_t)0xffffffffffffffffULL)
#endif

/* basic_op definitions & types *
 ********************************/

typedef char Char;
typedef uint8_t UWord8;         /*  8 bits, unsigned */
typedef int8_t  Word8;          /*  8 bits,   signed */
typedef uint16_t UWord16;       /* 16 bits, unsigned */
typedef uint16_t UNS_Word16;    /* 16 bits, unsigned */
typedef int16_t Word16;         /* 16 bits,   signed */
typedef uint32_t UWord32;       /* 32 bits, unsigned */
typedef int32_t Word32;         /* 32 bits,   signed */
typedef int64_t Word40;         /* 40 bits,   signed */
typedef int64_t Word64;         /* 64 bits,   signed */
typedef uint64_t UWord64;       /* 64 bits, unsigned */
typedef int Flag;

#ifndef MIN_32
#define MIN_32 INT32_MIN
#endif
#ifndef MAX_32
#define MAX_32 INT32_MAX
#endif

#ifndef MIN_16
#define MIN_16 INT16_MIN
#endif
#ifndef MAX_16
#define MAX_16 INT16_MAX
#endif

#define MAX_40 ((Word40) 549755813887LL)
#define MIN_40 ((Word40)-549755813888LL)

#define MAX_40_2 ((Word40) 549755813887LL/2)
#define MIN_40_2 ((Word40)-549755813888LL/2)

#ifndef ULONG_MAX 
#define ULONG_MAX           0xffffffffUL
#endif

#define minWord16     INT16_MIN
#define maxWord16     INT16_MAX
#define minUWord16    0
#define maxUWord16    UINT16_MAX
#define minWord32     INT32_MIN
#define maxWord32     INT32_MAX
#define minUWord32    0
#define maxUWord32    UINT32_MAX

#ifndef SHRT_MAX 
#define SHRT_MAX    0x7fff
#endif

#ifndef SIGN_32
#define SIGN_32 ((int32_t)0x80000000)   /* sign bit */
#endif
#ifndef SIGN_16
#define SIGN_16 ((int16_t)0x8000)       /* sign bit for int16_t  type */
#endif

/* boolean type *
 ******************/
typedef int Bool;

typedef uint32_t    boolean_t;
typedef uint32_t    bool32_t;
typedef uint16_t    bool16_t;
typedef uint8_t     bool8_t;

#ifndef bool
#define bool boolean_t
#endif

#ifndef false
#define false 0
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef true
#define true  1
#endif
#ifndef TRUE
#define TRUE  1
#endif

#endif /* _TYPEDEFS_H_ */

