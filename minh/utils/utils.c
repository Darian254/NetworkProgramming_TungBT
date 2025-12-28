#include "utils.h"
#include "../player/player.h"
#include <sys/socket.h>
#include <string.h>

void send_to_fd(int fd, const char *msg) {
    if (!msg) return;

    send(fd, msg, strlen(msg), 0);
    send(fd, "\n", 1, 0); 
}

