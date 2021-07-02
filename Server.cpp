#include "HttpWebServer.h"
#include <cstdio>
#include <iostream>
#include <pthread.h>

void* AcceptRequest(void* s){
    HttpWebServer *server = static_cast<HttpWebServer*>(s);
    server->HandleRequest();
}

int main(){
    HttpWebServer server;
    u_short port = 30405;
    server.InitServer(port);
    printf("httpd running on port %d\n", port);
    sockaddr_in clientaddr{};
    pthread_t newThread;
    while (true){
        clientaddr = server.Accept();
        printf("New connection... ip: %s, port: %d\n",
               inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
        // 启动线程处理新的连接
        if(pthread_create(&newThread, NULL, AcceptRequest, &server) != 0){
            perror("thread_create");
        }
        std::cout << "thread_id: " << newThread << std::endl << std::endl;
    }
    return 0;
}
