//
// Created by wt on 2021/3/3.
//

#ifndef MYTINYHTTPD_HTTPWEBSERVER_H
#define MYTINYHTTPD_HTTPWEBSERVER_H

#include <arpa/inet.h>

class HttpWebServer {
public:
    HttpWebServer();
    void InitServer(u_short &port);
    sockaddr_in Accept();
    int RecvLine(char *buf, int buflen) const;
    void HandleRequest();
    ~HttpWebServer();

protected:
    void CanNotExecute() const;
    void NotFound() const;
    void BadRequest() const;
    void Unimplemented() const;

private:
    int m_listenfd;
    int m_clientfd;
};


#endif //MYTINYHTTPD_HTTPWEBSERVER_H
