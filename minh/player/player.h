#ifndef PLAYER_H
#define PLAYER_H

#include <stdarg.h>

#define MAX_PLAYERS 128

typedef struct {
    char username[32];
    int fd;
    int team_id;
    int is_captain;
    int hp;
    int coin;
} Player;

extern Player players[MAX_PLAYERS];
extern int player_count;

Player* find_player_by_name(const char *name);
Player* find_player_by_fd(int fd);
Player* create_player(const char *username, int fd);
void handle_login(int fd, const char *username);

int player_in_team(int fd);
Player* get_captain_of_team(int team_id);

#endif
