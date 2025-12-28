/**
 * @file server.c
 * @brief Hybrid TCP Server: Multiplexing (select) + Multi-threading
 */

#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <pthread.h>
#include <time.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include "users.h"
#include "users_io.h"
#include "hash.h"
#include "file_transfer.h"
#include "session.h"
#include "config.h"
#include "db_schema.h"
#include "team_handler.h"

#define BUFF_SIZE 2048
#define DESIRED_NOFILE_LIMIT 65535
#define USERS_FILE "TCP_Server/users.txt"
#define HASH_SIZE 101

#define MAX_CONN_PER_WORKER 900    
#define MAX_WORKERS 100            

/* Global resources (protected by mutexes) */
UserTable *g_user_table = NULL; 
pthread_mutex_t g_user_table_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    pthread_t thread_id;
    int active;
    int conn_count;
    int pipe_fd[2];
    pthread_mutex_t lock;
} WorkerThread;

static WorkerThread g_workers[MAX_WORKERS];
static int g_worker_count = 0;
static pthread_mutex_t g_workers_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Print usage */
static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s <PortNumber>\n", prog);
    fprintf(stderr, "Example: %s 5500\n", prog);
}

/**
 * @brief Try to raise RLIMIT_NOFILE (soft limit) to accommodate >1024 sockets.
 */
static void try_raise_nofile_limit(void) {
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
        rlim_t desired = DESIRED_NOFILE_LIMIT;
        if (desired > rl.rlim_max) desired = rl.rlim_max;
        if (rl.rlim_cur < desired) {
            rl.rlim_cur = desired;
            if (setrlimit(RLIMIT_NOFILE, &rl) != 0) {
                fprintf(stderr, "[WARN] Could not raise RLIMIT_NOFILE (soft): %s\n", strerror(errno));
            } else {
                fprintf(stderr, "[INFO] RLIMIT_NOFILE raised to %lu\n", (unsigned long)rl.rlim_cur);
            }
        }
    } else {
        fprintf(stderr, "[WARN] getrlimit(RLIMIT_NOFILE) failed: %s\n", strerror(errno));
    }
}

/**
 * @brief Handle a command from a client (thread-safe)
 */
static int handle_client_command(ServerSession *session, const char *line, 
                                 char *response_buf, size_t response_size) {
    char cmd[32], arg1[BUFF_SIZE], arg2[BUFF_SIZE];
    memset(arg1, 0, sizeof(arg1));
    memset(arg2, 0, sizeof(arg2));
    int parsed = sscanf(line, "%31s %s %[^\r\n]", cmd, arg1, arg2);
    
    // DEBUG: In ra lệnh nhận được để kiểm tra
    printf("[DEBUG] User '%s' sent: '%s' (Parsed CMD: %s)\n", session->username, line, cmd);

    int response_code;
    
    /* ========== AUTHENTICATION COMMANDS ========== */
    if (strcmp(cmd, "REGISTER") == 0) {
        if (parsed != 3) {
            response_code = RESP_SYNTAX_ERROR;
        } else {
            pthread_mutex_lock(&g_user_table_mutex);
            response_code = server_handle_register(g_user_table, arg1, arg2);
            pthread_mutex_unlock(&g_user_table_mutex);
        }
        snprintf(response_buf, response_size, "%d", response_code);
    }
    else if (strcmp(cmd, "LOGIN") == 0) {
        if (parsed != 3) {
            response_code = RESP_SYNTAX_ERROR;
        } else {
            pthread_mutex_lock(&g_user_table_mutex);
            response_code = server_handle_login(session, g_user_table, arg1, arg2);
            pthread_mutex_unlock(&g_user_table_mutex);
        }
        snprintf(response_buf, response_size, "%d", response_code);
    }
    else if (strcmp(cmd, "WHOAMI") == 0) {
        char username[MAX_USERNAME];
        response_code = server_handle_whoami(session, username);
        if (response_code == RESP_WHOAMI_OK) {
            snprintf(response_buf, response_size, "%d %s", response_code, username);
        } else {
            snprintf(response_buf, response_size, "%d", response_code);
        }
    }
    else if (strcmp(cmd, "BYE") == 0 || strcmp(cmd, "LOGOUT") == 0) {
        if (session->isLoggedIn) {
            clear_user_requests(session->username);
        }
        response_code = server_handle_bye(session);
        snprintf(response_buf, response_size, "%d", response_code);
    }
    
    /* ========== GAME COMMANDS ========== */
    else if (strcmp(cmd, "GETCOIN") == 0) {
        if (!session->isLoggedIn) {
            response_code = RESP_NOT_LOGGED;
            snprintf(response_buf, response_size, "%d", response_code);
        } else {
            pthread_mutex_lock(&g_user_table_mutex);
            User *user = findUser(g_user_table, session->username);
            if (user) {
                response_code = RESP_COIN_OK;
                snprintf(response_buf, response_size, "%d %ld", 
                         response_code, user->coin);
            } else {
                response_code = RESP_INTERNAL_ERROR;
                snprintf(response_buf, response_size, "%d", response_code);
            }
            pthread_mutex_unlock(&g_user_table_mutex);
        }
    }
    else if (strcmp(cmd, "GETARMOR") == 0) {
        // ... (Logic cũ giữ nguyên) ...
        if (!session->isLoggedIn) {
             response_code = RESP_NOT_LOGGED;
             snprintf(response_buf, response_size, "%d", response_code);
        } else {
             // Mock response for now if not implemented
             response_code = RESP_NOT_IN_MATCH;
             snprintf(response_buf, response_size, "%d", response_code);
        }
    }
    else if (strcmp(cmd, "BUYARMOR") == 0) {
        if (parsed < 2) {
            response_code = RESP_SYNTAX_ERROR;
            snprintf(response_buf, response_size, "%d", response_code);
        } else {
            int armor_type = atoi(arg1);
            pthread_mutex_lock(&g_user_table_mutex);
            response_code = server_handle_buyarmor(session, g_user_table, armor_type);
            pthread_mutex_unlock(&g_user_table_mutex);
            snprintf(response_buf, response_size, "%d", response_code);
        }
    }
    
    /* ========== TEAM COMMANDS (ĐÃ SỬA DUAL CHECK) ========== */
    // Chấp nhận cả "CREATE_TEAM" (Server prefer) VÀ "CREATETEAM" (Client send)
    
    else if (strcmp(cmd, "CREATE_TEAM") == 0 || strcmp(cmd, "CREATETEAM") == 0) {
        if (parsed < 2) {
            response_code = RESP_SYNTAX_ERROR;
        } else {
            pthread_mutex_lock(&g_user_table_mutex);
            response_code = handle_create_team(session, g_user_table, arg1);
            pthread_mutex_unlock(&g_user_table_mutex);
        }
        snprintf(response_buf, response_size, "%d", response_code);
    }
    else if (strcmp(cmd, "DELETE_TEAM") == 0 || strcmp(cmd, "DELETETEAM") == 0) {

        response_code = handle_delete_team(session);
        snprintf(response_buf, response_size, "%d", response_code);
    }
    /* ========== SỬA ĐOẠN LIST_TEAMS ========== */
    else if (strcmp(cmd, "LIST_TEAMS") == 0 || strcmp(cmd, "LISTTEAMS") == 0) {
        char teams_buf[4096] = ""; 
        response_code = handle_list_teams(session, teams_buf, sizeof(teams_buf));
        
        if (response_code == RESP_LOGIN_OK && strlen(teams_buf) > 0) {
            snprintf(response_buf, response_size, "%s", teams_buf);
        } else {
            snprintf(response_buf, response_size, "%d", response_code);
        }
    }

    /* ========== SỬA ĐOẠN TEAM_MEMBER_LIST ========== */
    else if (strcmp(cmd, "TEAM_MEMBER_LIST") == 0 || strcmp(cmd, "TEAMMEMBERS") == 0) {
        
        char members_buf[2048] = "";
        
        response_code = handle_team_member_list(session, members_buf, sizeof(members_buf));
        
        if (response_code == RESP_TEAM_MEMBERS_LIST_OK) {

            snprintf(response_buf, response_size, "%d %s", response_code, members_buf);
        } 
        else if (response_code == RESP_NOT_IN_TEAM) {
             snprintf(response_buf, response_size, "%d NOT_IN_TEAM", response_code);
        }
        else {
            snprintf(response_buf, response_size, "%d", response_code);
        }
    }
    else if (strcmp(cmd, "LEAVE_TEAM") == 0 || strcmp(cmd, "LEAVETEAM") == 0) {
        response_code = handle_leave_team(session);
        snprintf(response_buf, response_size, "%d", response_code);
    }
    else if (strcmp(cmd, "KICK_MEMBER") == 0 || strcmp(cmd, "KICK") == 0) {
            if (parsed < 2) {
                response_code = RESP_SYNTAX_ERROR;
            } else {
            response_code = handle_kick_member(session, arg1);
        }
        snprintf(response_buf, response_size, "%d", response_code);
    }
    else if (strcmp(cmd, "JOIN_REQUEST") == 0 || strcmp(cmd, "JOINREQ") == 0) {
        if (parsed < 2) {
            response_code = RESP_SYNTAX_ERROR;
        } else {
            response_code = handle_join_request(session, arg1);
        }
        snprintf(response_buf, response_size, "%d", response_code);
    }
    else if (strcmp(cmd, "JOIN_APPROVE") == 0 || strcmp(cmd, "APPROVEJOIN") == 0) {
        if (parsed < 2) {
            response_code = RESP_SYNTAX_ERROR;
        } else {
            response_code = handle_join_approve(session, arg1);
        }
        snprintf(response_buf, response_size, "%d", response_code);
    }
    else if (strcmp(cmd, "JOIN_REJECT") == 0 || strcmp(cmd, "REJECTJOIN") == 0) {
        if (parsed < 2) {
            response_code = RESP_SYNTAX_ERROR;
        } else {
            response_code = handle_join_reject(session, arg1);
        }
        snprintf(response_buf, response_size, "%d", response_code);
    }
    else if (strcmp(cmd, "INVITE") == 0) {
        if (parsed < 2) {
            response_code = RESP_SYNTAX_ERROR;
        } else {
            response_code = handle_invite(session, arg1);
        }
        snprintf(response_buf, response_size, "%d", response_code);
    }
    else if (strcmp(cmd, "INVITE_ACCEPT") == 0 || strcmp(cmd, "INVITEACCEPT") == 0) {
        if (parsed < 2) {
            response_code = RESP_SYNTAX_ERROR;
        } else {
            response_code = handle_invite_accept(session, arg1);
        }
        snprintf(response_buf, response_size, "%d", response_code);
    }
    else if (strcmp(cmd, "INVITE_REJECT") == 0 || strcmp(cmd, "INVITEREJECT") == 0) {
        if (parsed < 2) {
            response_code = RESP_SYNTAX_ERROR;
        } else {
            response_code = handle_invite_reject(session, arg1);
        }
        snprintf(response_buf, response_size, "%d", response_code);
    }
    else {
        response_code = RESP_SYNTAX_ERROR;
        snprintf(response_buf, response_size, "%d", response_code);
    }
    
    return response_code;
}

/**
 * @brief Worker thread function
 */
static void *worker_thread_func(void *arg) {
    WorkerThread *worker = (WorkerThread *)arg;
    
    fd_set master_set, read_set;
    int max_fd = worker->pipe_fd[0];
    
    FD_ZERO(&master_set);
    FD_SET(worker->pipe_fd[0], &master_set);
    
    ServerSession sessions[MAX_CONN_PER_WORKER];
    for (int i = 0; i < MAX_CONN_PER_WORKER; i++) {
        initServerSession(&sessions[i]);
    }
    
    printf("[WORKER %lu] Started\n", (unsigned long)pthread_self());
    
    while (1) {
        read_set = master_set;
        int activity = select(max_fd + 1, &read_set, NULL, NULL, NULL);
        
        if (activity < 0) {
            if (errno == EINTR) continue;
            perror("[WORKER] select() error");
            break;
        }
        
        /* Check pipe for new connections */
        if (FD_ISSET(worker->pipe_fd[0], &read_set)) {
            int new_sock;
            struct sockaddr_in client_addr;
            
            ssize_t nr = read(worker->pipe_fd[0], &new_sock, sizeof(new_sock));
            if (nr == sizeof(new_sock)) {
                 read(worker->pipe_fd[0], &client_addr, sizeof(client_addr));
                 
                 int slot = -1;
                 for (int i = 0; i < MAX_CONN_PER_WORKER; i++) {
                     if (sessions[i].socket_fd == -1) {
                         slot = i;
                         break;
                     }
                 }
                 
                 if (slot != -1) {
                     FD_SET(new_sock, &master_set);
                     if (new_sock > max_fd) max_fd = new_sock;
                     sessions[slot].socket_fd = new_sock;
                     sessions[slot].client_addr = client_addr;
                     sessions[slot].isLoggedIn = false;
                     sessions[slot].username[0] = '\0';
                     sessions[slot].current_match_id = -1;
                     sessions[slot].current_team_id = -1;
                     
                     add_session(&sessions[slot]);
                     send_line(new_sock, "100");
                 } else {
                     send_line(new_sock, "503");
                     close(new_sock);
                 }
            }
        }
        
        /* Check client sockets */
        for (int i = 0; i < MAX_CONN_PER_WORKER; i++) {
            int sock = sessions[i].socket_fd;
            if (sock == -1) continue;
            
            if (FD_ISSET(sock, &read_set)) {
                char line[BUFF_SIZE];
                int n = recv_line(sock, line, sizeof(line));
                
                if (n <= 0) {
                    if (sessions[i].isLoggedIn) {
                        clear_user_requests(sessions[i].username);
                    }
                    close(sock);
                    FD_CLR(sock, &master_set);
                    remove_session_by_socket(sock);
                    initServerSession(&sessions[i]);
                    pthread_mutex_lock(&worker->lock);
                    worker->conn_count--;
                    pthread_mutex_unlock(&worker->lock);
                } else {
                    // TĂNG KÍCH THƯỚC BUFFER ĐỂ TRÁNH TRUNCATION
                    char response[8192]; 
                    
                    int response_code = handle_client_command(&sessions[i], line, 
                                                              response, sizeof(response));
                    
                    // Xử lý Login success
                    char cmd[32];
                    sscanf(line, "%31s", cmd);
                    if (strcmp(cmd, "LOGIN") == 0 && response_code == RESP_LOGIN_OK) {
                        char session_id[64];
                        snprintf(session_id, sizeof(session_id), "sess_%d_%ld", sock, (long)time(NULL));
                        snprintf(response, sizeof(response), "%d %s", response_code, session_id);
                    }
                    
                    if (send_line(sock, response) < 0) {
                        close(sock);
                        FD_CLR(sock, &master_set);
                        remove_session_by_socket(sock);
                        initServerSession(&sessions[i]);
                        pthread_mutex_lock(&worker->lock);
                        worker->conn_count--;
                        pthread_mutex_unlock(&worker->lock);
                    }
                }
            }
        }
    }
    return NULL;
}

// ... (Phần get_available_worker và main giữ nguyên như cũ) ...
static WorkerThread *get_available_worker(void) {
    pthread_mutex_lock(&g_workers_mutex);
    
    for (int i = 0; i < g_worker_count; i++) {
        pthread_mutex_lock(&g_workers[i].lock);
        if (g_workers[i].active && g_workers[i].conn_count < MAX_CONN_PER_WORKER) {
            g_workers[i].conn_count++;
            pthread_mutex_unlock(&g_workers[i].lock);
            pthread_mutex_unlock(&g_workers_mutex);
            return &g_workers[i];
        }
        pthread_mutex_unlock(&g_workers[i].lock);
    }
    
    if (g_worker_count < MAX_WORKERS) {
        WorkerThread *worker = &g_workers[g_worker_count];
        
        if (pipe(worker->pipe_fd) != 0) {
            pthread_mutex_unlock(&g_workers_mutex);
            return NULL;
        }
        
        worker->active = 1;
        worker->conn_count = 1;
        pthread_mutex_init(&worker->lock, NULL);
        
        if (pthread_create(&worker->thread_id, NULL, worker_thread_func, worker) != 0) {
            close(worker->pipe_fd[0]);
            close(worker->pipe_fd[1]);
            pthread_mutex_unlock(&g_workers_mutex);
            return NULL;
        }
        
        pthread_detach(worker->thread_id);
        g_worker_count++;
        printf("[INFO] Created worker thread #%d (total: %d)\n", g_worker_count, g_worker_count);
        
        pthread_mutex_unlock(&g_workers_mutex);
        return worker;
    }
    
    pthread_mutex_unlock(&g_workers_mutex);
    return NULL;
}

int main(int argc, char *argv[]) {
    try_raise_nofile_limit();
    if (argc != 2) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port number.\n");
        return EXIT_FAILURE;
    }

    g_user_table = initUserTable(HASH_SIZE);
    if (!g_user_table) {
        fprintf(stderr, "Failed to allocate user table.\n");
        return EXIT_FAILURE;
    }

    UserIOStatus status = loadUsers(g_user_table, USERS_FILE);
    if (status != USER_IO_OK) {
        fprintf(stderr, "Failed to load users (status=%d).\n", status);
        freeUserTable(g_user_table);
        return EXIT_FAILURE;
    }
    
    printf("Loaded users successfully.\n");

    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        perror("socket() error");
        freeUserTable(g_user_table);
        return EXIT_FAILURE;
    }

    int opt = 1;
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt() error");
        close(listen_sock);
        freeUserTable(g_user_table);
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind() error");
        close(listen_sock);
        freeUserTable(g_user_table);
        return EXIT_FAILURE;
    }

    if (listen(listen_sock, SOMAXCONN) < 0) {
        perror("listen() error");
        close(listen_sock);
        freeUserTable(g_user_table);
        return EXIT_FAILURE;
    }
    
    init_session_manager();
    
    printf("========================================\n");
    printf("Hybrid Server (Multiplexing + Threading)\n");
    printf("With Team Management System (Fixed Protocols)\n");
    printf("Port: %d\n", port);
    printf("========================================\n");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t sin_size = sizeof(client_addr);
        int new_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &sin_size);
        
        if (new_sock < 0) {
            perror("accept() error");
            continue;
        }
        
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("[MAIN] Accepted connection from %s:%d on socket %d\n",
               client_ip, ntohs(client_addr.sin_port), new_sock);
        
        WorkerThread *worker = get_available_worker();
        
        if (!worker) {
            fprintf(stderr, "[MAIN] Server at full capacity, rejecting connection\n");
            send_line(new_sock, "503");
            close(new_sock);
            continue;
        }
        
        if (write(worker->pipe_fd[1], &new_sock, sizeof(new_sock)) != sizeof(new_sock)) {
            close(new_sock);
            pthread_mutex_lock(&worker->lock);
            worker->conn_count--;
            pthread_mutex_unlock(&worker->lock);
            continue;
        }
        
        if (write(worker->pipe_fd[1], &client_addr, sizeof(client_addr)) != sizeof(client_addr)) {
            close(new_sock);
            pthread_mutex_lock(&worker->lock);
            worker->conn_count--;
            pthread_mutex_unlock(&worker->lock);
            continue;
        }
    }

    close(listen_sock);
    cleanup_session_manager();
    freeUserTable(g_user_table);
    return EXIT_SUCCESS;
}