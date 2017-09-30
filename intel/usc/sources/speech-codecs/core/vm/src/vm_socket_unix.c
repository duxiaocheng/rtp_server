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
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include "vm_socket.h"

static Ipp32s fill_sockaddr(struct sockaddr_in *iaddr, vm_socket_host *host)
{
    struct hostent *ent;

    memset(iaddr, 0, sizeof(*iaddr));

    iaddr->sin_family      = AF_INET;
    iaddr->sin_port        = host ? htons(host->port) : 0;
    iaddr->sin_addr.s_addr = htons(INADDR_ANY);

    if (!host)
        return 0;

    if (!host->hostname)
        return 0;

    iaddr->sin_addr.s_addr = inet_addr(host->hostname);
    if (-1 != iaddr->sin_addr.s_addr)
        return 0;

    ent = gethostbyname(host->hostname);
    if (0 == ent)
        return 1;

    iaddr->sin_addr.s_addr = *((in_addr_t *)ent->h_addr_list[0]);
    return 0;
}

vm_status vm_socket_init(vm_socket *hd, vm_socket_host *local, vm_socket_host *remote, Ipp32s flags)
{
    Ipp32s i;

    /* clean everything */
    memset(&hd->chns, -1, sizeof(hd->chns));
    FD_ZERO(&hd->r_set);
    FD_ZERO(&hd->w_set);

    for (i = 0; i <= VM_SOCKET_QUEUE; i++)
    {
        memset(&(hd->peers[i]), 0, sizeof(hd->peers[i]));
    }
    /* flags shaping */
    if (flags & VM_SOCKET_MCAST)
        flags |= VM_SOCKET_UDP;
    hd->flags = flags;

    /* create socket */
    hd->chns[0] = socket(AF_INET,
                         (flags & VM_SOCKET_UDP) ? SOCK_DGRAM : SOCK_STREAM,
                         0);
    if (hd->chns[0] < 0)
        return VM_OPERATION_FAILED;

    if (fill_sockaddr(&hd->sal, local))
        return VM_OPERATION_FAILED;

    if (fill_sockaddr(&hd->sar, remote))
        return VM_OPERATION_FAILED;

    if (bind(hd->chns[0], (struct sockaddr*)&hd->sal, sizeof(hd->sal)))
        return VM_OPERATION_FAILED;

    if (flags & VM_SOCKET_SERVER)
    {
        if (flags & VM_SOCKET_UDP)
            return VM_OK;
        if (listen(hd->chns[0], VM_SOCKET_QUEUE)<0)
            return VM_OPERATION_FAILED;

        return VM_OK;
    }

    /* network client */
    if (flags&VM_SOCKET_MCAST)
    {
#if defined OSX || defined __FreeBSD__
        struct ip_mreq imr;
        imr.imr_multiaddr.s_addr = hd->sar.sin_addr.s_addr;
        imr.imr_interface.s_addr = INADDR_ANY;
#else
        struct ip_mreqn imr;
        imr.imr_multiaddr.s_addr = hd->sar.sin_addr.s_addr;
        imr.imr_address.s_addr   = INADDR_ANY;
        imr.imr_ifindex          = 0;
#endif
        setsockopt(hd->chns[0], IPPROTO_IP, IP_ADD_MEMBERSHIP, &imr, sizeof(imr));
    }
    if (flags & VM_SOCKET_UDP)
        return VM_OK;

    if (connect(hd->chns[0], (struct sockaddr *)&hd->sar, sizeof(hd->sar)) < 0)
        return VM_OPERATION_FAILED;

    return VM_OK;
}

Ipp32s vm_socket_select(vm_socket *hds, Ipp32s nhd, Ipp32s masks)
{
    Ipp32s i, j, maxsd = -1;

    FD_ZERO(&hds->r_set);
    FD_ZERO(&hds->w_set);

    for (i = 0; i < nhd; i++)
    {
        Ipp32s flags = hds[i].flags;
        if (hds[i].chns[0] < 0)
            continue;

        if (masks & VM_SOCKET_ACCEPT || (masks & VM_SOCKET_READ &&
            (flags & VM_SOCKET_UDP || !(flags & VM_SOCKET_SERVER))))
        {
            FD_SET(hds[i].chns[0], &hds->r_set);
            if (hds[i].chns[0] > maxsd)
                maxsd = hds[i].chns[0];
        }

        if (masks & VM_SOCKET_WRITE &&
            (flags&VM_SOCKET_UDP || !(flags&VM_SOCKET_SERVER)))
        {
            FD_SET(hds[i].chns[0], &hds->w_set);
            if (hds[i].chns[0] > maxsd)
                maxsd = hds[i].chns[0];
        }

        for (j = 1; j <= VM_SOCKET_QUEUE; j++)
        {
            if (hds[i].chns[j] < 0)
                continue;

            if (masks & VM_SOCKET_READ)
            {
                FD_SET(hds[i].chns[j], &hds->r_set);
                if (hds[i].chns[j] > maxsd)
                    maxsd = hds[i].chns[j];
            }

            if (masks & VM_SOCKET_WRITE)
            {
                FD_SET(hds[i].chns[j], &hds->w_set);
                if (hds[i].chns[j] > maxsd)
                    maxsd = hds[i].chns[j];
            }
        }
    }

    if (maxsd < 0)
        return 0;
    i = select(maxsd + 1, &hds->r_set, &hds->w_set, 0, 0);

    return (i<0) ? -1 : i;
}

Ipp32s vm_socket_next(vm_socket *hds, Ipp32s nhd, Ipp32s *idx, Ipp32s *chn, Ipp32s *type)
{
    Ipp32s i, j;

    for (i = 0;i<nhd;i++)
    {
        for (j = 0; j <= VM_SOCKET_QUEUE; j++)
        {
            Ipp32s flags = hds[i].flags;
            if (hds[i].chns[j] < 0)
                continue;

            if (FD_ISSET(hds[i].chns[j], &hds->r_set))
            {
                FD_CLR(hds[i].chns[j], &hds->r_set);

                if (idx) *idx=i;
                if (chn) *chn=j;
                if (type)
                {
                    *type = VM_SOCKET_READ;
                    if (j > 0)
                        return 1;
                    if (flags & VM_SOCKET_UDP)
                        return 1;
                    if (flags & VM_SOCKET_SERVER)
                        *type = VM_SOCKET_ACCEPT;
                }
                return 1;
            }

            if (FD_ISSET(hds[i].chns[j], &hds->w_set))
            {
                FD_CLR(hds[i].chns[j], &hds->w_set);

                if (idx)
                    *idx = i;
                if (chn)
                    *chn = j;
                if (type)
                    *type = VM_SOCKET_WRITE;
                return 1;
            }
        }
    }
    return 0;
}

Ipp32s vm_socket_accept(vm_socket *hd)
{
    Ipp32s psize, chn;

    if (hd->chns[0] < 0)
        return -1;
    if (hd->flags & VM_SOCKET_UDP)
        return 0;
    if (!(hd->flags & VM_SOCKET_SERVER))
        return 0;

    for (chn = 1; chn <= VM_SOCKET_QUEUE; chn++)
    {
        if (hd->chns[chn]<0)
            break;
    }
    if (chn > VM_SOCKET_QUEUE)
        return -1;

    psize = sizeof(hd->peers[chn]);
    hd->chns[chn] = accept(hd->chns[0], (struct sockaddr*)&(hd->peers[chn]), (socklen_t *)&psize);
    if (hd->chns[chn] < 0)
    {
        memset(&(hd->peers[chn]), 0, sizeof(hd->peers[chn]));
        return -1;
    }
    return chn;
}

Ipp32s vm_socket_read(vm_socket *hd, Ipp32s chn, void *buffer, Ipp32s nbytes)
{
    Ipp32s retr;

    if (chn < 0 || chn > VM_SOCKET_QUEUE)
        return -1;
    if (hd->chns[chn] < 0)
        return -1;

    if (hd->flags & VM_SOCKET_UDP)
    {
        Ipp32s ssize = sizeof(hd->sar);
        retr = recvfrom(hd->chns[chn],
                        buffer,
                        nbytes,
                        MSG_WAITALL,
                        (struct sockaddr *)&hd->sar,
                        (socklen_t *)&ssize);
    }
    else
        retr = recv(hd->chns[chn], buffer, nbytes, 0);

    if (retr < 1)
        vm_socket_close_chn(hd, chn);

    return retr;
}

Ipp32s vm_socket_write(vm_socket *hd, Ipp32s chn, void *buffer, Ipp32s nbytes)
{
   Ipp32s retw;

   if (chn<0 || chn>VM_SOCKET_QUEUE)
       return -1;
   if (hd->chns[chn]<0)
       return -1;

   if (hd->flags&VM_SOCKET_UDP)
   {
      retw = sendto(hd->chns[chn], buffer, nbytes, 0,
                    (struct sockaddr *)&hd->sar, sizeof(hd->sar));
   }
   else
       retw=send(hd->chns[chn], buffer, nbytes, 0);

   if (retw < 1)
       vm_socket_close_chn(hd,chn);

   return retw;
}

void vm_socket_close_chn(vm_socket *hd, Ipp32s chn)
{
   if (hd->chns[chn] >= 0)
       close(hd->chns[chn]);

   hd->chns[chn] = -1;
}

void vm_socket_close(vm_socket *hd)
{
   Ipp32s i;

   for (i = VM_SOCKET_QUEUE; i >= 0; i--)
       vm_socket_close_chn(hd, i);
}

/* Old part */
Ipp32s vm_sock_host_by_name(vm_char *name, struct in_addr *paddr)
{
    struct hostent *hostinfo;

    hostinfo = gethostbyname (name);
    if (NULL == hostinfo)
        return 1;

    *paddr = *((struct in_addr *) hostinfo->h_addr);

     return 0;
}

Ipp32s vm_socket_get_client_ip(vm_socket *hd, Ipp8u* buffer, Ipp32s len)
{
    Ipp32s iplen;
    if (hd == NULL)
        return -1;
    if (hd->chns[0] == INVALID_SOCKET)
        return -1;

    iplen = strlen(inet_ntoa(hd->sal.sin_addr));

    if (iplen >= len || buffer == NULL)
        return -1;

    memcpy(buffer,inet_ntoa(hd->sal.sin_addr), iplen);
    buffer[iplen]='\0';

    return 1;
}
#endif
