#include "router.h"
#include <string.h>
#include <stdio.h>
#include "command.h"
#include "response.h"
#include "connect.h"
#include <time.h>

static void handle_register(int fd, const char *payload) { printf("[REGISTER] %s\n", payload); }
static void handle_login(int fd, const char *payload)    { printf("[LOGIN] %s\n", payload); }
static void handle_logout(int fd, const char *payload)   { printf("[LOGOUT] %s\n", payload); }
static void handle_whoami(int fd, const char *payload)   { printf("[WHOAMI] %s\n", payload); }
static void handle_create_team(int fd, const char *p)    { printf("[CREATE_TEAM] %s\n", p); }
static void handle_delete_team(int fd, const char *p)    { printf("[DELETE_TEAM] %s\n", p); }
static void handle_list_teams(int fd, const char *p)     { printf("[LIST_TEAMS] %s\n", p); }
static void handle_join_request(int fd, const char *p)   { printf("[JOIN_REQUEST] %s\n", p); }
static void handle_team_invite(int fd, const char *p)    { printf("[TEAM_INVITE] %s\n", p); }
static void handle_accept_invite(int fd, const char *p)  { printf("[ACCEPT_INVITE] %s\n", p); }
static void handle_decline_invite(int fd, const char *p) { printf("[DECLINE_INVITE] %s\n", p); }
static void handle_leave_team(int fd, const char *p)     { printf("[LEAVE_TEAM] %s\n", p); }
static void handle_team_member_list(int fd, const char *p){ printf("[TEAM_MEMBER_LIST] %s\n", p); }
static void handle_kick_member(int fd, const char *p)    { printf("[KICK_MEMBER] %s\n", p); }
static void handle_send_challenge(int fd, const char *p) { printf("[SEND_CHALLENGE] %s\n", p); }
static void handle_accept_challenge(int fd, const char *p){ printf("[ACCEPT_CHALLENGE] %s\n", p); }
static void handle_decline_challenge(int fd, const char *p){ printf("[DECLINE_CHALLENGE] %s\n", p); }
static void handle_cancel_challenge(int fd, const char *p){ printf("[CANCEL_CHALLENGE] %s\n", p); }
static void handle_start_match(int fd, const char *p) { 
    printf("[START_MATCH] team_id=%s\n", p);
    char match_id[32];
    snprintf(match_id, sizeof(match_id), "%ld", (long)time(NULL));
    printf("Generated match_id: %s\n", match_id);
    char response[64];
    snprintf(response, sizeof(response), START_MATCH_OK, match_id);
    printf("Response: %s", response);
}
static void handle_get_match_result(int fd, const char *p) { 
    printf("[GET_MATCH_RESULT] match_id=%s\n", p);
}
static void handle_unknown(int fd, const char *cmd) { printf("[UNKNOWN] %s\n", cmd); }

void command_routes(int client_sock, char *command) {
    Command c = parse_command(command);
    const char *type = c.type;
    const char *payload = c.user_input;

    if (strcmp(type, "REGISTER") == 0) {
        handle_register(client_sock, payload);
    } else if (strcmp(type, "LOGIN") == 0) {
        handle_login(client_sock, payload);
    } else if (strcmp(type, "LOGOUT") == 0) {
        handle_logout(client_sock, payload);
    } else if (strcmp(type, "WHOAMI") == 0) {
        handle_whoami(client_sock, payload);
    } else if (strcmp(type, "CREATE_TEAM") == 0) {
        handle_create_team(client_sock, payload);
    } else if (strcmp(type, "DELETE_TEAM") == 0) {
        handle_delete_team(client_sock, payload);
    } else if (strcmp(type, "LIST_TEAMS") == 0) {
        handle_list_teams(client_sock, payload);
    } else if (strcmp(type, "JOIN_REQUEST") == 0) {
        handle_join_request(client_sock, payload);
    } else if (strcmp(type, "TEAM_INVITE") == 0) {
        handle_team_invite(client_sock, payload);
    } else if (strcmp(type, "ACCEPT_INVITE") == 0) {
        handle_accept_invite(client_sock, payload);
    } else if (strcmp(type, "DECLINE_INVITE") == 0) {
        handle_decline_invite(client_sock, payload);
    } else if (strcmp(type, "LEAVE_TEAM") == 0) {
        handle_leave_team(client_sock, payload);
    } else if (strcmp(type, "TEAM_MEMBER_LIST") == 0) {
        handle_team_member_list(client_sock, payload);
    } else if (strcmp(type, "KICK_MEMBER") == 0) {
        handle_kick_member(client_sock, payload);
    } else if (strcmp(type, "SEND_CHALLENGE") == 0) {
        handle_send_challenge(client_sock, payload);
    } else if (strcmp(type, "ACCEPT_CHALLENGE") == 0) {
        handle_accept_challenge(client_sock, payload);
    } else if (strcmp(type, "DECLINE_CHALLENGE") == 0) {
        handle_decline_challenge(client_sock, payload);
    } else if (strcmp(type, "CANCEL_CHALLENGE") == 0) {
        handle_cancel_challenge(client_sock, payload);
    } else if (strcmp(type, "START_MATCH") == 0) {
        handle_start_match(client_sock, payload); 
    } else if (strcmp(type, "GET_MATCH_RESULT") == 0) {
        handle_get_match_result(client_sock, payload); 
    } else {
        handle_unknown(client_sock, type);
    }
}
