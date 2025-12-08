#ifndef UTILS_H
#define UTILS_H

void send_to_fd(int fd, const char *msg);
void broadcast_all(const char *msg);

#endif
