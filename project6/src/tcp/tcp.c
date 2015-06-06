#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define HOST "45.50.5.238"
#define PORT "38006"

#define IP_VERSION 4
#define IPV4_HEADER_LENGTH 20

#define TCP_HEADER_LENGTH 20
#define TCP_SRC_PORT 6666
#define TCP_DST_PORT 6666

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

struct tcphdr craft_tcp_header(struct iphdr* ip_header, uint32_t seq, uint32_t ack_seq, int syn, int ack, int fin, uint16_t data_size, uint8_t* data){
    struct tcphdr tcp_header;
    tcp_header.source = htons(TCP_SRC_PORT);
    tcp_header.dest = htons(TCP_DST_PORT);
    tcp_header.seq = seq;
    tcp_header.ack_seq = ack_seq;
    tcp_header.res1 = 0;
    tcp_header.doff = TCP_HEADER_LENGTH / 4;
    tcp_header.fin = fin;
    tcp_header.syn = syn;
    tcp_header.rst = 0;
    tcp_header.psh = 0;
    tcp_header.ack = ack;
    tcp_header.urg = 0;
    tcp_header.res2 = 0;
    tcp_header.window = 0;
    tcp_header.check = 0;
    tcp_header.urg_ptr = 0;

    uint8_t pseudo_header[sizeof(tcp_header) + 12 + data_size];
    uint8_t* ptr = pseudo_header;

    memcpy(ptr, &ip_header->saddr, sizeof(ip_header->saddr));
    ptr += sizeof(ip_header->saddr);

    memcpy(ptr, &ip_header->daddr, sizeof(ip_header->daddr));
    ptr += sizeof(ip_header->daddr);

    *ptr = 0; ptr++;

    memcpy(ptr, &ip_header->protocol, sizeof(ip_header->protocol));
    ptr += sizeof(ip_header->protocol);

    uint16_t tcp_length = htons(TCP_HEADER_LENGTH + data_size);
    memcpy(ptr, &tcp_length, sizeof(tcp_length));
    ptr += sizeof(tcp_length);

    memcpy(ptr, &tcp_header, sizeof(tcp_header));
    ptr += sizeof(tcp_header);

    memcpy(ptr, data, data_size);

    tcp_header.check = checksum((uint16_t*)pseudo_header, sizeof(pseudo_header));

    return tcp_header;
}

uint16_t handshake(int remote_socket, uint8_t* buffer, uint32_t* ack_seq){
    uint32_t seq = rand();
    struct iphdr ip_header = craft_v4_header(TCP_HEADER_LENGTH);
    struct tcphdr tcp_header = craft_tcp_header(&ip_header, htonl(seq), 0, 1, 0, 0, 0, NULL);
    //syn
    printf("Sending Handshake Syn...\n");
    memcpy(buffer, &ip_header, sizeof(ip_header));
    memcpy(buffer + sizeof(ip_header), &tcp_header, sizeof(tcp_header));
    send(remote_socket, buffer, sizeof(ip_header) + sizeof(tcp_header), 0);
    recv(remote_socket, buffer, sizeof(uint32_t), 0);
    printf("Server Response: %#08x\n", ntohl(*(uint32_t*)buffer));
    recv(remote_socket, &tcp_header, sizeof(tcp_header), 0);
    printf("Received TCP Sequence Number: %u\n\n", ntohl(tcp_header.seq));
    
    *ack_seq = ntohl(tcp_header.seq) + 1;

    //ack
    tcp_header = craft_tcp_header(&ip_header, htonl(++seq), htonl(ntohl(tcp_header.seq)+1), 0, 1, 0, 0, NULL);
    memcpy(buffer, &ip_header, sizeof(ip_header));
    memcpy(buffer + sizeof(ip_header), &tcp_header, sizeof(tcp_header));
    printf("Sending Handshake Ack...\n");
    send(remote_socket, buffer, sizeof(ip_header) + sizeof(tcp_header), 0);
    recv(remote_socket, buffer, sizeof(uint32_t), 0);
    printf("Server Response: %#08x\n", ntohl(*(uint32_t*)buffer));

    return seq;
}

int main(){
    struct iphdr ip_header;
    struct tcphdr tcp_header;
    uint8_t buffer[IP_MAXPACKET];
    int remote_socket = inet_connect();

    if(remote_socket == -1){
        _exit(-1);
    }

    printf("Attempting Handshake\n");
    printf("-----------------------------------------\n");
    uint32_t ack_seq;
    uint32_t seq = handshake(remote_socket, buffer, &ack_seq)+1;
    printf("Handshake Complete!\n\n\nSending Data\n");
    printf("-----------------------------------------\n");

    for(int i = 1; i < 13; i++){
        ip_header = craft_v4_header(sizeof(tcp_header)+pow(2,i));
        uint8_t random_data[(int)pow(2,i)];
        //fill the buffer with random data
        for(int j = 0; j < pow(2,i); j++){
            random_data[j] = rand();
        }
        tcp_header = craft_tcp_header(&ip_header, htonl(seq), htonl(ack_seq), 0, 1, 0, pow(2,i), random_data);
        memcpy(buffer, &ip_header, sizeof(ip_header));
        memcpy(buffer + sizeof(ip_header), &tcp_header, sizeof(tcp_header));
        memcpy(buffer + sizeof(ip_header) + sizeof(tcp_header), &random_data, sizeof(random_data));
        printf("Sending Packet #%d, Data Size: %d bytes...\n", i, (int)pow(2,i));
        send(remote_socket, buffer, sizeof(ip_header) + sizeof(tcp_header) + pow(2,i), 0);
        seq += pow(2,i);
        recv(remote_socket, buffer, sizeof(uint32_t), 0);
        printf("Server Response: %#08x\n\n", ntohl(*(unsigned int*)buffer));
        sleep(1);
    }

    return 0;
}
