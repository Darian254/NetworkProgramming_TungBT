#ifndef SERVER_H
#define SERVER_H

/**
 * @file server.h
 * @brief Server lifecycle management API
 * 
 * Clean interface for server initialization, execution, and shutdown.
 * Based on phu/server.h pattern.
 */

/**
 * @brief Set socket to non-blocking mode
 * 
 * Uses ioctl(FIONBIO) for portability.
 * 
 * @param fd Socket file descriptor
 * @return 0 on success, -1 on failure
 * 
 * TODO: Implementation already exists in phu/server.c - copy directly
 */
int set_nonblocking(int fd);

/**
 * @brief Initialize server (socket, bind, listen, epoll)
 * 
 * Creates listening socket, sets non-blocking, binds to PORT,
 * calls epoll_init(), and initializes app context.
 * 
 * @return 0 on success, -1 on failure
 * 
 * TODO: Implement this to:
 * 1. Call app_context_init() first
 * 2. Create socket
 * 3. set_nonblocking(listen_sock)
 * 4. setsockopt(SO_REUSEADDR)
 * 5. bind() to PORT from config.h
 * 6. listen() with BACKLOG
 * 7. epoll_init(listen_sock)
 * 8. Print success message
 * 9. Return 0 if all succeed
 */
int server_init(void);

/**
 * @brief Run server main event loop
 * 
 * Blocks here until shutdown signal received.
 * Delegates to epoll_run() from epoll_loop.c.
 * 
 * TODO: Simply call epoll_run() - that's it!
 */
void server_run(void);

/**
 * @brief Shutdown server and cleanup resources
 * 
 * Closes sockets, saves state, frees memory.
 * 
 * TODO: Implement this to:
 * 1. Close listen_sock if >= 0
 * 2. Call app_context_cleanup()
 * 3. Print "Server shutdown." message
 */
void server_shutdown(void);

#endif // SERVER_H
