#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "../protocol/protocol.h"
#include "../player/player.h"
#include "../log/log.h"
#include "server.h"
#include "../utils/utils.h"

static int client_fds[MAX_CLIENT];

static void add_client(int fd) {
    char buf[128];

    for (int i = 0; i < MAX_CLIENT; i++) {
        if (client_fds[i] == -1) {
            client_fds[i] = fd;
            snprintf(buf, sizeof(buf), "[SERVER] New client fd=%d", fd);
            log_msg(buf);
            return;
        }
    }

    snprintf(buf, sizeof(buf), "[ERROR] Client list full, rejecting fd=%d", fd);
    log_msg(buf);
    close(fd);
}

static void remove_client(int fd) {
    char buf[128];

    for (int i = 0; i < MAX_CLIENT; i++) {
        if (client_fds[i] == fd) {
            client_fds[i] = -1;
            snprintf(buf, sizeof(buf), "[SERVER] Client disconnected fd=%d", fd);
            log_msg(buf);
            close(fd);
            return;
        }
    }
}

static void handle_client_message(int fd) {
    char buffer[BUFFER_SIZE];
    int bytes = recv(fd, buffer, sizeof(buffer)-1, 0);

    if (bytes <= 0) {
        remove_client(fd);
        return;
    }

    buffer[bytes] = '\0';  

    char cmd_text[32];
    char payload[BUFFER_SIZE];
    int n = sscanf(buffer, "%31s %[^\n]", cmd_text, payload);

    int cmd = parse_cmd(cmd_text);
    if (cmd == -1) {
        char buf[512];
        snprintf(buf, sizeof(buf), "300 BAD_COMMAND %s", cmd_text);
        send_to_fd(fd, buf);
        return;
    }

    protocol_route(fd, cmd, n==2 ? payload : "");
}



static int create_listen_socket(int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(sockfd, 32) < 0) {
        perror("listen");
        exit(1);
    }

    return sockfd;
}


void server_run(int port) {
    for (int i = 0; i < MAX_CLIENT; i++)
        client_fds[i] = -1;

    int server_fd = create_listen_socket(port);

    fd_set readfds;

    while (1) {
        FD_ZERO(&readfds);

        FD_SET(server_fd, &readfds);
        int max_fd = server_fd;

        for (int i = 0; i < MAX_CLIENT; i++) {
            int fd = client_fds[i];
            if (fd != -1) {
                FD_SET(fd, &readfds);
                if (fd > max_fd) max_fd = fd;
            }
        }

        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("select");
            continue;
        }

        if (FD_ISSET(server_fd, &readfds)) {
            int newfd = accept(server_fd, NULL, NULL);
            if (newfd >= 0) {
                add_client(newfd);
            }
        }

        for (int i = 0; i < MAX_CLIENT; i++) {
            int fd = client_fds[i];
            if (fd != -1 && FD_ISSET(fd, &readfds)) {
                handle_client_message(fd);
            }
        }
    }
}
