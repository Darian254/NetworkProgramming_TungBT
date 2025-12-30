#include "session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "util.h"
#include "config.h"
#include "hash.h"
#include "users.h"
#include "users_io.h"
#include "db_schema.h"  // For FILE_USERS
#include "db.c"         // For find_current_match_by_username()

/* Global session manager with mutex for thread safety */
static SessionManager session_mgr = {NULL, 0};
static pthread_mutex_t session_mutex = PTHREAD_MUTEX_INITIALIZER;

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
    // khong can thiet
    // if (match_id <= 0) {
    //     /* Fallback */
    //     match_id = find_current_match_by_username(session->username);
    //     if (match_id > 0) {
    //         session->current_match_id = match_id; 
    //     }
    // }
    
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

int server_handle_repair(ServerSession *session, UserTable *ut, int repair_amount, RepairResult *out) {
    if (!session || !ut) {
        return RESP_INTERNAL_ERROR;
    }

    if (!session->isLoggedIn) {
        return RESP_NOT_LOGGED;
    }

    if (repair_amount <= 0) {
        return RESP_SYNTAX_ERROR;
    }

    int match_id = session->current_match_id;
    if (match_id <= 0) { 
        return RESP_NOT_IN_MATCH;
    }

    Ship *ship = find_ship(session->current_match_id, session->username);
    User *user = findUser(ut, session->username);

    if (!ship || !user) {
        return RESP_INTERNAL_ERROR;
    }

    int maxHP = SHIP_DEFAULT_HP;
    int currentHP = ship->hp;

    if (currentHP >= maxHP) {
        return RESP_ALREADY_FULL_HP;
    }

    int missingHP = maxHP - currentHP;
    int actualRepair = (repair_amount < missingHP) ? repair_amount : missingHP;
    int cost = actualRepair;

    if (user->coin < cost) { 
        return RESP_NOT_ENOUGH_COIN;
    }

    /* Apply changes */
    ship->hp += actualRepair;
    user->coin -= cost;

    if (out) {
        out->hp = ship->hp;
        out->coin = user->coin;
    }

    // TODO: persist ship & user to DB
    return RESP_REPAIR_OK;
}


/* ====== Session Manager Implementation ====== */

void init_session_manager(void) {
    /* Initialize session manager */
    session_mgr.head = NULL;
    session_mgr.count = 0;
}

void cleanup_session_manager(void) {
    pthread_mutex_lock(&session_mutex);
    SessionNode *current = session_mgr.head;
    while (current != NULL) {
        SessionNode *next = current->next;
        free(current);
        current = next;
    }
    session_mgr.head = NULL;
    session_mgr.count = 0;
    pthread_mutex_unlock(&session_mutex);
}

SessionNode *find_session_by_username(const char *username) {
    if (!username) return NULL;
    
    pthread_mutex_lock(&session_mutex);
    SessionNode *current = session_mgr.head;
    SessionNode *result = NULL;
    while (current != NULL) {
        if (current->session.isLoggedIn && 
            strcmp(current->session.username, username) == 0) {
            result = current;
            break;
        }
        current = current->next;
    }
    pthread_mutex_unlock(&session_mutex);
    return result;
}

SessionNode *find_session_by_socket(int socket_fd) {
    pthread_mutex_lock(&session_mutex);
    SessionNode *current = session_mgr.head;
    SessionNode *result = NULL;
    while (current != NULL) {
        if (current->session.socket_fd == socket_fd) {
            result = current;
            break;
        }
        current = current->next;
    }
    pthread_mutex_unlock(&session_mutex);
    return result;
}

bool add_session(ServerSession *session) {
    if (!session || session->socket_fd < 0) return false;
    
    pthread_mutex_lock(&session_mutex);
    
    /* Check if session with this socket already exists */
    SessionNode *existing = session_mgr.head;
    while (existing != NULL) {
        if (existing->session.socket_fd == session->socket_fd) {
            pthread_mutex_unlock(&session_mutex);
            return false; /* Already exists */
        }
        existing = existing->next;
    }
    
    /* Create new session node */
    SessionNode *new_node = (SessionNode *)malloc(sizeof(SessionNode));
    if (!new_node) {
        pthread_mutex_unlock(&session_mutex);
        return false;
    }
    
    new_node->session = *session;
    new_node->next = session_mgr.head;
    session_mgr.head = new_node;
    session_mgr.count++;
    
    pthread_mutex_unlock(&session_mutex);
    return true;
}

bool remove_session_by_socket(int socket_fd) {
    pthread_mutex_lock(&session_mutex);
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
            pthread_mutex_unlock(&session_mutex);
            return true;
        }
        prev = current;
        current = current->next;
    }
    
    pthread_mutex_unlock(&session_mutex);
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
    
    pthread_mutex_lock(&session_mutex);
    SessionNode *current = session_mgr.head;
    while (current != NULL) {
        if (current->session.socket_fd == socket_fd) {
            current->session = *session;
            pthread_mutex_unlock(&session_mutex);
            return true;
        }
        current = current->next;
    }
    pthread_mutex_unlock(&session_mutex);
    
    return false;
}

int get_active_session_count(void) {
    pthread_mutex_lock(&session_mutex);
    int count = session_mgr.count;
    pthread_mutex_unlock(&session_mutex);
    return count;
}
