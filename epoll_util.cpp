#include "epoll_util.h"

static int RawRegisterEvent(int iEpollFd, int iSockfd, uint8_t uiOP) {
    struct epoll_event stEv;
    if(uiOP & ZEPOLL_READ) {
        stEv.events |= EPOLLIN;
    }
    if(uiOP & ZEPOLL_WRITE) {
        stEv.events |= EPOLLOUT;
    }
 //   // set edge triger
//    stEv.events |= EPOLLET;
    stEv.data.fd = iSockfd;
    if(epoll_ctl(iEpollFd, EPOLL_CTL_ADD, iSockfd, &stEv) != 0) {
        fprintf(stderr, "epoll_ctl fail -> %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int RegisterReadEvent(int iEpollFd, int iSockfd) {
    return RawRegisterEvent(iEpollFd, iSockfd, ZEPOLL_READ);
}

int RegisterWriteEvent(int iEpollFd, int iSockfd) {
    return RawRegisterEvent(iEpollFd, iSockfd, ZEPOLL_WRITE);
}

int DeleteEvent(int iEpollFd, int iSockfd) {
    return epoll_ctl(iEpollFd, EPOLL_CTL_DEL, iSockfd, NULL);
}

int set_nonblocking(int iSockFd) {
    int iFlag = fcntl(iSockFd, F_GETFL, 0);
    if(iFlag < 0) {
        fprintf(stderr, "fcntl get flag fail -> %s\n", strerror(errno));
        return -1;
    }
    iFlag |= O_NONBLOCK;
    if(fcntl(iSockFd, F_SETFL, iFlag) < 0) {
        fprintf(stderr, "fcntl set flag fail -> %s\n", strerror(errno));
        return -1;
    }
    return 0;
}
