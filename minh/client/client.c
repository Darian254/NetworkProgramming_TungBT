#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 2048

int sockfd;

void* recv_thread(void* arg) {
    char buf[BUFFER_SIZE];
    while (1) {
        int n = recv(sockfd, buf, sizeof(buf)-1, 0);
        if (n <= 0) {
            printf("\n[INFO] Server disconnected\n");
            exit(0);
        }
        printf("%s\n> ", buf);
        fflush(stdout);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <server_ip> <port>\n", argv[0]);
        return 1;
    }

    char *server_ip = argv[1];
    int port = atoi(argv[2]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip, &servaddr.sin_addr) <= 0) {
        perror("inet_pton");
        return 1;
    }

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        return 1;
    }

    printf("Connected to %s:%d\n> ", server_ip, port);
    fflush(stdout);

    pthread_t tid;
    pthread_create(&tid, NULL, recv_thread, NULL);

    char buf[BUFFER_SIZE];
    while (fgets(buf, sizeof(buf), stdin)) {
        buf[strcspn(buf, "\n")] = 0;
        if (strlen(buf) == 0) continue;

        if (send(sockfd, buf, strlen(buf), 0) <= 0) {
            perror("send");
            break;
        }
    }

    close(sockfd);
    return 0;
}
