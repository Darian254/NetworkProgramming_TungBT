/**
 * ============================================================================
 * DATABASE OPERATIONS
 * ============================================================================
 * 
 * Implementation of database operations for file-based storage.
 * 
 * MODULE STRUCTURE:
 *   - users.h/c         : User struct & HashTable operations
 *   - users_io.h/c      : User file I/O (users.txt)
 *   - session.h/c       : Session manager with mutex
 *   - hash.h/c          : Hash function (djb2)
 * 
 * This file handles:
 *   - Teams, team members, join requests, invites
 *   - Matches, challenges
 *   - Ships (in-match only, temporary)
 * ============================================================================
 */

/**
 * ============================================================================
 * DATABASE OPERATIONS - COMPLETE IMPLEMENTATION
 * ============================================================================ */


#include "db_schema.h"
#include "hash.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * IN-MEMORY STORAGE
 * ============================================================================ */
Team         teams[MAX_TEAMS];
static int          team_count = 0;

TeamMember   team_members[MAX_TEAMS * MAX_TEAM_MEMBERS];
int          team_member_count = 0;

JoinRequest  join_requests[MAX_JOIN_REQUESTS];
int          join_request_count = 0;

TeamInvite   team_invites[MAX_TEAM_INVITES];
int          team_invite_count = 0;

Challenge    challenges[MAX_CHALLENGES];
int          challenge_count = 0;

Match        matches[MAX_MATCHES];
int          match_count = 0;

Ship         ships[MAX_SHIPS];  // Temporary, in-memory only
int          ship_count = 0;

Ship active_ships[100];
int num_active_ships = 0;
// TODO: UserTable from users.h/c
UserTable *g_user_table = NULL;

TreasureChest active_chests[MAX_TEAMS]; 
ChestPuzzle puzzles[] = {
    {"1 + 1 = ?", "2"},             // Đồng
    {"Thủ đô của Việt Nam?", "Ha Noi"}, // Bạc
    {"Giao thức tầng giao vận nào tin cậy?", "TCP"} // Vàng
};

WeaponTemplate weapon_templates[] = {
    {1, "Pháo",    10, 1000, 50},
    {2, "Laze",   100, 1000, 10},
    {3, "Tên lửa", 800, 2000, 1}
};

 

/* ============================================================================
 * AUTO-INCREMENT IDs
 * ============================================================================ */
int next_team_id = 1;
int next_request_id = 1;
int next_invite_id = 1;
int next_challenge_id = 1;
int next_match_id = 1;
/* ============================================================================

 * LOOKUP HELPERS: Find match/team from username
 * ============================================================================ */

/**
 * Find team_id of a player by username
 * @return team_id or -1 if not in any team
 */
int find_team_id_by_username(const char *username) {
    if (!username) return -1;
    
    for (int i = 0; i < team_member_count; i++) {
        if (strcmp(team_members[i].username, username) == 0) {
            return team_members[i].team_id;
        }
    }
    return -1;
}

/**
 * Find current running match_id of a team
 * @return match_id or -1 if team not in any running match
 */
int find_running_match_by_team(int team_id) {
    if (team_id <= 0) return -1;
    
    for (int i = 0; i < match_count; i++) {
        if (matches[i].status == MATCH_RUNNING &&
            (matches[i].team1_id == team_id || matches[i].team2_id == team_id)) {
            return matches[i].match_id;
        }
    }
    return -1;
}

/**
 * Find current match_id of a player (combines both lookups)
 * Flow: username -> team_id -> match_id
 * @return match_id or -1 if not in any running match
 */

int find_current_match_by_username(const char *username) {
    int team_id = find_team_id_by_username(username);
    if (team_id <= 0) return -1;
    return find_running_match_by_team(team_id);
}

/**
 * Find an available opponent team for matchmaking
 * Returns first team that is not the user's team and not currently in a match
 * @param user_team_id The user's team ID to exclude
 * @return opponent team_id or -1 if no available teams
 */
int find_available_opponent_team(int user_team_id) {
    if (user_team_id <= 0) return -1;
    
    for (int i = 0; i < team_count; i++) {
        int candidate_team_id = teams[i].team_id;
        
        // Skip user's own team
        if (candidate_team_id == user_team_id) continue;
        
        // Skip teams with status != TEAM_ACTIVE
        if (teams[i].status != TEAM_ACTIVE) continue;
        
        // Check if this team is already in a running match
        if (find_running_match_by_team(candidate_team_id) >= 0) continue;
        
        // Found an available team!
        return candidate_team_id;
    }
    
    // No available teams found
    return -1;
}

Ship* find_ship(int match_id, const char *username) {  
    if (!match_id || !username) return NULL;

    for (int i = 0; i < ship_count; i++) {
        if (ships[i].match_id == match_id && strcmp(ships[i].player_username, username) == 0) return &ships[i];
    }
    return NULL;
}


/* ============================================================================
 * HELPER: Get item info by type
 * ============================================================================ */
int get_armor_price(ArmorType type) {
    switch (type) {
        case ARMOR_BASIC:    return ARMOR_BASIC_PRICE;
        case ARMOR_ENHANCED: return ARMOR_ENHANCED_PRICE;
        default:             return 0;
    }
}

int get_armor_value(ArmorType type) {
    switch (type) {
        case ARMOR_BASIC:    return ARMOR_BASIC_VALUE;
        case ARMOR_ENHANCED: return ARMOR_ENHANCED_VALUE;
        default:             return 0;
    }
}

//Tìm tàu theo id
Ship* find_ship_by_id(int target_id) {
    // Duyệt mảng ships chính 
    for (int i = 0; i < ship_count; i++) {
        if (ships[i].player_id == target_id) {
            return &ships[i];
        }
    }
    return NULL;
}


// //Lấy template vũ khí
// WeaponTemplate* get_weapon_template(int weapon_id) {
//     for (int i = 0; i < 3; i++) {
//         if (weapon_templates[i].weapon_id == weapon_id) {
//             return &weapon_templates[i];
//         }
//     }
//     return NULL;
// }


void update_ship_state(Ship* ship) {
    if (ship->hp == 0) {
        printf("[DEBUG] Tàu %d đã bị phá hủy!\n", ship->player_id);
    }
}


/* ============================================================================
 * TEAM OPERATIONS
 * ============================================================================ */

Team* find_team_by_id(int team_id) {
    if (team_id <= 0) return NULL;
    
    for (int i = 0; i < team_count; i++) {
        if (teams[i].team_id == team_id && teams[i].status == TEAM_ACTIVE) {
            return &teams[i];
        }
    }
    return NULL;
}

Team* find_team_by_name(const char *name) {
    if (!name) return NULL;
    for (int i = 0; i < team_count; i++) {
        if (strcmp(teams[i].name, name) == 0 && teams[i].status == TEAM_ACTIVE) {
            return &teams[i];
        }
    }
    return NULL;
}

Team* create_team(const char *name, const char *creator_username) {
    if (!name || !creator_username) return NULL;
    
    // Check capacity
    if (team_count >= MAX_TEAMS) {
        return NULL;
    }
    
    // Create team
    Team *team = &teams[team_count++];
    team->team_id = next_team_id++;
    strncpy(team->name, name, TEAM_NAME_LEN - 1);
    team->name[TEAM_NAME_LEN - 1] = '\0';
    strncpy(team->creator_username, creator_username, MAX_USERNAME - 1);
    team->creator_username[MAX_USERNAME - 1] = '\0';
    team->member_limit = MAX_TEAM_MEMBERS;
    team->status = TEAM_ACTIVE;
    team->created_at = time(NULL);
    team_count++;
    
    // Add creator as team member
    if (team_member_count < MAX_TEAMS * MAX_TEAM_MEMBERS) {
        TeamMember *member = &team_members[team_member_count];
        member->team_id = team->team_id;
        strncpy(member->username, creator_username, MAX_USERNAME - 1);
        member->username[MAX_USERNAME - 1] = '\0';
        member->role = ROLE_CREATOR;
        member->joined_at = time(NULL);
        team_member_count++;
    }
    
    return team;
}

int get_team_member_count(int team_id) {
    if (team_id <= 0) return 0;
    
    int count = 0;
    for (int i = 0; i < team_member_count; i++) {
        if (team_members[i].team_id == team_id) {
            count++;
        }
    }

    return count;
}

bool delete_team(int team_id) {
    if (team_id <= 0) return false;

    Team *team = find_team_by_id(team_id);
    if (!team) return false;
    
    team->status = TEAM_DELETED;
    
    return true;
    
    // Remove all team members
    for (int i = team_member_count - 1; i >= 0; i--) {
        if (team_members[i].team_id == team_id) {
            for (int j = i; j < team_member_count - 1; j++) {
                team_members[j] = team_members[j + 1];
            }
            team_member_count--;
        }
    }
    
    for (int i = join_request_count - 1; i >= 0; i--) {
        if (join_requests[i].team_id == team_id) {
            for (int j = i; j < join_request_count - 1; j++) {
                join_requests[j] = join_requests[j + 1];
            }
            join_request_count--;
        }
    }
     
    // Remove all invites for this team
    for (int i = team_invite_count - 1; i >= 0; i--) {
        if (team_invites[i].team_id == team_id) {
            for (int j = i; j < team_invite_count - 1; j++) {
                team_invites[j] = team_invites[j + 1];
            }
            team_invite_count--;
        }
    }
    
    return true;
}

/* ============================================================================
 * CLEAR REQUESTS
 * ============================================================================ */
void clear_user_requests(const char *username) {
    if (!username) return;

    int user_id = hashFunc(username);

    int i = 0;
    while (i < join_request_count) {
        if (join_requests[i].user_id == user_id) {
            
            for (int j = i; j < join_request_count - 1; j++) {
                join_requests[j] = join_requests[j + 1];
            }
            join_request_count--;
            
        } else {
            i++; 
        }
    }

    printf("[INFO] Cleared join requests for user: %s\n", username);
}

/* ============================================================================
 * SHIP OPERATIONS
 * ============================================================================ */

Ship* create_ship(int match_id, const char *username) {
    if (!username) return NULL;
    if (ship_count >= MAX_SHIPS) return NULL;
    
    Ship *ship = &ships[ship_count];
    ship->match_id = match_id;
    strncpy(ship->player_username, username, MAX_USERNAME - 1);
    ship->player_username[MAX_USERNAME - 1] = '\0';
    ship->hp = SHIP_DEFAULT_HP;
    ship->armor_slot_1_type = ARMOR_NONE;
    ship->armor_slot_1_value = 0;
    ship->armor_slot_2_type = ARMOR_NONE;
    ship->armor_slot_2_value = 0;
    ship->cannon_ammo = SHIP_DEFAULT_CANNON;
    ship->laser_count = SHIP_DEFAULT_LASER;
    ship->missile_count = SHIP_DEFAULT_MISSILE;
    ship_count++;
    return ship;
}

void delete_ships_by_match(int match_id) {
    for (int i = ship_count - 1; i >= 0; i--) {
        if (ships[i].match_id == match_id) {
            for (int j = i; j < ship_count - 1; j++) {
                ships[j] = ships[j + 1];
            }
            ship_count--;
        }
    }
}

// /**
//  * Apply damage to ship
//  * Damage goes to armor first, then HP
//  * Returns remaining HP
//  */
// int ship_take_damage(Ship *s, int damage) {
//     if (!s || damage <= 0) return s ? s->hp : 0;
    
//     int remaining_damage = damage;
    
//     // Apply to armor slot 1 first
//     if (s->armor_slot_1_value > 0) {
//         if (s->armor_slot_1_value >= remaining_damage) {
//             s->armor_slot_1_value -= remaining_damage;
//             return s->hp;
//         } else {
//             remaining_damage -= s->armor_slot_1_value;
//             s->armor_slot_1_value = 0;
//             s->armor_slot_1_type = ARMOR_NONE;
//         }
//     }
    
//     // Apply to armor slot 2
//     if (s->armor_slot_2_value > 0 && remaining_damage > 0) {
//         if (s->armor_slot_2_value >= remaining_damage) {
//             s->armor_slot_2_value -= remaining_damage;
//             return s->hp;
//         } else {
//             remaining_damage -= s->armor_slot_2_value;
//             s->armor_slot_2_value = 0;
//             s->armor_slot_2_type = ARMOR_NONE;
//         }
//     }
    
//     // Apply remaining damage to HP

//     if (remaining_damage > 0) {
//         s->hp -= remaining_damage;
//         if (s->hp < 0) s->hp = 0;
//     }
//     return s->hp;
// }

int get_team_id_by_player_id(int player_id) {
    // Duyệt qua danh sách thành viên để tìm user_id khớp
    for (int i = 0; i < team_member_count; i++) {
        if (team_members[i].user_id == player_id) {
            return team_members[i].team_id;
        }
    }
    return -1; // Không tìm thấy
}
/**
 * Buy armor for ship (with mutex)
 * 
 * @param user_table  Bảng user để trừ coin
 * @param ship        Con tàu cần gắn giáp (đã được tìm sẵn)
 * @param username    Username để trừ coin
 * @param type        Loại giáp muốn mua
 * @return ResponseCode:
 *   - RESP_BUY_ITEM_OK (334): Thành công
 *   - RESP_ARMOR_NOT_FOUND (520): Loại giáp không tồn tại
 *   - RESP_NOT_ENOUGH_COIN (521): Không đủ coin
 *   - RESP_ARMOR_SLOT_FULL (522): Đã có 2 lớp giáp
 *   - RESP_DATABASE_ERROR (501): Lỗi khi trừ coin
 */
ResponseCode ship_buy_armor(UserTable *user_table, Ship *ship, const char *username, ArmorType type) {
    if (!user_table || !ship || !username) return RESP_INTERNAL_ERROR;
    
    // 3.1: Kiểm tra loại giáp tồn tại
    int price = get_armor_price(type);
    int value = get_armor_value(type);
    if (price == 0 || value == 0) {
        return RESP_ARMOR_NOT_FOUND;  // 520
    }
    
    // 3.2: Kiểm tra đủ coin
    User *u = findUser(user_table, username);
    if (!u) return RESP_INTERNAL_ERROR;  // 500 
    
    if (u->coin < price) {
        return RESP_NOT_ENOUGH_COIN;  // 521
    }
    
    // 3.3: Kiểm tra slot giáp (tối đa 2 lớp)
    int target_slot = 0;  // 0 = không có slot trống
    if (ship->armor_slot_1_type == ARMOR_NONE) { 
        target_slot = 1;
    } else if (ship->armor_slot_2_type == ARMOR_NONE) {
        target_slot = 2;
    }
    
    if (target_slot == 0) {
        return RESP_ARMOR_SLOT_FULL;  // 522
    }  
    
    /* ========== XỬ LÝ LOGIC ========== */
    // 4.1: Trừ coin 
    // updateUserCoin already got its own mutex lock inside
    int coin_result = updateUserCoin(user_table, username, -price);
    if (coin_result != 0) {
        return RESP_DATABASE_ERROR;  // 501 - lỗi khi trừ coin
    }
    
    // 4.2: Gắn giáp vào tàu
    if (target_slot == 1) {
        ship->armor_slot_1_type = type;
        ship->armor_slot_1_value = value;
    } else {
        ship->armor_slot_2_type = type;
        ship->armor_slot_2_value = value;
    }
    
    // Bước 5 (log) và Bước 6 (phản hồi) sẽ do handler xử lý
    return RESP_BUY_ITEM_OK;  // 334
}

ResponseCode ship_buy_weapon(UserTable *user_table, Ship *ship, const char *username, WeaponType type) {
    if(!user_table || !ship || !username) return RESP_INTERNAL_ERROR;
    int price = 0;
    switch(type) {
        case WEAPON_CANNON:
            price = CANNON_AMMO_PRICE;
            break;
        case WEAPON_LASER:
            price = LASER_PRICE;
            break;
        case WEAPON_MISSILE:
            price = MISSILE_PRICE;
            break;
        default:
            return RESP_INTERNAL_ERROR;
    }
    User *u = findUser(user_table, username);
    if(!u) return RESP_ACCOUNT_NOT_FOUND;
    if(u->coin < price) {
        return RESP_NOT_ENOUGH_COIN;
    }
    int coin_result = updateUserCoin(user_table, username, -price);
    if(coin_result != 0) {
        return RESP_DATABASE_ERROR;
    }
    switch(type) {
        case WEAPON_CANNON:
            ship->cannon_ammo += CANNON_AMMO_PER_PURCHASE;
            break;
        case WEAPON_LASER:
            if(ship->laser_count >= LASER_MAX) {
                return RESP_BUY_ITEM_FAILED;
            }
            ship->laser_count += 1;
            break;
        case WEAPON_MISSILE:
            if(ship->missile_count >= MISSILE_MAX) {
                return RESP_BUY_ITEM_FAILED;
            }
            ship->missile_count += 1;
            break;
        default:
            return RESP_BUY_ITEM_FAILED;
    }
    return RESP_BUY_ITEM_OK;
}

/* ============================================================================
 * MATCH OPERATIONS
 * ============================================================================ */
Match* find_match_by_id(int match_id) {
    for (int i = 0; i < match_count; i++) {
        if (matches[i].match_id == match_id) {
            return &matches[i];
        }
    }
    return NULL;
}

Match* create_match(int team1_id, int team2_id) {
    if (team1_id <= 0 || team2_id <= 0) return NULL;
    if (team1_id == team2_id) return NULL;  // Can't match same team
    if (match_count >= MAX_MATCHES) return NULL;
    
    // Verify both teams exist
    Team *team1 = find_team_by_id(team1_id);
    Team *team2 = find_team_by_id(team2_id);
    if (!team1 || !team2) return NULL;
    
    // Check if either team is already in a running match
    if (find_running_match_by_team(team1_id) >= 0) return NULL;
    if (find_running_match_by_team(team2_id) >= 0) return NULL;
    
    Match *match = &matches[match_count];
    match->match_id = next_match_id++;
    match->team1_id = team1_id;
    match->team2_id = team2_id;
    match->started_at = time(NULL);
    match->duration = 0;
    match->status = MATCH_RUNNING;
    match->winner_team_id = -1;  // No winner yet
    
    match_count++;
    
    // Create a ship for each player in team1
    for (int i = 0; i < team_member_count; i++) {
        if (team_members[i].team_id == team1_id) {
            create_ship(match->match_id, team_members[i].username);
        }
    }
    // Create a ship for each player in team2
    for (int i = 0; i < team_member_count; i++) {
        if (team_members[i].team_id == team2_id) {
            create_ship(match->match_id, team_members[i].username);
        }
    }
    
    return match;  // Return the new match pointer
}

void end_match(int match_id, int winner_team_id) {
    Match *match = find_match_by_id(match_id);
    if (!match) return;
    
    match->status = MATCH_FINISHED;
    match->winner_team_id = winner_team_id;
    match->duration = (int)(time(NULL) - match->started_at);
    
    // Delete all ships for this match
    delete_ships_by_match(match_id);
}

bool can_end_match(int match_id, int *winner_team_id) {
    Match *match = find_match_by_id(match_id);
    if (!match) return false;

    int team1_alive = 0;
    int team2_alive = 0;

    // Count alive ships per team for this match
    for (int i = 0; i < ship_count; i++) {
        if (ships[i].match_id != match_id) continue;

        // Resolve team of this ship via team_members
        int ship_team_id = -1;
        for (int tm = 0; tm < team_member_count; tm++) {
            if (strcmp(ships[i].player_username, team_members[tm].username) == 0) {
                ship_team_id = team_members[tm].team_id;
                break;
            }
        }

        if (ship_team_id == match->team1_id) {
            if (ships[i].hp > 0) team1_alive++;
        } else if (ship_team_id == match->team2_id) {
            if (ships[i].hp > 0) team2_alive++;
        }
    }

    // Determine if match can be ended and winner
    if (team1_alive == 0 && team2_alive == 0) {
        if (winner_team_id) // Check if pointer is not NULL
            *winner_team_id = -1; // draw
        return true;
    }
    if (team1_alive == 0 && team2_alive >= 0) {
        if (winner_team_id) *winner_team_id = match->team2_id;
        return true;
    }
    if (team2_alive == 0 && team1_alive >= 0) {
        if (winner_team_id) *winner_team_id = match->team1_id;
        return true;
    }

    return false; // Both teams still have alive ships
}

int get_match_result(int match_id) {
    Match *match = find_match_by_id(match_id);
    if (!match) return -2; // Not found
    return match->winner_team_id;
}




// Hàm tạo bản ghi thách đấu
int create_challenge_record(int sender_team_id, int target_team_id) {
    if (challenge_count >= MAX_CHALLENGES) return -1;
    
    Challenge *ch = &challenges[challenge_count];
    ch->challenge_id = next_challenge_id++;
    ch->sender_team_id = sender_team_id;
    ch->target_team_id = target_team_id;
    ch->created_at = time(NULL);
    ch->status = CHALLENGE_PENDING;
    ch->responded_at = 0;
    
    challenge_count++;
    return ch->challenge_id;
}

// Hàm tìm bản ghi thách đấu
Challenge* find_challenge_by_id(int challenge_id) {
    if (challenge_id <= 0) return NULL;
    
    for (int i = 0; i < challenge_count; i++) {
        if (challenges[i].challenge_id == challenge_id) {
            return &challenges[i];
        }
    }
    return NULL;
}
