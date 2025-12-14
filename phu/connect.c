#include "connect.h"
#include "config.h"
#include "server.h"
#include "router.h"
#include "epoll.h"
// #include "protocol.h"
// #include "buffer.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>

typedef struct connection {
    int sockfd;
    char read_buffer[BUFF_SIZE];
    size_t read_buffer_len;
    char write_buffer[BUFF_SIZE];
    size_t write_buffer_len;
} connection_t;

static connection_t *connections[MAX_CLIENTS] = {0};

void connection_enable_write(connection_t* conn) {
    struct epoll_event ev;
    ev.data.fd = conn->sockfd;
    ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
    epoll_mod(conn->sockfd, ev.events);
}

void connection_disable_write(connection_t* conn) {
    struct epoll_event ev;
    ev.data.fd = conn->sockfd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_mod(conn->sockfd, ev.events);
}

void connection_create(int client_sock) {
    connection_t *conn = (connection_t *)malloc(sizeof(connection_t));
    if(!conn) {
        perror("malloc() error:");
        close(client_sock);
        return;
    }

    memset(conn, 0, sizeof(connection_t));

    conn->sockfd = client_sock;
    conn->read_buffer_len = 0;
    conn->write_buffer_len = 0;

    connections[client_sock] = conn;
    printf("Connection created for socket %d\n", client_sock);
}

void connection_on_read(int client_sock) {
    connection_t *conn = connections[client_sock];
    if(!conn) return;
    while(1) {
        ssize_t n = recv(client_sock, conn->read_buffer + conn->read_buffer_len,
                         BUFF_SIZE - conn->read_buffer_len, 0);
        if (n < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                break; // No more data to read
            } else {
                perror("recv() error:");
                connection_close(client_sock);
                return;
            }
        } else if (n == 0) {
            // Connection closed by the client
            connection_close(client_sock);
            return;
        } else {
            conn->read_buffer_len += n;
            command_routes(client_sock, conn->read_buffer);
            conn->read_buffer_len = 0; // Reset buffer after processing
        }
    }
}

void connection_on_write(int client_sock) {
    connection_t *conn = connections[client_sock];
    if(!conn) return;

    while (conn->write_buffer_len > 0) {
        ssize_t n = send(client_sock, conn->write_buffer, conn->write_buffer_len, 0);
        if (n < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                break; // Socket not ready for writing
            } else {
                perror("send() error:");
                connection_close(client_sock);
                return;
            }
        } else if(n > 0) {
            memmove(conn->write_buffer, conn->write_buffer + n, conn->write_buffer_len - n);
            conn->write_buffer_len -= n;
        }

        if(conn->write_buffer_len == 0) {
            connection_disable_write(conn);
        }
    }
}

void connection_close(int client_sock) {
    connection_t *conn = connections[client_sock];
    if(!conn) return;
    epoll_del(client_sock);
    close(client_sock);
    free(conn);
    connections[client_sock] = NULL;
    printf("Connection closed for socket %d\n", client_sock);
}