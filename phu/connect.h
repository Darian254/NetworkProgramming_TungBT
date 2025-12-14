#ifndef CONNECTION_H
#define CONNECTION_H

#include <stdint.h>

typedef struct connection connection_t;

void connection_create(int fd);
void connection_on_read(int fd);
void connection_on_write(int fd);
void connection_close(int fd);

#endif
