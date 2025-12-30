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

#include "db_schema.h"
#include "hash.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

/* ============================================================================
 * MUTEX FOR THREAD SAFETY
 * ============================================================================ */
static pthread_mutex_t team_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t ship_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_user_table_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ============================================================================
 * IN-MEMORY STORAGE (Arrays for game tables)
 * Note: User data is handled by users.h UserTable
 * ============================================================================ */
static Team         teams[MAX_TEAMS];
static int          team_count = 0;

static TeamMember   team_members[MAX_TEAMS * MAX_TEAM_MEMBERS];
static int          team_member_count = 0;

static JoinRequest  join_requests[MAX_JOIN_REQUESTS];
static int          join_request_count = 0;

static TeamInvite   team_invites[MAX_TEAM_INVITES];
static int          team_invite_count = 0;

static Challenge    challenges[MAX_CHALLENGES];
static int          challenge_count = 0;

static Match        matches[MAX_MATCHES];
static int          match_count = 0;

static Ship         ships[MAX_SHIPS];  // Temporary, in-memory only
static int          ship_count = 0;

/* ============================================================================
 * AUTO-INCREMENT IDs
 * ============================================================================ */
static int next_team_id = 1;
static int next_request_id = 1;
static int next_invite_id = 1;
static int next_challenge_id = 1;
static int next_match_id = 1;

/* ============================================================================
 * LOOKUP HELPERS: Find match/team from username
 * ============================================================================ */

/**
 * Find team_id of a player by username
 * @return team_id or -1 if not in any team
 */
int find_team_id_by_username(const char *username) {
    if (!username) return -1;
    
    pthread_mutex_lock(&team_mutex);
    for (int i = 0; i < team_member_count; i++) {
        if (strcmp(team_members[i].username, username) == 0) {
            int team_id = team_members[i].team_id;
            pthread_mutex_unlock(&team_mutex);
            return team_id;
        }
    }
    pthread_mutex_unlock(&team_mutex);
    return -1;
}

/**
 * Find current running match_id of a team
 * @return match_id or -1 if team not in any running match
 */
int find_running_match_by_team(int team_id) {
    if (team_id <= 0) return -1;
    
    pthread_mutex_lock(&team_mutex);  // Using team_mutex for matches too
    for (int i = 0; i < match_count; i++) {
        if (matches[i].status == MATCH_RUNNING &&
            (matches[i].team1_id == team_id || matches[i].team2_id == team_id)) {
            int match_id = matches[i].match_id;
            pthread_mutex_unlock(&team_mutex);
            return match_id;
        }
    }
    pthread_mutex_unlock(&team_mutex);
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

/* ============================================================================
 * TEAM OPERATIONS (with mutex)
 * ============================================================================ */
Team* find_team_by_id(int team_id) {
}

Team* find_team_by_name(const char *name) {
}

Team* create_team(const char *name, const char *creator_username) {
}

int get_team_member_count(int team_id) {
}

bool delete_team(int team_id) {
}

/* ============================================================================
 * SHIP OPERATIONS (IN-MATCH ONLY, with mutex)
 * ============================================================================ */
Ship* find_ship(int match_id, const char *username) {
}

Ship* create_ship(int match_id, const char *username) {
}

void delete_ships_by_match(int match_id) {
}

/**
 * Apply damage to ship (with mutex)
 * Damage goes to armor first, then HP
 * Returns remaining HP
 */
int ship_take_damage(Ship *s, int damage) {
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
    
    // 3.3: Kiểm tra slot giáp (tối đa 2 lớp) - lock with mutex bc if 2 threads buy at a time, both may see 1 slot free, causing overflow
    pthread_mutex_lock(&ship_mutex);
    
    int target_slot = 0;  // 0 = không có slot trống
    if (ship->armor_slot_1_type == ARMOR_NONE) { 
        target_slot = 1;
    } else if (ship->armor_slot_2_type == ARMOR_NONE) {
        target_slot = 2;
    }
    
    if (target_slot == 0) {
        pthread_mutex_unlock(&ship_mutex);
        return RESP_ARMOR_SLOT_FULL;  // 522
    }

    pthread_mutex_unlock(&ship_mutex);  
    
    /* ========== XỬ LÝ LOGIC ========== */
    // 4.1: Trừ coin 
    // updateUserCoin already got its own mutex lock inside
    int coin_result = updateUserCoin(user_table, username, -price);
    if (coin_result != 0) {
        return RESP_DATABASE_ERROR;  // 501 - lỗi khi trừ coin
    }
    
    // 4.2: Gắn giáp vào tàu
    pthread_mutex_lock(&ship_mutex);
    
    if (target_slot == 1) {
        ship->armor_slot_1_type = type;
        ship->armor_slot_1_value = value;
    } else {
        ship->armor_slot_2_type = type;
        ship->armor_slot_2_value = value;
    }
    
    pthread_mutex_unlock(&ship_mutex);
    
    // Bước 5 (log) và Bước 6 (phản hồi) sẽ do handler xử lý
    return RESP_BUY_ITEM_OK;  // 334
}

/* ============================================================================
 * MATCH OPERATIONS
 * ============================================================================ */
Match* find_match_by_id(int match_id) {
}

Match* create_match(int team1_id, int team2_id) {
}

void end_match(int match_id, int winner_team_id) {
}
