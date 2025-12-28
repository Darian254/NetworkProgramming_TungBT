#include "invite.h"
#include "../player/player.h"
#include "../team/team.h"
#include "../utils/utils.h"
#include "../log/log.h"
#include <string.h>
#include <stdio.h>

void handle_invite(int fd, const char *team_name, const char *username) {
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

    Player *invitee = find_player_by_name(username);
    if (!invitee) {
        char buf[512];
        snprintf(buf, sizeof(buf), "302 PLAYER_NOT_FOUND %s", username);
        send_to_fd(fd, buf);
        return;
    }

    if (t->member_count == 0 || strcmp(t->members[0], p->username) != 0) {
        char buf[512];
        snprintf(buf, sizeof(buf), "326 NOT_CREATOR %s", username);
        send_to_fd(fd, buf);
        return;
    }

    if (t->invite_req_count >= MAX_INVITE_REQ) {
        char buf[512];
        snprintf(buf, sizeof(buf), "328 INVITE_QUEUE_FULL %s", team_name);
        send_to_fd(fd, buf);
        return;
    }

    strncpy(t->invite_requests[t->invite_req_count], username, 31);
    t->invite_requests[t->invite_req_count][31] = '\0';
    t->invite_req_count++;

    if (invitee->fd > 0) {
        char buf[128];
        snprintf(buf, sizeof(buf),
                 "125 TEAM_INVITE_RECEIVED %s|%s",
                 t->name, p->username);
        send_to_fd(invitee->fd, buf);
    }

    char buf[128];
    snprintf(buf, sizeof(buf),
             "124 TEAM_INVITED %s|%s",
             t->name, username);
    send_to_fd(fd, buf);

    char log_buf[256];
    snprintf(log_buf, sizeof(log_buf), "INVITE %s invited to team=%s", username, t->name);
    log_action(log_buf);
}

void handle_invite_accept(int fd, const char *team_name) {
    Player *p = find_player_by_fd(fd);
    Team *t = get_team_by_name(team_name);

    if (!p) {
        send_to_fd(fd, "315 NOT_LOGGED");
        return;
    }
    if (!t) {
        char buf[512];
        snprintf(buf, sizeof(buf), "323 TEAM_NOT_FOUND %s", team_name);
        send_to_fd(fd, buf);
        return;
    }

    int idx = -1;
    for (int i = 0; i < t->invite_req_count; i++) {
        if (strcmp(t->invite_requests[i], p->username) == 0) {
            idx = i;
            break;
        }
    }

    if (idx == -1) {
        send_to_fd(fd, "ERR no_invite");
        return;
    }

    for (int i = idx; i < t->invite_req_count - 1; i++)
        strcpy(t->invite_requests[i], t->invite_requests[i + 1]);
    t->invite_req_count--;

    if (t->member_count >= MAX_TEAM_MEM) {
        char buf[512];
        snprintf(buf, sizeof(buf), "328 TEAM_FULL %s", team_name);
        send_to_fd(fd, buf);
        return;
    }

    strcpy(t->members[t->member_count++], p->username);
    p->team_id = t->id;

    Player *capt = find_player_by_name(t->members[0]);
    if (capt && capt->fd > 0) {
        char buf[128];
        snprintf(buf, sizeof(buf),
                "126 TEAM_INVITE_ACCEPTED %s|%s",
                t->name, p->username);
        send_to_fd(capt->fd, buf);
    }

    char buf[128];
    snprintf(buf, sizeof(buf),
             "126 TEAM_INVITE_ACCEPTED %s|%s",
             t->name, p->username);
    send_to_fd(fd, buf);

    char log_buf[256];
    snprintf(log_buf, sizeof(log_buf), "INVITE_ACCEPT %s team=%s", p->username, t->name);
    log_action(log_buf);
}

void handle_invite_reject(int fd, const char *team_name) {
    Player *p = find_player_by_fd(fd);
    Team *t = get_team_by_name(team_name);

    if (!p) {
        send_to_fd(fd, "315 NOT_LOGGED");
        return;
    }
    if (!t) {
        char buf[512];
        snprintf(buf, sizeof(buf), "323 TEAM_NOT_FOUND %s", team_name);
        send_to_fd(fd, buf);
        return;
    }

    int idx = -1;
    for (int i = 0; i < t->invite_req_count; i++) {
        if (strcmp(t->invite_requests[i], p->username) == 0) {
            idx = i;
            break;
        }
    }

    if (idx == -1) {
        send_to_fd(fd, "ERR no_such_invite");
        return;
    }

    for (int i = idx; i < t->invite_req_count - 1; i++)
        strcpy(t->invite_requests[i], t->invite_requests[i + 1]);
    t->invite_req_count--;

    Player *capt = find_player_by_name(t->members[0]);
    if (capt && capt->fd > 0) {
        char buf[128];
        snprintf(buf, sizeof(buf),
                "127 TEAM_INVITE_REJECTED %s|%s",
                t->name, p->username);
        send_to_fd(capt->fd, buf);
    }

    char buf[128];
    snprintf(buf, sizeof(buf),
             "127 TEAM_INVITE_REJECTED %s|%s",
             t->name, p->username);
    send_to_fd(fd, buf);

    char log_buf[256];
    snprintf(log_buf, sizeof(log_buf), "INVITE_REJECT %s team=%s", p->username, t->name);
    log_action(log_buf);
}
