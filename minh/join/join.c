#include "join.h"
#include "../player/player.h"
#include "../team/team.h"
#include "../utils/utils.h"
#include "../log/log.h"
#include <string.h>
#include <stdio.h>

void handle_join_request(int fd, const char *team_name) {
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

    if (p->team_id != -1) {
        char buf[512];
        snprintf(buf, sizeof(buf), "303 ALREADY_IN_TEAM %d", p->team_id);
        send_to_fd(fd, buf);
        return;
    }

    for (int i = 0; i < t->join_req_count; i++)
        if (strcmp(t->join_requests[i], p->username) == 0) {
            send_to_fd(fd, "301 SYNTAX_ERROR duplicate_request");
            return;
        }

    strcpy(t->join_requests[t->join_req_count++], p->username);

    Player *capt = find_player_by_name(t->members[0]);
    if (capt && capt->fd > 0) {
        char buf[128];
        snprintf(buf, sizeof(buf),
                 "125 JOIN_REQUEST_RECEIVED %s|%s",
                 t->name, p->username);
        send_to_fd(capt->fd, buf);
    }

    char buf[128];
    snprintf(buf, sizeof(buf),
             "121 JOIN_REQUEST_SENT %s|%s",
             t->name, p->username);
    send_to_fd(fd, buf);

    snprintf(buf, sizeof(buf), "JOIN_REQUEST %s team=%s", p->username, t->name);
    log_msg(buf);
}


void handle_join_approve(int fd, const char *team_name, const char *username) {
    Player *capt = find_player_by_fd(fd);
    if (!capt) {
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

    Player *p = find_player_by_name(username);
    if (!p) {
        char buf[512];
        snprintf(buf, sizeof(buf), "302 PLAYER_NOT_FOUND %s", username);
        send_to_fd(fd, buf);
        return;
    }

    if (strcmp(t->members[0], capt->username) != 0) {
        char buf[512];
        snprintf(buf, sizeof(buf), "326 NOT_CREATOR %s", username);
        send_to_fd(fd, buf);
        return;
    }

    int idx = -1;
    for (int i = 0; i < t->join_req_count; i++) {
        if (strcmp(t->join_requests[i], username) == 0) {
            idx = i;
            break;
        }
    }
    if (idx == -1) {
        send_to_fd(fd, "ERR no_such_request");
        return;
    }

    for (int i = idx; i < t->join_req_count - 1; i++)
        strcpy(t->join_requests[i], t->join_requests[i + 1]);
    t->join_req_count--;

    if (t->member_count >= MAX_TEAM_MEM) {
        char buf[512];
        snprintf(buf, sizeof(buf), "328 TEAM_FULL %s", team_name);
        send_to_fd(fd, buf);
        return;
    }

    strcpy(t->members[t->member_count++], p->username);
    p->team_id = t->id;

    char buf[128];
    snprintf(buf, sizeof(buf),
             "111 JOIN_APPROVED %s|%s",
             t->name, p->username);
    send_to_fd(capt->fd, buf);

    if (p->fd > 0)
        send_to_fd(p->fd, buf);

    snprintf(buf, sizeof(buf), "JOIN_APPROVE %s team=%s", username, t->name);
    log_msg(buf);
}


void handle_join_reject(int fd, const char *team_name, const char *username) {
    Player *capt = find_player_by_fd(fd);
    if (!capt) {
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

    if (strcmp(t->members[0], capt->username) != 0) {
        char buf[512];
        snprintf(buf, sizeof(buf), "326 NOT_CREATOR %s", username);
        send_to_fd(fd, buf);
        return;
    }

    int idx = -1;
    for (int i = 0; i < t->join_req_count; i++) {
        if (strcmp(t->join_requests[i], username) == 0) {
            idx = i;
            break;
        }
    }
    if (idx == -1) {
        send_to_fd(fd, "ERR no_such_request");
        return;
    }

    for (int i = idx; i < t->join_req_count - 1; i++)
        strcpy(t->join_requests[i], t->join_requests[i + 1]);
    t->join_req_count--;

    Player *p = find_player_by_name(username);
    if (p && p->fd > 0) {
        char buf[128];
        snprintf(buf, sizeof(buf),
                 "112 JOIN_REJECTED %s|%s",
                 t->name, username);
        send_to_fd(p->fd, buf);
    }

    char buf[128];
    snprintf(buf, sizeof(buf),
             "112 JOIN_REJECTED %s|%s",
             t->name, username);
    send_to_fd(fd, buf);

    snprintf(buf, sizeof(buf), "JOIN_REJECT %s team=%s", username, t->name);
    log_msg(buf);
}
