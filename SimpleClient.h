//
// Created by wt on 2021/3/4.
//

#ifndef MYTINYHTTPD_SIMPLECLIENT_H
#define MYTINYHTTPD_SIMPLECLIENT_H


class SimpleClient {
public:
    SimpleClient();
    void InitClient();
    void Connect(const char *ip, const char *port);
    void Send(const char *buf, int buflen);
    int Recv(char *buf, int buflen);
    ~SimpleClient();

private:
    int m_clientfd;
};


#endif //MYTINYHTTPD_SIMPLECLIENT_H
