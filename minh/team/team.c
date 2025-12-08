#include "team.h"
#include "../player/player.h"
#include "../utils/utils.h"
#include "../log/log.h"
#include <string.h>
#include <stdio.h>

Team teams[MAX_TEAMS];
int team_count = 0;


Team* get_team(int id) {
    if (id < 0 || id >= team_count) return NULL;
    return &teams[id];
}

Team* get_team_by_name(const char *name) {
    for (int i = 0; i < team_count; i++) {
        if (strcmp(teams[i].name, name) == 0)
            return &teams[i];
    }
    return NULL;
}

void handle_create_team(int fd, const char *team_name) {
    Player *p = find_player_by_fd(fd);
    if (!p) {
        send_to_fd(fd, "315 NOT_LOGGED");
        return;
    }

    if (p->team_id != -1) {
        send_to_fd(fd, "332 TEAM_CREATE_FAILED already_in_team");
        return;
    }

    if (!team_name || strlen(team_name) == 0) {
        send_to_fd(fd, "301 SYNTAX_ERROR invalid_team_name");
        return;
    }

    for (int i = 0; i < team_count; i++) {
        if (strcmp(teams[i].name, team_name) == 0) {
            send_to_fd(fd, "332 TEAM_CREATE_FAILED team_exists");
            return;
        }
    }

    Team *t = &teams[team_count];
    t->id = team_count;
    strcpy(t->name, team_name);
    t->member_count = 1;
    strcpy(t->members[0], p->username);
    t->invite_req_count = 0;

    p->team_id = t->id;
    p->is_captain = 1;

    team_count++;

    char buf[128];
    snprintf(buf, sizeof(buf), "120 TEAM_CREATED %d|%s", t->id, t->name);
    send_to_fd(fd, buf);

    char log_buf[256];
    snprintf(log_buf, sizeof(log_buf), "CREATE_TEAM %s captain=%s", team_name, p->username);
    log_action(log_buf);
}

void handle_delete_team(int fd, const char *team_name) {
    if (!team_name || strlen(team_name) == 0) {
        send_to_fd(fd, "323 TEAM_NOT_FOUND");
        return;
    }

    int idx = -1;
    for (int i = 0; i < team_count; i++) {
        if (strcmp(teams[i].name, team_name) == 0) {
            idx = i;
            break;
        }
    }

    if (idx < 0) {
        send_to_fd(fd, "323 TEAM_NOT_FOUND");
        return;
    }

    Team *t = &teams[idx];

    Player *captain = find_player_by_name(t->members[0]);
    if (!captain || captain->fd != fd) {
        send_to_fd(fd, "326 NOT_CREATOR");
        return;
    }

    char deleted_name[32];
    strcpy(deleted_name, t->name);

    for (int i = 0; i < t->member_count; i++) {
        Player *p = find_player_by_name(t->members[i]);
        if (p) p->team_id = -1;
    }

    for (int i = idx; i < team_count - 1; i++)
        teams[i] = teams[i + 1];

    team_count--;

    char buf[512];
    snprintf(buf, sizeof(buf), "128 TEAM_DELETED %s", deleted_name);
    send_to_fd(fd, buf);

    snprintf(buf, sizeof(buf), "DELETE_TEAM %s", deleted_name);
    log_action(buf);
}

void handle_list_teams(int fd) {
    char buf[512] = {0};
    for (int i = 0; i < team_count; i++) {
        char tmp[64];
        snprintf(tmp, sizeof(tmp), "%d:%s|%d\n", teams[i].id, teams[i].name, teams[i].member_count);
        strcat(buf, tmp);
    }
    send_to_fd(fd, buf);
}

void handle_team_member_list(int fd, const char *team_name) {
    Team *t = get_team_by_name(team_name);
    if (!t) {
        char buf[512];
        snprintf(buf, sizeof(buf), "323 TEAM_NOT_FOUND %s", team_name);
        send_to_fd(fd, buf);
        return;
    }

    char buf[512] = "";
    for (int i = 0; i < t->member_count; i++) {
        if (i > 0) strcat(buf, ",");
        strcat(buf, t->members[i]);
    }

    send_to_fd(fd, buf);
}

void handle_leave_team(int fd) {
    Player *p = find_player_by_fd(fd);
    if (!p) {
        send_to_fd(fd, "315 NOT_LOGGED");
        return;
    }

    if (p->team_id == -1) {
        send_to_fd(fd, "327 NOT_IN_TEAM");
        return;
    }

    Team *t = get_team(p->team_id);
    if (!t) {
        send_to_fd(fd, "323 TEAM_NOT_FOUND");
        return;
    }

    char log_buf[256];
    char buf[128];

    if (t->member_count == 1 && strcmp(t->members[0], p->username) == 0) {
        char deleted[32];
        strcpy(deleted, t->name);

        p->team_id = -1;

        int idx = t->id;
        for (int i = idx; i < team_count - 1; i++)
            teams[i] = teams[i + 1];
        team_count--;

        snprintf(buf, sizeof(buf), "128 TEAM_DELETED %s", deleted);
        send_to_fd(fd, buf);

        snprintf(log_buf, sizeof(log_buf), "DELETE_TEAM %s (captain left)", deleted);
        log_action(log_buf);
        return;
    }

    int is_captain = (strcmp(t->members[0], p->username) == 0);
    int found_idx = -1;
    for (int i = 0; i < t->member_count; i++) {
        if (strcmp(t->members[i], p->username) == 0) {
            found_idx = i;
            break;
        }
    }

    if (found_idx < 0) {
        snprintf(buf, sizeof(buf), "302 PLAYER_NOT_FOUND %s", p->username);
        send_to_fd(fd, buf);
        return;
    }

    for (int j = found_idx; j < t->member_count - 1; j++)
        strcpy(t->members[j], t->members[j + 1]);

    t->member_count--;
    p->team_id = -1;

    if (is_captain && t->member_count > 0) {
        Player *new_captain = find_player_by_name(t->members[0]);
        if (new_captain) new_captain->is_captain = 1;

        snprintf(buf, sizeof(buf), "123 TEAM_LEAVE_OK new_captain=%s", t->members[0]);
        send_to_fd(fd, buf);

        snprintf(log_buf, sizeof(log_buf), "LEAVE_TEAM captain=%s → new_captain=%s in team %s",
                 p->username, t->members[0], t->name);
        log_action(log_buf);
    } else {
        snprintf(buf, sizeof(buf), "123 TEAM_LEAVE_OK %s", p->username);
        send_to_fd(fd, buf);
    }
}

void handle_kick_member(int fd, const char *team_name, const char *username) {
    Player *p = find_player_by_fd(fd);
    if (!p) {
        send_to_fd(fd, "315 NOT_LOGGED");
        return;
    }

    Team *t = get_team_by_name(team_name);
    if (!t) {
        char buf[512];
        snprintf(buf, sizeof(buf), "323 TEAM_NOT_FOUND %s", team_name);
        send_to_fd(fd, buf);
        return;
    }

    if (t->member_count == 0 || strcmp(t->members[0], p->username) != 0) {
        char buf[512];
        snprintf(buf, sizeof(buf), "326 NOT_CREATOR %s", p->username);
        send_to_fd(fd, buf);
        return;
    }

    Player *target = find_player_by_name(username);
    if (!target || target->team_id != t->id) {
        char buf[512];
        snprintf(buf, sizeof(buf), "302 PLAYER_NOT_FOUND %s", username);
        send_to_fd(fd, buf);
        return;
    }

    int found = 0;
    for (int i = 0; i < t->member_count; i++) {
        if (strcmp(t->members[i], username) == 0) {
            found = 1;
            for (int j = i; j < t->member_count - 1; j++)
                strcpy(t->members[j], t->members[j + 1]);
            t->member_count--;
            break;
        }
    }

    if (!found) {
        char buf[128];
        snprintf(buf, sizeof(buf), "302 PLAYER_NOT_FOUND %s", username);
        send_to_fd(fd, buf);
        return;
    }

    target->team_id = -1;

    char buf[128];
    snprintf(buf, sizeof(buf), "129 KICK_MEMBER_OK %s", username);
    send_to_fd(fd, buf);

    char log_buf[256];
    snprintf(log_buf, sizeof(log_buf), "KICK_MEMBER: %s kicked %s from team %s",
             p->username, username, t->name);
    log_action(log_buf);
}
