/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2013 Intel Corporation. All Rights Reserved.
//
*/

#include "vm_time.h"

#define VM_TIMEOP(A,B,C,OP) A = vvalue(B) OP vvalue(C)

#define VM_TIMEDEST \
  destination[0].tv_sec = (Ipp32u)(cv0 / 1000000); \
  destination[0].tv_usec = (long)(cv0 % 1000000)

static Ipp64u vvalue( struct vm_timeval* B )
{
    Ipp64u rtv;
    rtv = B[0].tv_sec;
    return ((rtv * 1000000) + B[0].tv_usec);
}

/* common (Linux and Windows time functions
 * may be placed before compilation fence. */
void vm_time_timeradd(struct vm_timeval* destination,  struct vm_timeval* src1, struct vm_timeval* src2)
{
    Ipp64u cv0;
    VM_TIMEOP(cv0, src1, src2, + );
    VM_TIMEDEST;
}

void vm_time_timersub(struct vm_timeval* destination,  struct vm_timeval* src1, struct vm_timeval* src2)
{
    Ipp64u cv0;
    VM_TIMEOP(cv0, src1, src2, - );
    VM_TIMEDEST;
}

/* returning value:
//   0 - equal
//  -1 - src1 less than src2
//   1 - src1 more than src2 */
int vm_time_timercmp(struct vm_timeval* src1, struct vm_timeval* src2, struct vm_timeval *threshold)
{
    Ipp64u val1 = vvalue(src1);
    Ipp64u val2 = vvalue(src2);
    int rtval = 0;
    if ( val1 != val2 )
    {
        Ipp64u thr  = vvalue(threshold);
        if (thr != 0)
        {
            val2 = thr;
            val1 = (val1 > val2 ) ? val1 - val2 : val2 - val1;
        }
        rtval = (val1 < val2) ? -1 : 1;
    }
    return rtval;
}
