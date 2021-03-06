//
// Created by wt on 2021/3/4.
//

#include "SimpleClient.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]){
    SimpleClient client;
    client.InitClient();
    client.Connect(argv[1], argv[2]);
    char buf[1024];
    sprintf(buf, "GET / HTTP/1.1\r\n\r\n");
    client.Send(buf, sizeof(buf));
    printf("Send: %s", buf);

    memset(buf, 0, sizeof(buf));
    int n = client.Recv(buf, sizeof(buf));
    while(n > 0){
        printf("Receive: %s", buf);
        n = client.Recv(buf, sizeof(buf));
    }
}