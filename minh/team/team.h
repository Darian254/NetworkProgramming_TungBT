#ifndef TEAM_H
#define TEAM_H

#define MAX_TEAMS 32
#define MAX_TEAM_MEM 3
#define MAX_JOIN_REQ 32
#define MAX_INVITE_REQ 32

typedef struct {
    int id;
    char name[32];

    char members[MAX_TEAM_MEM][32];
    int member_count;
 
    char join_requests[MAX_JOIN_REQ][32];
    int join_req_count;

    char invite_requests[MAX_INVITE_REQ][32];
    int invite_req_count;

} Team;

Team* get_team(int id);
Team* get_team_by_name(const char *name);
void team_update_notify(Team *t);
void handle_create_team(int fd, const char *team_name);
void handle_delete_team(int fd, const char *team_name);
void handle_list_teams(int fd);
void handle_team_member_list(int fd, const char *team_name);
void handle_leave_team(int fd);
void handle_kick_member(int fd, const char *team_name, const char *username);

#endif
