#ifndef PROTOCOL_H
#define PROTOCOL_H


#define CMD_LOGIN              1
#define CMD_CREATE_TEAM        2
#define CMD_DELETE_TEAM        3
#define CMD_LIST_TEAMS         4
#define CMD_TEAM_MEMBER_LIST   5
#define CMD_JOIN_REQUEST       6
#define CMD_JOIN_APPROVE       7
#define CMD_JOIN_REJECT        8
#define CMD_INVITE             9
#define CMD_INVITE_ACCEPT      10
#define CMD_INVITE_REJECT      11
#define CMD_LEAVE_TEAM         12
#define CMD_KICK_MEMBER        13

void protocol_route(int fd, int cmd, char *payload);
int parse_cmd(const char *text);

#endif
