/**
 * @file server.c (SIMPLIFIED VERSION)
 * @brief Server lifecycle implementation - epoll-based architecture
 * 
 * This replaces the complex select/worker thread model with clean epoll.
 * Based on phu/server.c pattern but integrated with ducanh business logic.
 * 
 * MIGRATION STEPS:
 * 1. Backup original server.c (already contains valuable session logic)
 * 2. Replace with this simpler version
 * 3. Business logic stays in session.c - untouched!
 * 4. Build and test incrementally
 */

#define _GNU_SOURCE

#include "server.h"
#include "epoll.h"
#include "config.h"
#include "app_context.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>

// TODO: Global listen socket
static int listen_sock = -1;

int set_nonblocking(int fd) {
    // TODO: Set socket to non-blocking mode using ioctl
    int on = 1;
    if (ioctl(fd, FIONBIO, (char *)&on) < 0) {
        return -1;
    }
    return 0;
}

int server_init(void) {
    int rc;
    int on = 1;
    struct sockaddr_in server_addr;

    // TODO Step 1: Initialize application context (users, sessions)
    if (app_context_init() != 0) {
        fprintf(stderr, "[ERROR] Failed to initialize application context\n");
        return -1;
    }

    // TODO Step 2: Create TCP socket
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        perror("[ERROR] socket() failed");
        app_context_cleanup();
        return -1;
    }

    // TODO Step 3: Set non-blocking
    if (set_nonblocking(listen_sock) < 0) {
        perror("[ERROR] set_nonblocking() failed");
        close(listen_sock);
        app_context_cleanup();
        return -1;
    }

    // TODO Step 4: Set SO_REUSEADDR to allow quick restart
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0) {
        perror("[WARN] setsockopt(SO_REUSEADDR) failed");
        // Non-fatal, continue
    }

    // TODO Step 5: Bind to address
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

    // TODO Step 6: Listen for connections
    rc = listen(listen_sock, BACKLOG);
    if (rc < 0) {
        perror("[ERROR] listen() failed");
        close(listen_sock);
        app_context_cleanup();
        return -1;
    }

    // TODO Step 7: Initialize epoll
    epoll_init(listen_sock);

    // TODO Step 8: Print success message
    printf("========================================\n");
    printf("Server (Epoll + Non-Blocking IO)\n");
    printf("Port: %d\n", PORT);
    printf("Max clients: %d\n", MAX_CLIENTS);
    printf("Max events: %d\n", MAX_EVENTS);
    printf("========================================\n");
    printf("[INFO] Server listening on port %d\n", PORT);

    return 0;
}

void server_run(void) {
    // TODO: Delegate to epoll event loop
    // This blocks until shutdown signal
    epoll_run();
}

void server_shutdown(void) {
    // TODO: Close listening socket
    if (listen_sock >= 0) {
        close(listen_sock);
        listen_sock = -1;
    }

    // TODO: Cleanup application context
    app_context_cleanup();

    printf("[INFO] Server shutdown complete.\n");
}

// TODO: Main entry point
int main(int argc, char *argv[]) {
    (void)argc; // PORT is from config.h, not command line
    (void)argv;

    // TODO: Initialize server
    if (server_init() != 0) {
        fprintf(stderr, "[ERROR] Server initialization failed\n");
        return EXIT_FAILURE;
    }

    // TODO: Run event loop (blocks here)
    server_run();

    // TODO: Cleanup (if we ever exit loop)
    server_shutdown();

    return EXIT_SUCCESS;
}
