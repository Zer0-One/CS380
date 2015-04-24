#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "inet.h"

struct descriptor_list* inet_listen(int address_family, char* host, char* port, int sock_type){
    struct addrinfo hints;
    struct addrinfo* result;
    int listen_socket = 0;
    struct descriptor_list* sock_list = malloc(sizeof(struct descriptor_list));
    memset(sock_list, 0, sizeof(struct descriptor_list));
    struct descriptor_list* sock_index = sock_list;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = sock_type;
    hints.ai_family = address_family;
    hints.ai_flags = AI_PASSIVE;

    int s = getaddrinfo(host, port, &hints, &result);
    if(s != 0){
        printf("Error getting address info: %s\n", gai_strerror(s));
        return NULL;
    }

    for(; result != NULL; result = result->ai_next){
        listen_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if(listen_socket == -1){
            printf("Warning: Error creating socket: %s\n", strerror(errno));
            continue;
        }

        int optval = 1;
        if(setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1){
            close(listen_socket);
            freeaddrinfo(result);
            printf("Error setting socket option SO_REUSEADDR: %s\n", strerror(errno));
            return NULL;
        }
        if(result->ai_family == AF_INET6){
            if(setsockopt(listen_socket, IPPROTO_IPV6, IPV6_V6ONLY, &optval, sizeof(optval)) == -1){
                close(listen_socket);
                freeaddrinfo(result);
                printf("Error setting socket option IPV6_V6ONLY: %s\n", strerror(errno));
                return NULL;
            }
        }

        if(bind(listen_socket, result->ai_addr, result->ai_addrlen) != 0){
            close(listen_socket);
            printf("Warning: Could not bind socket to address %s: %s\n", result->ai_addr->sa_data, strerror(errno));
            continue;
        }

        if(sock_type == SOCK_STREAM){
            if(listen(listen_socket, BACKLOG) == -1){
                close(listen_socket);
                printf("Warning: Could not listen on socket: %s", strerror(errno));
                continue;
            }
        }

        sock_index->next = malloc(sizeof(struct descriptor_list));
        sock_index = sock_index->next;
        sock_index->sockfd = listen_socket;
        sock_index->next = NULL;
    }

    freeaddrinfo(result);
    return sock_list;
}
