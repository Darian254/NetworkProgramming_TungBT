#define _POSIX_C_SOURCE 200112L
#define USE_NCURSES
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

    while (1) {
// #ifdef USE_NCURSES
//         choice = display_menu_ncurses();
//         if (choice == -1 || choice == FUNC_EXIT) {
//             break;
//         }
// #else
        char line[64];
        displayMenu();
        fflush(stdout);
        safeInput(line, sizeof(line));
        if (strlen(line) == 0) break;
        if (sscanf(line, "%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n\n");
            continue;
        }
// #endif

        char cmd[512];
        cmd[0] = '\0'; // Reset lệnh
        switch (choice) {
            case FUNC_REGISTER: { /* Register */
#ifdef USE_NCURSES
                char username[128], password[128];
                if (!register_ui_ncurses(username, sizeof(username), password, sizeof(password))) {
                    continue;
                }
#else
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
#endif
                char cmd[512];
                snprintf(cmd, sizeof(cmd), "REGISTER %s %s", username, password);
                if (send_line(sock, cmd) < 0) {
                    perror("send() error");
                    break;
                }

                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
#ifdef USE_NCURSES
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
                    show_message_ncurses("Register Result", pretty);
#else
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
                    printf("%s", pretty);
#endif
                }
                break;
            }
            case FUNC_LOGIN: { /* Login */
#ifdef USE_NCURSES
                char username[128], password[128];
                if (!login_ui_ncurses(username, sizeof(username), password, sizeof(password))) {
                    continue;
                }
#else
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
#endif
                char cmd[512];
                snprintf(cmd, sizeof(cmd), "LOGIN %s %s", username, password);
                if (send_line(sock, cmd) < 0) {
                    perror("send() error");
                    break;
                }

                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
#ifdef USE_NCURSES
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
                    show_message_ncurses("Login Result", pretty);
#else
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
                    printf("%s", pretty);
#endif
                }
                break;
            }
            case FUNC_LOGOUT: { /* Logout */
#ifdef USE_NCURSES
                if (!logout_ui_ncurses()) {
                    continue;
                }
#endif
                if (send_line(sock, "BYE") < 0) {
                    perror("send() error");
                    break;
                }

                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
#ifdef USE_NCURSES
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
                    show_message_ncurses("Logout Result", pretty);
#else
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
                    printf("%s", pretty);
#endif
                }
                break;
            }
            case FUNC_WHOAMI: { /* Who am I? */
                if (send_line(sock, "WHOAMI") < 0) {
                    perror("send() error");
                    break;
                }

                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
#ifdef USE_NCURSES
                    whoami_ui_ncurses(recvbuf);
#else
                    // Parse response: "201 username" or error code
                    int code;
                    char username[128] = "";
                    sscanf(recvbuf, "%d %127s", &code, username);
                    
                    if (code == RESP_WHOAMI_OK && strlen(username) > 0) {
                        printf("You are logged in as: %s\n", username);
                    } else {
                        char pretty[1024];
                        beautify_result(recvbuf, pretty, sizeof(pretty));
                        printf("%s", pretty);
                    }
#endif
                }
                break;
            }
            case FUNC_EXIT: { /* Exit */
                printf("Exiting program...\n");
                close(sock);
                return EXIT_SUCCESS;
            }

            case FUNC_CHECK_COIN: { /* Check my coin */
                if (send_line(sock, "GETCOIN") < 0) {
                    perror("send() error");
                    break;
                }

                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    int code;
                    long coin = 0;
                    if (sscanf(recvbuf, "%d %ld", &code, &coin) >= 2 && code == RESP_COIN_OK) {
                        printf("Your coin balance: %ld\n", coin);
                    } else {
                        char pretty[1024];
                        beautify_result(recvbuf, pretty, sizeof(pretty));
                        printf("%s", pretty);
                    }
                }
                break;
            }
            case FUNC_CHECK_ARMOR: { /* Check my armor */
                if (send_line(sock, "GETARMOR") < 0) {
                    perror("send() error");
                    break;
                }

                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    int code;
                    int slot1_type = 0, slot1_value = 0, slot2_type = 0, slot2_value = 0;
                    
                    if (sscanf(recvbuf, "%d %d %d %d %d", &code, 
                               &slot1_type, &slot1_value, &slot2_type, &slot2_value) >= 5 
                        && code == RESP_ARMOR_INFO_OK) {
                        printf("=== Your Ship Armor ===\n");
                        printf("Slot 1: Type=%d, Value=%d\n", slot1_type, slot1_value);
                        printf("Slot 2: Type=%d, Value=%d\n", slot2_type, slot2_value);
                    } else {
                        char pretty[1024];
                        beautify_result(recvbuf, pretty, sizeof(pretty));
                        printf("%s", pretty);
                    }
                }
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
                if (armor_type < 1 || armor_type > 2) { printf("Invalid armor type.\n"); continue; }
                
                snprintf(cmd, sizeof(cmd), "BUYARMOR %d", armor_type);
                if (send_line(sock, cmd) < 0) break;
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                     char pretty[1024];
                     beautify_result(recvbuf, pretty, sizeof(pretty));
                     printf("%s", pretty);
                }
                break;
            }

            case FUNC_START_MATCH: {
                printf("Enter opponent team ID to start match: "); fflush(stdout);
                char team_id_str[16];
                safeInput(team_id_str, sizeof(team_id_str));
                
                int opponent_team_id = atoi(team_id_str);
                if (opponent_team_id <= 0) { printf("Invalid team ID.\n"); continue; }
                
                snprintf(cmd, sizeof(cmd), "START_MATCH %d", opponent_team_id);
                if (send_line(sock, cmd) < 0) break;
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                     char pretty[1024];
                     beautify_result(recvbuf, pretty, sizeof(pretty));
                     printf("%s", pretty);
                }
                break;
            }

            case FUNC_GET_MATCH_RESULT: {
                printf("Enter match ID: "); fflush(stdout);
                char match_id_str[16];
                safeInput(match_id_str, sizeof(match_id_str));
                
                int match_id = atoi(match_id_str);
                if (match_id <= 0) { printf("Invalid match ID.\n"); continue; }
                
                snprintf(cmd, sizeof(cmd), "GET_MATCH_RESULT %d", match_id);
                if (send_line(sock, cmd) < 0) break;
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                     char pretty[1024];
                     beautify_result(recvbuf, pretty, sizeof(pretty));
                     printf("%s", pretty);
                }
                break;
            }

            case FUNC_CREATE_TEAM: { 
                char team_name[128];
                printf("Enter new team name: "); fflush(stdout); safeInput(team_name, sizeof(team_name));
                if (strlen(team_name) == 0) continue;

                snprintf(cmd, sizeof(cmd), "CREATE_TEAM %s", team_name);
                if (send_line(sock, cmd) < 0) break;
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
                    printf("%s", pretty);
                }
                break;
            }

            case FUNC_DELETE_TEAM: { 
                if (send_line(sock, "DELETE_TEAM") < 0) break;
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
                    printf("%s", pretty);
                }
                break;
            }

            case FUNC_LIST_TEAMS: { 
                if (send_line(sock, "LIST_TEAMS") < 0) break;
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    char *payload = strchr(recvbuf, ' ');
                    if (payload) {
                        printf("\n>>> TEAM LIST:\n%s\n", payload + 1);
                    } else {
                        printf("\n>>> TEAM LIST: (Empty)\n");
                    }
                }
                break;
            }
            case FUNC_REPAIR: { /* Repair HP */
                printf("Enter HP amount to repair: ");
                fflush(stdout);
                char hp_input[16];
                safeInput(hp_input, sizeof(hp_input));
                int repair_amount = atoi(hp_input);
                if (repair_amount <= 0) {
                    printf("Invalid repair amount.\n");
                    break;
                }
                char cmd[64];
                snprintf(cmd, sizeof(cmd), "REPAIR %d", repair_amount);
                if (send_line(sock, cmd) < 0) {
                    perror("send() error");
                    break;
                }
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    int code, newHP = 0;
                    long newCoin = 0;
                    int n = sscanf(recvbuf, "%d %d %ld", &code, &newHP, &newCoin);
                    if (code == RESP_REPAIR_OK && n == 3) {
                        printf("Repair successful! New HP: %d, New Coin: %ld\n", newHP, newCoin);
                    } else {
                        char pretty[1024];
                        beautify_result(recvbuf, pretty, sizeof(pretty));
                        printf("%s", pretty);
                    }
                }
                break;
            }

            case FUNC_JOIN_REQUEST: { 
                char team_name[128];
                printf("Enter team name to join: "); fflush(stdout); safeInput(team_name, sizeof(team_name));
                if (strlen(team_name) == 0) continue;

                snprintf(cmd, sizeof(cmd), "JOIN_REQUEST %s", team_name);
                if (send_line(sock, cmd) < 0) break;
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
                    printf("%s", pretty);
                }
                break;
            }

            case FUNC_LEAVE_TEAM: { 
                if (send_line(sock, "LEAVE_TEAM") < 0) break;
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
                    printf("%s", pretty);
                }
                break;
            }

            case FUNC_TEAM_MEMBERS: { 
                if (send_line(sock, "TEAM_MEMBER_LIST") < 0) break;
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    char *payload = strchr(recvbuf, ' ');
                    if (payload) {
                        printf("\n>>> MEMBERS:\n%s\n", payload + 1);
                    } else {
                        printf("\n>>> MEMBERS: (Empty)\n");
                    }
                }
                break;
            }

            case FUNC_KICK_MEMBER: { 
                char target_user[128];
                printf("Enter username to kick: "); fflush(stdout); safeInput(target_user, sizeof(target_user));
                if (strlen(target_user) == 0) continue;

                snprintf(cmd, sizeof(cmd), "KICK_MEMBER %s", target_user);
                if (send_line(sock, cmd) < 0) break;
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
                    printf("%s", pretty);
                }
                break;
            }

            case FUNC_APPROVE_JOIN: { 
                char target_user[128];
                printf("Enter username to approve: "); fflush(stdout); safeInput(target_user, sizeof(target_user));
                if (strlen(target_user) == 0) continue;

                snprintf(cmd, sizeof(cmd), "JOIN_APPROVE %s", target_user);
                if (send_line(sock, cmd) < 0) break;
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
                    printf("%s", pretty);
                }
                break;
            }

            case FUNC_REJECT_JOIN: { 
                char target_user[128];
                printf("Enter username to reject: "); fflush(stdout); safeInput(target_user, sizeof(target_user));
                if (strlen(target_user) == 0) continue;

                snprintf(cmd, sizeof(cmd), "JOIN_REJECT %s", target_user);
                if (send_line(sock, cmd) < 0) break;
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
                    printf("%s", pretty);
                }
                break;
            }

            case FUNC_INVITE_MEMBER: { 
                char target_user[128];
                printf("Enter username to invite: "); fflush(stdout); safeInput(target_user, sizeof(target_user));
                
                snprintf(cmd, sizeof(cmd), "INVITE %s", target_user);
                if (send_line(sock, cmd) < 0) break;
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
                    printf("%s", pretty);
                }
                break;
            }

            case FUNC_ACCEPT_INVITE: { 
                char team_name[128];
                printf("Enter team name to accept invite: "); fflush(stdout); safeInput(team_name, sizeof(team_name));
                
                snprintf(cmd, sizeof(cmd), "INVITE_ACCEPT %s", team_name);
                if (send_line(sock, cmd) < 0) break;
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
                    printf("%s", pretty);
                }
                break;
            }

            case FUNC_REJECT_INVITE: { 
                char team_name[128];
                printf("Enter team name to reject invite: "); fflush(stdout); safeInput(team_name, sizeof(team_name));
                
                snprintf(cmd, sizeof(cmd), "INVITE_REJECT %s", team_name);
                if (send_line(sock, cmd) < 0) break;
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
                    printf("%s", pretty);
                }
                break;
            }
            
            // Quick login options
            case FUNC_QUICK_LOGIN_TEST1:
            case FUNC_QUICK_LOGIN_TEST2:
            case FUNC_QUICK_LOGIN_TEST3:
            case FUNC_QUICK_LOGIN_TEST4:
            case FUNC_QUICK_LOGIN_TEST5:
            case FUNC_QUICK_LOGIN_TEST6: {
                int test_num = choice - FUNC_QUICK_LOGIN_TEST1 + 1;
                char username[20], password[20];
                snprintf(username, sizeof(username), "test%d", test_num);
                snprintf(password, sizeof(password), "Admin@2024");
                
                snprintf(cmd, sizeof(cmd), "LOGIN %s %s", username, password);
                if (send_line(sock, cmd) < 0) {
                    perror("send() error");
                    break;
                }
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
#ifdef USE_NCURSES
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
                    show_message_ncurses("Quick Login Result", pretty);
#else
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
                    printf("%s", pretty);
#endif
                }
                break;
            }
            
            // test1: Create team abc and invite test2, test3
            case FUNC_SETUP_TEAM_ABC: {
                // Create team
                snprintf(cmd, sizeof(cmd), "CREATE_TEAM abc");
                if (send_line(sock, cmd) < 0) break;
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
#ifdef USE_NCURSES
                    show_message_ncurses("Create Team", pretty);
#else
                    printf("%s", pretty);
#endif
                }
                sleep(1);
                
                // Invite test2
                snprintf(cmd, sizeof(cmd), "INVITE test2");
                if (send_line(sock, cmd) < 0) break;
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
#ifdef USE_NCURSES
                    show_message_ncurses("Invite test2", pretty);
#else
                    printf("%s", pretty);
#endif
                }
                sleep(1);
                
                // Invite test3
                snprintf(cmd, sizeof(cmd), "INVITE test3");
                if (send_line(sock, cmd) < 0) break;
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
#ifdef USE_NCURSES
                    show_message_ncurses("Invite test3", pretty);
#else
                    printf("%s", pretty);
#endif
                }
                break;
            }
            
            // test4: Create team def and invite test5, test6
            case FUNC_SETUP_TEAM_DEF: {
                // Create team
                snprintf(cmd, sizeof(cmd), "CREATE_TEAM def");
                if (send_line(sock, cmd) < 0) break;
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
#ifdef USE_NCURSES
                    show_message_ncurses("Create Team", pretty);
#else
                    printf("%s", pretty);
#endif
                }
                sleep(1);
                
                // Invite test5
                snprintf(cmd, sizeof(cmd), "INVITE test5");
                if (send_line(sock, cmd) < 0) break;
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
#ifdef USE_NCURSES
                    show_message_ncurses("Invite test5", pretty);
#else
                    printf("%s", pretty);
#endif
                }
                sleep(1);
                
                // Invite test6
                snprintf(cmd, sizeof(cmd), "INVITE test6");
                if (send_line(sock, cmd) < 0) break;
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
#ifdef USE_NCURSES
                    show_message_ncurses("Invite test6", pretty);
#else
                    printf("%s", pretty);
#endif
                }
                break;
            }
            
            // Accept invite to team abc
            case FUNC_ACCEPT_ABC: {
                snprintf(cmd, sizeof(cmd), "INVITE_ACCEPT abc");
                if (send_line(sock, cmd) < 0) break;
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
#ifdef USE_NCURSES
                    show_message_ncurses("Accept Invite", pretty);
#else
                    printf("%s", pretty);
#endif
                }
                break;
            }
            
            // Accept invite to team def
            case FUNC_ACCEPT_DEF: {
                snprintf(cmd, sizeof(cmd), "INVITE_ACCEPT def");
                if (send_line(sock, cmd) < 0) break;
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
#ifdef USE_NCURSES
                    show_message_ncurses("Accept Invite", pretty);
#else
                    printf("%s", pretty);
#endif
                }
                break;
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

