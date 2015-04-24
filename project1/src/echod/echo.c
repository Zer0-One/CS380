#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "echo.h"

void echo(int sockfd){
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    int sock_type;
    socklen_t optlen = sizeof(sock_type);

    getsockopt(sockfd, SOL_SOCKET, SO_TYPE, &sock_type, &optlen);

    if(sock_type == SOCK_STREAM){
        while((bytes_read = recv(sockfd, buffer, BUFFER_SIZE, 0)) > 0){
            if(send(sockfd, buffer, bytes_read, 0) != bytes_read){
                printf("Error copying data to socket: %s\n", strerror(errno));
                _exit(-1);
            }
        }
    }
    else{
        struct sockaddr_storage* src_addr = NULL;
        socklen_t addrlen = sizeof(*src_addr);
        while((bytes_read = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)src_addr, &addrlen)) > 0){
            if(sendto(sockfd, buffer, bytes_read, 0, (struct sockaddr*)src_addr, addrlen) != bytes_read){
                printf("Error copying data to socket: %s\n", strerror(errno));
                _exit(-1);
            }
        }
    }

    if(bytes_read == -1){
        printf("Error reading from socket: %s\n", strerror(errno));
        _exit(-1);
    }
}
