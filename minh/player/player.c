#include "player.h"
#include <string.h>
#include <stdio.h>
#include "../utils/utils.h"
#include "../log/log.h"
#include "../team/team.h"

Player players[MAX_PLAYERS];
int player_count = 0;

Player* find_player_by_name(const char *name) {
    for (int i = 0; i < player_count; i++)
        if (strcmp(players[i].username, name) == 0) return &players[i];
    return NULL;
}

Player* find_player_by_fd(int fd) {
    for (int i = 0; i < player_count; i++)
        if (players[i].fd == fd) return &players[i];
    return NULL;
}

Player* create_player(const char *name, int fd) {
    Player *p = &players[player_count++];
    strcpy(p->username, name);
    p->fd = fd;
    p->team_id = -1;
    p->is_captain = 0;
    p->hp = 1000;
    p->coin = 500;
    return p;
}

void handle_login(int fd, const char *username) {
    Player *exist_fd = find_player_by_fd(fd);
    if (exist_fd) {
        send_to_fd(fd, "313 ALREADY_LOGGED");
        return;
    }

    if (!username || strlen(username) == 0) {
        send_to_fd(fd, "301 SYNTAX_ERROR invalid_username");
        return;
    }

    Player *p = find_player_by_name(username);
    if (p) {
        if (p->fd != -1 && p->fd != fd) {
            send_to_fd(fd, "313 ALREADY_LOGGED");
            return;
        }

        p->fd = fd;
        send_to_fd(fd, "110 LOGIN_OK");
        return;
    }

    Player *np = create_player(username, fd);
    send_to_fd(fd, "110 LOGIN_OK");
}

int player_in_team(int fd) {
    Player *p = find_player_by_fd(fd);
    return p ? (p->team_id != -1) : 0;
}

Player* get_captain_of_team(int team_id) {
    Team *t = get_team(team_id);
    if (!t || t->member_count == 0) return NULL;
    return find_player_by_name(t->members[0]);
}

