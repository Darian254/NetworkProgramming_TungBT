#pragma once
#include <time.h>

#define USERNAME_MAX 21
#define PASSWORD_MAX 65

typedef struct {
    char username[USERNAME_MAX];
    char password[PASSWORD_MAX];
    int is_locked;
    char created_at[20];
} User;

typedef enum {
    SESSION_IDLE,
    SESSION_IN_TEAM,
    SESSION_IN_MATCH
} SessionStatus;

typedef struct {
    char session_id[64];
    char username[USERNAME_MAX];
    int sockfd;
    time_t login_time;
    SessionStatus status;
} Session;
