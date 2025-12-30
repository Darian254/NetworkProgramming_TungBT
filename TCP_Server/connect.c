#include "connect.h"
#include "config.h"
#include "server.h"
#include "router.h"
#include "epoll.h"
#include "session.h"
// #include "protocol.h"
// #include "buffer.h"
#include "file_transfer.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>

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
    int on = 1;
    if (ioctl(client_sock, FIONBIO, (char *)&on) < 0) {
        perror("ioctl() error:");
        close(client_sock);
        return;
    }
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

    // Create empty session for this connection
    ServerSession new_session;
    initServerSession(&new_session);
    new_session.socket_fd = client_sock;
    add_session(&new_session);

    // Send initial greeting so client recv_line() doesn't block
    // Use a dedicated welcome code to avoid confusion with REGISTER_OK
    const char *greeting = "120"; // RESP_WELCOME
    if (send_line(client_sock, greeting) < 0) {
        // Fallback: enqueue via epoll-driven buffer if immediate send fails
        const char *fallback = "120\r\n";
        connection_send(client_sock, fallback, strlen(fallback));
    }
}

void connection_on_read(int client_sock) {
    connection_t *conn = connections[client_sock];
    if(!conn) return;
    while (1) {
        char line[BUFF_SIZE];
        ssize_t m = recv_line(client_sock, line, sizeof(line));

        if (m < 0) {
            // Treat anything other than EAGAIN/EWOULDBLOCK as hard error/EOF
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                // Drained all available data for now (edge-triggered)
                break;
            }
            perror("recv_line() error");
            connection_close(client_sock);
            return;
        }

        if (m > 0) {
            // Process a full command line (without CRLF)
            command_routes(client_sock, line);
            // Continue looping to drain any pipelined commands
            continue;
        }

        // m == 0 is not expected from recv_line(); treat conservatively
        // Break to avoid busy loop; next EPOLLIN will resume if more data arrives
        break;
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

int connection_send(int client_sock, const char *response, size_t len) {
    connection_t *conn = connections[client_sock];
    if(!conn) return -1;

    if(len > BUFF_SIZE - conn->write_buffer_len) {
        return -1;
    }

    memcpy(conn->write_buffer + conn->write_buffer_len, response, len);
    conn->write_buffer_len += len;

    connection_enable_write(conn);
    return 0;
}

void connection_close(int client_sock) {
    connection_t *conn = connections[client_sock];
    if(!conn) return;
    // Remove associated session to avoid leaks
    remove_session_by_socket(client_sock);
    epoll_del(client_sock);
    close(client_sock);
    free(conn);
    connections[client_sock] = NULL;
    printf("Connection closed for socket %d\n", client_sock);
}