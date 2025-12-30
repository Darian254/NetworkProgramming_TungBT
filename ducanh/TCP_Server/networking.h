#ifndef NETWORKING_H
#define NETWORKING_H

// Khai báo hàm mạng
void send_to_client(int socket_fd, const char* message);
void send_error_response(int socket_fd, int error_code, const char* details);
void send_fire_ok(int attacker_socket, int target_id, int damage, int hp, int armor);
void broadcast_fire_event(int attacker_id, int target_id, int damage, int hp, int armor);

#endif // NETWORKING_H