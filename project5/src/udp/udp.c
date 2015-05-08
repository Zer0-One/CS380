#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define HOST "45.50.5.238"
#define PORT "38005"

#define IP_VERSION 4
#define IPV4_HEADER_LENGTH 20

#define UDP_HEADER_LENGTH 8
#define UDP_SRC_PORT 6666

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

struct udphdr craft_udp_header(uint16_t dport, struct iphdr ip_header, uint16_t data_size, uint8_t* data){
    struct udphdr udp_header;
    udp_header.uh_sport = htons(UDP_SRC_PORT);
    udp_header.uh_dport = htons(dport);
    udp_header.uh_ulen = htons(UDP_HEADER_LENGTH + data_size);
    udp_header.uh_sum = 0;
    
    //Create the pseudo-header. The magic number here is the number of bytes
    //that come from the ip header
    uint8_t pseudo_header[sizeof(udp_header) + 12 + data_size];
    uint8_t* ptr = pseudo_header;

    memcpy(ptr, &ip_header.saddr, sizeof(ip_header.saddr));
    ptr += sizeof(ip_header.saddr);

    memcpy(ptr, &ip_header.daddr, sizeof(ip_header.daddr));
    ptr += sizeof(ip_header.daddr);
   
    *ptr = 0; ptr++;

    memcpy(ptr, &ip_header.protocol, sizeof(ip_header.protocol));
    ptr += sizeof(ip_header.protocol);

    memcpy(ptr, &udp_header.uh_ulen, sizeof(udp_header.uh_ulen));
    ptr += sizeof(udp_header.uh_ulen);

    memcpy(ptr, &udp_header.uh_sport, sizeof(udp_header.uh_sport));
    ptr += sizeof(udp_header.uh_sport);
    
    memcpy(ptr, &udp_header.uh_dport, sizeof(udp_header.uh_dport));
    ptr += sizeof(udp_header.uh_dport);
   
    //Still not clear on why udp includes this twice in the checksum
    memcpy(ptr, &udp_header.uh_ulen, sizeof(udp_header.uh_ulen));
    ptr += sizeof(udp_header.uh_ulen);
    
    memcpy(ptr, &udp_header.uh_sum, sizeof(udp_header.uh_sum));
    ptr += sizeof(udp_header.uh_sum);

    memcpy(ptr, data, data_size);
    
    udp_header.uh_sum = checksum((uint16_t*)pseudo_header, sizeof(pseudo_header));
    return udp_header;
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
    ip_header.protocol = IPPROTO_UDP;
    ip_header.daddr = daddr;
    ip_header.saddr = saddr;
    ip_header.check = 0;
    ip_header.check = checksum((uint16_t*)&ip_header, IPV4_HEADER_LENGTH);
    return ip_header;
}

uint16_t handshake(int remote_socket, uint8_t* buffer){
    struct iphdr ip_header = craft_v4_header(4);
    uint32_t handshake = htonl(0xDEADBEEF);
    uint16_t dport = 0;

    memcpy(buffer, &ip_header, sizeof(ip_header));
    memcpy(buffer + sizeof(ip_header), &handshake, sizeof(handshake));

    printf("Sending Handshake: %#08x\n", ntohl(handshake));
    send(remote_socket, buffer, sizeof(ip_header) + sizeof(handshake), 0);
    recv(remote_socket, buffer, sizeof(handshake), 0);
    printf("Server Handshake Response: %#08x\n", ntohl(*(unsigned int*)buffer));
    recv(remote_socket, &dport, sizeof(dport), 0);
    printf("UDP Destination Port: %d\n\n", ntohs(dport));
    return ntohs(dport);
}

int main(){
    uint8_t buffer[IP_MAXPACKET];
    struct iphdr ip_header;
    struct udphdr udp_header;
    uint16_t dport = 0;
    int remote_socket = inet_connect();
    
    if(remote_socket == -1){
        _exit(-1);
    }

    dport = handshake(remote_socket, buffer);

    for(int i = 1; i < 13; i++){
        ip_header = craft_v4_header(sizeof(udp_header) + pow(2, i));
        uint8_t random_data[(int)pow(2,i)];
        //fill the buffer with random data
        for(int j = 0; j < pow(2,i); j++){
            random_data[j] = rand();
        }
        udp_header = craft_udp_header(dport, ip_header, pow(2, i), random_data);
        memcpy(buffer, &ip_header, sizeof(ip_header));
        memcpy(buffer + sizeof(ip_header), &udp_header, sizeof(udp_header));
        memcpy(buffer + sizeof(ip_header) + sizeof(udp_header), random_data, sizeof(random_data));
        printf("Sending Packet #%d, Data Size: %d bytes...\n", i, (int)pow(2, i));
        send(remote_socket, buffer, sizeof(ip_header) + sizeof(udp_header) + pow(2, i), 0);
        recv(remote_socket, buffer, IP_MAXPACKET, 0);
        printf("Server Response: %#08x\n\n", ntohl(*(unsigned int*)buffer));
        sleep(1);
    }
    
    return 0;
}
