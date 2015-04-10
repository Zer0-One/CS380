#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 2048
#define DEFAULT_PORT "7777"

void print_usage(){
    printf("Usage: echo_client [-p port_number] [-u] hostname\n");
    printf("\n-p,\tPort number (7777 by default)");
    printf("\n-u,\tUDP mode\n");
    printf("\n-h,\tPrints this usage information\n");
}

int inet_connect(char* host, char* service, int sock_type){
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = sock_type;

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

int main(int argc, char* argv[]){
    int opt;
    int sock_type = SOCK_STREAM;
    char* host;
    char* port = DEFAULT_PORT;
    bool udp = false;
    while((opt = getopt(argc, argv, "p:u:h")) != -1){
        switch(opt){
            case 'h':
                break;
            case 'p':
                port = optarg;
                break;
            case 'u':
                sock_type = SOCK_DGRAM;
                break;
            case '?':
                print_usage();
                break;
        }
    }
    if(argv[optind] == NULL){
        print_usage();
        _exit(-1);
    }
    host = argv[optind];

    int remote_socket = inet_connect(host, port, sock_type);
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
                printf("\n%s>%.*s", host, (int)bytes_read, buffer);
            }

        default:
            for(;;){
                printf("echo>");
                fflush(stdout);
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
                    printf("Error: Could not send to server: %s\n", strerror(errno));
                    _exit(-1);
                }
            }
    }
}
