#include "epoll.h"
#include "config.h"
#include "connect.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

static int epollfd;
static int listen_sock;
static volatile sig_atomic_t epoll_should_stop = 0;

void epoll_init(int listen_fd) {
    listen_sock = listen_fd;
    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1() error:");
        close(listen_sock);
        exit(EXIT_FAILURE);
    }
    
    struct epoll_event ev; // Create event structure
    ev.events = EPOLLIN; // Monitor for input events
    ev.data.fd = listen_sock; // Associate with listening socket
    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1) {
        perror("epoll_ctl() error:");
        close(listen_sock);
        exit(EXIT_FAILURE);
    }
}

static void handle_accept(void) {
    while(1) {
        int client_sock = accept(listen_sock, NULL, NULL);
        if (client_sock < 0) {
            if(errno == EWOULDBLOCK || errno == EAGAIN) {
                break; // No more incoming connections
            } else {
                perror("accept() error:");
                break;
            }
        }
        connection_create(client_sock);

        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET; // Edge-triggered for client sockets
        ev.data.fd = client_sock;
        if(epoll_ctl(epollfd, EPOLL_CTL_ADD, client_sock, &ev) == -1) {
            perror("epoll_ctl() error:");
            close(client_sock);
        }
    }
}

void epoll_run(void) {
    struct epoll_event events[MAX_EVENTS];
    while (1) {
        if (epoll_should_stop) {
            break;
        }
        int n = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (n < 0) {
            if (errno == EINTR) {
                // Interrupted by signal; check stop flag
                if (epoll_should_stop) break;
                continue;
            }
            perror("epoll_wait() error:");
            break;
        }

        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;
            if (fd == listen_sock) {
                handle_accept();
            } else {
                if(events[i].events & EPOLLIN) {
                    connection_on_read(fd);
                }
                if(events[i].events & EPOLLOUT) {
                    connection_on_write(fd);
                }
            }
        }
    }
}

int epoll_mod(int fd, unsigned int events) {
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = events;
    return epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
}

int epoll_del(int fd) {
    return epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
}

void epoll_request_stop(void) {
    epoll_should_stop = 1;
}