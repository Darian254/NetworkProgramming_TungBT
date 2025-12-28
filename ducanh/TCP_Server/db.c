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
#include <pthread.h>

/* ============================================================================
 * MUTEX FOR THREAD SAFETY
 * ============================================================================ */
pthread_mutex_t team_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t ship_mutex = PTHREAD_MUTEX_INITIALIZER;

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

static Challenge    challenges[MAX_CHALLENGES];
static int          challenge_count = 0;

static Match        matches[MAX_MATCHES];
static int          match_count = 0;

static Ship         ships[MAX_SHIPS];
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
 * LOOKUP HELPERS
 * ============================================================================ */

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

int find_running_match_by_team(int team_id) {
    if (team_id <= 0) return -1;
    
    pthread_mutex_lock(&team_mutex);
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
 * TEAM OPERATIONS
 * ============================================================================ */

Team* find_team_by_id(int team_id) {
    if (team_id <= 0) return NULL;
    
    pthread_mutex_lock(&team_mutex);
    for (int i = 0; i < team_count; i++) {
        if (teams[i].team_id == team_id && teams[i].status == TEAM_ACTIVE) {
            pthread_mutex_unlock(&team_mutex);
            return &teams[i];
        }
    }
    pthread_mutex_unlock(&team_mutex);
    return NULL;
}

Team* find_team_by_name(const char *name) {
    if (!name) return NULL;
    
    pthread_mutex_lock(&team_mutex);
    for (int i = 0; i < team_count; i++) {
        if (strcmp(teams[i].name, name) == 0 && teams[i].status == TEAM_ACTIVE) {
            pthread_mutex_unlock(&team_mutex);
            return &teams[i];
        }
    }
    pthread_mutex_unlock(&team_mutex);
    return NULL;
}

Team* create_team(const char *name, const char *creator_username) {
    if (!name || !creator_username) return NULL;
    
    pthread_mutex_lock(&team_mutex);
    
    // Check capacity
    if (team_count >= MAX_TEAMS) {
        pthread_mutex_unlock(&team_mutex);
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
    
    // Add creator as first member
    if (team_member_count < MAX_TEAMS * MAX_TEAM_MEMBERS) {
        TeamMember *member = &team_members[team_member_count++];
        member->team_id = team->team_id;
        strncpy(member->username, creator_username, MAX_USERNAME - 1);
        member->username[MAX_USERNAME - 1] = '\0';
        member->role = ROLE_CREATOR;
        member->joined_at = time(NULL);
    }
    
    pthread_mutex_unlock(&team_mutex);
    return team;
}

int get_team_member_count(int team_id) {
    if (team_id <= 0) return 0;
    
    pthread_mutex_lock(&team_mutex);
    int count = 0;
    for (int i = 0; i < team_member_count; i++) {
        if (team_members[i].team_id == team_id) {
            count++;
        }
    }
    pthread_mutex_unlock(&team_mutex);
    return count;
}

bool delete_team(int team_id) {
    if (team_id <= 0) return false;
    
    pthread_mutex_lock(&team_mutex);
    
    // Find and mark team as deleted
    Team *team = NULL;
    for (int i = 0; i < team_count; i++) {
        if (teams[i].team_id == team_id) {
            teams[i].status = TEAM_DELETED;
            team = &teams[i];
            break;
        }
    }
    
    if (!team) {
        pthread_mutex_unlock(&team_mutex);
        return false;
    }
    
    // Remove all team members
    for (int i = team_member_count - 1; i >= 0; i--) {
        if (team_members[i].team_id == team_id) {
            // Shift remaining members
            for (int j = i; j < team_member_count - 1; j++) {
                team_members[j] = team_members[j + 1];
            }
            team_member_count--;
        }
    }
    
    // Remove all join requests for this team
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
    
    pthread_mutex_unlock(&team_mutex);
    return true;
}

/* ============================================================================
 * CLEAR REQUESTS
 * ============================================================================ */
void clear_user_requests(const char *username) {
    if (!username) return;

    int user_id = hashFunc(username);

    pthread_mutex_lock(&team_mutex);

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

    pthread_mutex_unlock(&team_mutex);
    printf("[INFO] Cleared join requests for user: %s\n", username);
}

/* ============================================================================
 * SHIP OPERATIONS
 * ============================================================================ */

Ship* find_ship(int match_id, const char *username) {
    if (match_id <= 0 || !username) return NULL;
    
    pthread_mutex_lock(&ship_mutex);
    int player_id = hashFunc(username);
    
    for (int i = 0; i < ship_count; i++) {
        if (ships[i].match_id == match_id && ships[i].player_id == player_id) {
            pthread_mutex_unlock(&ship_mutex);
            return &ships[i];
        }
    }
    pthread_mutex_unlock(&ship_mutex);
    return NULL;
}

Ship* create_ship(int match_id, const char *username) {
    if (match_id <= 0 || !username) return NULL;
    
    pthread_mutex_lock(&ship_mutex);
    
    // Check capacity
    if (ship_count >= MAX_SHIPS) {
        pthread_mutex_unlock(&ship_mutex);
        return NULL;
    }
    
    // Check if ship already exists
    int player_id = hashFunc(username);
    for (int i = 0; i < ship_count; i++) {
        if (ships[i].match_id == match_id && ships[i].player_id == player_id) {
            pthread_mutex_unlock(&ship_mutex);
            return &ships[i];
        }
    }
    
    // Create new ship
    Ship *ship = &ships[ship_count++];
    ship->match_id = match_id;
    ship->player_id = player_id;
    ship->hp = SHIP_DEFAULT_HP;
    ship->armor_slot_1_type = ARMOR_NONE;
    ship->armor_slot_1_value = 0;
    ship->armor_slot_2_type = ARMOR_NONE;
    ship->armor_slot_2_value = 0;
    ship->cannon_ammo = SHIP_DEFAULT_CANNON;
    ship->laser_count = SHIP_DEFAULT_LASER;
    ship->missile_count = SHIP_DEFAULT_MISSILE;
    
    pthread_mutex_unlock(&ship_mutex);
    return ship;
}

void delete_ships_by_match(int match_id) {
    if (match_id <= 0) return;
    
    pthread_mutex_lock(&ship_mutex);
    for (int i = ship_count - 1; i >= 0; i--) {
        if (ships[i].match_id == match_id) {
            // Shift remaining ships
            for (int j = i; j < ship_count - 1; j++) {
                ships[j] = ships[j + 1];
            }
            ship_count--;
        }
    }
    pthread_mutex_unlock(&ship_mutex);
}

int ship_take_damage(Ship *s, int damage) {
    if (!s || damage < 0) return -1;
    
    pthread_mutex_lock(&ship_mutex);
    
    int remaining_damage = damage;
    
    // Apply to slot 1 armor first
    if (s->armor_slot_1_value > 0) {
        if (s->armor_slot_1_value >= remaining_damage) {
            s->armor_slot_1_value -= remaining_damage;
            remaining_damage = 0;
        } else {
            remaining_damage -= s->armor_slot_1_value;
            s->armor_slot_1_value = 0;
            s->armor_slot_1_type = ARMOR_NONE;
        }
    }
    
    // Apply to slot 2 armor
    if (remaining_damage > 0 && s->armor_slot_2_value > 0) {
        if (s->armor_slot_2_value >= remaining_damage) {
            s->armor_slot_2_value -= remaining_damage;
            remaining_damage = 0;
        } else {
            remaining_damage -= s->armor_slot_2_value;
            s->armor_slot_2_value = 0;
            s->armor_slot_2_type = ARMOR_NONE;
        }
    }
    
    // Apply to HP
    if (remaining_damage > 0) {
        s->hp -= remaining_damage;
        if (s->hp < 0) s->hp = 0;
    }
    
    int result = s->hp;
    pthread_mutex_unlock(&ship_mutex);
    return result;
}

ResponseCode ship_buy_armor(UserTable *user_table, Ship *ship, const char *username, ArmorType type) {
    if (!user_table || !ship || !username) return RESP_INTERNAL_ERROR;
    
    // Check armor type validity
    int price = get_armor_price(type);
    int value = get_armor_value(type);
    if (price == 0 || value == 0) {
        return RESP_ARMOR_NOT_FOUND;
    }
    
    // Check coin balance
    User *u = findUser(user_table, username);
    if (!u) return RESP_INTERNAL_ERROR;
    
    if (u->coin < price) {
        return RESP_NOT_ENOUGH_COIN;
    }
    
    // Check armor slots
    pthread_mutex_lock(&ship_mutex);
    
    int target_slot = 0;
    if (ship->armor_slot_1_type == ARMOR_NONE) {
        target_slot = 1;
    } else if (ship->armor_slot_2_type == ARMOR_NONE) {
        target_slot = 2;
    }
    
    if (target_slot == 0) {
        pthread_mutex_unlock(&ship_mutex);
        return RESP_ARMOR_SLOT_FULL;
    }
    
    pthread_mutex_unlock(&ship_mutex);
    
    // Deduct coin
    int coin_result = updateUserCoin(user_table, username, -price);
    if (coin_result != 0) {
        return RESP_DATABASE_ERROR;
    }
    
    // Apply armor
    pthread_mutex_lock(&ship_mutex);
    
    if (target_slot == 1) {
        ship->armor_slot_1_type = type;
        ship->armor_slot_1_value = value;
    } else {
        ship->armor_slot_2_type = type;
        ship->armor_slot_2_value = value;
    }
    
    pthread_mutex_unlock(&ship_mutex);
    
    return RESP_BUY_ITEM_OK;
}

/* ============================================================================
 * MATCH OPERATIONS
 * ============================================================================ */

Match* find_match_by_id(int match_id) {
    if (match_id <= 0) return NULL;
    
    pthread_mutex_lock(&team_mutex);
    for (int i = 0; i < match_count; i++) {
        if (matches[i].match_id == match_id) {
            pthread_mutex_unlock(&team_mutex);
            return &matches[i];
        }
    }
    pthread_mutex_unlock(&team_mutex);
    return NULL;
}

Match* create_match(int team1_id, int team2_id) {
    if (team1_id <= 0 || team2_id <= 0) return NULL;
    
    pthread_mutex_lock(&team_mutex);
    
    // Check capacity
    if (match_count >= MAX_MATCHES) {
        pthread_mutex_unlock(&team_mutex);
        return NULL;
    }
    
    // Create match
    Match *match = &matches[match_count++];
    match->match_id = next_match_id++;
    match->team1_id = team1_id;
    match->team2_id = team2_id;
    match->started_at = time(NULL);
    match->duration = 0;
    match->status = MATCH_RUNNING;
    match->winner_team_id = -1;
    
    pthread_mutex_unlock(&team_mutex);
    return match;
}

void end_match(int match_id, int winner_team_id) {
    if (match_id <= 0) return;
    
    pthread_mutex_lock(&team_mutex);
    
    for (int i = 0; i < match_count; i++) {
        if (matches[i].match_id == match_id) {
            matches[i].status = MATCH_FINISHED;
            matches[i].winner_team_id = winner_team_id;
            matches[i].duration = (int)(time(NULL) - matches[i].started_at);
            break;
        }
    }
    
    pthread_mutex_unlock(&team_mutex);
    
    // Delete all ships for this match
    delete_ships_by_match(match_id);
}