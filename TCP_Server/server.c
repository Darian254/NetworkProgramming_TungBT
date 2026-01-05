/**
 * @file server.c (HYBRID VERSION)
 * @brief TCP Server implementation using Epoll 
 */

#define _GNU_SOURCE

#include "server.h"
#include "epoll.h"
#include "config.h"
#include "app_context.h"
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>

static int listen_sock = -1;

static void handle_signal(int sig) {
    (void)sig;
    // Request epoll loop to stop; cleanup happens after server_run returns
    epoll_request_stop();
}

int set_nonblocking(int fd) {
    int on = 1;
    if (ioctl(fd, FIONBIO, (char *)&on) < 0) return -1;
    return 0;
}

int server_init(void) {
    int rc;
    int on = 1;
    struct sockaddr_in server_addr;

    // Step 1: Initialize context
    if (app_context_init() != 0) {
        fprintf(stderr, "[ERROR] Failed to initialize application context\n");
        return -1;
    }

    // Step 2: Create socket
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        perror("[ERROR] socket() failed");
        app_context_cleanup();
        return -1;
    }

    if (set_nonblocking(listen_sock) < 0) {
        perror("[ERROR] set_nonblocking() failed");
        close(listen_sock);
        app_context_cleanup();
        return -1;
    }

    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0) {
        perror("[WARN] setsockopt(SO_REUSEADDR) failed");
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    rc = bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (rc < 0) {
        perror("[ERROR] bind() failed");
        close(listen_sock);
        app_context_cleanup();
        return -1;
    }

    rc = listen(listen_sock, BACKLOG);
    if (rc < 0) {
        perror("[ERROR] listen() failed");
        close(listen_sock);
        app_context_cleanup();
        return -1;
    }

    epoll_init(listen_sock);

    printf("========================================\n");
    printf("Server Hybrid (Epoll + Non-blocking I/O)\n");
    printf("Port: %d\n", PORT);
    printf("========================================\n");

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
    app_context_cleanup();
    printf("[INFO] Server shutdown complete.\n");
}

int main(int argc, char *argv[]) {
    (void)argc; // PORT is from config.h, not command line
    (void)argv;

    // Register signal handlers for graceful shutdown
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    (void)argc; (void)argv;

    if (server_init() != 0) {
        fprintf(stderr, "[ERROR] Server initialization failed\n");
        return EXIT_FAILURE;
    }

    printf("[INFO] Game Data Loaded (Ships & Weapons).\n");

    server_run();
    server_shutdown();

    return EXIT_SUCCESS;
}