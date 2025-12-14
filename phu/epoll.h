#ifndef EPOLL_H
#define EPOLL_H

void epoll_init(int listen_sock);
void epoll_run(void);

// Helpers to modify/delete fd subscriptions without exposing internal epollfd
int epoll_mod(int fd, unsigned int events);
int epoll_del(int fd);

#endif // EPOLL_H