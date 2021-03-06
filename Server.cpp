#include "HttpWebServer.h"
#include <cstdio>

int main(){
    HttpWebServer server;
    u_short port = 30405;
    server.InitServer(port);
    printf("httpd running on port %d\n", port);
    sockaddr_in clientaddr{};
    while (true){
        clientaddr = server.Accept();
        printf("New connection... ip: %s, port: %d\n",
               inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
        server.HandleRequest();
    }
    return 0;
}
