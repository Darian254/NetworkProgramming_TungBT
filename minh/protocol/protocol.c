#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "protocol.h"
#include "../join/join.h"
#include "../invite/invite.h"
#include "../player/player.h"
#include "../team/team.h"
#include "../utils/utils.h"
#include "../log/log.h"
#include "../server/server.h"

void protocol_route(int fd, int cmd, char *payload) {
    switch(cmd) {

        case CMD_LOGIN:
            handle_login(fd, payload);
            break;

        case CMD_CREATE_TEAM:
            handle_create_team(fd, payload);
            break;

        case CMD_DELETE_TEAM: {      
            handle_delete_team(fd, payload);
            break;
        }


        case CMD_LIST_TEAMS:
            handle_list_teams(fd);
            break;

        case CMD_TEAM_MEMBER_LIST: {
            char team_name[32];
            if (sscanf(payload, "%s", team_name) != 1) {
                send_to_fd(fd, "301 SYNTAX_ERROR invalid_format");
                break;
            }
            handle_team_member_list(fd, team_name);
            break;
        }


        case CMD_LEAVE_TEAM:
            handle_leave_team(fd);
            break;

        case CMD_KICK_MEMBER: {
            char team_name[32], user[32];
            if (sscanf(payload, "%31s %31s", team_name, user) != 2) {
                send_to_fd(fd, "301 SYNTAX_ERROR invalid_format");
                break;
            }
            handle_kick_member(fd, team_name, user);  
            break;
        }


        case CMD_JOIN_REQUEST:
            handle_join_request(fd, payload); 
            break;

        case CMD_JOIN_APPROVE: {
            char team_name[32], user[32];
            if (sscanf(payload, "%s %s", team_name, user) != 2)
                return send_to_fd(fd, "301 SYNTAX_ERROR invalid_format");
            handle_join_approve(fd, team_name, user);
            break;
        }

        case CMD_JOIN_REJECT: {
            char team_name[32], user[32];
            if (sscanf(payload, "%s %s", team_name, user) != 2)
                return send_to_fd(fd, "301 SYNTAX_ERROR invalid_format");
            handle_join_reject(fd, team_name, user);
            break;
        }


        case CMD_INVITE: {
            char team_name[32], user[32];
            if (sscanf(payload, "%s %s", team_name, user) != 2) {
                send_to_fd(fd, "301 SYNTAX_ERROR invalid_format");
                break;
            }
            handle_invite(fd, team_name, user);
            break;
        }

        case CMD_INVITE_ACCEPT: {
            char team_name[32];
            if (sscanf(payload, "%s", team_name) != 1) {
                send_to_fd(fd, "301 SYNTAX_ERROR invalid_format");
                break;
            }
            handle_invite_accept(fd, team_name);
            break;
        }

        case CMD_INVITE_REJECT: {
            char team_name[32];
            if (sscanf(payload, "%s", team_name) != 1) {
                send_to_fd(fd, "301 SYNTAX_ERROR invalid_format");
                break;
            }
            handle_invite_reject(fd, team_name);
            break;
        }


    }
}


int parse_cmd(const char *text) {
    if (strncmp(text, "LOGIN", 5) == 0) return CMD_LOGIN;

    if (strncmp(text, "CREATE_TEAM", 11) == 0) return CMD_CREATE_TEAM;
    if (strncmp(text, "DELETE_TEAM", 11) == 0) return CMD_DELETE_TEAM;
    if (strncmp(text, "LIST_TEAMS", 10) == 0) return CMD_LIST_TEAMS;
    if (strncmp(text, "TEAM_MEMBER_LIST", 16) == 0) return CMD_TEAM_MEMBER_LIST;
    if (strncmp(text, "LEAVE_TEAM", 10) == 0) return CMD_LEAVE_TEAM;
    if (strncmp(text, "KICK_MEMBER", 11) == 0) return CMD_KICK_MEMBER;

    if (strncmp(text, "JOIN_REQUEST", 12) == 0) return CMD_JOIN_REQUEST;
    if (strncmp(text, "JOIN_APPROVE", 12) == 0) return CMD_JOIN_APPROVE;
    if (strncmp(text, "JOIN_REJECT", 11) == 0) return CMD_JOIN_REJECT;

    if (strncmp(text, "INVITE", 7) == 0) return CMD_INVITE;
    if (strncmp(text, "INVITE_ACCEPT", 13) == 0) return CMD_INVITE_ACCEPT;
    if (strncmp(text, "INVITE_REJECT", 13) == 0) return CMD_INVITE_REJECT;

    return -1;
}
