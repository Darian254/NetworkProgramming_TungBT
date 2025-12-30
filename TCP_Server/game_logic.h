#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include "ship_defs.h"
#include "session.h" // Cần cho ServerSession

// Định nghĩa struct kết quả bắn (để router sử dụng)
typedef struct {
    int attacker_id;
    int target_id;
    int damage_dealt;
    int target_remaining_hp;
    int target_remaining_armor;
} FireResult;

// Nguyên mẫu hàm xử lý logic
void process_fire_request(int attacker_id, int target_id, int weapon_id);
int calculate_and_update_damage(GameShip* attacker, GameShip* target, int weapon_id);

// Hàm mới cho router.c gọi
int server_handle_fire(ServerSession *session, int target_id, int weapon_id, FireResult *result);

// Các hàm tìm kiếm (nếu chưa có trong db_schema.h thì khai báo tạm ở đây)
int find_current_match_by_username(char *username);
GameShip* find_ship_by_id(int ship_id);  // From ship_defs.c

#endif // GAME_LOGIC_H