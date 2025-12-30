#ifndef CONNECT_H
#define CONNECT_H

#include <stdint.h>
#include<stddef.h>

typedef struct connection connection_t;

void connection_create(int fd);
void connection_on_read(int fd);
void connection_on_write(int fd);
void connection_close(int fd);
int connection_send(int fd, const char *response, size_t len);

#endif
