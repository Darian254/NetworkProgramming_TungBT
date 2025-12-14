#ifndef DATA_STRUCTS_H
#define DATA_STRUCTS_H

#define MAX_USERNAME_LEN 20
#define MAX_PASSWORD_LEN 64
#define MAX_TEAM_MEMBERS 3
#define MAX_ITEMS 5

//Account 
typedef struct {
    int user_id;
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
    int coin;
    int item_counts[MAX_ITEMS];
    int is_deleted; //0: Active, 1: Deleted
} Account;

//Session 
typedef struct {
    char session_id[20];
    int user_id;
    int login_time;
} Session;

//Team
typedef struct {
    int team_id;
    char team_name[30];
    int captain_id;
    int member_ids[MAX_TEAM_MEMBERS];
    int member_count;
} Team;


//Request/Invite
typedef struct {
    int request_id;
    int target_user_id;
    int team_id;
    enum {
        REQ_JOIN,
        REQ_INVITE
    } type;

} TeamRequest;

#endif // DATA_STRUCTS_H