#include <netinet/in.h>

struct client_socket_info {
    int connfd;
    struct sockaddr_in client_addr;
};


