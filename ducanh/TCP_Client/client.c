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

#define BUFF_SIZE 8192 // Tăng kích thước buffer để nhận danh sách dài

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
 */
int main(int argc, char *argv[]) {
    /* =========================================
     * 1. KẾT NỐI SERVER
     * ========================================= */
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

    /* =========================================
     * 2. NHẬN TIN NHẮN CHÀO MỪNG
     * ========================================= */
    char recvbuf[BUFF_SIZE];
    if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
        char pretty[1024];
        beautify_result(recvbuf, pretty, sizeof(pretty));
        printf("%s", pretty);
    }

    /* =========================================
     * 3. VÒNG LẶP CHÍNH (MAIN LOOP)
     * ========================================= */
    int choice;
    char line[64];

    while (1) {
        displayMenu();
        fflush(stdout);
        
        // Nhập lựa chọn an toàn
        safeInput(line, sizeof(line));
        
        if (strlen(line) == 0) continue; 
        if (sscanf(line, "%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n\n");
            continue;
        }

        char cmd[512];
        cmd[0] = '\0'; // Reset lệnh

        /* =========================================
         * XỬ LÝ LỰA CHỌN CỦA NGƯỜI DÙNG
         * ========================================= */
        switch (choice) {
            // --- AUTHENTICATION ---
            case FUNC_REGISTER: { 
                char username[128], password[128];
                printf("Username: "); fflush(stdout); safeInput(username, sizeof(username));
                if (strlen(username) == 0) { printf("Username cannot be empty.\n"); continue; }
                
                printf("Password: "); fflush(stdout); safeInput(password, sizeof(password));
                if (strlen(password) == 0) { printf("Password cannot be empty.\n"); continue; }

                snprintf(cmd, sizeof(cmd), "REGISTER %s %s", username, password);
                break;
            }
            case FUNC_LOGIN: { 
                char username[128], password[128];
                printf("Username: "); fflush(stdout); safeInput(username, sizeof(username));
                if (strlen(username) == 0) { printf("Username cannot be empty.\n"); continue; }
                
                printf("Password: "); fflush(stdout); safeInput(password, sizeof(password));
                if (strlen(password) == 0) { printf("Password cannot be empty.\n"); continue; }

                snprintf(cmd, sizeof(cmd), "LOGIN %s %s", username, password);
                break;
            }
            case FUNC_LOGOUT: { 
                snprintf(cmd, sizeof(cmd), "BYE");
                break;
            }
            case FUNC_WHOAMI: { 
                snprintf(cmd, sizeof(cmd), "WHOAMI");
                break;
            }
            case FUNC_EXIT: { 
                printf("Exiting program...\n");
                close(sock);
                return EXIT_SUCCESS;
            }

            // --- PERSONAL ---
            case FUNC_CHECK_COIN: { 
                snprintf(cmd, sizeof(cmd), "GETCOIN");
                break;
            }
            case FUNC_CHECK_ARMOR: { 
                snprintf(cmd, sizeof(cmd), "GETARMOR");
                break;
            }
            case FUNC_BUY_ARMOR: { 
                printf("=== Armor Shop ===\n");
                printf("1. Basic Armor (1000 coin, +500 armor)\n");
                printf("2. Enhanced Armor (2000 coin, +1500 armor)\n");
                printf("0. Cancel\n");
                printf("Select armor type: "); fflush(stdout);
                
                char armor_choice[16];
                safeInput(armor_choice, sizeof(armor_choice));
                int armor_type = atoi(armor_choice);
                if (armor_type == 0) { printf("Purchase cancelled.\n"); continue; }
                
                snprintf(cmd, sizeof(cmd), "BUYARMOR %d", armor_type);
                break;
            }
            
            // --- TEAM OPERATIONS (SỬA LẠI ĐỂ DÙNG ENUM) ---
            
            case FUNC_CREATE_TEAM: { 
                char team_name[128];
                printf("Enter new team name: "); fflush(stdout); safeInput(team_name, sizeof(team_name));
                if (strlen(team_name) == 0) continue;
                snprintf(cmd, sizeof(cmd), "CREATE_TEAM %s", team_name);
                break;
            }
            case FUNC_DELETE_TEAM: { 
                snprintf(cmd, sizeof(cmd), "DELETE_TEAM ");
                break;
            }
            case FUNC_LIST_TEAMS: { 
                snprintf(cmd, sizeof(cmd), "LIST_TEAMS");
                break;
            }
            case FUNC_JOIN_REQUEST: { 
                char team_name[128];
                printf("Enter team name to join: "); fflush(stdout); safeInput(team_name, sizeof(team_name));
                if (strlen(team_name) == 0) continue;
                snprintf(cmd, sizeof(cmd), "JOIN_REQUEST %s", team_name);
                break;
            }
            case FUNC_LEAVE_TEAM: { 
                snprintf(cmd, sizeof(cmd), "LEAVE_TEAM");
                break;
            }
            
            
            case FUNC_TEAM_MEMBERS: { 
                snprintf(cmd, sizeof(cmd), "TEAM_MEMBER_LIST");
                break;
            }
            case FUNC_KICK_MEMBER: { 
                char target_user[128];
                
                printf("Enter username to kick: "); 
                fflush(stdout); 
                safeInput(target_user, sizeof(target_user));
                
                if (strlen(target_user) == 0) continue;

                snprintf(cmd, sizeof(cmd), "KICK_MEMBER %s", target_user);
                break;
            }
            case FUNC_APPROVE_JOIN: { 
                char target_user[128];
                
                printf("Enter username to approve: "); 
                fflush(stdout); 
                safeInput(target_user, sizeof(target_user));
                
                if (strlen(target_user) == 0) continue;

                snprintf(cmd, sizeof(cmd), "JOIN_APPROVE %s", target_user);
                break;
            }
            case FUNC_REJECT_JOIN: { 
                char target_user[128];
                printf("Enter username to reject: "); fflush(stdout); safeInput(target_user, sizeof(target_user));
                snprintf(cmd, sizeof(cmd), "JOIN_REJECT %s", target_user);
                break;
            }
            case FUNC_INVITE_MEMBER: { 
                char target_user[128];
                printf("Enter username to invite: "); fflush(stdout); safeInput(target_user, sizeof(target_user));
                snprintf(cmd, sizeof(cmd), "INVITE %s", target_user);
                break;
            }
            case FUNC_ACCEPT_INVITE: { 
                char team_name[128];
                printf("Enter team name to accept invite: "); fflush(stdout); safeInput(team_name, sizeof(team_name));
                snprintf(cmd, sizeof(cmd), "INVITE_ACCEPT %s", team_name);
                break;
            }
            case FUNC_REJECT_INVITE: { 
                char team_name[128];
                printf("Enter team name to reject invite: "); fflush(stdout); safeInput(team_name, sizeof(team_name));
                snprintf(cmd, sizeof(cmd), "INVITE_REJECT %s", team_name);
                break;
            }

            default: {
                printf("Invalid choice, please try again.\n");
                continue;
            }
        }

        /* =========================================
         * GỬI LỆNH VÀ NHẬN PHẢN HỒI
         * ========================================= */
        if (strlen(cmd) > 0) {
            if (send_line(sock, cmd) < 0) {
                perror("send() error");
                break;
            }

            if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                int code;
                
                if (sscanf(recvbuf, "%d", &code) == 1) {
                    
                    if (code == RESP_LIST_TEAMS_OK || code == RESP_TEAM_MEMBERS_LIST_OK) {
                        char *payload = strchr(recvbuf, ' ');
                        if (payload) {
                            printf("\n>>> SERVER RESPONSE:\n%s\n", payload + 1);
                        } else {
                            printf("\n>>> SERVER RESPONSE: (Empty List)\n");
                        }
                        goto end_loop; 
                    }
                }

                char pretty[1024];
                beautify_result(recvbuf, pretty, sizeof(pretty));
                printf("\n>>> SERVER RESPONSE:\n%s\n", pretty);
                
                if (sscanf(recvbuf, "%d", &code) == 1) {
                    if (code == RESP_WHOAMI_OK) {
                        char current_user[128];
                        if (sscanf(recvbuf, "%*d %127s", current_user) == 1) {
                            printf("Logged in as: %s\n", current_user);
                        }
                    }
                    else if (code == RESP_COIN_OK) {
                        long coin;
                        if (sscanf(recvbuf, "%*d %ld", &coin) == 1) {
                             printf("Your coin balance: %ld\n", coin);
                        }
                    }
                    else if (code == RESP_ARMOR_INFO_OK) {
                        int t1, v1, t2, v2;
                        if(sscanf(recvbuf, "%*d %d %d %d %d", &t1, &v1, &t2, &v2) == 4) {
                             printf("=== Your Ship Armor ===\n");
                             printf("Slot 1: Type=%d, Value=%d\n", t1, v1);
                             printf("Slot 2: Type=%d, Value=%d\n", t2, v2);
                        }
                    }
                }
            }
            
            end_loop:;
        }
        
        printf("\n");
    }

    close(sock);
    printf("Client terminated.\n");
    return EXIT_SUCCESS;
}