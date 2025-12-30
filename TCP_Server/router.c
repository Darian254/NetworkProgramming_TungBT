#include "router.h"
#include "command.h"
#include "connect.h"
#include "session.h"
#include "users.h"
#include "app_context.h"
#include "config.h"
#include "db_schema.h"
#include "util.h"
#include <stdio.h>
#include <string.h>

/**
 * @file router.c
 * @brief Command routing implementation
 * Routes incoming commands to appropriate handlers.
 * Routes commands to existing handlers in session.c, users.c, db.c
 * This is the glue layer between network I/O and business logic.
 */

void command_routes(int client_sock, char *command) {
    // TODO Step 1: Parse the command
    Command cmd = parse_command(command);
    const char *type = cmd.type ? cmd.type : "";
    const char *payload = cmd.user_input ? cmd.user_input : "";

    // TODO Step 2: Find session by socket
    // This gives us the ServerSession for this connection
    SessionNode *node = find_session_by_socket(client_sock);
    if (!node) {
        // No session found - this shouldn't happen since connection_create() creates session
        fprintf(stderr, "[ERROR] No session for socket %d\n", client_sock);
        const char *err = "500 INTERNAL_ERROR no_session\r\n";
        connection_send(client_sock, err, strlen(err));
        return;
    }
    ServerSession *session = &node->session;

    // Prepare response buffer
    char response[512];
    int response_code;

    // TODO Step 3: Route commands to handlers
    
    // ========== Authentication Commands ==========
    if (strcmp(type, "REGISTER") == 0) {
        // TODO: Parse username and password from payload
        // Expected format: "username password"
        char username[128], password[128];
        if (sscanf(payload, "%127s %127s", username, password) != 2) {
            response_code = RESP_SYNTAX_ERROR;
            snprintf(response, sizeof(response), "%d\r\n", response_code);
            log_activity("REGISTER", NULL, false, payload, response_code);
        } else {
            // Call handler (single-threaded, no locking needed)
            response_code = server_handle_register(app_context_get_user_table(), username, password);
            snprintf(response, sizeof(response), "%d\r\n", response_code);
            log_activity("REGISTER", username, false, payload, response_code);
        }
    }
    else if (strcmp(type, "LOGIN") == 0) {
        // TODO: Parse username and password
        char username[128], password[128];
        if (sscanf(payload, "%127s %127s", username, password) != 2) {
            response_code = RESP_SYNTAX_ERROR;
            snprintf(response, sizeof(response), "%d\r\n", response_code);
            log_activity("LOGIN", NULL, false, payload, response_code);
        } else {
            // Call handler (single-threaded, no locking needed)
            response_code = server_handle_login(session, app_context_get_user_table(), username, password);
            snprintf(response, sizeof(response), "%d\r\n", response_code);
            log_activity("LOGIN", username, false, payload, response_code);
        }
    }
    else if (strcmp(type, "WHOAMI") == 0) {
        // TODO: Get username from session
        char username[MAX_USERNAME];
        response_code = server_handle_whoami(session, username);
        if (response_code == RESP_WHOAMI_OK) {
            snprintf(response, sizeof(response), "%d %s\r\n", response_code, username);
            log_activity("WHOAMI", username, true, payload, response_code);
        } else {
            snprintf(response, sizeof(response), "%d\r\n", response_code);
            log_activity("WHOAMI", NULL, false, payload, response_code);
        }
    }
    else if (strcmp(type, "BYE") == 0 || strcmp(type, "LOGOUT") == 0) {
        // TODO: Call logout handler
        response_code = server_handle_bye(session);
        snprintf(response, sizeof(response), "%d\r\n", response_code);
        log_activity("LOGOUT", session->username, true, payload, response_code);
    }

    // ========== Game Commands ==========
    else if (strcmp(type, "GETCOIN") == 0) {
        // TODO: Check if logged in
        if (!session->isLoggedIn) {
            response_code = RESP_NOT_LOGGED;
            snprintf(response, sizeof(response), "%d\r\n", response_code);
            log_activity("GETCOIN", session->username, false, payload, response_code);
        } else {
            // TODO: Find user and get coin balance
            User *user = findUser(app_context_get_user_table(), session->username);
            if (user) {
                response_code = RESP_COIN_OK;
                snprintf(response, sizeof(response), "%d %ld\r\n", response_code, user->coin);
                log_activity("GETCOIN", session->username, true, payload, response_code);
            } else {
                response_code = RESP_INTERNAL_ERROR;
                snprintf(response, sizeof(response), "%d\r\n", response_code);
                log_activity("GETCOIN", session->username, true, payload, response_code);
            }
        }
    }
    else if (strcmp(type, "GETARMOR") == 0) {
        // TODO: Check if logged in
        if (!session->isLoggedIn) {
            response_code = RESP_NOT_LOGGED;
            snprintf(response, sizeof(response), "%d\r\n", response_code);
            log_activity("GETARMOR", session->username, false, payload, response_code);
        } else {
            // TODO: Get match_id from session or lookup
            int match_id = session->current_match_id;
            if (match_id <= 0) {
                match_id = find_current_match_by_username(session->username);
            }
            
            if (match_id <= 0) {
                response_code = RESP_NOT_IN_MATCH;
                snprintf(response, sizeof(response), "%d\r\n", response_code);
                log_activity("GETARMOR", session->username, true, payload, response_code);
            } else {
                // TODO: Find ship and format armor info
                Ship *ship = find_ship(match_id, session->username);
                if (ship) {
                    response_code = RESP_ARMOR_INFO_OK;
                    snprintf(response, sizeof(response), "%d %d %d %d %d\r\n",
                             response_code,
                             ship->armor_slot_1_type, ship->armor_slot_1_value,
                             ship->armor_slot_2_type, ship->armor_slot_2_value);
                    log_activity("GETARMOR", session->username, true, payload, response_code);
                } else {
                    response_code = RESP_INTERNAL_ERROR;
                    snprintf(response, sizeof(response), "%d\r\n", response_code);
                    log_activity("GETARMOR", session->username, true, payload, response_code);
                }
            }
        }
    }
    else if (strcmp(type, "BUYARMOR") == 0) {
        // TODO: Parse armor type
        int armor_type;
        if (sscanf(payload, "%d", &armor_type) != 1) {
            response_code = RESP_SYNTAX_ERROR;
            snprintf(response, sizeof(response), "%d\r\n", response_code);
            log_activity("BUYARMOR", session->username, false, payload, response_code);
        } else {
            // Call handler (single-threaded, no locking needed)
            response_code = server_handle_buyarmor(session, app_context_get_user_table(), armor_type);
            snprintf(response, sizeof(response), "%d\r\n", response_code);
            log_activity("BUYARMOR", session->username, true, payload, response_code);
        }
    }
    else if (strcmp(type,"GET_MATCH_RESULT") == 0) {
        int match_id = -1;
        if (payload && sscanf(payload, "%d", &match_id) == 1 && match_id > 0) {
            response_code = server_handle_get_match_result(session, match_id);
            log_activity("GET_MATCH_RESULT", session->username, true, payload, response_code);
        } else {
            response_code = RESP_SYNTAX_ERROR;
            log_activity("GET_MATCH_RESULT", session->username, true, payload, response_code);
        }
        snprintf(response, sizeof(response), "%d\r\n", response_code);
        log_activity("GET_MATCH_RESULT", session->username, true, payload, response_code);
    }
    else if (strcmp(type, "START_MATCH") == 0) {
        int opponent_team_id = -1;
        if (payload && sscanf(payload, "%d", &opponent_team_id) == 1 && opponent_team_id > 0) {
            response_code = server_handle_start_match(session, opponent_team_id);
            log_activity("START_MATCH", session->username, true, payload, response_code);
        } else {
            response_code = RESP_SYNTAX_ERROR;
            log_activity("START_MATCH", session->username, true, payload, response_code);
        }
        snprintf(response, sizeof(response), "%d\r\n", response_code);
        log_activity("START_MATCH", session->username, true, payload, response_code);
    }

    // ========== Team/Challenge Commands (Future) ==========
    // TODO: Add more commands here as you implement team features
    // Examples:
    // - CREATE_TEAM
    // - DELETE_TEAM
    // - JOIN_REQUEST
    // - TEAM_INVITE
    // - SEND_CHALLENGE
    // etc.

    // ========== Unknown Command ==========
    else {
        // TODO: Send syntax error for unknown commands
        response_code = RESP_SYNTAX_ERROR;
        snprintf(response, sizeof(response), "%d\r\n", response_code);
        log_activity("UNKNOWN_COMMAND", session->username, session->isLoggedIn, command, response_code);
    }

    // TODO Step 4: Send response back to client
    connection_send(client_sock, response, strlen(response));
}
