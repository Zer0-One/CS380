#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_SIZE 2048
#define PORT "7777"
#define BACKLOG 50
#define PTR_HOST 100 //maximum length for DNS names retrieved via reverse-lookup

/**
 * Reaps dead children (y u so morbid unix)
 **/
void reap(int signal){
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0){
        continue;
    }
    errno = saved_errno;
}

void echo(int sockfd){
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    while((bytes_read = recv(sockfd, buffer, BUFFER_SIZE, 0)) > 0){
        if(send(sockfd, buffer, bytes_read, 0) != bytes_read){
            printf("Error copying data to socket: %s\n", strerror(errno));
            _exit(-1);
        }
    }

    if(bytes_read == -1){
        printf("Error reading from socket: %s\n", strerror(errno));
        _exit(-1);
    }
}

/**
 * Returns listening socket
 **/
int inet_listen(){
    struct addrinfo hints;
    struct addrinfo* result;
    int l_socket = 0;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_PASSIVE;

    int s = getaddrinfo(NULL, PORT, &hints, &result);
    if(s != 0){
        printf("Error getting addr info: %s\n", gai_strerror(s));
        _exit(-1);
    }

    for(; result != NULL; result = result->ai_next){
        l_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if(l_socket == -1){
            continue;
        }

        int optval = 1;
        if(setsockopt(l_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1){
            close(l_socket);
            freeaddrinfo(result);
            printf("Error setting socket options: %s\n", strerror(errno));
            return -1;
        }

        if(bind(l_socket, result->ai_addr, result->ai_addrlen) == 0){
            break;
        }

        close(l_socket);
    }

    if(result != NULL){
        if(listen(l_socket, BACKLOG) == -1){
            freeaddrinfo(result);
            return -1;
        }
    }

    freeaddrinfo(result);
    return (result == NULL) ? -1 : l_socket;
}

int main(){
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = reap;
    if(sigaction(SIGCHLD, &sa, NULL) == -1){
        printf("Error installing signal handler for SIGCHLD: %s\n", strerror(errno));
        _exit(-1);
    }

    int listen_socket = inet_listen();
    if(listen_socket == -1){
        printf("Error: Could not create server socket\n");
        _exit(-1);
    }

    int client_socket = 0;
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    char ptr_host[PTR_HOST];
    int ptr;
    while(1){
        client_socket = accept(listen_socket, (struct sockaddr*)&addr, &addr_len);
        if(client_socket == -1){
            printf("Error: Could not accept incoming socket connection: %s\n", strerror(errno));
            _exit(-1);
        }
        
        switch(fork()){
            case -1:
                printf("Error: Could not create child process: %s\n", strerror(errno));
                close(client_socket);
                break;
            case 0:
                close(listen_socket);
                ptr = getnameinfo((struct sockaddr*)&addr, addr_len, ptr_host, (socklen_t)PTR_HOST, NULL, 0, 0);
                if(ptr != 0){
                    printf("Error: Unable to perform reverse-lookup for connected client: %s\n", strerror(ptr));
                }
                printf("Client Connected: %s\n", ptr_host);
                echo(client_socket);
                _exit(0);
            default:
                close(client_socket);
                break;
        }
    }
}
