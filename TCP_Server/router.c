#include "router.h"
#include "command.h"
#include "connect.h"
#include "session.h"
#include "users.h"
#include "app_context.h"
#include "config.h"
#include "db_schema.h"
#include "util.h"
#include "team_handler.h" // Team management handlers
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

    // Prepare response buffer (increased for MATCH_INFO)
    char response[8192];
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
            log_activity("LOGIN", username, session->isLoggedIn, payload, response_code);
        }
    }
    else if (strcmp(type, "WHOAMI") == 0) {
        // TODO: Get username from session
        char username[MAX_USERNAME];
        response_code = server_handle_whoami(session, username);
        if (response_code == RESP_WHOAMI_OK) {
            snprintf(response, sizeof(response), "%d %s\r\n", response_code, username);
            log_activity("WHOAMI", username, session->isLoggedIn, payload, response_code);
        } else {
            snprintf(response, sizeof(response), "%d\r\n", response_code);
            log_activity("WHOAMI", NULL, false, payload, response_code);
        }
    }
    else if (strcmp(type, "BYE") == 0 || strcmp(type, "LOGOUT") == 0) {
        // Preserve username before logout for accurate logging
        char user_before[MAX_USERNAME];
        strncpy(user_before, session->username, MAX_USERNAME - 1);
        user_before[MAX_USERNAME - 1] = '\0';

        // Call logout handler
        response_code = server_handle_bye(session);
        snprintf(response, sizeof(response), "%d\r\n", response_code);

        // After logout, mark is_logged_in=false and use preserved username
        log_activity("LOGOUT", user_before, false, payload, response_code);
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
                log_activity("GETCOIN", session->username, session->isLoggedIn, payload, response_code);
            } else {
                response_code = RESP_INTERNAL_ERROR;
                snprintf(response, sizeof(response), "%d\r\n", response_code);
                log_activity("GETCOIN", session->username, session->isLoggedIn, payload, response_code);
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
                log_activity("GETARMOR", session->username, session->isLoggedIn, payload, response_code);
            } else {
                // TODO: Find ship and format armor info
                Ship *ship = find_ship(match_id, session->username);
                if (ship) {
                    response_code = RESP_ARMOR_INFO_OK;
                    snprintf(response, sizeof(response), "%d %d %d %d %d\r\n",
                             response_code,
                             ship->armor_slot_1_type, ship->armor_slot_1_value,
                             ship->armor_slot_2_type, ship->armor_slot_2_value);
                    log_activity("GETARMOR", session->username, session->isLoggedIn, payload, response_code);
                } else {
                    response_code = RESP_INTERNAL_ERROR;
                    snprintf(response, sizeof(response), "%d\r\n", response_code);
                    log_activity("GETARMOR", session->username, session->isLoggedIn, payload, response_code);
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
            log_activity("BUYARMOR", session->username, session->isLoggedIn, payload, response_code);
        }
    }
    else if (strcmp(type, "GET_WEAPON") == 0) {
        if(session->isLoggedIn == false) {
            response_code = RESP_NOT_LOGGED;
            snprintf(response, sizeof(response), "%d\r\n", response_code);
            log_activity("GET_WEAPON", session->username, false, payload, response_code);
        } else {
            int match_id = session->current_match_id;
            if (match_id <= 0) {
                match_id = find_current_match_by_username(session->username);
            }
            if (match_id <= 0) {
                response_code = RESP_NOT_IN_MATCH;
                snprintf(response, sizeof(response), "%d\r\n", response_code);
                log_activity("GET_WEAPON", session->username, session->isLoggedIn, payload, response_code);
            } else {
                Ship *ship = find_ship(match_id, session->username);
                if (ship) {
                    response_code = RESP_MATCH_INFO_OK;
                    snprintf(response, sizeof(response), "%d %d %d %d\r\n",
                             response_code,
                             ship->cannon_ammo,
                             ship->laser_count,
                             ship->missile_count);
                    log_activity("GET_WEAPON", session->username, session->isLoggedIn, payload, response_code);
                } else {
                    response_code = RESP_INTERNAL_ERROR;
                    snprintf(response, sizeof(response), "%d\r\n", response_code);
                    log_activity("GET_WEAPON", session->username, session->isLoggedIn, payload, response_code);
                }
            }
        }
    }
    else if (strcmp(type, "BUY_WEAPON") == 0) {
        int weapon_type;
        if (sscanf(payload, "%d", &weapon_type) != 1) {
            response_code = RESP_SYNTAX_ERROR;
            snprintf(response, sizeof(response), "%d\r\n", response_code);
            log_activity("BUY_WEAPON", session->username, false, payload, response_code);
        } else {
            response_code = server_handle_buy_weapon(session, app_context_get_user_table(), weapon_type);
            snprintf(response, sizeof(response), "%d\r\n", response_code);
            log_activity("BUY_WEAPON", session->username, session->isLoggedIn, payload, response_code);
        }
    }
    else if (strcmp(type,"GET_MATCH_RESULT") == 0) {
        int match_id = -1;
        if (payload && sscanf(payload, "%d", &match_id) == 1 && match_id > 0) {
            response_code = server_handle_get_match_result(session, match_id);
        } else {
            response_code = RESP_SYNTAX_ERROR;
        }
        if (response_code == RESP_MATCH_RESULT_OK) {
            int winner = get_match_result(match_id);
            snprintf(response, sizeof(response), "%d %d %d\r\n", response_code, match_id, winner);
        } else {
            snprintf(response, sizeof(response), "%d\r\n", response_code);
        }
        log_activity("GET_MATCH_RESULT", session->username, session->isLoggedIn, payload, response_code);
    }
    else if (strcmp(type, "START_MATCH") == 0) {
        int opponent_team_id = -1;
        if (payload && sscanf(payload, "%d", &opponent_team_id) == 1 && opponent_team_id > 0) {
            response_code = server_handle_start_match(session, opponent_team_id);
        } else {
            response_code = RESP_SYNTAX_ERROR;
        }
        snprintf(response, sizeof(response), "%d\r\n", response_code);
        log_activity("START_MATCH", session->username, session->isLoggedIn, payload, response_code);
    }
    else if (strcmp(type, "END_MATCH") == 0) {
        int match_id = -1;
        if (payload && sscanf(payload, "%d", &match_id) == 1 && match_id > 0) {
            response_code = server_handle_end_match(session, match_id);
        } else {
            response_code = RESP_SYNTAX_ERROR;
        }
        snprintf(response, sizeof(response), "%d\r\n", response_code);
        log_activity("END_MATCH", session->username, session->isLoggedIn, payload, response_code);
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

    // else if (strcmp(type, "FIRE") == 0) {
    //     int target_id, weapon_id;
    //     // Parse: FIRE <target> <weapon>
    //     if (sscanf(payload, "%d %d", &target_id, &weapon_id) == 2) {
    //         FireResult result;
    //         memset(&result, 0, sizeof(FireResult));
            
    //         // Gọi Logic (Lưu ý: session ở đây đã có sẵn từ code gốc, không cần tạo mới)
    //         response_code = server_handle_fire(session, target_id, weapon_id, &result);
            
    //         if (response_code == RESP_COIN_OK) {
    //             // 1. Chuẩn bị response cho người bắn (sẽ được gửi ở cuối hàm)
    //             snprintf(response, sizeof(response), "131 %d %d %d %d\r\n", 
    //                      result.target_id, result.damage_dealt, 
    //                      result.target_remaining_hp, result.target_remaining_armor);
                
    //             // 2. Broadcast cho mọi người khác (Gửi ngay lập tức)
    //             broadcast_fire_event(result.attacker_id, result.target_id, result.damage_dealt, result.target_remaining_hp, result.target_remaining_armor);
    //         } else {
    //             // Gửi lỗi
    //             snprintf(response, sizeof(response), "%d FIRE_FAIL\r\n", response_code);
    //         }
    //     } else {
    //         snprintf(response, sizeof(response), "301 SYNTAX_ERROR\r\n");
    //     }
    // }

    // // --- XỬ LÝ CHALLENGE (Thách đấu) ---
    // else if (strcmp(type, "SEND_CHALLENGE") == 0) {
    //     int target_team = atoi(payload);
    //     int cid = 0;
    //     response_code = server_handle_send_challenge(session, target_team, &cid);
        
    //     if (response_code == RESP_CHALLENGE_SENT)
    //         snprintf(response, sizeof(response), "130 CHALLENGE_SENT %d\r\n", cid);
    //     else 
    //         snprintf(response, sizeof(response), "%d CHALLENGE_FAIL\r\n", response_code);
    // }
    // else if (strcmp(type, "ACCEPT_CHALLENGE") == 0) {
    //     int cid = atoi(payload);
    //     response_code = server_handle_accept_challenge(session, cid);
    //     snprintf(response, sizeof(response), "%d CHALLENGE_ACCEPTED %d\r\n", response_code, cid);
    // }
    // else if (strcmp(type, "DECLINE_CHALLENGE") == 0) {
    //     int cid = atoi(payload);
    //     response_code = server_handle_decline_challenge(session, cid);
    //     snprintf(response, sizeof(response), "%d CHALLENGE_DECLINED %d\r\n", response_code, cid);
    // }
    // else if (strcmp(type, "CANCEL_CHALLENGE") == 0) {
    //     int cid = atoi(payload);
    //     response_code = server_handle_cancel_challenge(session, cid);
    //     snprintf(response, sizeof(response), "%d CHALLENGE_CANCELED %d\r\n", response_code, cid);
    // }

    // // --- XỬ LÝ CHEST (Rương) ---
    // else if (strcmp(type, "CHEST_OPEN") == 0) {
    //     int chest_id;
    //     char answer[128];
    //     if (sscanf(payload, "%d %[^\n]", &chest_id, answer) == 2) {
    //         response_code = server_handle_open_chest(session, chest_id, answer);
            
    //         if (response_code == RESP_CHEST_OPEN_OK)
    //             snprintf(response, sizeof(response), "127 CHEST_OPEN_SUCCESS\r\n");
    //         else
    //             snprintf(response, sizeof(response), "%d CHEST_OPEN_FAIL\r\n", response_code);
    //         log_activity("CHEST_OPEN", session->username, session->isLoggedIn, payload, response_code);
    //     } else {
    //         snprintf(response, sizeof(response), "301 SYNTAX_ERROR\r\n");
    //         log_activity("CHEST_OPEN", session->username, session->isLoggedIn, payload, RESP_SYNTAX_ERROR);
    //     }
    // }

    // ========== TEAM COMMANDS (moved from server.c) ==========
    else if (strcmp(type, "CREATE_TEAM") == 0 || strcmp(type, "CREATETEAM") == 0) {
        if (!payload || strlen(payload) == 0) {
            response_code = RESP_SYNTAX_ERROR;
        } else {
            response_code = handle_create_team(session, app_context_get_user_table(), payload);
        }
        snprintf(response, sizeof(response), "%d\r\n", response_code);
        log_activity("CREATE_TEAM", session->username, session->isLoggedIn, payload, response_code);
    }
    else if (strcmp(type, "DELETE_TEAM") == 0) {
        response_code = handle_delete_team(session);
        snprintf(response, sizeof(response), "%d\r\n", response_code);
        log_activity("DELETE_TEAM", session->username, session->isLoggedIn, payload, response_code);
    }
    else if (strcmp(type, "LIST_TEAMS") == 0) {
        char list_buf[4096] = "";
        response_code = handle_list_teams(session, list_buf, sizeof(list_buf));
        if (response_code == RESP_LIST_TEAMS_OK && strlen(list_buf) > 0)
            snprintf(response, sizeof(response), "%s\r\n", list_buf);
        else
            snprintf(response, sizeof(response), "%d\r\n", response_code);
        log_activity("LIST_TEAMS", session->username, session->isLoggedIn, payload, response_code);
    }
    else if (strcmp(type, "JOIN_REQUEST") == 0) {
        if (!payload || strlen(payload) == 0) response_code = RESP_SYNTAX_ERROR;
        else response_code = handle_join_request(session, payload);
        snprintf(response, sizeof(response), "%d\r\n", response_code);
        log_activity("JOIN_REQUEST", session->username, session->isLoggedIn, payload, response_code);
    }
    else if (strcmp(type, "JOIN_APPROVE") == 0) {
        if (!payload || strlen(payload) == 0) response_code = RESP_SYNTAX_ERROR;
        else response_code = handle_join_approve(session, payload, app_context_get_user_table());
        snprintf(response, sizeof(response), "%d\r\n", response_code);
        log_activity("JOIN_APPROVE", session->username, session->isLoggedIn, payload, response_code);
    }
    else if (strcmp(type, "JOIN_REJECT") == 0) {
        if (!payload || strlen(payload) == 0) response_code = RESP_SYNTAX_ERROR;
        else response_code = handle_join_reject(session, payload);
        snprintf(response, sizeof(response), "%d\r\n", response_code);
        log_activity("JOIN_REJECT", session->username, session->isLoggedIn, payload, response_code);
    }
    else if (strcmp(type, "TEAM_MEMBER_LIST") == 0) {
        char members_buf[2048] = "";
        response_code = handle_team_member_list(session, members_buf, sizeof(members_buf));
        if (response_code == RESP_TEAM_MEMBERS_LIST_OK)
            snprintf(response, sizeof(response), "%d %s\r\n", response_code, members_buf);
        else
            snprintf(response, sizeof(response), "%d\r\n", response_code);
        log_activity("TEAM_MEMBER_LIST", session->username, session->isLoggedIn, payload, response_code);
    }
    else if (strcmp(type, "LEAVE_TEAM") == 0) {
        response_code = handle_leave_team(session);
        snprintf(response, sizeof(response), "%d\r\n", response_code);
        log_activity("LEAVE_TEAM", session->username, session->isLoggedIn, payload, response_code);
    }
    else if (strcmp(type, "KICK_MEMBER") == 0) {
        if (!payload || strlen(payload) == 0) response_code = RESP_SYNTAX_ERROR;
        else response_code = handle_kick_member(session, payload);
        snprintf(response, sizeof(response), "%d\r\n", response_code);
        log_activity("KICK_MEMBER", session->username, session->isLoggedIn, payload, response_code);
    }
    else if (strcmp(type, "INVITE") == 0) {
        if (!payload || strlen(payload) == 0) response_code = RESP_SYNTAX_ERROR;
        else response_code = handle_invite(session, payload, app_context_get_user_table());
        snprintf(response, sizeof(response), "%d\r\n", response_code);
        log_activity("INVITE", session->username, session->isLoggedIn, payload, response_code);
    }
    else if (strcmp(type, "INVITE_ACCEPT") == 0) {
        if (!payload || strlen(payload) == 0) response_code = RESP_SYNTAX_ERROR;
        else response_code = handle_invite_accept(session, payload);
        snprintf(response, sizeof(response), "%d\r\n", response_code);
        log_activity("INVITE_ACCEPT", session->username, session->isLoggedIn, payload, response_code);
    }
    else if (strcmp(type, "INVITE_REJECT") == 0) {
        if (!payload || strlen(payload) == 0) response_code = RESP_SYNTAX_ERROR;
        else response_code = handle_invite_reject(session, payload);
        snprintf(response, sizeof(response), "%d\r\n", response_code);
        log_activity("INVITE_REJECT", session->username, session->isLoggedIn, payload, response_code);
    }

    // ========== REPAIR ==========
    else if (strcmp(type, "REPAIR") == 0) {
        int amt = -1;
        if (payload && sscanf(payload, "%d", &amt) == 1) {
            RepairResult repair_result = {0};
            response_code = server_handle_repair(session, app_context_get_user_table(), amt, &repair_result);
            if (response_code == RESP_REPAIR_OK)
                snprintf(response, sizeof(response), "%d %d %d\r\n", response_code, repair_result.hp, repair_result.coin);
            else
                snprintf(response, sizeof(response), "%d\r\n", response_code);
        } else {
            response_code = RESP_SYNTAX_ERROR;
            snprintf(response, sizeof(response), "%d\r\n", response_code);
        }
        log_activity("REPAIR", session->username, session->isLoggedIn, payload, response_code);
    }

    // ========== MATCH INFO ==========
    else if (strcmp(type, "MATCH_INFO") == 0) {
        int match_id = -1;
        if (payload && sscanf(payload, "%d", &match_id) == 1) {
            char match_info[4096] = {0};
            response_code = server_handle_match_info(match_id, match_info, sizeof(match_info), app_context_get_user_table());
            printf("[DEBUG] MATCH_INFO response_code=%d, info_len=%zu\n", response_code, strlen(match_info));
            if (response_code == RESP_MATCH_INFO_OK) {
                // Replace newlines with | to send as single line
                for (int i = 0; match_info[i] != '\0'; i++) {
                    if (match_info[i] == '\n') match_info[i] = '|';
                }
                int written = snprintf(response, sizeof(response), "%d %s\r\n", response_code, match_info);
                printf("[DEBUG] Response length: %d bytes, response: %.100s...\n", written, response);
            } else {
                snprintf(response, sizeof(response), "%d\r\n", response_code);
            }
        } else {
            response_code = RESP_SYNTAX_ERROR;
            snprintf(response, sizeof(response), "%d\r\n", response_code);
        }
        log_activity("MATCH_INFO", session->username, session->isLoggedIn, payload, response_code);
    }

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
