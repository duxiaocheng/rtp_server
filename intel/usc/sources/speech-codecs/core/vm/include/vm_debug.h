/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2013 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __VM_DEBUG_H__
#define __VM_DEBUG_H__

#ifdef _DEBUG
#include <assert.h>
#define VM_ASSERT(EXPR) assert(EXPR)
#else
#define VM_ASSERT(EXPR) ((void)0)
#endif

#endif
