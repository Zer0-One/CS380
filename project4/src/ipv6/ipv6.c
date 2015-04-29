#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <netdb.h>
#include <netinet/ip6.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 2048
#define HOST "45.50.5.238"
#define HOST_v6_MAPPED "::FFFF:45.50.5.238"
#define PORT "38004"

#define IP_VERSION 6
#define IPV6_HEADER_LENGTH 40

int inet_connect(){
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int s = getaddrinfo(HOST, PORT, &hints, &result);
    if(s != 0){
        printf("Error: Could not get address info: %s\n", gai_strerror(s));
        return -1;
    }
    
    int remote_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if(remote_socket == -1){
        printf("Error: Could not open remote socket: %s\n", strerror(errno));
        return -1;
    }
    if(connect(remote_socket, result->ai_addr, result->ai_addrlen) == -1){
        printf("Error: Could not connect to remote socket: %s\n", strerror(errno));
        close(remote_socket);
        return -1;
    }
    freeaddrinfo(result);
    return remote_socket;
}

struct ip6_hdr craft_v6_header(size_t data_size){
    struct ip6_hdr ip_header;
    struct in6_addr daddr, saddr;
    inet_pton(AF_INET6, HOST_v6_MAPPED, &daddr);
    inet_pton(AF_INET6, "::FFFF:6.6.6.0", &saddr);
  
    ip_header.ip6_flow = htonl(0x60000000);
    ip_header.ip6_plen = htons(data_size);
    ip_header.ip6_nxt = IPPROTO_UDP;
    ip_header.ip6_hlim = 20;
    ip_header.ip6_src = saddr;
    ip_header.ip6_dst = daddr;

    return ip_header;
}

int main(){
    char buffer[BUFFER_SIZE];
    struct ip6_hdr ip_header;
    int remote_socket = inet_connect();
    
    if(remote_socket == -1){
        _exit(-1);
    }

    for(int i = 1; i < 13; i++){
        ip_header = craft_v6_header(pow(2, i));
        memcpy(buffer, &ip_header, sizeof(ip_header));
        printf("Sending Packet #%d, Data Size: %d bytes...\n", i, (int)pow(2, i));
        send(remote_socket, buffer, sizeof(ip_header) + pow(2, i), 0);
        recv(remote_socket, buffer, BUFFER_SIZE, 0);
        //ignore this disgusting hack. the only reason i can do this is because
        //i know very specifically what the server is going to respond with
        printf("Server Response: %#08x\n\n", ntohl(*(unsigned int*)buffer));
        sleep(1);
    }
    
    return 0;
}
