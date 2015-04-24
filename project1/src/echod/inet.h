#ifndef __INET__
#define __INET__

#define BACKLOG 50

/**
 * A linked list of socket file descriptors
 */
struct descriptor_list{
    int sockfd;
    struct descriptor_list* next;
};

/**
 * Returns an array of listening socket file descriptors bound to either
 *  1. All local addresses that match the given address family, or 
 *  2. The given local address
 * One of \p address_family or \p host must be NULL, but not both
 * \param address_family If not null, the address family on which to create
 * listening sockets. Valid values are AF_INET for IPv4, AF_INET6 for IPv6, or
 * AF_UNSPEC for both
 * \param port The port on the given address family to listen on
 * \param host If not null, the address on which to bind a socket
 * \param sock_type The socket type. Valid values are SOCK_STREAM for tcp
 * stream sockets, SOCK_DGRAM for udp datagram sockets, and 0 for both
 * \param result The resultant list of bound sockets
 * \return On success, returns a pointer to a linked list of socket file
 * descriptors. On failure, returns null.
 */
struct descriptor_list* inet_listen(int address_family, char* host, char* port, int sock_type);

#endif
