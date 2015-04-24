#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 2048
#define HOST "45.50.5.238"
#define PORT "38002"

int inet_connect(char* host, char* service){
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int s = getaddrinfo(host, service, &hints, &result);
    if(s != 0){
        printf("Error: Could not get address info: %s\n", gai_strerror(s));
        return -1;
    }

    int remote_socket = 0;
    for(; result != NULL; result = result->ai_next){
        remote_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if(remote_socket == -1){
            continue;
        }
        if(connect(remote_socket, result->ai_addr, result->ai_addrlen) != -1){
            break;
        }
        close(remote_socket);
    }
    freeaddrinfo(result);
    return (result == NULL) ? -1 : remote_socket;
}

int main(){
    int remote_socket = inet_connect(HOST, PORT);
    if(remote_socket == -1){
        _exit(-1);
    }
    
    ssize_t bytes_read;
    char buffer[BUFFER_SIZE];
    switch(fork()){
        case -1:
            printf("Error: Unable to fork: %s\n", strerror(errno));
            _exit(-1);
        case 0:
            for(;;){
                bytes_read = recv(remote_socket, buffer, BUFFER_SIZE, 0);
                if(bytes_read == -1){
                    printf("Error: Could not read from remote socket: %s\n", strerror(errno));
                    _exit(-1);
                }
                if(bytes_read == 0){
                    printf("The remote host ended the connection\n");
                    close(remote_socket);
                    _exit(0);
                }
                printf("%.*s", (int)bytes_read, buffer);
            }
        default:
            for(;;){
                bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE);
                if(bytes_read == -1){
                    printf("Error: Could not read from stdin: %s\n", strerror(errno));
                    _exit(-1);
                }
                if(bytes_read == 0){
                    shutdown(remote_socket, SHUT_WR);
                    _exit(0);
                }
                if(send(remote_socket, buffer, bytes_read, 0) == -1){
                    printf("Error: Could not send to serveer: %s\n", strerror(errno));
                    _exit(-1);
                }
            }
    }
}
