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
#include "vm_socket.h"
#include "vm_time.h"
#include "vm_debug.h"

/*#if defined(_WIN32_WINNT) && (WINVER < 0x0501)
#  include <ws2tcpip.h>
#endif */ /* defined(_WIN32_WINNT) && (WINVER < 0x0501) */
/* to remove conditional expression is constant for FD_xx macros
   from  winsock.h */

#pragma warning(disable:4127)

#define LINE_SIZE 128

static Ipp32s fill_sockaddr(struct sockaddr_in *iaddr, vm_socket_host *host)
{
    struct hostent *ent;
#if defined(UNICODE) || defined(_UNICODE)
    Ipp8s hostname[LINE_SIZE];
#endif
    Ipp8s *pName;

    memset(iaddr, 0, sizeof(*iaddr));

    iaddr->sin_family      = AF_INET;
    iaddr->sin_port        = (unsigned short)(host ? htons(host->port) : 0);
    iaddr->sin_addr.s_addr = htons(INADDR_ANY);

    if (!host)
        return 0;
    if (!host->hostname)
        return 0;

#if defined(UNICODE) || defined(_UNICODE)
    vm_line_to_ascii(host->hostname, vm_string_strlen(host->hostname), hostname, LINE_SIZE);
    pName = (Ipp8s*)hostname;
#else
    pName = (Ipp8s*)host->hostname;
#endif

    iaddr->sin_addr.s_addr = inet_addr((const char*)pName);
    if (iaddr->sin_addr.s_addr != INADDR_NONE)
        return 0;

    ent = gethostbyname((const char*)pName);
    if (!ent)
        return 1;
    iaddr->sin_addr.s_addr = *((u_long *)ent->h_addr_list[0]);

    return 0;
}

vm_status vm_socket_init(vm_socket *hd, vm_socket_host *local, vm_socket_host *remote, Ipp32s flags)
{
    Ipp32s i;
    Ipp32s error = 0;
    Ipp32s connection_attempts = 7, reattempt = 0;
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(1,1), &wsaData))
        return VM_OPERATION_FAILED;

    /* clean everything */
    for (i = 0; i <= VM_SOCKET_QUEUE; i++)
        hd->chns[i] = INVALID_SOCKET;

    FD_ZERO(&hd->r_set);
    FD_ZERO(&hd->w_set);

    /* flags shaping */
    if (flags & VM_SOCKET_MCAST)
        flags |= VM_SOCKET_UDP;

    hd->flags=flags;

    /* create socket */
    hd->chns[0] = socket(AF_INET, (flags&VM_SOCKET_UDP) ? SOCK_DGRAM : SOCK_STREAM, 0);

    if (hd->chns[0] == INVALID_SOCKET)
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
        if (listen(hd->chns[0], VM_SOCKET_QUEUE))
            return VM_OPERATION_FAILED;

        return VM_OK;
    }

    /* network client */
    if (flags & VM_SOCKET_MCAST)
    {
        struct ip_mreq imr;

        imr.imr_multiaddr.s_addr = hd->sar.sin_addr.s_addr;
        imr.imr_interface.s_addr = INADDR_ANY;
        if (setsockopt(hd->chns[0],
            IPPROTO_IP,
            IP_ADD_MEMBERSHIP,
            (Ipp8s *) &imr,
                    sizeof(imr))==SOCKET_ERROR)
              return VM_OPERATION_FAILED;
    }

    if (flags & VM_SOCKET_UDP)
        return VM_OK;

    while (connect(hd->chns[0], (struct sockaddr *)&hd->sar, sizeof(hd->sar)) &&
        reattempt < connection_attempts )

    {
        error = WSAGetLastError();

        if (error ==  WSAENETUNREACH || error == WSAECONNREFUSED
            || error ==  WSAEADDRINUSE || error ==  WSAETIMEDOUT)
        {
            reattempt++;
        }
        else
        {
            return VM_OPERATION_FAILED;
        }
        vm_time_sleep(1);
    }
    return VM_OK;
}

Ipp32s vm_socket_select(vm_socket *hds, Ipp32s nhd, Ipp32s masks)
{
    Ipp32s i, j;

    FD_ZERO(&hds->r_set);
    FD_ZERO(&hds->w_set);

    for (i = 0; i < nhd; i++)
    {
        Ipp32s flags = hds[i].flags;
        if (hds[i].chns[0] == INVALID_SOCKET)
            continue;

        if (masks & VM_SOCKET_ACCEPT ||
                (masks & VM_SOCKET_READ &&
                 (flags & VM_SOCKET_UDP || !(flags & VM_SOCKET_SERVER))))
            FD_SET(hds[i].chns[0], &hds->r_set);

        if (masks & VM_SOCKET_WRITE &&
                (flags & VM_SOCKET_UDP || !(flags & VM_SOCKET_SERVER)))
            FD_SET(hds[i].chns[0], &hds->w_set);

        for (j = 1; j <= VM_SOCKET_QUEUE; j++)
        {
            if (hds[i].chns[j] == INVALID_SOCKET)
                continue;

            if (masks & VM_SOCKET_READ)
                FD_SET(hds[i].chns[j], &hds->r_set);
            if (masks & VM_SOCKET_WRITE)
                FD_SET(hds[i].chns[j], &hds->w_set);
        }
    }

    i = select(0, &hds->r_set, &hds->w_set, 0, 0);

    return (i < 0) ? -1 : i;
}

Ipp32s vm_socket_next(vm_socket *hds, Ipp32s nhd, Ipp32s *idx, Ipp32s *chn, Ipp32s *type)
{
    Ipp32s i, j;

    for (i = 0; i < nhd; i++)
    {
        for (j = 0; j <= VM_SOCKET_QUEUE; j++)
        {
            Ipp32s flags = hds[i].flags;
            if (hds[i].chns[j] == INVALID_SOCKET)
                continue;

            if (FD_ISSET(hds[i].chns[j], &hds->r_set))
            {
                FD_CLR(hds[i].chns[j],&hds->r_set);

                if (idx)
                    *idx=i;
                if (chn)
                    *chn=j;
                if (type)
                {
                    *type = VM_SOCKET_READ;
                    if (j > 0)
                        return 1;
                    if (flags&VM_SOCKET_UDP)
                        return 1;
                    if (flags&VM_SOCKET_SERVER)
                        *type=VM_SOCKET_ACCEPT;
                }
                return 1;
            }

            if (FD_ISSET(hds[i].chns[j],&hds->w_set))
            {
                FD_CLR(hds[i].chns[j],&hds->w_set);

                if (idx)
                    *idx=i;
                if (chn)
                    *chn=j;
                if (type)
                    *type=VM_SOCKET_WRITE;
                return 1;
            }
        }
    }

    return 0;
}

Ipp32s vm_socket_accept(vm_socket *hd)
{
    Ipp32s psize, chn;

    if (hd->chns[0] == INVALID_SOCKET)
        return -1;
    if (hd->flags & VM_SOCKET_UDP)
        return 0;
    if (!(hd->flags & VM_SOCKET_SERVER))
        return 0;

    for (chn = 1; chn <= VM_SOCKET_QUEUE; chn++)
    {
        if (hd->chns[chn] == INVALID_SOCKET)
            break;
    }
    if (chn > VM_SOCKET_QUEUE)
        return -1;

    psize = sizeof(hd->peers[chn]);
    hd->chns[chn] = accept(hd->chns[0], (struct sockaddr *)&(hd->peers[chn]), &psize);
    if (hd->chns[chn] == INVALID_SOCKET)
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
    if (hd->chns[chn] == INVALID_SOCKET)
        return -1;

    if (hd->flags & VM_SOCKET_UDP)
    {
        Ipp32s ssize = sizeof(hd->sar);
        retr = recvfrom(hd->chns[chn],
                        buffer,
                        nbytes,
                        0,
                        (struct sockaddr *)&hd->sar,
                        &ssize);
    }
    else
    {
        Ipp32s time_out_count = 0;

        retr = recv(hd->chns[chn], buffer, nbytes, 0);

        // Process 2 seconds time out
        while (retr < 0 && WSAEWOULDBLOCK == WSAGetLastError() && ++time_out_count < 20000)
        {
            vm_time_sleep(1);
            retr = recv(hd->chns[chn], buffer, nbytes, 0);
        }
    }

    if (retr < 1)
        vm_socket_close_chn(hd, chn);

    return retr;
}

Ipp32s vm_socket_write(vm_socket *hd, Ipp32s chn, void *buffer, Ipp32s nbytes)
{
    Ipp32s retw;

    if (chn < 0 || chn > VM_SOCKET_QUEUE)
        return -1;
    if (hd->chns[chn] == INVALID_SOCKET)
        return -1;

    if (hd->flags & VM_SOCKET_UDP)
    {
        retw = sendto(hd->chns[chn],
                      buffer,
                      nbytes,
                      0,
                      (struct sockaddr *)&hd->sar,
                      sizeof(hd->sar));
    }
    else
    {
        retw = send(hd->chns[chn], buffer, nbytes, 0);
    }

    if (retw < 1)
        vm_socket_close_chn(hd, chn);

    return retw;
}

void vm_socket_close_chn(vm_socket *hd, Ipp32s chn)
{
    if (hd->chns[chn] != INVALID_SOCKET)
        closesocket(hd->chns[chn]);

    hd->chns[chn] = INVALID_SOCKET;
}

void vm_socket_close(vm_socket *hd)
{
    Ipp32s i;

    if ((hd->flags & VM_SOCKET_MCAST) && !(hd->flags&VM_SOCKET_SERVER))
    {
        struct ip_mreq imr;

        imr.imr_multiaddr.s_addr = hd->sar.sin_addr.s_addr;
        imr.imr_interface.s_addr = INADDR_ANY;
        setsockopt(hd->chns[0],
                IPPROTO_IP,
                IP_DROP_MEMBERSHIP,
                (Ipp8s *) &imr,
                sizeof(imr));
    }
    for (i= VM_SOCKET_QUEUE; i >= 0; i--)
        vm_socket_close_chn(hd, i);

    WSACleanup();
}

Ipp32s vm_sock_startup(vm_char a, vm_char b)
{
    Ipp16u wVersionRequested = MAKEWORD(a, b);
    WSADATA wsaData;
//    Ipp32s nRet;

    /*
    // Initialize WinSock and check version
    */
//    nRet = WSAStartup(wVersionRequested, &wsaData);
    WSAStartup(wVersionRequested, &wsaData);
    if (wsaData.wVersion != wVersionRequested)
    {
        return 1;
    }

    return 0;
}

Ipp32s vm_sock_cleanup(void)
{
    return WSACleanup();
}

static Ipp32u vm_inet_addr(const vm_char *szName)
{
    Ipp32u ulRes;

#if defined(UNICODE)||defined(_UNICODE)
    Ipp32s iStrSize = (Ipp32s)vm_string_strlen(szName);
    Ipp8s *szAnciStr = malloc(iStrSize + 2);

    if (NULL == szAnciStr)
        return INADDR_NONE;
    if (0 == WideCharToMultiByte(CP_ACP, 0, szName, iStrSize, szAnciStr,
                                 iStrSize + 2, 0, NULL))
    {
        free(szAnciStr);
        return INADDR_NONE;
    }
    ulRes = inet_addr(szAnciStr);
    free(szAnciStr);

#else
    ulRes = inet_addr(szName);
#endif

    return ulRes;
}

static struct hostent *vm_gethostbyname(const vm_char *szName)
{
    struct hostent *pRes = NULL;

#if defined(UNICODE)||defined(_UNICODE)
    Ipp32s iStrSize = (Ipp32s)vm_string_strlen(szName);
    Ipp8s *szAnciStr = malloc(iStrSize + 2);

    if (NULL == szAnciStr)
        return NULL;
    if (0 == WideCharToMultiByte(CP_ACP, 0, szName, iStrSize, szAnciStr,
                                 iStrSize + 2, 0, NULL))
    {
        free(szAnciStr);
        return NULL;
    }

    pRes = gethostbyname(szAnciStr);
    free(szAnciStr);
#else
    pRes = gethostbyname(szName);
#endif

    return pRes;
}

Ipp32s vm_sock_host_by_name(vm_char * name, struct in_addr * paddr)
{
    LPHOSTENT lpHostEntry;

    paddr->S_un.S_addr = vm_inet_addr(name);

    if (paddr->S_un.S_addr == INADDR_NONE)
    {
        lpHostEntry = vm_gethostbyname(name);
        if (NULL == lpHostEntry)
            return 1;
        else
            *paddr = *((LPIN_ADDR)*lpHostEntry->h_addr_list);
    }

    return 0;
}

Ipp32s vm_socket_get_client_ip(vm_socket *hd, Ipp8u* buffer, Ipp32s len)
{
    size_t iplen;
    if (hd == NULL)
        return -1;
    if (hd->chns[0] == INVALID_SOCKET)
        return -1;

    iplen = strlen(inet_ntoa(hd->sal.sin_addr));

    if (iplen >= (size_t)len || buffer == NULL)
        return -1;

    memcpy(buffer,inet_ntoa(hd->sal.sin_addr),iplen);
    buffer[iplen]='\0';

    return 0;
}
#endif
