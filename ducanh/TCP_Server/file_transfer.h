/**
 * @brief Header file for file transfer TCP client-server.
 * Provides helper functions for sending and receiving files
 * with defined protocol and logging mechanism.
 *
 * Server:
 *   $ ./server <PortNumber> <StorageDirectory>
 * Client:
 *   $ ./client <ServerIP> <PortNumber>
 *
 * Communication:
 *   Client -> Server: TCP connect
 *   Server -> Client: +OK Welcome to file server\r\n
 *   Client -> Server: UPLD <filename> <filesize>\r\n
 *   Server -> Client: +OK Please send file\r\n
 *   Client -> Server: [File data ...]
 *   Server -> Client: +OK Successful upload\r\n
 */

#ifndef FILE_TRANSFER_H
#define FILE_TRANSFER_H

#include <stddef.h>
#include <sys/types.h>

/**
 * @brief Send all bytes in buffer over a TCP socket.
 * This ensures the entire message is transmitted even if send() sends partial data.
 *
 * @param sockfd   Connected socket descriptor
 * @param buffer   Pointer to data buffer
 * @param length   Number of bytes to send
 * @return ssize_t Number of bytes sent, or -1 on error
 */
ssize_t send_all(int sockfd, const void *buffer, size_t length);

/**
 * @brief Receive a line terminated by "\r\n" from socket.
 *
 * @param sockfd   Connected socket descriptor
 * @param buffer   Output buffer
 * @param maxlen   Maximum buffer size
 * @return ssize_t Number of bytes read (excluding terminator), -1 on error
 */
ssize_t recv_line(int sockfd, char *buffer, size_t maxlen);

/**
 * @brief Send a text message with "\r\n" appended automatically.
 *
 * @param sockfd   Connected socket descriptor
 * @param msg      Null-terminated message string
 * @return int     0 on success, -1 on error
 */
int send_line(int sockfd, const char *msg);

/**
 * @brief Send a file through a TCP socket.
 *
 * @param sockfd   Connected socket descriptor
 * @param filepath Path to file to send
 * @param filesize File size in bytes
 * @return int     0 on success, -1 on error
 */
int send_file(int sockfd, const char *filepath, size_t filesize);

/**
 * @brief Receive a file from a TCP socket and write to local storage.
 *
 * @param sockfd   Connected socket descriptor
 * @param filepath Destination file path
 * @param filesize Expected size in bytes
 * @return int     0 on success, -1 on error
 */
int recv_file(int sockfd, const char *filepath, size_t filesize);

#endif /* FILE_TRANSFER_H */

