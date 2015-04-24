#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <netdb.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include "echo.h"
#include "inet.h"

#define BUFFER_SIZE 2048
#define PORT "7777"
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

void print_usage(){
    printf("Usage: echod [-a address | -f [4 | 6]] [-s [udp | tcp]] [-p port_number]\n");
    printf("\n-a,\tAddress to listen on. echod binds to sockets on all addresses by default");
    printf("\n-f,\tAddress family to listen on. Use 4 for all IPv4 addresses, or 6 for all IPv6 addresses");
    printf("\n-s,\tSocket type. Use 'udp' for a datagram socket, or 'tcp' for a stream socket. By default, both are enabled");
    printf("\n-h,\tPrints this usage information\n");
}

int main(int argc, char* argv[]){
    int opt;
    int sock_type = 0;
    int address_family = AF_UNSPEC;
    char* port = PORT;
    char* host = NULL;
    while((opt = getopt(argc, argv, "a:f:p:s:h")) != -1){
        switch(opt){
            case 'h':
                print_usage();
                _exit(0);
            case 'a':
                if(address_family != AF_UNSPEC){
                    print_usage();
                    _exit(-1);
                }
                host = optarg;
                break;
            case 'f':
                if(host != NULL){
                    print_usage();
                    _exit(-1);
                }
                if(strncmp(optarg, "4",1) == 0){
                    address_family = AF_INET;
                }
                else if(strncmp(optarg, "6",1) == 0){
                    address_family = AF_INET6;
                }
                else{
                    print_usage();
                    _exit(-1);
                }
                break;
            case 's':
                if(strncmp(optarg, "udp", 3) == 0){
                    sock_type = SOCK_DGRAM;
                }
                else if(strncmp(optarg, "tcp", 3) == 0){
                    sock_type = SOCK_STREAM;
                }
                else{
                    print_usage();
                    _exit(-1);
                }
                break;
            case 'p':
                port = optarg;
                break;
            case '?':
                print_usage();
                _exit(-1);
        }
    }

    //install handler for SIGCHLD
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = reap;
    if(sigaction(SIGCHLD, &sa, NULL) == -1){
        printf("Error installing signal handler for SIGCHLD: %s\n", strerror(errno));
        _exit(-1);
    }

    //create listening sockets
    struct descriptor_list* sock_list = inet_listen(address_family, host, port, sock_type);
    if(sock_list == NULL){
        printf("Error: Could not create server sockets\n");
        _exit(-1);
    }

    struct descriptor_list* sock_index = sock_list;
    int client_socket = 0;
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    char ptr_host[PTR_HOST];
    int ptr;

    for(; sock_index->next != NULL; sock_index = sock_index->next){
        client_socket = accept(sock_index->sockfd, (struct sockaddr*)&addr, &addr_len);
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
                close(sock_index->sockfd);
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
