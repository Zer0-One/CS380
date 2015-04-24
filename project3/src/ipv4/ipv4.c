#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 2048
#define HOST "45.50.5.238"
#define PORT "38003"

#define IP_VERSION 4
#define IPV4_HEADER_LENGTH 20

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
    //if(fcntl(remote_socket, F_SETFL, O_NONBLOCK) == -1){
    //    printf("Error: Could not put remote socket into non-blocking mode: %s\n", strerror(errno));
    //    close(remote_socket);
    //    return -1;
    //}
    freeaddrinfo(result);
    return remote_socket;
}

uint16_t checksum(uint16_t* ip_header, size_t length){
    uint32_t sum = 0;
    //add up 16-bit chunks
    while(length > 1){
        sum += *(ip_header++);
        length -= 2;
    }
    
    //add left-over byte. This won't ever happen for our example, but i'm
    //putting it here for posterity
    if(length > 0){
        sum += *(uint8_t*)ip_header;
    }

    //shift over the carry until the left 16 bits are clear
    while(sum >> 16){
        sum = (sum & 0xffff) + (sum >> 16);
    }

    return ~sum;
}

struct iphdr craft_v4_header(size_t data_size){
    struct iphdr ip_header;
    uint32_t daddr, saddr;
    inet_pton(AF_INET, HOST, &daddr);
    inet_pton(AF_INET, "6.6.6.0", &saddr);
   
    ip_header.ihl = IPV4_HEADER_LENGTH / 4;
    ip_header.version = IP_VERSION;
    ip_header.tos = 0;
    ip_header.tot_len = htons(IPV4_HEADER_LENGTH + data_size);
    ip_header.id = 0;
    ip_header.frag_off = 0x40;
    ip_header.ttl = 50;
    ip_header.protocol = IPPROTO_TCP;
    ip_header.daddr = daddr;
    ip_header.saddr = saddr;
    ip_header.check = 0;
    ip_header.check = checksum((uint16_t*)&ip_header, IPV4_HEADER_LENGTH);
    return ip_header;
}

int main(){
    char buffer[BUFFER_SIZE];
    struct iphdr ip_header;
    int remote_socket = inet_connect();
    
    if(remote_socket == -1){
        _exit(-1);
    }

    for(int i = 1; i < 13; i++){
        ip_header = craft_v4_header(pow(2, i));
        memcpy(buffer, &ip_header, sizeof(ip_header));
        printf("Sending Packet #%d, Data Size: %d bytes...\n", i, (int)pow(2, i));
        send(remote_socket, buffer, sizeof(ip_header) + pow(2, i), 0);
        recv(remote_socket, buffer, BUFFER_SIZE, 0);
        printf("Server Response: %s\n", buffer);
        sleep(1);
    }
    
    return 0;
}
