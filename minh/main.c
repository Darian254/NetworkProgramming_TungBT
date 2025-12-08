#include "server/server.h"
#include "../log/log.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);

    char buf[128];
    snprintf(buf, sizeof(buf), "Starting server on port %d", port);
    log_msg(buf);

    server_run(port);

    return 0;
}
