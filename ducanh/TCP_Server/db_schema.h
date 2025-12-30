#ifndef DB_SCHEMA_H
#define DB_SCHEMA_H

#include <time.h>
#include <stdbool.h>
#define MAX_CHALLENGES 50
#define MAX_TEAMS 50
#define MAP_WIDTH  1000
#define MAP_HEIGHT 1000
// Định nghĩa trạng thái lời mời theo yêu cầu của bạn
typedef enum {
    CHALLENGE_PENDING,
    CHALLENGE_ACCEPTED,
    CHALLENGE_DECLINED,
    CHALLENGE_CANCELED
} ChallengeStatus;

// Cấu trúc bảng CHALLENGES
typedef struct {
    int challenge_id;
    int sender_team_id;
    int target_team_id;
    time_t created_at;
    ChallengeStatus status;
} Challenge;



typedef struct {
    int team_id;
    char creator_username[64];
} Team;

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

#endif