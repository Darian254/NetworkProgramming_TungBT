#define _GNU_SOURCE

#include "server.h"
#include "epoll.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/select.h>
#include <stdbool.h>

static int listen_sock = -1;

int set_nonblocking(int fd) {
    int rc;
    int on = 1;
    rc = ioctl(fd, FIONBIO, (char *)&on);
    if (rc < 0) {
        return -1;
    }
    return 0;
}

int server_init(void) {
    int rc, on = 1;
    struct sockaddr_in server_addr; 

    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        perror("socket() error:");
        exit(EXIT_FAILURE);
    }

    if (set_nonblocking(listen_sock) < 0) {
        perror("set_nonblocking() error:");
        close(listen_sock);
        exit(EXIT_FAILURE);
    }
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);
    rc = bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(rc < 0) {
        perror("bind() error:");
        close(listen_sock);
        exit(EXIT_FAILURE);
    }

    rc = listen(listen_sock, BACKLOG);
    if(rc < 0) {
        perror("listen() error:");
        close(listen_sock);
        exit(EXIT_FAILURE);
    }

    epoll_init(listen_sock);
    printf("Server listening on port %d\n", PORT); 
    return 0;
}

void server_run(void) {
    epoll_run();
}

void server_shutdown(void) {
    if (listen_sock >= 0) {
        close(listen_sock);
        listen_sock = -1;
    }
    printf("Server shutdown.\n");
}
