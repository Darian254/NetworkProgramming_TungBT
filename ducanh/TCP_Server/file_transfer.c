#include "file_transfer.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>

#define CHUNK_SIZE 65536  /* 64KB transfer buffer */

/* Ensure all data is sent */
ssize_t send_all(int sockfd, const void *buffer, size_t length) {
    size_t total_sent = 0;
    const char *ptr = buffer;
    while (total_sent < length) {
        ssize_t sent = send(sockfd, ptr + total_sent, length - total_sent, 0);
        if (sent <= 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        total_sent += sent;
    }
    return (ssize_t)total_sent;
}

/* Send message + CRLF terminator */
int send_line(int sockfd, const char *msg) {
    char buf[2048];
    snprintf(buf, sizeof(buf), "%s\r\n", msg);
    return send_all(sockfd, buf, strlen(buf)) < 0 ? -1 : 0;
}

/* Receive until CRLF or LF */
ssize_t recv_line(int sockfd, char *buffer, size_t maxlen) {
    size_t idx = 0;
    char c;
    while (idx < maxlen - 1) {
        ssize_t n = recv(sockfd, &c, 1, 0);
        if (n <= 0) return -1;
        
        if (c == '\n') {
            if (idx > 0 && buffer[idx-1] == '\r') {
                buffer[idx-1] = '\0';  // Remove \r
                return idx - 1;
            } else {
                buffer[idx] = '\0';
                return idx;
            }
        }
        
        buffer[idx++] = c;
    }
    buffer[maxlen-1] = '\0';
    return idx;
}

/* Send file data in chunks */
int send_file(int sockfd, const char *filepath, size_t filesize) {
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        perror("fopen() file send error");
        return -1;
    }

    char buf[CHUNK_SIZE];
    size_t total_sent = 0;
    while (total_sent < filesize) {
        size_t read_bytes = fread(buf, 1, sizeof(buf), fp);
        if (read_bytes == 0) break;

        size_t offset = 0;
        while (offset < read_bytes) {
            ssize_t sent = send(sockfd, buf + offset, read_bytes - offset, 0);
            if (sent <= 0) {
                perror("send() file data error");
                fclose(fp);
                return -1;
            }
            offset += sent;
        }
        total_sent += read_bytes;
    }

    fclose(fp);
    return total_sent == filesize ? 0 : -1;
}

/* Receive file data and write to local storage */
int recv_file(int sockfd, const char *filepath, size_t filesize) {
    FILE *fp = fopen(filepath, "wb");
    if (!fp) {
        perror("fopen() file recv error");
        return -1;
    }

    char buf[CHUNK_SIZE];
    size_t total_received = 0;

    while (total_received < filesize) {
        ssize_t n = recv(sockfd, buf, sizeof(buf), 0);
        if (n <= 0) {
            perror("recv() file data error");
            fclose(fp);
            return -1;
        }
        fwrite(buf, 1, n, fp);
        total_received += n;
    }

    fclose(fp);
    return total_received == filesize ? 0 : -1;
}

