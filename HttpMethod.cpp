//
// Created by wt on 2021/3/2.
//

#include "HttpMethod.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>          // for isspace
#include <sys/stat.h>       // for stat
#include <unistd.h>         // for pipe
#include <sys/socket.h>
#include <sys/wait.h>       // for waitpid

#define STDIN   0
#define STDOUT  1

#define URL_LEN 255
#define METH_ENV 255
#define QUERY_ENV 512
#define LENGTH_ENV 255

#define ISSpace(x) isspace((int)(x))

#define DEFAULT_URL "index.html"
#define PATH_FORMAT "htdocs%s"

HttpMethod::HttpMethod() {
    cgi = 0;
}

Get::Get(): HttpMethod() {
    memset(queryString, 0, sizeof(queryString));
}

Post::Post(): HttpMethod() {
    contentLen = -1;
}

/*******公共方法*********/
/* 格式化路径 */
/***********************/
int HttpMethod::FormatPath(const char *url) {
    memset(path, 0, sizeof(path));
    sprintf(path, PATH_FORMAT, url);
    if(path[strlen(path) - 1] == '/'){
        strcat(path, DEFAULT_URL);
    }
    char *test = path;
    struct stat st{};
    if(stat(path, &st) == -1){  /* stat 获取文件、文件夹的信息 */
        cgi = 0;
        return 0;
    }
    else{
        if((st.st_mode & S_IFMT) == S_IFDIR){   /* S_IFMT 遮照 S_IFDIR 目录 */
            strcat(path, "/");
            strcat(path, DEFAULT_URL);
        }
        /* 判断文件是否具有可执行权限（html是纯文本文件，不可执行）*/
        if((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH)){
            cgi = 1;
        }
    }
    return 1;
}

/*********************************公共方法************************************/
/* 获取报文头部的url，并格式化为path；判断path所指文件是否具有可执行权限。
 * 参数：buf 报文头部首行
 *      index url起始下标
 * 返回值：1 成功得到path，且path指向文件具有可执行权限
 *        0 文件不具有可执行权限 */
/****************************************************************************/
int HttpMethod::GetURL(const char *buf, int index) {
    int i = 0, buflen = strlen(buf);
    char url[URL_LEN];
    while(index < buflen && !ISSpace(buf[index]) && i < PATH_LEN){
        url[i] = buf[index];
        ++i, ++index;
    }
    if(index >= buflen || HttpMethod::FormatPath(url) == 0) return 0;
    return 1;
}

/****************公共方法*********************/
/* 发送一个常规文件给客户端
 * 返回值： 0 文件打开失败　1　文件发送成功*/
/********************************************/
int HttpMethod::ServeFile(int clientfd) {
    /* 打开文件 */
    FILE *file = nullptr;
    file = fopen(path, "r");
    if(file == nullptr){
        return 0;
    }

    /* 发送头部 */
    /* ps：下面的strlen换成sizeof浏览器会无法打开网页 */
    char buf[1024];
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    send(clientfd, buf, strlen(buf), 0);
    sprintf(buf, "Server: myhttpd/0.1.0\r\n");
    send(clientfd, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(clientfd, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(clientfd, buf, strlen(buf), 0);

    /* 发送文件内容 */
    memset(buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf), file);
    while (!feof(file)){
        send(clientfd, buf, strlen(buf), 0);
        fgets(buf, sizeof(buf), file);
    }

    fclose(file);
    return 1;
}

/*********************************GET方法************************************/
/* 获取报文头部的url，并格式化为path(如果url带参数，则将参数保存至queryString);
 * 判断path所指文件是否具有可执行权限。
 * 参数：buf 报文头部首行
 *      index url起始下标
 * 返回值：1 成功得到path及queryString(如果有的话)，且path指向文件具有可执行权限
 *        0 文件不具有可执行权限 */
/****************************************************************************/
int Get::GetURL(const char *buf, int index) {
    char url[URL_LEN];
    int i = 0, buflen = strlen(buf);
    while(index < buflen && i < URL_LEN && !ISSpace(buf[index]) && buf[index] != '?'){
        url[i] = buf[index];
        ++i, ++index;
    }
    url[i] = '\0';

    /* 如果url带参数 */
    if(index < buflen && buf[index] == '?'){
        cgi = 1;
        i = 0, ++index;
        while(index < buflen && !ISSpace(buf[index]) && i < QUERY_LEN){
            queryString[i] = buf[index];
            ++i, ++index;
        }
    }

    if(HttpMethod::FormatPath(url) == 0){
        return 0;
    }
    return 1;
}

/********************GET方法*******************/
/* 执行请求
 * 返回值: 0 执行成功
 *        1　CGI执行失败
 *        2  常规文件打开失败 */
/*********************************************/
int Get::ExecuteRequest(int clientfd) {
    if(cgi){
        if((Get::ExecuteCGI(clientfd)) == 0)
            return 1;
    }
    else{
        if((HttpMethod::ServeFile(clientfd)) == 0)
            return 2;
    }
    return 0;
}

/********************GET方法*************************/
/* 执行CGI脚本 */
/* stdin --> stdout
 * cgi_xxx[1] --> cgi_xxx[0]  0 读入端　１写入端 */
/***************************************************/
int Get::ExecuteCGI(int clientfd) {
    /* 打开管道 */
    int cgi_input[2];
    int cgi_output[2];

    char buf[] = "HTTP/1.0 200 OK\r\n";
    send(clientfd, buf, sizeof(buf), 0);

    if(pipe(cgi_input) < 0 || pipe(cgi_output) < 0){
        return 0;
    }

    /* 开启一个子进程 */
    pid_t pid;
    if((pid = fork()) < 0) return 0;

    if(pid == 0){   /* 子进程：执行CGI脚本 */
        /* 重定向到标准输出、输入端 */
        dup2(cgi_output[1], STDOUT);
        dup2(cgi_input[0], STDIN);

        close(cgi_input[1]);
        close(cgi_output[0]);

        /* 设置环境变量 */
        char meth_env[METH_ENV];
        char query_env[QUERY_ENV];
        sprintf(meth_env, "REQUEST_METHOD=%s", "GET");
        sprintf(query_env, "QUERY_STRING=%s", queryString);
        putenv(meth_env);
        putenv(query_env);

        /* 执行CGI脚本 */
        execl(path, path, NULL);
        exit(0);
    }
    else{   /* 父进程 */
        close(cgi_output[1]);
        close(cgi_input[0]);

        char c;
        while(read(cgi_output[0], &c, 1) > 0){
            send(clientfd, &c, 1, 0);
        }

        close(cgi_output[0]);
        close(cgi_input[1]);

        int status;
        waitpid(pid, &status, 0);
    }
    return 1;
}

/********************POST方法*******************/
/* 执行请求
 * 返回值: 0 执行成功
 *        1　CGI执行失败
 *        2  常规文件打开失败 */
/*********************************************/
int Post::ExecuteRequest(int clientfd) {
    if(cgi){
        if((Post::ExecuteCGI(clientfd)) == 0)
            return 1;
    }
    else{
        if((HttpMethod::ServeFile(clientfd)) == 0)
            return 2;
    }
    return 0;
}

/*************POST方法*************/
/* 执行CGI脚本
 * 返回值：0 执行失败 1 执行成功*/
/*********************************/
int Post::ExecuteCGI(int clientfd) {
    /* 打开管道 */
    int cgi_input[2];
    int cgi_output[2];

    char buf[] = "HTTP/1.0 200 OK\r\n";
    send(clientfd, buf, sizeof(buf), 0);

    if(pipe(cgi_output) < 0){
        return 0;
    }

    if(pipe(cgi_input) < 0){
        return 0;
    }

    /* 开启一个子进程 */
    pid_t pid;
    if((pid = fork()) < 0) return 0;

    if(pid == 0){   /* 子进程：执行CGI脚本 */
        /* 重定向到标准输出、输入端 */
        dup2(cgi_output[1], STDOUT);
        dup2(cgi_input[0], STDIN);

        close(cgi_input[1]);
        close(cgi_output[0]);

        /* 设置环境变量 */
        char meth_env[METH_ENV];
        char length_env[LENGTH_ENV];
        sprintf(meth_env, "REQUEST_METHOD=%s", "POST"); /* 这里的方法错填成GET时，color.cgi只会变蓝色 */
        putenv(meth_env);
        sprintf(length_env, "CONTENT_LENGTH=%d", contentLen);
        putenv(length_env);

        /* 执行CGI脚本 */
        execl(path, path, NULL);
        exit(0);
    }
    else{   /* 父进程 */
        close(cgi_output[1]);
        close(cgi_input[0]);

        char c;
        for(int i = 0;i < contentLen;++i){
            recv(clientfd, &c, 1, 0);
            write(cgi_input[1], &c, 1);
        }

        /* 读取CGI脚本返回数据 */
        while(read(cgi_output[0], &c, 1) > 0){
            send(clientfd, &c, 1, 0);
        }

        close(cgi_output[0]);
        close(cgi_input[1]);

        int status;
        waitpid(pid, &status, 0);
    }
    return 1;
}

/***********************POST方法**********************************/
/* POST需要保存头部中的content-length
 * 返回值：1 得到合法的content-length或者还未接收content-length字段
 *        0 不合法的content-length
 *       -1 不合法的buf */
/*****************************************************************/
int Post::DiscardHeader(char *buf) {
    if(buf == nullptr || strlen(buf) < 16) return -1;

    buf[15] = '\0';
    if(strcasecmp(buf, "Content-Length:") == 0){
        contentLen = atoi(&(buf[16]));
        if(contentLen == -1) return 0;
    }
    return 1;
}

HttpMethod* MethodFactory::GetMethod(const char *method) {
    if(strcasecmp(method, "GET") == 0){
        return new Get();
    }
    else if(strcasecmp(method, "POST") == 0){
        return new Post();
    }
    else
        return nullptr;
}