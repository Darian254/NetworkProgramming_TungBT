#ifndef SERVER_H
#define SERVER_H

/**
 * @brief Set a socket to non-blocking mode.
 * @param fd The file descriptor of the socket.
 * @return 0 on success, -1 on failure.
 */
int set_nonblocking(int fd);

/**
 * @brief Initialize the server.
 * @return 0 on success, -1 on failure.
 */
int server_init(void);

/**
 * @brief Run the server main loop.
 */
void server_run(void);

/**
 * @brief Shutdown the server and release resources.
 */
void server_shutdown(void);

#endif // SERVER_H