//
// Created by wt on 2021/3/3.
//

#include "HttpWebServer.h"
#include "HttpMethod.h"
#include <cstdio>          // for perror
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <cstring>         // for memset
#include <cctype>          // for isspace

#define ISSpace(x) isspace((int)(x))

#define BUFF_SIZE 1024
#define METHOD_LEN 255
#define SERVER_STRING "Server: myhttpd/0.1.0\r\n"

inline void ErrorDie(const char *msg){
    perror(msg);
    exit(1);
}

HttpWebServer::HttpWebServer() {
    m_clientfd = 0;
    m_listenfd = 0;
}

HttpWebServer::~HttpWebServer() {
    if(m_listenfd != 0) close(m_listenfd);
    if(m_clientfd != 0) close(m_clientfd);
}

/* 初始化服务器：socket()->bind()->listetn()*/
void HttpWebServer::InitServer(u_short &port) {
    /* 1.创建套接字 */
    if((m_listenfd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        ErrorDie("socket");
    }

    /* 保证端口释放后可以立即使用 */
    int on = 1;
    if((setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0){
        ErrorDie("setsockopt failed.");
    }

    /* 2.绑定套接字 */
    sockaddr_in serveraddr{};
    memset(&serveraddr, 0, sizeof(serveraddr));
    socklen_t serveraddrlen = sizeof(serveraddr);
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(m_listenfd, (struct sockaddr*)&serveraddr, serveraddrlen) < 0){
        ErrorDie("bind");
    }

    /* 保存动态绑定的端口 */
    if(port == 0){
        if(getsockname(m_listenfd, (struct sockaddr*)&serveraddr, &serveraddrlen) < 0){
            ErrorDie("getsockname");
        }
        port = ntohs(serveraddr.sin_port);
    }

    /* 3.开启监听 */
    if(listen(m_listenfd, 5) < 0){
        ErrorDie("listen");
    }
}

/* 从请求队列中取出一个请求，建立连接 */
sockaddr_in HttpWebServer::Accept() {
    if(m_listenfd == 0){
        ErrorDie("Please call InitServer first.");
    }

    sockaddr_in clientaddr{};
    socklen_t clientlen = sizeof(clientaddr);
    if((m_clientfd = accept(m_listenfd, (struct sockaddr*)&clientaddr, &clientlen)) < 0){
        ErrorDie("accept");
    }
    return clientaddr;
}

/* 从客户端发过来的报文中读取一行
 * 返回：读取的行的字符数 */
int HttpWebServer::RecvLine(char *buf, int buflen) const {
    char ch = '\0';
    int i = 0, n;
    while((i < buflen - 1) && (ch != '\n')){
        if((recv(m_clientfd, &ch, 1, 0)) <= 0){
            ch = '\n';
        }
        else{
            if(ch == '\r'){
                n = recv(m_clientfd, &ch, 1, MSG_PEEK);
                if(n > 0 && ch == '\n'){
                    recv(m_clientfd, &ch, 1, 0);
                }
                else{
                    ch = '\n';
                }
            }
            buf[i] = ch;
            ++i;
        }
    }
    buf[i] = '\0';
    return i;
}

/* 处理请求 */
void HttpWebServer::HandleRequest() {
    /* 接收报文首行 */
    char buf[BUFF_SIZE];
    memset(buf, 0, BUFF_SIZE);
    int buflen = RecvLine(buf, BUFF_SIZE);
    //printf("Receive: %s\n", buf);

    /* 提取方法 */
    char method[METHOD_LEN];
    int i, j;
    for(i = 0, j = 0;i < METHOD_LEN - 1 && j < buflen && !ISSpace(buf[j]);++i, ++j){
        method[i] = buf[j];
    }
    method[i] = '\0';

    /* 处理请求：根据不同方法进行不同处理 */
    HttpMethod *httpMethod = MethodFactory::GetMethod(method);
    if(httpMethod == nullptr){ /* 返回nullptr说明方法未实现 */
        Unimplemented();
        return;
    }
    if((httpMethod->GetURL(buf, ++j)) == 0){  /* 得到的资源path不合理或不存在 */
        while((buflen > 0) && strcmp(buf, "\n") != 0){   /* 丢弃头部 */
            buflen = RecvLine(buf, BUFF_SIZE);
        }
        NotFound();
    }
    else{
        while((buflen > 0) && strcmp(buf, "\n") != 0){   /* 丢弃头部 */
            buflen = RecvLine(buf, BUFF_SIZE);
            //printf("Receive: %s\n", buf);
            if((httpMethod->DiscardHeader(buf)) == 0){    /* 报头中的content-length字段值不合法 */
                BadRequest();
                return;
            }
        }
        int res = httpMethod->ExecuteRequest(m_clientfd);
        if(res == 1){    /* CGI：打开管道或开启子进程失败，无法执行CGI脚本 */
            CanNotExecute();
            return;
        }
        else if(res == 2){  /* 常规文件：打开文件失败　*/
            NotFound();
            return;
        }
    }
    close(m_clientfd);  /* 执行完请求后需要关闭套接字（无连接的） */
    printf("connection close... clientfd: %d\n", m_clientfd);
}

/* 给客户端发送一个404 not found状态信息 */
void HttpWebServer::NotFound() const {
    char buf[BUFF_SIZE];

    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(m_clientfd, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(m_clientfd, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(m_clientfd, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(m_clientfd, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(m_clientfd, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(m_clientfd, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(m_clientfd, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(m_clientfd, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(m_clientfd, buf, strlen(buf), 0);
}

/* 通知客户端CGI脚本无法执行 */
void HttpWebServer::CanNotExecute() const {
    char buf[BUFF_SIZE];

    sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
    send(m_clientfd, buf, strlen(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(m_clientfd, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(m_clientfd, buf, strlen(buf), 0);
    sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
    send(m_clientfd, buf, strlen(buf), 0);
}

/* 通知客户端请求有问题 */
void HttpWebServer::BadRequest() const {
    char buf[BUFF_SIZE];

    sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
    send(m_clientfd, buf, sizeof(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(m_clientfd, buf, sizeof(buf), 0);
    sprintf(buf, "\r\n");
    send(m_clientfd, buf, sizeof(buf), 0);
    sprintf(buf, "<P>Your browser sent a bad request, ");
    send(m_clientfd, buf, sizeof(buf), 0);
    sprintf(buf, "such as a POST without a Content-Length.\r\n");
    send(m_clientfd, buf, sizeof(buf), 0);
}

/* 通知客户端请求的wed方法未实现 */
void HttpWebServer::Unimplemented() const {
    char buf[BUFF_SIZE];

    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
    send(m_clientfd, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(m_clientfd, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(m_clientfd, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(m_clientfd, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    send(m_clientfd, buf, strlen(buf), 0);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(m_clientfd, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
    send(m_clientfd, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(m_clientfd, buf, strlen(buf), 0);
}