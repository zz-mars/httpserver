#ifdef _HTTP_SERV_H
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include "epoll_util.h"
#include <assert.h>
#include "hashtable.h"


// continue flag
static int g_iContinue = 0;
static int g_iEpollFd = -1;
static struct epoll_event g_szstEvents[MAX_EVENT_NR];

static int SetListenSocket(const char *pIp, const int iPort) {
    struct sockaddr_in stHttpServer;
    memset(&stHttpServer, 0, sizeof(stHttpServer));

    stHttpServer.sin_family = AF_INET;
    stHttpServer.sin_port = htons(iPort);
    if(inet_pton(AF_INET, pIp, &(stHttpServer.sin_addr)) <= 0) {
        fprintf(stderr, "inet_pton fail -> %s\n", strerror(errno));
        return -1;
    }

    int iListenFd = socket(AF_INET, SOCK_STREAM, 0);
    if(iListenFd < 0) {
        fprintf(stderr, "socket fail -> %s\n", strerror(errno));
        return -1;
    }

    // set reuse addr
    int iFlag = 1;
    if(setsockopt(iListenFd, SOL_SOCKET, SO_REUSEADDR, &iFlag, sizeof(iFlag)) != 0) {
        fprintf(stderr, "set reuse addr fail -> %s\n", strerror(errno));
        goto close_and_fail;
    }

    // set non-block
    if(set_nonblocking(iListenFd) != 0) {
        goto close_and_fail;
    }

    // bind
    if(bind(iListenFd, (struct sockaddr*)&stHttpServer, sizeof(stHttpServer)) != 0) {
        fprintf(stderr, "bind fail -> %s\n", strerror(errno));
        goto close_and_fail;
    }

    // listen
    if(listen(iListenFd, 1024) != 0) {
        fprintf(stderr, "set reuse addr fail -> %s\n", strerror(errno));
        goto close_and_fail;
    }

    return iListenFd;

close_and_fail:
    close(iListenFd);
    return -1;
}

static int SetEpollFd(int iMaxEpollNr) {
    if((g_iEpollFd = epoll_create(iMaxEpollNr)) == -1) {
        fprintf(stderr, "epoll_create fail -> %s\n", strerror(errno));
        return -1;
    }
    return g_iEpollFd;
}

/* DoAccept : accept new connection 
 * set new connection non-blocking and register read event
 * return new connected socket fd on success, -1 on error */
static int DoAccept(int iListenFd) {
    struct sockaddr_in stNewConnectAddr;
    socklen_t iSockLen;
    // accept new connection
    int iNewConnectionFd = accept(iListenFd, (struct sockaddr*)&stNewConnectAddr, &iSockLen);
    if(iNewConnectionFd < 0) {
        if(errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        return -1;
    }

    // set non-blocking
    if(set_nonblocking(iNewConnectionFd) == -1) {
        goto close_new_connection_and_fail;
    }

    // register for read
    if(RegisterReadEvent(g_iEpollFd, iNewConnectionFd) == -1) {
        goto close_new_connection_and_fail;
    }
    return iNewConnectionFd;
close_new_connection_and_fail:
    close(iNewConnectionFd);
    return -1;
}

/* DoCleanUp : delete event and close fd */
static void DoCleanUp(int iSockFd) {
    DeleteEvent(g_iEpollFd, iSockFd);
    close(iSockFd);
}

// currently only GET request will be processed 
enum {
    REQ_INVALID = 0,
    REQ_GET,
    REQ_UNKNOWN
};

#define REQ_GET_STR "GET"
#define REQ_GET_STRLEN  3

/* ParseRequest : parse request, get the request type and argument */
static int ParseRequest(const char *pBuffer, char **pArg) {
    int iReqType = REQ_UNKNOWN;
    if(strncmp(pBuffer, REQ_GET_STR, REQ_GET_STRLEN) == 0) {
        iReqType = REQ_GET;
        char *pBuf = (char*)pBuffer + REQ_GET_STRLEN;
        // skip space
        while(*pBuf == ' ' || *pBuf == '\t'){ pBuf++; }
        // req : http:x.x.x.x:xxxxx/index.html
        // skip the '/' before the real argment
        *pArg = ++pBuf;
        while(*pBuf != ' ' && *pBuf != '\t') { pBuf++; }
        *pBuf = '\0';
    }
    return iReqType;
}

enum {
    HTTP_RESP_OK = 200,
    HTTP_RESP_BAD_REQ = 400,
    HTTP_RESP_FORBIDDEN = 403,
    HTTP_RESP_NOT_FOUND = 404
};


/* SendResp : prepare request data and send back to client */
static int SendResp(int iSockFd, int iRespCode, 
        char * pVal, char *pBuf, int iBufSz) {
#define MAX_MSG_SZ  1024
    static char szMsg[MAX_MSG_SZ] = {0};
    int iMsgLen = snprintf(szMsg, MAX_MSG_SZ, "<html><head>From grantchen's http server</head><body>");
    switch (iRespCode) {
        case HTTP_RESP_OK:
            iMsgLen += snprintf(szMsg+iMsgLen, MAX_MSG_SZ-iMsgLen, " Get ok -> %s </body></html>", pVal);
            break;
        case HTTP_RESP_BAD_REQ:
            iMsgLen += snprintf(szMsg+iMsgLen, MAX_MSG_SZ-iMsgLen, " bad request </body></html>");
            break;
        case HTTP_RESP_FORBIDDEN:
            iMsgLen += snprintf(szMsg+iMsgLen, MAX_MSG_SZ-iMsgLen, " forbidden </body></html>");
            break;
        case HTTP_RESP_NOT_FOUND:
            iMsgLen += snprintf(szMsg+iMsgLen, MAX_MSG_SZ-iMsgLen, " not found </body></html>");
            break;
        default:
            iMsgLen += snprintf(szMsg+iMsgLen, MAX_MSG_SZ-iMsgLen, " unknown request </body></html>");
            break;
    }
    // prepare data
    memset(pBuf, 0, iBufSz);
    int iDataLen = snprintf(pBuf, iBufSz, "HTTP/1.0 %u OK\r\n", iRespCode);
    iDataLen += snprintf(pBuf+iDataLen, iBufSz-iDataLen, "Content-Type: text/html; charset=utf-8\r\n");
    iDataLen += snprintf(pBuf+iDataLen, iBufSz-iDataLen, "Content-Length: %u\r\n\r\n", iMsgLen);
    memcpy(pBuf+iDataLen, szMsg, iMsgLen);
    iDataLen += iMsgLen;
    // send to client
    int iWriteLen = 0;
    while(iWriteLen < iDataLen) {
        int iOneWrite = write(iSockFd, pBuf+iWriteLen, iDataLen-iWriteLen);
        if(iOneWrite < 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                return 0;
            }
            // no retry for error situation
            return -1;
        }
        iWriteLen += iOneWrite;
    }
    printf("response -> %s\n", pBuf);
    return 0;
}

/* DoOnRead : get the request, process it and send response
 * return value : 0 on success
 *                -1 on error */
static int DoOnRead(int iSockFd) {
    // 1MB of public buffer for all requests
#define MAX_BUF_SZ  (1<<20)
    static char szPublicBuf[MAX_BUF_SZ];
    memset(szPublicBuf, 0, sizeof(szPublicBuf));

    if(read(iSockFd, szPublicBuf, MAX_BUF_SZ) < 0) {
        if(errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        return -1;
    }
    // for debug
    printf("request -> \n%s\n", szPublicBuf);
    char *pArg;
    int iRespCode;
    char * pVal = NULL;
    switch(ParseRequest(szPublicBuf,&pArg)) {
        case REQ_GET:
            pVal = C2DHashTable::GetInstance().Get(pArg);
            iRespCode = pVal==NULL?HTTP_RESP_NOT_FOUND:HTTP_RESP_OK;
            break;
        case REQ_INVALID:
        default:
            iRespCode = HTTP_RESP_BAD_REQ;
            break;
    }
    SendResp(iSockFd, iRespCode, pVal, szPublicBuf, MAX_BUF_SZ);
    return 0;
}

#ifdef _OLD_HOMEWORK
int main(int argc, char *argv[]) {
    if(argc != 5) {
        fprintf(stderr, "Usage : %s [ip] [port] [shm_key] [max_element_sz]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int iShmKey = atoi(argv[3]);
    uint32_t uiMaxElementNum = atoi(argv[4]);
    if(C2DHashTable::GetInstance().Init(iShmKey, 
                uiMaxElementNum, DEFAULT_HASH_TIMES) != 0) {
        exit(EXIT_FAILURE);
    }

    char *pIp = argv[1];
    int iPort = atoi(argv[2]);
    
    int iExitStatus = EXIT_FAILURE;
    int iListenFd = SetListenSocket(pIp, iPort);
    if(iListenFd == -1) {
        fprintf(stderr, "set listen socket fail!\n");
        goto destroy_ht_and_fail;
    }

    if(SetEpollFd(MAX_EVENT_NR) == -1) {
        goto close_listen_socket_and_fail;
    }

    if(RegisterReadEvent(g_iEpollFd, iListenFd) == -1) {
        goto close_epollfd_and_fail;
    }

    g_iContinue = 1;
    while(g_iContinue) {
        int iReadyFdNr = epoll_wait(g_iEpollFd, g_szstEvents, MAX_EVENT_NR, -1);
        if(iReadyFdNr == -1) {
            fprintf(stderr, "epoll_wait fail -> %s\n", strerror(errno));
            goto close_epollfd_and_fail;
        }
        // process ready fds
        for(int iEventIdx=0;iEventIdx<iReadyFdNr;iEventIdx++) {
            struct epoll_event *pEvent = &g_szstEvents[iEventIdx];
            if(pEvent->data.fd == iListenFd) {
                // unexpected listen socket epoll err
                if(EPOLL_ERR_OR_NOT_IN(pEvent)){
                    goto close_epollfd_and_fail;
                }
                // accept new connection
                if(DoAccept(iListenFd) == -1) {
                    // new connection fail
                    // leave it alone
                }
            } else {
                // unexpected client socket epoll err
                if(EPOLL_ERR_OR_NOT_IN(pEvent)){
                    // leave it alone
                } else if(DoOnRead(pEvent->data.fd) == -1) {
                    // leave it alone
                }
                // clean up anyway
                DoCleanUp(pEvent->data.fd);
            }
        }
    }

    iExitStatus = EXIT_SUCCESS;
close_epollfd_and_fail:
    close(g_iEpollFd);
close_listen_socket_and_fail:
    close(iListenFd);
destroy_ht_and_fail:
    HashTableDestory();
    exit(iExitStatus);
}

#endif

#endif
