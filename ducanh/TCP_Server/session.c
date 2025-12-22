#include "session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "config.h"
#include "hash.h"
#include "users.h"
#include "users_io.h"
#include "db_schema.h"  // For FILE_USERS

/* Global session manager (single-threaded epoll, no locking needed) */
static SessionManager session_mgr = {NULL, 0};

void initServerSession(ServerSession *s) {
    if (!s) return;
    s->isLoggedIn = false;
    s->username[0] = '\0';
    s->socket_fd = -1;
    memset(&s->client_addr, 0, sizeof(s->client_addr));
    s->current_match_id = -1;
    s->current_team_id = -1;
}

int server_handle_login(ServerSession *session, UserTable *ut, const char *username, const char *password) {
    if (!session || !ut || !username || !password) {
        return RESP_SYNTAX_ERROR;
    }
    
    /* Check if already logged in on this session */
    if (session->isLoggedIn) {
        return RESP_ALREADY_LOGGED;
    }
    
    /* Find user */
    User *user = findUser(ut, username);
    if (!user) {
        return RESP_ACCOUNT_NOT_FOUND;
    }
    
    if (user->status == USER_BANNED) {
        return RESP_ACCOUNT_LOCKED;
    }
    
    /* Validate password */
    if (!verifyPassword(password, user->password_hash)) {
        return RESP_WRONG_PASSWORD;
    }
    
    /* Login successful - update session */
    session->isLoggedIn = true;
    strncpy(session->username, user->username, MAX_USERNAME - 1);
    session->username[MAX_USERNAME - 1] = '\0';
    
    /* Update session in manager */
    if (!update_session_by_socket(session->socket_fd, session)) {
        /* If update failed, try to add new session */
        add_session(session);
    }
    
    return RESP_LOGIN_OK;
}

int server_handle_register(UserTable *ut, const char *username, const char *password) {
    if (!ut || !username || !password) {
        return RESP_SYNTAX_ERROR;
    }
    
    /* Validate username format */
    if (!validateUsername(username)) {
        return RESP_INVALID_USERNAME;
    }
    
    /* Validate password format */
    if (!validatePassword(password)) {
        return RESP_WEAK_PASSWORD;
    }
    
    /* Check if username already exists */
    if (findUser(ut, username)) {
        return RESP_USERNAME_EXISTS;
    }
    
    /* Hash password */
    char password_hash[MAX_PASSWORD_HASH];
    hashPassword(password, password_hash);
    
    /* Create new user */
    User *user = createUser(ut, username, password_hash);
    if (!user) {
        return RESP_INTERNAL_ERROR;
    }
    
    /* Save to file for persistence */
    saveUsers(ut, FILE_USERS);
    
    return RESP_REGISTER_OK;
}

int server_handle_bye(ServerSession *session) {
    if (!session) {
        return RESP_SYNTAX_ERROR;
    }
    
    if (!session->isLoggedIn) {
        return RESP_NOT_LOGGED;
    }
    
    /* Update local session first */
    session->isLoggedIn = false;
    session->username[0] = '\0';
    
    /* Update session in manager */
    update_session_by_socket(session->socket_fd, session);
    
    return RESP_LOGOUT_OK;
}

bool server_is_logged_in(ServerSession *session) {
    return session && session->isLoggedIn;
}

int server_handle_whoami(ServerSession *session, char *username_out) {
    if (!session || !username_out) {
        return RESP_SYNTAX_ERROR;
    }
    
    if (!session->isLoggedIn) {
        return RESP_NOT_LOGGED;
    }
    
    strncpy(username_out, session->username, MAX_USERNAME - 1);
    username_out[MAX_USERNAME - 1] = '\0';
    
    return RESP_WHOAMI_OK;
}

int server_handle_buyarmor(ServerSession *session, UserTable *ut, int armor_type) {
    if (!session || !ut) {
        return RESP_SYNTAX_ERROR;
    }
    
    if (!session->isLoggedIn) {
        return RESP_NOT_LOGGED;
    }
    
    int match_id = session->current_match_id;
    if (match_id <= 0) {
        /* Fallback */
        match_id = find_current_match_by_username(session->username);
        if (match_id > 0) {
            session->current_match_id = match_id; 
        }
    }
    
    if (match_id <= 0) {
        return RESP_NOT_IN_MATCH;
    }
    
    if (armor_type < ARMOR_BASIC || armor_type > ARMOR_ENHANCED) {
        return RESP_ARMOR_NOT_FOUND;
    }
    
    Ship *ship = find_ship(match_id, session->username);
    if (!ship) {
        return RESP_INTERNAL_ERROR; 
    }
         
    return ship_buy_armor(ut, ship, session->username, armor_type);
}

int server_handle_start_match(ServerSession *session, int opponent_team_id) {
    // 1. Validate input
    if (!session) {
        return RESP_SYNTAX_ERROR;
    }
    
    if (opponent_team_id <= 0) {
        return RESP_SYNTAX_ERROR;
    }
    
    // 2. Check if user is logged in
    if (!session->isLoggedIn) {
        return RESP_NOT_LOGGED;
    }
    
    // 3. Get user's current team
    int user_team_id = session->current_team_id;
    if (user_team_id <= 0) {
        // Fallback: try to find team by username
        user_team_id = find_team_id_by_username(session->username);
        if (user_team_id > 0) {
            session->current_team_id = user_team_id;
        }
        else {
            return RESP_TEAM_NOT_FOUND; // 4. Validate user's team exists
        }
    }
    
    // 5. Validate user's team exists
    Team *user_team = find_team_by_id(user_team_id);
    if (!user_team) {
        return RESP_TEAM_NOT_FOUND;
    }
    
    // 6. Check if user is the team creator
    int user_id = (int)hashFunc(session->username);
    if (user_team->creator_id != user_id) {
        return RESP_NOT_CREATOR;
    }
    
    // 7. Check if trying to match with own team
    if (opponent_team_id == user_team_id) {
        return RESP_SYNTAX_ERROR;
    }
    
    // 8. Validate opponent team exists
    Team *opponent_team = find_team_by_id(opponent_team_id);
    if (!opponent_team) {
        return RESP_OPPONENT_NOT_FOUND;
    }
    
    // 9. Check if user's team is already in a match
    int existing_match = find_running_match_by_team(user_team_id);
    if (existing_match >= 0) {
        return RESP_TEAM_IN_MATCH;
    }
    
    // 10. Check if opponent team is already in a match
    existing_match = find_running_match_by_team(opponent_team_id);
    if (existing_match >= 0) {
        return RESP_TEAM_IN_MATCH;
    }
    
    // 11. Create the match
    Match *new_match = create_match(user_team_id, opponent_team_id);
    if (!new_match) {
        return RESP_MATCH_CREATE_FAILED;
    }
    
    // 12. Update session with new match ID
    session->current_match_id = new_match->match_id;
    update_session_by_socket(session->socket_fd, session);
    
    // 13. Success
    return RESP_START_MATCH_OK;
}

int server_handle_get_match_result(ServerSession *session, int match_id) {
    if (!session) return RESP_SYNTAX_ERROR;
    if (!session->isLoggedIn) return RESP_NOT_LOGGED;
    if (match_id <= 0) return RESP_SYNTAX_ERROR;
    
    // Find match by ID
    Match *match = find_match_by_id(match_id);
    if (!match) {
        return RESP_MATCH_NOT_FOUND;
    }
    
    // Verify user is in this match (either team1 or team2)
    int user_team_id = session->current_team_id;
    if (user_team_id <= 0) {
        user_team_id = find_team_id_by_username(session->username);
    }
    
    if (user_team_id != match->team1_id && user_team_id != match->team2_id) {
        return RESP_NOT_AUTHORIZED;
    }
    
    // Check match status
    if (match->status == MATCH_RUNNING) {
        return RESP_MATCH_RUNNING;
    }
    
    if (match->status == MATCH_FINISHED) {
        return RESP_MATCH_RESULT_OK;
    }
    
    return RESP_INTERNAL_ERROR;
}

/* ====== Session Manager Implementation ====== */

void init_session_manager(void) {
    /* Initialize session manager */
    session_mgr.head = NULL;
    session_mgr.count = 0;
}

void cleanup_session_manager(void) {
    SessionNode *current = session_mgr.head;
    while (current != NULL) {
        SessionNode *next = current->next;
        free(current);
        current = next;
    }
    session_mgr.head = NULL;
    session_mgr.count = 0;
}


SessionNode *find_session_by_socket(int socket_fd) {
    SessionNode *current = session_mgr.head;
    SessionNode *result = NULL;
    while (current != NULL) {
        if (current->session.socket_fd == socket_fd) {
            result = current;
            break;
        }
        current = current->next;
    }
    return result;
}

bool add_session(ServerSession *session) {
    if (!session || session->socket_fd < 0) return false;
    
    /* Check if session with this socket already exists */
    SessionNode *existing = session_mgr.head;
    while (existing != NULL) {
        if (existing->session.socket_fd == session->socket_fd) {
            return false; /* Already exists */
        }
        existing = existing->next;
    }
    
    /* Create new session node */
    SessionNode *new_node = (SessionNode *)malloc(sizeof(SessionNode));
    if (!new_node) {
        return false;
    }
    
    new_node->session = *session;
    new_node->next = session_mgr.head;
    session_mgr.head = new_node;
    session_mgr.count++;
    
    return true;
}

bool remove_session_by_socket(int socket_fd) {
    SessionNode *current = session_mgr.head;
    SessionNode *prev = NULL;
    
    while (current != NULL) {
        if (current->session.socket_fd == socket_fd) {
            if (prev == NULL) {
                session_mgr.head = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            session_mgr.count--;
            return true;
        }
        prev = current;
        current = current->next;
    }
    
    return false;
}

bool remove_session_by_username(const char *username) {
    if (!username) return false;
    
    SessionNode *current = session_mgr.head;
    SessionNode *prev = NULL;
    
    while (current != NULL) {
        if (current->session.isLoggedIn && 
            strcmp(current->session.username, username) == 0) {
            if (prev == NULL) {
                session_mgr.head = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            session_mgr.count--;
            return true;
        }
        prev = current;
        current = current->next;
    }
    
    return false;
}

bool update_session_by_socket(int socket_fd, ServerSession *session) {
    if (!session || socket_fd < 0) return false;
    
    SessionNode *current = session_mgr.head;
    while (current != NULL) {
        if (current->session.socket_fd == socket_fd) {
            current->session = *session;
            return true;
        }
        current = current->next;
    }
    
    return false;
}

int get_active_session_count(void) {
    return session_mgr.count;
}
