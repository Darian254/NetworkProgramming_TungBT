#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H
#include "ship_defs.h"
// Khai báo hàm logic Tấn công
void process_fire_request(int attacker_id, int target_id, int weapon_id);
int calculate_and_update_damage(Ship* attacker, Ship* target, int weapon_id);
#endif // GAME_LOGIC_H