#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>

#include "router.h"
#include "networking.h"
#include "ship_defs.h"
#include "game_logic.h"
#include "challenge.h"
#include "chest_logic.h"
#include "session.h"

#define BUFF_SIZE 1024

// Extern các biến toàn cục để tìm session
extern Ship active_ships[];
extern int num_active_ships;

// ---------------------------------------------------------------------------
// Helper Functions
// ---------------------------------------------------------------------------

int sendData(int s, char *buff, int size, int flags) {
    int n = send(s, buff, size, flags);
    if(n < 0)
        perror("send() error");
    return n;
}

// Gửi tin nhắn đến một socket cụ thể
void send_to_client(int socket_fd, const char* message) {
    if (socket_fd > 0) {
        sendData(socket_fd, (char*)message, strlen(message), 0);
    }
}

/**
 * @brief Tạo Session tạm thời từ socket để truyền vào các hàm Logic
 * Vì hệ thống cũ dùng active_ships, ta cần map nó sang ServerSession
 */
static void get_session_from_socket(int sock, ServerSession *session) {
    memset(session, 0, sizeof(ServerSession));
    session->socket_fd = sock;
    session->isLoggedIn = false;

    // Tìm trong mảng active_ships xem socket này thuộc tàu nào
    for (int i = 0; i < num_active_ships; i++) {
        if (active_ships[i].socket_fd == sock) {
            session->isLoggedIn = true;
            session->current_team_id = active_ships[i].team_id;
            // Map thêm username nếu struct Ship có lưu, hoặc map ID
            snprintf(session->username, sizeof(session->username), "Ship_%d", active_ships[i].ship_id);
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// Main Processing
// ---------------------------------------------------------------------------

void process_command(int client_sock, char *raw_command) {
    char type[32];
    char payload[256]; // Tăng kích thước buffer để an toàn
    
    memset(type, 0, sizeof(type));
    memset(payload, 0, sizeof(payload));

    // Parse lệnh: "LỆNH [PAYLOAD]"
    // VD: "FIRE 102 1" -> type="FIRE", payload="102 1"
    // Nếu chỉ có lệnh (VD: "LOGOUT") thì sscanf trả về 1, payload rỗng
    int args = sscanf(raw_command, "%s %[^\n]", type, payload);
    if (args < 1) return; 

    // 1. Tạo biến session cho request này
    ServerSession session;
    get_session_from_socket(client_sock, &session);

    int res_code = 0;
    char response[BUFF_SIZE];
    memset(response, 0, BUFF_SIZE);

    // ========================================================================
    // 1. XỬ LÝ LỆNH FIRE (TẤN CÔNG) - Đã cập nhật theo chuẩn mới
    // ========================================================================
    if (strcmp(type, "FIRE") == 0) {
        int target_id, weapon_id;
        
        // Cú pháp: FIRE <TargetID> <WeaponID>
        if (sscanf(payload, "%d %d", &target_id, &weapon_id) == 2) {
            
            // Struct hứng kết quả chi tiết
            FireResult result; 
            memset(&result, 0, sizeof(FireResult));

            // Gọi Logic: LOCKING được xử lý bên trong hàm này
            res_code = server_handle_fire(&session, target_id, weapon_id, &result);

            if (res_code == RESP_FIRE_OK) {
                // A. Gửi thành công cho người bắn (131)
                snprintf(response, sizeof(response), "%d %d %d %d %d\r\n", 
                        RESP_FIRE_OK, result.target_id, result.damage_dealt, 
                        result.target_remaining_hp, result.target_remaining_armor);
                send_to_client(client_sock, response);

                // B. Broadcast sự kiện cho cả phòng (200)
                // Hàm này nằm ở networking.c
                broadcast_fire_event(result.attacker_id, result.target_id, result.damage_dealt, result.target_remaining_hp, result.target_remaining_armor);
                
                // Đã gửi xong, không cần gửi lại ở cuối hàm
                return; 

            } else {
                // C. Gửi mã lỗi (3xx, 5xx)
                snprintf(response, sizeof(response), "%d FIRE_FAIL\r\n", res_code);
            }
        } else {
            // Sai cú pháp
            snprintf(response, sizeof(response), "301 SYNTAX_ERROR\r\n");
        }
        send_to_client(client_sock, response);
    }

    // ========================================================================
    // 2. XỬ LÝ CHALLENGE (THÁCH ĐẤU) - Giữ nguyên logic cũ
    // ========================================================================
    else if (strcmp(type, "SEND_CHALLENGE") == 0) {
        int target_team_id = atoi(payload);
        int challenge_id = 0;
        
        // Sửa: Truyền &session thay vì session (vì hàm nhận con trỏ)
        res_code = server_handle_send_challenge(&session, target_team_id, &challenge_id);
        
        snprintf(response, sizeof(response), "%d CHALLENGE_SENT %d\r\n", res_code, challenge_id);
        send_to_client(client_sock, response);
    }
    else if (strcmp(type, "ACCEPT_CHALLENGE") == 0) {
        int challenge_id = atoi(payload);
        
        res_code = server_handle_accept_challenge(&session, challenge_id);
        
        snprintf(response, sizeof(response), "%d CHALLENGE_ACCEPTED %d\r\n", res_code, challenge_id);
        send_to_client(client_sock, response);
    }
    else if (strcmp(type, "DECLINE_CHALLENGE") == 0) {
        int challenge_id = atoi(payload);
        
        res_code = server_handle_decline_challenge(&session, challenge_id);
        
        snprintf(response, sizeof(response), "%d CHALLENGE_DECLINED %d\r\n", res_code, challenge_id);
        send_to_client(client_sock, response);
    }
    else if (strcmp(type, "CANCEL_CHALLENGE") == 0) {
        int challenge_id = atoi(payload);
        
        res_code = server_handle_cancel_challenge(&session, challenge_id);
        
        snprintf(response, sizeof(response), "%d CHALLENGE_CANCELED %d\r\n", res_code, challenge_id);
        send_to_client(client_sock, response);
    }

    // ========================================================================
    // 3. XỬ LÝ CHEST (RƯƠNG)
    // ========================================================================
    else if (strcmp(type, "CHEST_OPEN") == 0) { // Giữ lệnh CHEST_OPEN theo code của bạn
        int chest_id;
        char answer[128];
        
        // Cú pháp: CHEST_OPEN <ID> <ANSWER>
        if (sscanf(payload, "%d %[^\n]", &chest_id, answer) == 2) {
            
            // Sửa: Dùng &session, bỏ node->session vì node không tồn tại ở đây
            res_code = server_handle_open_chest(&session, chest_id, answer);
            
            // Format trả về tùy theo logic cũ của bạn, ví dụ:
            if (res_code == 127) { // RESP_CHEST_OPEN_OK
                 snprintf(response, sizeof(response), "%d CHEST_OPEN_SUCCESS\r\n", res_code);
            } else {
                 snprintf(response, sizeof(response), "%d CHEST_OPEN_FAIL\r\n", res_code);
            }

        } else {
            snprintf(response, sizeof(response), "301 SYNTAX_ERROR\r\n");
        }
        send_to_client(client_sock, response);
    }


    else {
        // Tùy chọn: Gửi báo lỗi nếu lệnh không tồn tại
        // send_to_client(client_sock, "404 UNKNOWN_COMMAND\r\n");
    }
}