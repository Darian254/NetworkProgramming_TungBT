#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include "ship_defs.h"
#include "networking.h"

extern Ship active_ships[100]; 
extern int num_active_ships;

// Gửi dữ liệu
int sendData(int s, char *buff, int size, int flags) {
    int n;
    n = send(s, buff, size, flags);
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

// Gửi 500/3xx Error, hết đạn, không có vũ khí, bắn nhầm đồng đội
void send_error_response(int socket_fd, int error_code, const char* details) {
    char buffer[256];
    sprintf(buffer, "%d %s\n", error_code, details);
    send_to_client(socket_fd, buffer);
}

// Gửi 131 FIRE_OK 
void send_fire_ok(int attacker_socket, int target_id, int damage, int hp, int armor) {
    char buffer[256];
    sprintf(buffer, "%d %d %d %d %d\n", PROTOCOL_FIRE_OK, target_id, damage, hp, armor);
    send_to_client(attacker_socket, buffer);
}

// Gửi 200 FIRE_EVENT 
void broadcast_fire_event(int attacker_id, int target_id, int damage, int hp, int armor) {
    char buffer[256];
    sprintf(buffer, "%d %d %d %d %d %d\n", PROTOCOL_FIRE_EVENT, attacker_id, target_id, damage, hp, armor);
    
    for (int i = 0; i < num_active_ships; i++) {
        // Chỉ gửi đến các client đang kết nối hợp lệ
        send_to_client(active_ships[i].socket_fd, buffer);
    }
}

// void send_fire_ok(int attacker_socket, int target_id, int damage, int hp, int armor) {
//     printf(" FIRE_OK: Target %d, HP %d, Armor %d\n", target_id, hp, armor);
// }
// void broadcast_fire_event(int attacker_id, int target_id, int damage, int hp, int armor) {
//     printf(" EVENT 200: Attacker %d hit %d. Dam %d.\n", attacker_id, target_id, damage);
// }