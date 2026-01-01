#define _POSIX_C_SOURCE 200112L
#define USE_NCURSES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include "ui.h"

#include "../TCP_Server/config.h"     
#include "../TCP_Server/file_transfer.h"
#include "../TCP_Server/util.h"


#define BUFF_SIZE 8192 // Tăng kích thước buffer để nhận danh sách dài

/**
 * @brief Check and handle broadcast messages (like chest drop 141)
 * Non-blocking check for incoming messages
 * @param sock Socket descriptor
 * @return 1 if message was handled, 0 if no message
 */
static int last_challenge_id = -1;
static int check_broadcast_messages(int sock) {
    int messages_handled = 0;
    
    // Vòng lặp để xử lý tất cả messages đang chờ
    while (1) {
        fd_set readfds;
        struct timeval timeout;
        
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 0; // Non-blocking
        
        if (select(sock + 1, &readfds, NULL, NULL, &timeout) > 0) {
            if (FD_ISSET(sock, &readfds)) {
                char msg[BUFF_SIZE];
                // Dùng MSG_PEEK để xem trước dữ liệu mà không lấy ra khỏi buffer
                // Điều này giúp tránh việc hàm này "ăn mất" response của các lệnh chính (như LOGIN, FIRE chủ động)
                // Tuy nhiên, logic hiện tại của bạn dùng recv_line luôn, nên ta giả định
                // server gửi broadcast khi client đang IDLE (không chờ response lệnh nào).
                
                ssize_t n = recv_line(sock, msg, sizeof(msg));
                
                if (n > 0) {
                    int code;
                    if (sscanf(msg, "%d", &code) == 1) {
                        
                        switch (code) {
                        // --- CASE 1: Rương rơi (141) ---
                        case RESP_CHEST_DROP_OK: { 
                            int chest_id, chest_type, pos_x, pos_y;
                            if (sscanf(msg, "%*d %d %d %d %d", &chest_id, &chest_type, &pos_x, &pos_y) == 4) {
                                printf("\n\n[EVENT] 🎁 MỘT RƯƠNG KHO BÁU VỪA RƠI! ID: %d (Tại: %d,%d)\n", chest_id, pos_x, pos_y);
                                printf("Select an option: "); fflush(stdout);
                            }
                            break;
                        }

                        // --- CASE 2: Nhận lời thách đấu (150) ---
                        case RESP_CHALLENGE_RECEIVED: { 
                            char enemy_team[64];
                            int enemy_id, c_id;
                            if (sscanf(msg, "%*d CHALLENGE_RECEIVED %s %d %d", enemy_team, &enemy_id, &c_id) == 3) {
                                last_challenge_id = c_id; 
                                printf("\n\n>>> [ALERT] ⚔️ Đội '%s' (ID: %d) ĐANG THÁCH ĐẤU BẠN!\n", enemy_team, enemy_id);
                                printf(">>> Nhập lệnh 42 để chấp nhận ngay!\n"); 
                                printf("Select an option: "); fflush(stdout);
                            }
                            break;
                        }

                        // --- CASE 3: Thông báo bắt đầu trận đấu (151) ---
                        case RESP_MATCH_STARTED_NOTIFY: { 
                            int m_id;
                            if (sscanf(msg, "%*d MATCH_STARTED %d", &m_id) == 1) {
                                printf(">>> Hãy chuẩn bị chiến đấu! (Vào Battle Screen - 44)\n");
                                printf("Select an option: "); fflush(stdout);
                            }
                            break;
                        }

                        // --- CASE 4: Fire Event (200) ---
                        // Sửa từ 131 thành 200 (RESP_FIRE_OK) theo config.h
                        // Server gửi broadcast dạng: 200 FIRE_EVENT <attacker> <target> <damage> <hp> <armor>
                        case RESP_FIRE_OK: { 
                            char event_type[64];
                            char attacker[128], target[128];
                            int damage, hp, armor;

                            // Kiểm tra xem có phải chuỗi "FIRE_EVENT" không để phân biệt với response bắn thường
                            if (sscanf(msg, "%*d %63s %127s %127s %d %d %d", event_type, attacker, target, &damage, &hp, &armor) == 6
                                && strcmp(event_type, "FIRE_EVENT") == 0) {
                                
                                printf("\n\n>>> [WARNING] 💥 Bạn bị %s bắn trúng! Mất %d máu. HP còn lại: %d\n", attacker, damage, hp);
                                printf("Select an option: "); fflush(stdout);
                            }
                            // Nếu code là 200 nhưng KHÔNG phải FIRE_EVENT, có thể đó là response cho lệnh bắn của chính mình
                            // nhưng bị lọt vào đây (race condition). Ta cứ in ra hoặc bỏ qua.
                            break;
                        }

                        // --- CASE 5: Challenge Accepted (131) ---
                        // Đây là code 131 thực sự theo config.h
                        case RESP_CHALLENGE_ACCEPTED: {
                            printf("\n\n>>> [INFO] ✅ Lời thách đấu của bạn đã được chấp nhận!\n");
                            printf("Select an option: "); fflush(stdout);
                            break;
                        }
                        
                        // --- CASE 6: Rương đã mở (145) ---
                        case RESP_CHEST_OPEN_OK: {
                            // Thường thì response này chờ ở hàm mở rương, nhưng nếu là broadcast người khác mở:
                            // Cần server gửi mã khác hoặc format khác. Tạm thời giữ nguyên log logic cũ.
                            // printf("\n[INFO] Một rương đã được mở.\n"); 
                            break;
                        }

                        // Các tin nhắn hệ thống khác
                        default:
                            break;
                        }
                        messages_handled = 1;
                    }
                }
            }
        } else {
            // Không còn message nào
            break;
        }
    }
    return messages_handled;
}
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
        // Kiểm tra broadcast messages trước khi hiển thị menu (để Client B thấy 150 ngay lập tức)
        check_broadcast_messages(sock);
        
        char line[64];
        displayMenu();
        fflush(stdout);
        
        // Kiểm tra broadcast messages một lần nữa trước khi chờ input
        check_broadcast_messages(sock);
        
        safeInput(line, sizeof(line));
        
        // Sau khi nhận input, kiểm tra lại broadcast messages (có thể có message đến trong lúc nhập)
        check_broadcast_messages(sock);
        if (strlen(line) == 0) {
            printf("Please enter an option number.\n\n");
            continue;
        }
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

            case FUNC_BUY_WEAPON: {
                printf("=== Weapon Shop ===\n");
                printf("1. Cannon Ammo (500 coin, +50 ammo)\n");
                printf("2. Laser (1500 coin, +1 laser, max 5)\n");
                printf("3. Missile (3000 coin, +1 missile, max 3)\n");
                printf("0. Cancel\n");
                printf("Select weapon type: "); fflush(stdout);
                
                char weapon_choice[16];
                safeInput(weapon_choice, sizeof(weapon_choice));
                int weapon_type = atoi(weapon_choice);
                
                if (weapon_type == 0) { printf("Purchase cancelled.\n"); continue; }
                if (weapon_type < 1 || weapon_type > 3) { printf("Invalid weapon type.\n"); continue; }
                
                // Map to server WeaponType enum
                int server_weapon_type = weapon_type - 1;
                
                snprintf(cmd, sizeof(cmd), "BUY_WEAPON %d", server_weapon_type);
                if (send_line(sock, cmd) < 0) break;
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                     char pretty[1024];
                     beautify_result(recvbuf, pretty, sizeof(pretty));
                     printf("%s", pretty);
                }
                break;
            }

            case FUNC_GET_WEAPON: {
                if (send_line(sock, "GET_WEAPON") < 0) {
                    perror("send() error");
                    break;
                }
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    int code;
                    int cannon_ammo = 0, laser_count = 0, missile_count = 0;
                    if (sscanf(recvbuf, "%d %d %d %d", &code, &cannon_ammo, &laser_count, &missile_count) == 4
                        && code == RESP_MATCH_INFO_OK) {
                        printf("=== Your Ship Weapons ===\n");
                        printf("Cannon Ammo: %d\n", cannon_ammo);
                        printf("Laser Count: %d\n", laser_count);
                        printf("Missile Count: %d\n", missile_count);
                    } else {
                        char pretty[1024];
                        beautify_result(recvbuf, pretty, sizeof(pretty));
                        printf("%s", pretty);
                    }
                }
                break;
            }

            case FUNC_START_MATCH: {
                printf("Enter opponent team ID to start match: "); fflush(stdout);
                char team_id_str[16];
                safeInput(team_id_str, sizeof(team_id_str));
                
                // Trim whitespace
                int len = strlen(team_id_str);
                while (len > 0 && (team_id_str[len-1] == ' ' || team_id_str[len-1] == '\t' || team_id_str[len-1] == '\n' || team_id_str[len-1] == '\r')) {
                    team_id_str[len-1] = '\0';
                    len--;
                }
                
                if (len == 0) {
                    printf("Team ID cannot be empty.\n");
                    continue;
                }
                
                int opponent_team_id = atoi(team_id_str);
                if (opponent_team_id <= 0) { 
                    printf("Invalid team ID. Please enter a positive number.\n"); 
                    continue; 
                }
                
                snprintf(cmd, sizeof(cmd), "START_MATCH %d", opponent_team_id);
                if (send_line(sock, cmd) < 0) {
                    perror("send() error");
                    break;
                }
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    int code;
                    if (sscanf(recvbuf, "%d", &code) == 1 && code == RESP_START_MATCH_OK) {
                        printf("Match started successfully!\n");
                        // Try to get match ID from session or response
                        printf("You can now use Battle Screen (option 44) to view and play.\n");
                        // Note: Match ID should be in session->current_match_id on server side
                        // Client would need to call MATCH_INFO or another command to get it
                    } else {
                        char pretty[1024];
                        beautify_result(recvbuf, pretty, sizeof(pretty));
                        printf("%s", pretty);
                    }
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
                    // Expect: 143 <match_id> <winner_team_id> on success
                    int code = 0, recv_match_id = 0, winner_team_id = 0;
                    if (sscanf(recvbuf, "%d %d %d", &code, &recv_match_id, &winner_team_id) == 3 && code == RESP_MATCH_RESULT_OK) {
                        if (winner_team_id == -1) {
                            printf("Match %d finished. Result: Draw.\n", recv_match_id);
                        } else {
                            printf("Match %d finished. Winner: Team %d.\n", recv_match_id, winner_team_id);
                        }
                    } else {
                        char pretty[1024];
                        beautify_result(recvbuf, pretty, sizeof(pretty));
                        printf("%s", pretty);
                    }
                }
                break;
            }

            case FUNC_END_MATCH: {
                printf("Enter match ID to end: "); fflush(stdout);
                char match_id_str[16];
                safeInput(match_id_str, sizeof(match_id_str));
                
                int match_id = atoi(match_id_str);
                if (match_id <= 0) { printf("Invalid match ID.\n"); continue; }
                
                snprintf(cmd, sizeof(cmd), "END_MATCH %d", match_id);
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

            case FUNC_FIRE: { /* Bắn tàu khác */
                char target_id[32];
                char weapon_id[16] = "0" ;//Mặc định súng 1 (Cannon)

                printf("Enter Target ID (User): ");
                fflush(stdout);
                safeInput(target_id, sizeof(target_id));

                if (strlen(target_id) == 0) {
                    printf("Target ID cannot be empty.\n");
                    break;
                }
                
                // (Tùy chọn) Nhập loại súng nếu game có nhiều súng
                // printf("Enter Weapon ID (default 1): "); 
                // safeInput(weapon_id, sizeof(weapon_id));

                snprintf(cmd, sizeof(cmd), "FIRE %s %s", target_id, weapon_id);
                
                // 1. Gửi lệnh
                if (send_line(sock, cmd) < 0) {
                    perror("send() error");
                    break;
                }

                // 2. Chờ phản hồi NGAY LẬP TỨC
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    int dam, hp, arm;
                    char atk_name[128], tar_name[128];
                    // Giả sử server trả về: "200 AtkID TarID Dam HP Armor" khi bắn trúng
                    if (sscanf(recvbuf, "200 %s %s %d %d %d", atk_name, tar_name, &dam, &hp, &arm) == 5) {
                        printf("\n>>> [HIT] You hit %s! Damage: %d | Enemy HP: %d\n", tar_name, dam, hp);
                    } else {
                        // Nếu bắn trượt hoặc lỗi, dùng beautify
                        char pretty[1024];
                        beautify_result(recvbuf, pretty, sizeof(pretty));
                        printf("%s", pretty);
                    }
                }
                break;
            }

            case FUNC_CHALLENGE: { /* Gửi lời thách đấu */
                char team_id_str[32];
                printf("Enter Team ID to challenge: ");
                fflush(stdout);
                safeInput(team_id_str, sizeof(team_id_str));

                if (strlen(team_id_str) == 0) break;

                // Gửi SEND_CHALLENGE (chỉ tạo challenge record, chưa tạo match)
                snprintf(cmd, sizeof(cmd), "SEND_CHALLENGE %s", team_id_str);

                if (send_line(sock, cmd) < 0) {
                    perror("send() error");
                    break;
                }
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    int code, challenge_id;
                    // Parse: 130 CHALLENGE_SENT <challenge_id>
                    if (sscanf(recvbuf, "%d CHALLENGE_SENT %d", &code, &challenge_id) == 2 && code == RESP_CHALLENGE_SENT) {
                        printf("Challenge sent successfully! Challenge ID: %d\n", challenge_id);
                        printf("Waiting for opponent to accept...\n");
                    } else {
                        char pretty[1024];
                        beautify_result(recvbuf, pretty, sizeof(pretty));
                        printf("%s", pretty);
                    }
                }
                break;
            }

            case FUNC_ACCEPT_CHALLENGE: { /* Accept Challenge */
                if (last_challenge_id == -1) {
                    break;
                }
                snprintf(cmd, sizeof(cmd), "ACCEPT_CHALLENGE"); // Gửi lệnh không kèm ID
                if (send_line(sock, cmd) < 0) break;
                
                // Đọc response chính (131 CHALLENGE_ACCEPTED)
                int response_received = 0;
                while (!response_received) {
                    if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                        int code_check;
                        if (sscanf(recvbuf, "%d", &code_check) == 1) {
                            if (code_check == RESP_CHALLENGE_ACCEPTED) {
                                // Đây là response chính từ ACCEPT_CHALLENGE
                                char pretty[1024];
                                beautify_result(recvbuf, pretty, sizeof(pretty));
                                printf("%s", pretty);
                                response_received = 1;
                            } else if (code_check == RESP_MATCH_STARTED_NOTIFY) {
                                // 151 MATCH_STARTED - xử lý broadcast
                                int m_id;
                                if (sscanf(recvbuf, "%*d MATCH_STARTED %d", &m_id) == 1) {
                                    printf("\n\n>>> [INFO]  Trận đấu %d bắt đầu.\n", m_id);
                                
                                }
                                // Tiếp tục đợi response chính
                            } else if (code_check == RESP_CHEST_DROP_OK) {
                                // 141 CHEST_DROP - xử lý broadcast
                                int chest_id, chest_type, pos_x, pos_y;
                                if (sscanf(recvbuf, "%d %d %d %d %d", &code_check, &chest_id, &chest_type, &pos_x, &pos_y) == 5) {
                                    printf("\n\n[EVENT]  MỘT RƯƠNG KHO BÁU VỪA RƠI! ID: %d (Tại: %d,%d)\n", chest_id, pos_x, pos_y);
                                }
                                // Tiếp tục đợi response chính
                            } else {
                                // Response không mong đợi, in ra và coi như đã nhận
                                char pretty[1024];
                                beautify_result(recvbuf, pretty, sizeof(pretty));
                                printf("%s", pretty);
                                response_received = 1;
                            }
                        } else {
                            response_received = 1; // Lỗi parse, thoát
                        }
                    } else {
                        break; // Không có dữ liệu, thoát
                    }
                }
                
                // Check thêm broadcast messages nếu có
                check_broadcast_messages(sock);
                break;
            }

            case FUNC_DECLINE_CHALLENGE: { /* Decline Challenge */
                char challenge_id_str[32];
                printf("Enter Challenge ID to decline: ");
                fflush(stdout);
                safeInput(challenge_id_str, sizeof(challenge_id_str));
                
                if (strlen(challenge_id_str) == 0) break;
                
                snprintf(cmd, sizeof(cmd), "DECLINE_CHALLENGE %s", challenge_id_str);
                if (send_line(sock, cmd) < 0) break;
                
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
                    printf("%s", pretty);
                }
                break;
            }

            // --- OPTION 41: MỞ RƯƠNG (HỎI -> TRẢ LỜI) ---
            case FUNC_OPEN_CHEST: { 
                char chest_id[16];
                char answer[128];

                // 1. Nhập ID
                printf("Enter Chest ID: ");
                fflush(stdout);
                safeInput(chest_id, sizeof(chest_id));
                if (strlen(chest_id) == 0) break;

                // 2. Gửi ID lên để lấy câu hỏi
                snprintf(cmd, sizeof(cmd), "CHEST_OPEN %s", chest_id);
                if (send_line(sock, cmd) < 0) break;

                // 3. Nhận câu hỏi - có thể nhận được 211 (câu hỏi), 151 (MATCH_STARTED), hoặc 141 (CHEST_DROP)
                int question_received = 0;
                while (!question_received) {
                    if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                        int code;
                        char question_text[256];
                        
                        // Server trả về: 211 <Nội dung câu hỏi>
                        if (sscanf(recvbuf, "%d %[^\n]", &code, question_text) == 2 && code == 211) {
                            // YÊU CẦU 3: In ra câu hỏi cho người dùng
                            printf("\n====================================\n");
                            printf(">>> CÂU HỎI: %s\n", question_text);
                            printf("====================================\n");
                            
                            // 4. Nhập đáp án
                            printf("Nhap dap an cua ban: ");
                            fflush(stdout);
                            safeInput(answer, sizeof(answer));
                            
                            if (strlen(answer) == 0) {
                                printf("Da huy tra loi.\n");
                                break;
                            }

                            // 5. Gửi ID + Đáp án
                            snprintf(cmd, sizeof(cmd), "CHEST_OPEN %s %s", chest_id, answer);
                            if (send_line(sock, cmd) < 0) break;
                            
                            // 6. Nhận kết quả cuối cùng - có thể nhận được 127 (success), 210 (broadcast), hoặc error
                            int result_received = 0;
                            while (!result_received) {
                                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                                    int result_code;
                                    if (sscanf(recvbuf, "%d", &result_code) == 1) {
                                        if (result_code == RESP_CHEST_OPEN_OK) {
                                            // 145 CHEST_OPEN_OK - thành công
                                            printf("\n[SUCCESS] Rương đã được mở thành công!\n");
                                            result_received = 1;
                                        } else if (result_code == RESP_CHEST_BROADCAST) {
                                            // 210 CHEST_COLLECTED - broadcast message, xử lý nhưng tiếp tục đợi response chính
                                            char collector[128];
                                            int cid;
                                            if (sscanf(recvbuf, "%*d CHEST_COLLECTED %s %d", collector, &cid) == 2) {
                                                printf("\n[INFO] %s đã thu thập rương %d.\n", collector, cid);
                                            }
                                            // Tiếp tục đợi response chính
                                        } else {
                                            // Lỗi hoặc response khác
                                            char pretty[1024];
                                            beautify_result(recvbuf, pretty, sizeof(pretty));
                                            printf("%s", pretty);
                                            result_received = 1;
                                        }
                                    } else {
                                        result_received = 1;
                                    }
                                } else {
                                    break;
                                }
                            }
                            question_received = 1;
                        } else {
                            // Kiểm tra xem có phải broadcast messages không
                            int code_check;
                            if (sscanf(recvbuf, "%d", &code_check) == 1) {
                                if (code_check == RESP_MATCH_STARTED_NOTIFY) {
                                    // 151 MATCH_STARTED - xử lý broadcast nhưng tiếp tục đợi câu hỏi
                                    int m_id;
                                    if (sscanf(recvbuf, "%*d MATCH_STARTED %d", &m_id) == 1) {
                                        printf("\n>>> [INFO] 🚀 Trận đấu %d bắt đầu.\n", m_id);
                                    }
                                } else if (code_check == RESP_CHEST_DROP_OK) {
                                    // 141 CHEST_DROP - xử lý broadcast nhưng tiếp tục đợi câu hỏi
                                    int chest_id_broadcast, chest_type, pos_x, pos_y;
                                    if (sscanf(recvbuf, "%d %d %d %d %d", &code_check, &chest_id_broadcast, &chest_type, &pos_x, &pos_y) == 5) {
                                        printf("\n[EVENT] 🎁 MỘT RƯƠNG KHO BÁU VỪA RƠI! ID: %d (Tại: %d,%d)\n", chest_id_broadcast, pos_x, pos_y);
                                    }
                                } else {
                                    // Response lỗi hoặc không mong đợi
                                    char pretty[1024];
                                    beautify_result(recvbuf, pretty, sizeof(pretty));
                                    printf("Lỗi: %s", pretty);
                                    question_received = 1;
                                }
                            } else {
                                question_received = 1;
                            }
                        }
                    } else {
                        break;
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
                printf("Enter team name to reject invite: "); fflush(stdout);
                safeInput(team_name, sizeof(team_name));
                if (strlen(team_name) == 0) break;

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
            
            // Login/Register Menu
            case FUNC_AUTHENTICATION_MENU: {
#ifdef USE_NCURSES
                int menu_choice = login_register_menu_ncurses();
                if (menu_choice == -1) {
                    continue;  // User cancelled
                }
                
                char username[128], password[128];
                int success = 0;
                
                if (menu_choice == 0) {
                    // Login selected
                    if (login_ui_ncurses(username, sizeof(username), password, sizeof(password))) {
                        snprintf(cmd, sizeof(cmd), "LOGIN %s %s", username, password);
                        if (send_line(sock, cmd) < 0) {
                            perror("send() error");
                            break;
                        }
                        success = 1;
                    }
                } else if (menu_choice == 1) {
                    // Register selected
                    if (register_ui_ncurses(username, sizeof(username), password, sizeof(password))) {
                        snprintf(cmd, sizeof(cmd), "REGISTER %s %s", username, password);
                        if (send_line(sock, cmd) < 0) {
                            perror("send() error");
                            break;
                        }
                        success = 1;
                    }
                }
                
                if (success && recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    char pretty[1024];
                    beautify_result(recvbuf, pretty, sizeof(pretty));
                    show_message_ncurses(menu_choice == 0 ? "Login Result" : "Register Result", pretty);
                }
#else
                printf("Login/Register menu is only available with ncurses.\n");
#endif
                break;
            }
            
            // View Match Info
            case FUNC_MATCH_INFO: {
                char match_id_str[64];
                printf("Enter Match ID: ");
                fflush(stdout);
                safeInput(match_id_str, sizeof(match_id_str));
                
                int match_id = atoi(match_id_str);
                if (match_id <= 0) {
                    printf("Invalid match ID.\n");
                    break;
                }
                
                snprintf(cmd, sizeof(cmd), "MATCH_INFO %d", match_id);
                if (send_line(sock, cmd) < 0) {
                    perror("send() error");
                    break;
                }
                
                if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    // Parse response: code and data separated by space
                    int code = 0;
                    char *data_start = strchr(recvbuf, ' ');
                    if (data_start && sscanf(recvbuf, "%d", &code) == 1 && code == RESP_MATCH_INFO_OK) {
                        data_start++; // Skip space
                        // Replace | back to newlines for display
                        for (int i = 0; data_start[i] != '\0'; i++) {
                            if (data_start[i] == '|') data_start[i] = '\n';
                        }
                        printf("%s\n", data_start);
                    } else {
                        char pretty[1024];
                        beautify_result(recvbuf, pretty, sizeof(pretty));
                        printf("%s", pretty);
                    }
                }
                break;
            }

            case FUNC_SHOP_MENU: {
#ifdef USE_NCURSES
                int shop_sel = shop_menu_ncurses();
                if (shop_sel == -1) {
                    break;
                }
                if (shop_sel == 0) {
                    // Buy Armor flow
                    // Fetch current coin from server
                    int coin = -1;
                    if (send_line(sock, "GETCOIN") >= 0 && recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                        int code_tmp = 0;
                        int coin_tmp = -1;
                        if (sscanf(recvbuf, "%d %d", &code_tmp, &coin_tmp) == 2) {
                            coin = coin_tmp;
                        }
                    }
                    int armor_sel = shop_armor_menu_ncurses(coin);
                    if (armor_sel == -1) break; // cancelled
                    int armor_type = (armor_sel == 0) ? 1 : 2; // 1 BASIC, 2 ENHANCED
                    snprintf(cmd, sizeof(cmd), "BUYARMOR %d", armor_type);
                    if (send_line(sock, cmd) < 0) break;
                    if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                        char pretty[1024];
                        beautify_result(recvbuf, pretty, sizeof(pretty));
                        printf("%s", pretty);
                    }
                } else if (shop_sel == 1) {
                    // Buy Weapon flow
                    // Fetch current coin from server
                    int coin = -1;
                    if (send_line(sock, "GETCOIN") >= 0 && recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                        int code_tmp = 0;
                        int coin_tmp = -1;
                        if (sscanf(recvbuf, "%d %d", &code_tmp, &coin_tmp) == 2) {
                            coin = coin_tmp;
                        }
                    }
                    int weapon_sel = shop_weapon_menu_ncurses(coin);
                    if (weapon_sel == -1) break; // cancelled
                    // Map: 0=CANNON, 1=LASER, 2=MISSILE (matches server WeaponType)
                    int weapon_type = weapon_sel;
                    snprintf(cmd, sizeof(cmd), "BUY_WEAPON %d", weapon_type);
                    if (send_line(sock, cmd) < 0) break;
                    if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                        char pretty[1024];
                        beautify_result(recvbuf, pretty, sizeof(pretty));
                        printf("%s", pretty);
                    }
                }
#else
                printf("Shop menu is only available with ncurses.\n");
#endif
                break;
            }
            
            case FUNC_BATTLE_SCREEN: { /* Battle Screen UI */
#ifdef USE_NCURSES
                char my_username[128] = "";
                // Lấy username hiện tại
                if (send_line(sock, "WHOAMI") >= 0 && recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                    int code;
                    sscanf(recvbuf, "%d %127s", &code, my_username);
                }
                
                // Nhập Match ID
                printf("Enter Match ID (or press Enter to auto-detect): ");
                fflush(stdout);
                char match_id_str[64] = "";
                safeInput(match_id_str, sizeof(match_id_str));
                
                int match_id = -1;
                if (strlen(match_id_str) > 0) {
                    match_id = atoi(match_id_str);
                    if (match_id <= 0) {
                        printf("Invalid match ID.\n");
                        break;
                    }
                } else {
                    printf("Please enter Match ID.\n");
                    break;
                }
                
                // Vòng lặp màn hình chiến đấu
                while (1) {
                    snprintf(cmd, sizeof(cmd), "MATCH_INFO %d", match_id);
                    if (send_line(sock, cmd) < 0) break;
                    if (recv_line(sock, recvbuf, sizeof(recvbuf)) <= 0) break;
                    
                    int code = 0;
                    char *data_start = strchr(recvbuf, ' ');
                    if (!data_start || sscanf(recvbuf, "%d", &code) != 1 || code != RESP_MATCH_INFO_OK) {
                        char pretty[1024];
                        beautify_result(recvbuf, pretty, sizeof(pretty));
                        printf("%s", pretty);
                        break;
                    }
                    data_start++; 
                    
                    // Chuyển ký tự | thành xuống dòng để dễ parse
                    char match_info_buf[4096];
                    strncpy(match_info_buf, data_start, sizeof(match_info_buf) - 1);
                    match_info_buf[sizeof(match_info_buf) - 1] = '\0';
                    for (int i = 0; match_info_buf[i] != '\0'; i++) {
                        if (match_info_buf[i] == '|') match_info_buf[i] = '\n';
                    }
                    
                    // --- KHAI BÁO DỮ LIỆU ---
                    // Dùng mảng riêng để quản lý cấp phát bộ nhớ
                    char *team_left_names[3] = {NULL, NULL, NULL};
                    char *team_right_names[3] = {NULL, NULL, NULL};
                    
                    // Dùng mảng const char* để truyền vào hàm UI
                    const char *team_left[3] = {NULL, NULL, NULL};
                    const char *team_right[3] = {NULL, NULL, NULL};
                    
                    int team_left_hp_vals[3] = {0};
                    int team_right_hp_vals[3] = {0};

                    int left_count = 0;
                    int right_count = 0;
                    int my_hp = 0;
                    int my_team_id = -1;
                    int team1_id = -1, team2_id = -1;
                    int in_team1 = 0, in_team2 = 0;
                    
                    char current_player[128] = "";
                    int current_player_hp = 0;
                    
                    // --- PARSE DỮ LIỆU ---
                    char *line = match_info_buf;
                    while (*line != '\0') {
                        char *line_end = line;
                        while (*line_end != '\0' && *line_end != '\n') line_end++;
                        char saved_char = *line_end;
                        *line_end = '\0'; // Cắt chuỗi tạm thời
                        
                        if (strstr(line, "TEAM 1:") != NULL) {
                             char *id_start = strstr(line, "(ID: ");
                             if (id_start) sscanf(id_start, "(ID: %d)", &team1_id);
                             in_team1 = 1; in_team2 = 0; left_count = 0;
                             current_player[0] = '\0';
                        } else if (strstr(line, "TEAM 2:") != NULL) {
                             char *id_start = strstr(line, "(ID: ");
                             if (id_start) sscanf(id_start, "(ID: %d)", &team2_id);
                             in_team2 = 1; in_team1 = 0; right_count = 0;
                             current_player[0] = '\0';
                        } else if (strstr(line, "Player:") != NULL) {
                            if (sscanf(line, "  Player: %127s", current_player) == 1) {
                                // Xử lý khoảng trắng thừa nếu có
                                int len = strlen(current_player);
                                while(len > 0 && (current_player[len-1] == ' ' || current_player[len-1] == '\t')) 
                                    current_player[--len]='\0';
                            }
                        } else if (strstr(line, "HP:") != NULL && current_player[0] != '\0') {
                            char *hp_pos = strstr(line, "HP:");
                            if (hp_pos && sscanf(hp_pos, "HP: %d", &current_player_hp) == 1) {
                                
                                if (strcmp(current_player, my_username) == 0) {
                                    my_hp = current_player_hp;
                                    my_team_id = in_team1 ? team1_id : (in_team2 ? team2_id : -1);
                                }
                                
                                // Cấp phát bộ nhớ an toàn thay cho strdup
                                char *name_copy = malloc(strlen(current_player) + 1);
                                if (name_copy) strcpy(name_copy, current_player);
                                
                                if (name_copy) {
                                    if (in_team1 && left_count < 3) {
                                        team_left_names[left_count] = name_copy;
                                        team_left[left_count] = name_copy; // Gán vào mảng const
                                        team_left_hp_vals[left_count] = current_player_hp;
                                        left_count++;
                                    } else if (in_team2 && right_count < 3) {
                                        team_right_names[right_count] = name_copy;
                                        team_right[right_count] = name_copy; // Gán vào mảng const
                                        team_right_hp_vals[right_count] = current_player_hp;
                                        right_count++;
                                    } else {
                                        free(name_copy);
                                    }
                                }
                                current_player[0] = '\0'; 
                            }
                        }
                        
                        *line_end = saved_char; // Trả lại ký tự cũ
                        if (*line_end == '\n') line = line_end + 1; else break;
                    }
                    
                    // --- XÁC ĐỊNH PHE TA / PHE ĐỊCH ---
                    const char **friendly_team = NULL;
                    const char **enemy_team = NULL;
                    int *friendly_hp = NULL;
                    int *enemy_hp = NULL;
                    int friendly_count = 0;
                    int enemy_count = 0;
                    
                    int setup_ok = 0;
                    if (my_team_id == team1_id && my_team_id > 0) {
                        friendly_team = team_left; friendly_hp = team_left_hp_vals; friendly_count = left_count;
                        enemy_team = team_right;   enemy_hp = team_right_hp_vals;   enemy_count = right_count;
                        setup_ok = 1;
                    } else if (my_team_id == team2_id && my_team_id > 0) {
                        friendly_team = team_right; friendly_hp = team_right_hp_vals; friendly_count = right_count;
                        enemy_team = team_left;     enemy_hp = team_left_hp_vals;     enemy_count = left_count;
                        setup_ok = 1;
                    } 
                    
                    if (!setup_ok) {
                        printf("Error: Could not determine your team (You might be kicked or match ended).\n");
                        // Cleanup
                        for (int i = 0; i < 3; i++) { if(team_left_names[i]) free(team_left_names[i]); if(team_right_names[i]) free(team_right_names[i]); }
                        break;
                    }
                    
                    // --- GỌI UI ---
                    char target_username[128] = "";
                    int weapon_id = 0;
                    int battle_result = battle_screen_ncurses(
                        my_username,
                        friendly_team, friendly_hp, friendly_count,
                        enemy_team, enemy_hp, enemy_count,
                        my_hp,
                        target_username, sizeof(target_username),
                        &weapon_id
                    );
                    
                    // --- GIẢI PHÓNG BỘ NHỚ ---
                    for (int i = 0; i < 3; i++) { 
                        if(team_left_names[i]) free(team_left_names[i]); 
                        if(team_right_names[i]) free(team_right_names[i]); 
                    }
                    
                    // --- XỬ LÝ KẾT QUẢ ---
                    if (battle_result == 1) { // FIRE
                         if (strlen(target_username) > 0) {
                             snprintf(cmd, sizeof(cmd), "FIRE %s %d", target_username, weapon_id);
                             if (send_line(sock, cmd) < 0) break;
                             if (recv_line(sock, recvbuf, sizeof(recvbuf)) > 0) {
                                 char pretty[1024];
                                 beautify_result(recvbuf, pretty, sizeof(pretty));
                                 show_message_ncurses("FIRE Result", pretty);
                             }
                         }
                         continue;
                    } else if (battle_result == 0) { // SHOP
                         int shop_sel = shop_menu_ncurses();
                         if (shop_sel == 0) { // Armor
                            send_line(sock, "GETCOIN"); recv_line(sock, recvbuf, sizeof(recvbuf));
                            int coin = 0; sscanf(recvbuf, "%*d %d", &coin);
                            int armor_sel = shop_armor_menu_ncurses(coin);
                            if (armor_sel != -1) {
                                snprintf(cmd, sizeof(cmd), "BUYARMOR %d", armor_sel + 1);
                                send_line(sock, cmd);
                                recv_line(sock, recvbuf, sizeof(recvbuf));
                                char pretty[1024]; beautify_result(recvbuf, pretty, sizeof(pretty));
                                show_message_ncurses("Purchase Result", pretty);
                            }
                         } else if (shop_sel == 1) { // Weapon
                            send_line(sock, "GETCOIN"); recv_line(sock, recvbuf, sizeof(recvbuf));
                            int coin = 0; sscanf(recvbuf, "%*d %d", &coin);
                            int wp_sel = shop_weapon_menu_ncurses(coin);
                            if (wp_sel != -1) {
                                snprintf(cmd, sizeof(cmd), "BUY_WEAPON %d", wp_sel);
                                send_line(sock, cmd);
                                recv_line(sock, recvbuf, sizeof(recvbuf));
                                char pretty[1024]; beautify_result(recvbuf, pretty, sizeof(pretty));
                                show_message_ncurses("Purchase Result", pretty);
                            }
                         }
                         continue;
                    } else { // ESC / Back
                        break;
                    }
                }
#else
                printf("Battle screen is only available with ncurses.\n");
#endif
                break;
            }
            
            default: {
                printf("Invalid choice, please try again.\n");
                break;
            }
        }
        
        // Check for broadcast messages (like chest drop) after each command
        check_broadcast_messages(sock);
        
        printf("\n");
    }

    close(sock);
    printf("Client terminated.\n");
    return EXIT_SUCCESS;

}
