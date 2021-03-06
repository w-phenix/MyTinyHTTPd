//
// Created by wt on 2021/3/4.
//

#include "SimpleClient.h"
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>

inline void ErrorDie(const char *msg){
    perror(msg);
    exit(1);
}

/* 构造函数 */
SimpleClient::SimpleClient() {
    m_clientfd = 0;
}

/* 析构函数 */
SimpleClient::~SimpleClient() {
    if(m_clientfd != 0) close(m_clientfd);
}

/* 初始化客户端 */
void SimpleClient::InitClient() {
    /* 1.建立套接字 */
    if((m_clientfd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        ErrorDie("socket");
    }
}

/* 连接服务器 */
void SimpleClient::Connect(const char *ip, const char *port) {
    sockaddr_in clientaddr;
    clientaddr.sin_family = AF_INET;
    clientaddr.sin_port = htons(atoi(port));
    clientaddr.sin_addr.s_addr = inet_addr(ip);
    if(connect(m_clientfd, (struct sockaddr*)&clientaddr, sizeof(clientaddr)) < 0){
        ErrorDie("connect");
    }
}

/* 发送数据 */
void SimpleClient::Send(const char *buf, int buflen) {
    if(send(m_clientfd, buf, buflen, 0) < 0){   /* ps:这里sizeof(buf)的话，得到的是指针的大小 */
        ErrorDie("send");                          /* 会导致每次只能发送8个字节(根据机器的不同而不同)的数据 */
    }
}

/* 接收数据 */
int SimpleClient::Recv(char *buf, int buflen) {
    if(recv(m_clientfd, buf, buflen, 0) < 0){
        ErrorDie("receive");
    }
    return buflen;
}

