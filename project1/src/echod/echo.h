#ifndef __ECHO__
#define __ECHO__

#define BUFFER_SIZE 2048

/**
 * Echoes data coming in from the given socket back to that socket until either
 * the client closes the socket, or a read/write operation fails
 * \param sockfd A socket file descriptor whose output will be echoed back to
 * itself
 */
void echo(int sockfd);

#endif
