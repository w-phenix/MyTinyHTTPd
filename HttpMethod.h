//
// Created by wt on 2021/3/2.
//

#ifndef MYTINYHTTPD_HTTPMETHOD_H
#define MYTINYHTTPD_HTTPMETHOD_H

#define PATH_LEN 512
#define QUERY_LEN 255


class HttpMethod {
public:
    HttpMethod();
    virtual int ExecuteRequest(int clientfd) = 0;
    virtual int ExecuteCGI(int clientfd) = 0;
    virtual int GetURL(const char *buf, int index);
    int FormatPath(const char *url);
    int ServeFile(int clientfd);
    virtual int DiscardHeader(char *buf) { return 1;};
    virtual ~HttpMethod() = default;

protected:
    int cgi;
    char path[PATH_LEN]{};
};

class Get: public HttpMethod{
public:
    Get();
    virtual int GetURL(const char *buf, int index);
    virtual int ExecuteRequest(int clientfd);
    virtual int ExecuteCGI(int clientfd);

private:
    char queryString[QUERY_LEN]{};
};

class Post: public HttpMethod{
public:
    Post();
    virtual int ExecuteRequest(int clientfd);
    virtual int ExecuteCGI(int clientfd);
    virtual int DiscardHeader(char *buf);

private:
    int contentLen;
};

class MethodFactory{
public:
    static HttpMethod* GetMethod(const char *method);
};

#endif //MYTINYHTTPD_HTTPMETHOD_H
