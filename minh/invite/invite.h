#ifndef INVITE_H
#define INVITE_H

void handle_invite(int fd, const char *team_name, const char *username);
void handle_invite_accept(int fd, const char *team_name);
void handle_invite_reject(int fd, const char *team_name);

#endif
