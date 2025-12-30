/**
 * @file server.c (HYBRID VERSION)
 * @brief Server chạy Epoll (Remote) nhưng chứa Logic xử lý (Local)
 */

#define _GNU_SOURCE

/* 1. INCLUDE CÁC THƯ VIỆN CỦA CẢ 2 BÊN */
#include "server.h"
#include "epoll.h"
#include "config.h"
#include "app_context.h"
#include <signal.h>
#include "session.h"
#include "users.h"
#include "team_handler.h" // Để xử lý team
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "users.h"
#include "users_io.h"
#include "hash.h"
#include "file_transfer.h"
#include "session.h"
#include "config.h"
#include "db_schema.h"
#include "db.c"
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <time.h> 

static int listen_sock = -1;

static void handle_signal(int sig) {
    (void)sig;
    // Request epoll loop to stop; cleanup happens after server_run returns
    epoll_request_stop();
}

/* * 2. HÀM XỬ LÝ LOGIC (LẤY TỪ LOCAL)
 * Lưu ý: Đã bỏ 'static' và bỏ 'mutex' vì Epoll chạy đơn luồng an toàn
 */
int handle_client_command(ServerSession *session, const char *line, 
                          char *response_buf, size_t response_size) {
    
    char cmd[32], arg1[2048], arg2[2048];
    memset(arg1, 0, sizeof(arg1));
    memset(arg2, 0, sizeof(arg2));
    
    int parsed = sscanf(line, "%31s %s %[^\r\n]", cmd, arg1, arg2);
    
    // Lấy UserTable từ AppContext của Remote
    UserTable *ut = get_global_user_table(); 
    int response_code = RESP_SYNTAX_ERROR;
    
    printf("[DEBUG] User '%s' sent: '%s'\n", session->username, line);

    /* ========== AUTHENTICATION ========== */
    if (strcmp(cmd, "REGISTER") == 0) {
        if (parsed != 3) response_code = RESP_SYNTAX_ERROR;
        else response_code = server_handle_register(ut, arg1, arg2);
        snprintf(response_buf, response_size, "%d", response_code);
    }
    else if (strcmp(cmd, "LOGIN") == 0) {
        if (parsed != 3) response_code = RESP_SYNTAX_ERROR;
        else response_code = server_handle_login(session, ut, arg1, arg2);
        
        if (response_code == RESP_LOGIN_OK) {
             char session_id[64];
             snprintf(session_id, sizeof(session_id), "sess_%d_%ld", session->socket_fd, (long)time(NULL));
             snprintf(response_buf, response_size, "%d %s", response_code, session_id);
             return response_code;
        }
        snprintf(response_buf, response_size, "%d", response_code);
    }
    else if (strcmp(cmd, "WHOAMI") == 0) {
        char username[MAX_USERNAME];
        response_code = server_handle_whoami(session, username);
        if (response_code == RESP_WHOAMI_OK) 
            snprintf(response_buf, response_size, "%d %s", response_code, username);
        else 
            snprintf(response_buf, response_size, "%d", response_code);
    }
    else if (strcmp(cmd, "BYE") == 0 || strcmp(cmd, "LOGOUT") == 0) {
        response_code = server_handle_bye(session);
        snprintf(response_buf, response_size, "%d", response_code);
    }
    
    /* ========== GAME COMMANDS ========== */
    else if (strcmp(cmd, "GETCOIN") == 0) {
        if (!session->isLoggedIn) snprintf(response_buf, response_size, "%d", RESP_NOT_LOGGED);
        else {
            User *user = findUser(ut, session->username);
            if(user) snprintf(response_buf, response_size, "%d %ld", RESP_COIN_OK, user->coin);
            else snprintf(response_buf, response_size, "%d", RESP_INTERNAL_ERROR);
        }
    }
    else if (strcmp(cmd, "BUYARMOR") == 0) {
        if (parsed < 2) response_code = RESP_SYNTAX_ERROR;
        else response_code = server_handle_buyarmor(session, ut, atoi(arg1));
        snprintf(response_buf, response_size, "%d", response_code);
    }
    else if (strcmp(cmd, "START_MATCH") == 0) {
        if (parsed < 2) response_code = RESP_SYNTAX_ERROR;
        else response_code = server_handle_start_match(session, atoi(arg1));
        snprintf(response_buf, response_size, "%d", response_code);
    }
    else if (strcmp(cmd, "GET_MATCH_RESULT") == 0) {
        if (parsed < 2) response_code = RESP_SYNTAX_ERROR;
        else response_code = server_handle_get_match_result(session, atoi(arg1));
        snprintf(response_buf, response_size, "%d", response_code);
    }
    
    /* ========== TEAM COMMANDS (Giữ lại từ Local) ========== */
    else if (strcmp(cmd, "CREATE_TEAM") == 0 || strcmp(cmd, "CREATETEAM") == 0) {
        if (parsed < 2) response_code = RESP_SYNTAX_ERROR;
        else response_code = handle_create_team(session, ut, arg1);
        snprintf(response_buf, response_size, "%d", response_code);
    }
    else if (strcmp(cmd, "DELETE_TEAM") == 0) {
        response_code = handle_delete_team(session);
        snprintf(response_buf, response_size, "%d", response_code);
    }
    else if (strcmp(cmd, "LIST_TEAMS") == 0) {
        char list_buf[4096] = ""; 
        response_code = handle_list_teams(session, list_buf, sizeof(list_buf));
        if (response_code == RESP_LOGIN_OK && strlen(list_buf) > 0)
            snprintf(response_buf, response_size, "%s", list_buf);
        else
            snprintf(response_buf, response_size, "%d", response_code);
    }
    else if (strcmp(cmd, "JOIN_REQUEST") == 0) {
        if (parsed < 2) response_code = RESP_SYNTAX_ERROR;
        else response_code = handle_join_request(session, arg1);
        snprintf(response_buf, response_size, "%d", response_code);
    }
    else if (strcmp(cmd, "JOIN_APPROVE") == 0) {
        if (parsed < 2) response_code = RESP_SYNTAX_ERROR;
        else response_code = handle_join_approve(session, arg1);
        snprintf(response_buf, response_size, "%d", response_code);
    }
    else if (strcmp(cmd, "JOIN_REJECT") == 0) {
        if (parsed < 2) response_code = RESP_SYNTAX_ERROR;
        else response_code = handle_join_reject(session, arg1);
        snprintf(response_buf, response_size, "%d", response_code);
    }
    else if (strcmp(cmd, "TEAM_MEMBER_LIST") == 0) {
        char members_buf[2048] = "";
        response_code = handle_team_member_list(session, members_buf, sizeof(members_buf));
        if (response_code == RESP_TEAM_MEMBERS_LIST_OK)
             snprintf(response_buf, response_size, "%d %s", response_code, members_buf);
        else
             snprintf(response_buf, response_size, "%d", response_code);
    }
    else if (strcmp(cmd, "LEAVE_TEAM") == 0) {
        response_code = handle_leave_team(session);
        snprintf(response_buf, response_size, "%d", response_code);
    }
    else if (strcmp(cmd, "KICK_MEMBER") == 0) {
        if (parsed < 2) response_code = RESP_SYNTAX_ERROR;
        else response_code = handle_kick_member(session, arg1);
        snprintf(response_buf, response_size, "%d", response_code);
    }
    else if (strcmp(cmd, "INVITE") == 0) {
        if (parsed < 2) response_code = RESP_SYNTAX_ERROR;
        else response_code = handle_invite(session, arg1);
        snprintf(response_buf, response_size, "%d", response_code);
    }
    else if (strcmp(cmd, "INVITE_ACCEPT") == 0) {
        if (parsed < 2) response_code = RESP_SYNTAX_ERROR;
        else response_code = handle_invite_accept(session, arg1);
        snprintf(response_buf, response_size, "%d", response_code);
    }
    else if (strcmp(cmd, "INVITE_REJECT") == 0) {
        if (parsed < 2) response_code = RESP_SYNTAX_ERROR;
        else response_code = handle_invite_reject(session, arg1);
        snprintf(response_buf, response_size, "%d", response_code);
    }
    else if (strcmp(cmd, "REPAIR") == 0) {
        if (parsed < 2) {
            response_code = RESP_SYNTAX_ERROR;
            snprintf(response_buf, response_size, "%d", response_code);
        } else {
            int repair_amount = atoi(arg1);
            RepairResult repair_result = {0};
            response_code = server_handle_repair(session, g_user_table, repair_amount, &repair_result);
            if (response_code == RESP_REPAIR_OK) {
                snprintf(response_buf, response_size, "%d %d %d", response_code, repair_result.hp, repair_result.coin);
            } else {
                snprintf(response_buf, response_size, "%d", response_code);
            }
        }
    }
    else {
        response_code = RESP_SYNTAX_ERROR;
        snprintf(response_buf, response_size, "%d", response_code);
    }
    
    return response_code;
}

/* * 3. HỆ THỐNG EPOLL SERVER (LẤY TỪ REMOTE)
 */

int set_nonblocking(int fd) {
    int on = 1;
    if (ioctl(fd, FIONBIO, (char *)&on) < 0) return -1;
    return 0;
}

int server_init(void) {
    int rc;
    int on = 1;
    struct sockaddr_in server_addr;

    // Step 1: Initialize context
    if (app_context_init() != 0) {
        fprintf(stderr, "[ERROR] Failed to initialize application context\n");
        return -1;
    }

    // Step 2: Create socket
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        perror("[ERROR] socket() failed");
        app_context_cleanup();
        return -1;
    }

    if (set_nonblocking(listen_sock) < 0) {
        perror("[ERROR] set_nonblocking() failed");
        close(listen_sock);
        app_context_cleanup();
        return -1;
    }

    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0) {
        perror("[WARN] setsockopt(SO_REUSEADDR) failed");
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    rc = bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (rc < 0) {
        perror("[ERROR] bind() failed");
        close(listen_sock);
        app_context_cleanup();
        return -1;
    }

    rc = listen(listen_sock, BACKLOG);
    if (rc < 0) {
        perror("[ERROR] listen() failed");
        close(listen_sock);
        app_context_cleanup();
        return -1;
    }

    epoll_init(listen_sock);

    printf("========================================\n");
    printf("Server Hybrid (Epoll + Local Logic)\n");
    printf("Port: %d\n", PORT);
    printf("========================================\n");

    return 0;
}

void server_run(void) {
    epoll_run();
}

void server_shutdown(void) {
    if (listen_sock >= 0) {
        close(listen_sock);
        listen_sock = -1;
    }
    app_context_cleanup();
    printf("[INFO] Server shutdown complete.\n");
}

int main(int argc, char *argv[]) {
    (void)argc; // PORT is from config.h, not command line
    (void)argv;

    // Register signal handlers for graceful shutdown
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    (void)argc; (void)argv;

    if (server_init() != 0) {
        fprintf(stderr, "[ERROR] Server initialization failed\n");
        return EXIT_FAILURE;
    }

    server_run();
    server_shutdown();

    return EXIT_SUCCESS;
}