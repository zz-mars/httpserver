#ifndef _EPOLL_UTIL_H
#define _EPOLL_UTIL_H

#include <stdint.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

enum {
    ZEPOLL_READ = 0x1,
    ZEPOLL_WRITE = 0x2
};

#define MAX_EVENT_NR    (1<<10)

// register for read
// return 0 on success, -1 on error
int RegisterReadEvent(int iEpollFd, int iSockfd);

// register for write
// return 0 on success, -1 on error
int RegisterWriteEvent(int iEpollFd, int iSockfd);

// delete 
// return 0 on success, -1 on error
int DeleteEvent(int iEpollFd, int iSockfd);

// set nonblocking
// return 0 on success, -1 on error
int set_nonblocking(int iSockFd);

#define EPOLL_ERR_OR_NOT_IN(pEvent) ({  \
        struct epoll_event * _pEvent = (struct epoll_event*)(pEvent); \
                ((_pEvent->events & EPOLLERR) || \
                        (_pEvent->events & EPOLLHUP) ||  \
                        !(_pEvent->events & EPOLLIN)); }) 

#endif
