#include "server.h"
#include <stdio.h>
#include <signal.h>

static void handle_sigint(int sig) {
    (void)sig;
    server_shutdown();
}

int main(void) {
    if (server_init() != 0) {
        fprintf(stderr, "Failed to initialize server\n");
        return 1;
    }

    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    server_run();
    return 0;
}
