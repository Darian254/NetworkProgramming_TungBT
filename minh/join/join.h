#ifndef JOIN_H
#define JOIN_H

void handle_join_request(int fd, const char *team_name);
void handle_join_approve(int fd, const char *team_name, const char *username);
void handle_join_reject(int fd, const char *team_name, const char *username);

#endif
