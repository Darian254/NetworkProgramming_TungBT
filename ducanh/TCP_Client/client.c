#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "ui.h"
#include "../TCP_Server/config.h"   
#include "../TCP_Server/file_transfer.h"
#include "../TCP_Server/util.h"

#define BUFF_SIZE 2048

/**
 * @brief Print program usage for the TCP client.
 * @param prog Executable name (argv[0]).
 */
static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s <ServerIP> <PortNumber>\n", prog);
    fprintf(stderr, "Example: %s 127.0.0.1 5500\n", prog);
}

/**
 * @brief TCP client entry point.
 *
 * Connects to the server, prints the welcome message, then presents
 * a simple menu to send USER/POST/BYE requests. Server responses are
 * beautified into human-friendly text via beautify_result().
 */
int main(int argc, char *argv[]) {
    if (argc != 3) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port number.\n");
        return EXIT_FAILURE;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket() error");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid IP address.\n");
        close(sock);
        return EXIT_FAILURE;
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect() error");
        close(sock);
        return EXIT_FAILURE;
    }

    char recvbuf[BUFF_SIZE];
    if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
        char pretty[1024];
        beautify_result(recvbuf, pretty, sizeof(pretty));
        printf("%s", pretty);
    }

    int choice;
    char line[32];

    while (1) {
        displayMenu();
        fflush(stdout);
        safeInput(line, sizeof(line));
        if (strlen(line) == 0) break;
        if (sscanf(line, "%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n\n");
            continue;
        }

        switch (choice) {
            case FUNC_REGISTER: { /* Register */
                char username[128], password[128];
                printf("Username: ");
                fflush(stdout);
                safeInput(username, sizeof(username));

                if (strlen(username) == 0) {
                    printf("Username cannot be empty.\n");
                    continue;
                }
                
                printf("Password: ");
                fflush(stdout);
                safeInput(password, sizeof(password));

                if (strlen(password) == 0) {
                    printf("Password cannot be empty.\n");
                    continue;
                }

                char cmd[512];
                snprintf(cmd, sizeof(cmd), "REGISTER %s %s", username, password);
                if (send_line(sock, cmd) < 0) {
                    perror("send() error");
                    break;
                }

                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
                    printf("%s", pretty);
                }
                break;
            }
            case FUNC_LOGIN: { /* Login */
                char username[128], password[128];
                printf("Username: ");
                fflush(stdout);
                safeInput(username, sizeof(username));

                if (strlen(username) == 0) {
                    printf("Username cannot be empty.\n");
                    continue;
                }
                
                printf("Password: ");
                fflush(stdout);
                safeInput(password, sizeof(password));

                if (strlen(password) == 0) {
                    printf("Password cannot be empty.\n");
                    continue;
                }

                char cmd[512];
                snprintf(cmd, sizeof(cmd), "LOGIN %s %s", username, password);
                if (send_line(sock, cmd) < 0) {
                    perror("send() error");
                    break;
                }

                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
                    printf("%s", pretty);
                }
                break;
            }
            case FUNC_LOGOUT: { /* Logout */
                if (send_line(sock, "BYE") < 0) {
                    perror("send() error");
                    break;
                }

                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
                    printf("%s", pretty);
                }
                break;
            }
            case FUNC_WHOAMI: { /* Who am I? */
                if (send_line(sock, "WHOAMI") < 0) {
                    perror("send() error");
                    break;
                }

                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    // Parse response: "201 WHOAMI_OK username" or error code
                    int code;
                    char status[64] = "";
                    char username[128] = "";
                    sscanf(recvbuf, "%d %63s %127s", &code, status, username);
                    
                    if (code == RESP_WHOAMI_OK && strlen(username) > 0) {
                        printf("You are logged in as: %s\n", username);
                    } else {
                        char pretty[1024];
                        beautify_result(recvbuf, pretty, sizeof(pretty));
                        printf("%s", pretty);
                    }
                }
                break;
            }
            case FUNC_EXIT: { /* Exit */
                printf("Exiting program...\n");
                close(sock);
                return EXIT_SUCCESS;
            }
            default: {
                printf("Invalid choice, please try again.\n");
                break;
            }
        }
        printf("\n");
    }

    close(sock);
    printf("Client terminated.\n");
    return EXIT_SUCCESS;
}
