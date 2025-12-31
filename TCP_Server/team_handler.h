/**
 * @file team_handler.h
 * @brief Team operation handlers for TCP server
 */

#ifndef TEAM_HANDLER_H
#define TEAM_HANDLER_H

#include "session.h"
#include "users.h"
// Không cần define lại RESP_... vì đã có trong config.h

/**
 * @brief Handle CREATE_TEAM command
 * Creates a new team with the logged-in user as captain
 */
int handle_create_team(ServerSession *session, UserTable *ut, const char *team_name);

/**
 * @brief Handle DELETE_TEAM command
 * Only team creator can delete the team
 */
int handle_delete_team(ServerSession *session);

/**
 * @brief Handle LIST_TEAMS command
 * Returns list of all active teams
 */
int handle_list_teams(ServerSession *session, char *output_buf, size_t buf_size);

/**
 * @brief Handle TEAM_MEMBER_LIST command
 * Returns comma-separated list of team members
 */
int handle_team_member_list(ServerSession *session, char *output_buf, size_t buf_size);

/**
 * @brief Handle LEAVE_TEAM command
 * Player leaves their current team
 */
int handle_leave_team(ServerSession *session);

/**
 * @brief Handle KICK_MEMBER command
 * Only team creator can kick members
 */
int handle_kick_member(ServerSession *session, const char *username);

/**
 * @brief Handle JOIN_REQUEST command
 * Player requests to join a team
 */
int handle_join_request(ServerSession *session, const char *team_name);

/**
 * @brief Handle JOIN_APPROVE command
 * Team creator approves a join request
 */
int handle_join_approve(ServerSession *session, const char *username, UserTable *user_table);

/**
 * @brief Handle JOIN_REJECT command
 * Team creator rejects a join request
 */
int handle_join_reject(ServerSession *session, const char *username);

/**
 * @brief Handle INVITE command
 * Team creator invites a player to join
 */
int handle_invite(ServerSession *session, const char *username, UserTable *user_table);

/**
 * @brief Handle INVITE_ACCEPT command
 * Player accepts a team invitation
 */
int handle_invite_accept(ServerSession *session, const char *team_name);

/**
 * @brief Handle INVITE_REJECT command
 * Player rejects a team invitation
 */
int handle_invite_reject(ServerSession *session, const char *team_name);

/**
 * @brief Handle CHECK_INVITES command
 * Lists all pending invites for the current user
 */
int handle_check_invites(ServerSession *session, char *output_buf, size_t buf_size);

int handle_check_join_requests(ServerSession *session, char *output_buf, size_t buf_size);
#endif // TEAM_HANDLER_H