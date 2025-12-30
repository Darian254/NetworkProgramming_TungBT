/**
 * ============================================================================
 * DATABASE SCHEMA CONFIGURATION
 * ============================================================================
 * 
 * This file defines all database table STRUCTS used in the system.
 * Database: File-based (.txt) + In-memory storage
 * 
 * CORE PRINCIPLES:
 * - Coin is persistent (stored in USERS, survives across matches)
 * - No persistent inventory (items bought in-match only)
 * - Ship state is temporary (deleted when match ends)
 * 
 * MODULE STRUCTURE:
 *   - users.h/c         : User struct & HashTable operations
 *   - users_io.h/c      : User file I/O (users.txt)
 *   - session.h/c       : Session manager with mutex, linked list
 *   - hash.h/c          : Hash function (djb2)
 *   - db_schema.h       : Struct definitions for other tables (this file)
 *   - db.c              : Operations for teams, matches, ships, etc.
 * ============================================================================
 */

#ifndef DB_SCHEMA_H
#define DB_SCHEMA_H

#include <time.h>
#include <stdbool.h>
#include "users.h"
#include "config.h"    

/* ============================================================================
 * CONSTANTS & LIMITS
 * ============================================================================ */
#define MAX_TEAMS           50
#define MAX_TEAM_MEMBERS    3
#define MAX_JOIN_REQUESTS   100
#define MAX_TEAM_INVITES    100
#define MAX_CHALLENGES      50
#define MAX_MATCHES         50
#define MAX_SHIPS           300  

#define TEAM_NAME_LEN       32

#define MAP_WIDTH  1000
#define MAP_HEIGHT 1000

/* ============================================================================
 * ENUMS
 * ============================================================================ */

// Team status
typedef enum {
    TEAM_ACTIVE = 0,
    TEAM_DELETED = 1
} TeamStatus;

// Team member role
typedef enum {
    ROLE_CREATOR = 0,
    ROLE_MEMBER = 1
} TeamRole;

// Request/Invite status
typedef enum {
    STATUS_PENDING = 0,
    STATUS_ACCEPTED = 1,
    STATUS_DECLINED = 2,
    STATUS_CANCELED = 3      // Only for challenges
} RequestStatus;

// Match status
typedef enum {
    MATCH_PENDING = 0,
    MATCH_RUNNING = 1,
    MATCH_FINISHED = 2,
    MATCH_CANCELED = 3
} MatchStatus;

// Armor type
typedef enum {
    ARMOR_NONE = 0,
    ARMOR_BASIC = 1,         // price 1000, armor 500
    ARMOR_ENHANCED = 2       // price 2000, armor 1500
} ArmorType;

// Weapon Type
typedef enum {
    WEAPON_CANNON = 0,
    WEAPON_LASER = 1,
    WEAPON_MISSILE = 2
} WeaponType;

// Item type
typedef enum {
    ITEM_WEAPON = 0,
    ITEM_ARMOR = 1
} ItemType;


typedef enum {
    CHALLENGE_PENDING,
    CHALLENGE_ACCEPTED,
    CHALLENGE_DECLINED,
    CHALLENGE_CANCELED
} ChallengeStatus;


typedef enum {
    CHEST_BRONZE = 0, // 100 coin
    CHEST_SILVER = 1, // 500 coin
    CHEST_GOLD = 2    // 2000 coin 
} ChestType;

typedef struct {
    int chest_id;
    int match_id;
    ChestType type;
    int position_x;   
    int position_y;
    bool is_collected;
    time_t spawn_time;
} TreasureChest;

/* ============================================================================
 * TABLE: USERS
 * Description: Represents a user account
 * File: users.txt
 * Format: <username> <password_hash> <status> <coin> <created_at> <updated_at>
 * ============================================================================ */
// Defined in users.h as User struct

/* ============================================================================
 * TABLE: SESSIONS
 * Description: Tracks login / active sessions
 * NOTE: Use session.h for session management (already has mutex, linked list)
 * ============================================================================ */
// Sessions are managed by session.h - no need to redefine here

/* ============================================================================
 * TABLE: TEAMS
 * Description: Represents a team (max 3 players)
 * File: teams.txt
 * ============================================================================ */
typedef struct {
    int         team_id;                        // PK, auto-increment
    char        name[TEAM_NAME_LEN];
    char        creator_username[MAX_USERNAME];                    
    int         creator_id;                     // FK -> USERS.user_id
    int         member_limit;                   // Default 3
    TeamStatus  status;                         // active | deleted
    time_t      created_at;
} Team;

/* ============================================================================
 * TABLE: TEAM_MEMBERS
 * Description: Maps users to teams
 * File: team_members.txt
 * PK: (team_id, user_id)
 * ============================================================================ */
typedef struct {
    int         team_id;                        // FK -> TEAMS.team_id
    int         user_id;                        // FK -> USERS.user_id (hash of username)
    char        username[MAX_USERNAME];         // Username for easy lookup
    TeamRole    role;                           // creator | member
    time_t      joined_at;
} TeamMember;

/* ============================================================================
 * TABLE: JOIN_REQUESTS
 * Description: Users request to join teams
 * File: join_requests.txt
 * ============================================================================ */
typedef struct {
    int             request_id;                 // PK, auto-increment
    int             team_id;                    // FK -> TEAMS.team_id
    int             user_id;                    // FK -> USERS.user_id
    time_t          requested_at;
    RequestStatus   status;                     // pending | accepted | declined
} JoinRequest;

/* ============================================================================
 * TABLE: TEAM_INVITES
 * Description: Team invites users
 * File: team_invites.txt
 * ============================================================================ */
typedef struct {
    int             invite_id;                  // PK, auto-increment
    int             team_id;                    // FK -> TEAMS.team_id
    int             inviter_id;                 // FK -> USERS.user_id
    int             invitee_id;                 // FK -> USERS.user_id
    time_t          invited_at;
    RequestStatus   status;                     // pending | accepted | declined
} TeamInvite;

/* ============================================================================
 * TABLE: CHALLENGES
 * Description: One team challenges another
 * File: challenges.txt
 * ============================================================================ */
typedef struct {
    int             challenge_id;               // PK, auto-increment
    int             sender_team_id;             // FK -> TEAMS.team_id
    int             target_team_id;             // FK -> TEAMS.team_id
    time_t          created_at;
    RequestStatus   status;                     // pending | accepted | declined | canceled
    time_t          responded_at;
} Challenge;

/* ============================================================================
 * TABLE: MATCHES
 * Description: Represents an actual battle between two teams
 * File: matches.txt
 * ============================================================================ */
typedef struct {
    int             match_id;                   // PK, auto-increment
    int             team1_id;                   // FK -> TEAMS.team_id
    int             team2_id;                   // FK -> TEAMS.team_id
    time_t          started_at;
    int             duration;                   // In seconds
    MatchStatus     status;                     // pending | running | finished | canceled
    int             winner_team_id;             // Nullable, FK -> TEAMS.team_id, -1 = no winner
} Match;

/* ============================================================================
 * TABLE: SHIPS (TEMPORARY - IN-MATCH ONLY)
 * Description: Tracks ship state per player per match
 * Storage: Memory only (deleted when match ends)
 * ============================================================================ */
typedef struct {
    int         match_id;                       // FK -> MATCHES.match_id
    int         player_id;                      // FK -> USERS.user_id
    int         hp;                             // Current health points
    
    // Armor slots (max 2)
    ArmorType   armor_slot_1_type;              // none | basic | enhanced
    int         armor_slot_1_value;             // 0 | 500 | 1500
    ArmorType   armor_slot_2_type;
    int         armor_slot_2_value;
    
    // Weapons
    int         cannon_ammo;                    // 30mm ammo count
    int         laser_count;                    // Max 4
    int         missile_count;                  // Missile count
} Ship;


/* ==========================================================================
 * STRUCT: RepairResult
 * Description: Output structure for ship repair operations
 * Used to return new HP and coin after a successful repair
 * ==========================================================================
 */
typedef struct {
    int hp;     /**< New HP after repair */
    int coin;   /**< New coin balance after repair */
} RepairResult;

/* ============================================================================
 * ITEMS CONFIG (STATIC - READ ONLY)
 * Description: Defines purchasable items
 * These are constants, not stored in file
 * ============================================================================ */

// Armor config
#define ARMOR_BASIC_PRICE       1000
#define ARMOR_BASIC_VALUE       500
#define ARMOR_ENHANCED_PRICE    2000
#define ARMOR_ENHANCED_VALUE    1500

// Weapon config
#define CANNON_AMMO_PRICE       100     // Per 50 ammo
#define CANNON_AMMO_PER_PURCHASE 50
#define CANNON_DAMAGE           10      // Per ammo

#define LASER_PRICE             1000
#define LASER_DAMAGE            100
#define LASER_MAX               4

#define MISSILE_PRICE           2000
#define MISSILE_DAMAGE          800
//TODO
#define MISSILE_MAX             4

// Ship defaults
#define SHIP_DEFAULT_HP         1000
#define SHIP_DEFAULT_ARMOR      0
#define SHIP_DEFAULT_CANNON     0
#define SHIP_DEFAULT_LASER      0
#define SHIP_DEFAULT_MISSILE    0

// User defaults
#define USER_DEFAULT_COIN       500

/* ============================================================================
 * FILE PATHS
 * ============================================================================ */
#define FILE_USERS          "users.txt"         // User accounts & game data
#define FILE_TEAMS          "teams.txt"
#define FILE_TEAM_MEMBERS   "team_members.txt"
#define FILE_JOIN_REQUESTS  "join_requests.txt"
#define FILE_TEAM_INVITES   "team_invites.txt"
#define FILE_CHALLENGES     "challenges.txt"
#define FILE_MATCHES        "matches.txt"

/* ============================================================================
 * FUNCTION DECLARATIONS (implemented in db.c)
 * 
 * NOTE: User operations are in users.h/c
 * ============================================================================ */

/* Team operations */
Team* find_team_by_id(int team_id);
Team* find_team_by_name(const char *name);
Team* create_team(const char *name, const char *creator_username);
int get_team_member_count(int team_id);
bool delete_team(int team_id);

/* Lookup helpers (username -> team -> match) */
int find_team_id_by_username(const char *username);
int find_running_match_by_team(int team_id);
int find_current_match_by_username(const char *username);
int find_available_opponent_team(int user_team_id);
/* Match operations */
Match* find_match_by_id(int match_id);
Match* create_match(int team1_id, int team2_id);
void end_match(int match_id, int winner_team_id);

/* Ship operations (in-match only) */
Ship* find_ship(int match_id, const char *username);
Ship* create_ship(int match_id, const char *username);
void delete_ships_by_match(int match_id);
int ship_take_damage(Ship *s, int damage);
ResponseCode ship_buy_armor(UserTable *user_table, Ship *ship, const char *username, ArmorType type);
ResponseCode ship_buy_weapon(UserTable *user_table, Ship *ship, const char *username, WeaponType type);

/* Item helpers */
int get_armor_price(ArmorType type);
int get_armor_value(ArmorType type);

#endif // DB_SCHEMA_H
