#ifndef SESSION_H
#define SESSION_H

#include <stdbool.h>
#include <netinet/in.h>

#define MAX_USERNAME 64

typedef struct {
    bool isLoggedIn;            /**< Trạng thái đăng nhập */
    char username[MAX_USERNAME]; /**< Tên người dùng */
    int socket_fd;               /**< Socket ID của client */
    int current_match_id;        /**< ID trận đấu hiện tại, -1 nếu không có */
    int current_team_id;         /**< ID đội hiện tại, -1 nếu không có */
    struct sockaddr_in client_addr; /**< Địa chỉ client */
} ServerSession;


typedef struct SessionNode {
    ServerSession session;
    struct SessionNode *next;
} SessionNode;

/* Các hàm tìm kiếm session (Sẽ dùng từ hệ thống chính sau khi merge) */
// SessionNode *find_session_by_socket(int socket_fd);
// SessionNode *find_session_by_username(const char *username);

#endif