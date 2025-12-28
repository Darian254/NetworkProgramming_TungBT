#ifndef SESSION_H
#define SESSION_H

#include <stdbool.h>
#include <netinet/in.h>
#include "users.h"

/**
 * @def MAX_SESSIONS
 * @brief Maximum number of concurrent active sessions the server will accept.
 *
 * This is an application-level guard to prevent resource exhaustion.
 * When the limit is reached new connections can be rejected gracefully 
 * with RESP_SERVER_BUSY instead of crashing.
 */
#define MAX_SESSIONS 4096

/**
 * @struct Session
 * @brief Stores the current user session state.
 */
typedef struct {
  bool isLoggedIn; /**< Login status flag */
  char username[MAX_USERNAME]; /**< Username of the currently logged-in account */
} Session;

/**
 * @struct ServerSession
 * @brief Stores the TCP server session state for a single client.
 */
typedef struct {
    bool isLoggedIn;
    char username[MAX_USERNAME];
    int socket_fd;              /**< Socket identifier on server */
    struct sockaddr_in client_addr; /**< Client address */
    int current_match_id;       /**< Current match ID, -1 if not in match */
    int current_team_id;        /**< Current team ID, -1 if not in team */
} ServerSession;

/**
 * @struct SessionNode
 * @brief Node in the linked list of active sessions.
 */
typedef struct SessionNode {
    ServerSession session;
    struct SessionNode *next;
} SessionNode;

/**
 * @struct SessionManager
 * @brief Global session manager to track all active sessions.
 * Uses simple linked list (no mutex needed for single-threaded select()).
 */
typedef struct {
    SessionNode *head;          /**< Head of the session linked list */
    int count;                  /**< Number of active sessions */
} SessionManager;

/**
 * @brief Initialize a new session (default: not logged in).
 * 
 * @param s Pointer to the Session to be initialized
 */
void initSession(Session * s);

/**
 * @brief Attempt to log in with a username from user input.
 *
 * - If already logged in → show error.  
 * - If the user does not exist or is banned → fail.  
 * - If valid → update Session with login state.
 *
 * @param s Pointer to the Session
 * @param ut Pointer to the UserTable containing users
 * @return true if login succeeds, false otherwise
 */
bool login(Session * s, UserTable * ut);

/**
 * @brief Log out from the current session (if logged in).
 * 
 * @param s Pointer to the Session
 */
void logout(Session * s);

/**
 * @brief Check whether the user is currently logged in.
 * 
 * @param s Pointer to the Session
 * @return true if logged in, false if not
 */
bool isLoggedIn(const Session * s);

/* ====== TCP Server Session Functions ====== */

/**
 * @brief Initialize a server session
 * 
 * @param s Pointer to the ServerSession
 */
void initServerSession(ServerSession *s);

/**
 * @brief Handle USER command for TCP server
 * Performs login validation including checking if user exists,
 * is not banned, session not already logged in, and username not
 * logged in elsewhere. Also validates password.
 * 
 * @param session Pointer to the ServerSession
 * @param ut Pointer to the UserTable containing users
 * @param username Username from command
 * @param password Plain text password from command
 * @return Response code (110 = success, 211 = banned, 212 = not exist, 
 *         213 = already logged in, 214 = account logged in elsewhere, 218 = wrong password)
 */
int server_handle_login(ServerSession *session, UserTable *ut, const char *username, const char *password);

/**
 * @brief Handle REGISTER command for TCP server
 * Creates a new user account with password validation
 * 
 * @param ut Pointer to the UserTable containing users
 * @param username Username from command
 * @param password Plain text password from command
 * @return Response code (115 = success, 215 = exists, 216 = invalid username, 217 = invalid password)
 */
int server_handle_register(UserTable *ut, const char *username, const char *password);

/**
 * @brief Handle BYE command for TCP server
 * 
 * @param session Pointer to the ServerSession
 * @return Response code (134, 315, 301)
 */
int server_handle_bye(ServerSession *session);

/**
 * @brief Handle WHOAMI command for TCP server
 * Returns the current logged-in username
 * 
 * @param session Pointer to the ServerSession
 * @param username_out Buffer to store username (output)
 * @return Response code (100 = success, 221 = not logged in)
 */
int server_handle_whoami(ServerSession *session, char *username_out);

/**
 * @brief Handle BUYARMOR command for TCP server
 * Purchases armor for the player's ship in current match
 * 
 * @param session Pointer to the ServerSession
 * @param ut Pointer to the UserTable (for coin deduction)
 * @param armor_type Armor type to purchase (1=basic, 2=enhanced)
 * @return Response code:
 *   - RESP_BUY_ITEM_OK (334): Success
 *   - RESP_NOT_LOGGED (221): Not logged in
 *   - RESP_NOT_IN_MATCH (223): Not in any running match
 *   - RESP_ARMOR_NOT_FOUND (520): Invalid armor type
 *   - RESP_NOT_ENOUGH_COIN (521): Insufficient coins
 *   - RESP_ARMOR_SLOT_FULL (522): Both armor slots occupied
 *   - RESP_INTERNAL_ERROR (500): Ship not found or other error
 */
int server_handle_buyarmor(ServerSession *session, UserTable *ut, int armor_type);

/**
 * @brief Check if session is logged in
 * 
 * @param session Pointer to the ServerSession
 * @return true if logged in, false otherwise
 */
bool server_is_logged_in(ServerSession *session);

/* ====== Session Manager Functions ====== */

/**
 * @brief Initialize the global session manager
 * Must be called once before using any session manager functions
 */
void init_session_manager(void);

/**
 * @brief Clean up the session manager (free all resources)
 * Should be called when server is shutting down
 */
void cleanup_session_manager(void);

/**
 * @brief Find a session by username
 * 
 * @param username Username to search for
 * @return Pointer to SessionNode if found, NULL otherwise
 */
SessionNode *find_session_by_username(const char *username);

/**
 * @brief Find a session by socket file descriptor
 * 
 * @param socket_fd Socket file descriptor
 * @return Pointer to SessionNode if found, NULL otherwise
 */

int get_fd_by_username(const char *username);
/**
 * 
 */
SessionNode *find_session_by_socket(int socket_fd);

/**
 * @brief Add a new session to the manager
 * 
 * @param session Pointer to the session to add
 * @return true if added successfully, false if session already exists
 */
bool add_session(ServerSession *session);

/**
 * @brief Remove a session from the manager by socket
 * 
 * @param socket_fd Socket file descriptor of the session to remove
 * @return true if removed successfully, false if not found
 */
bool remove_session_by_socket(int socket_fd);

/**
 * @brief Remove a session from the manager by username
 * 
 * @param username Username of the session to remove
 * @return true if removed successfully, false if not found
 */
bool remove_session_by_username(const char *username);

/**
 * @brief Update session in manager by socket
 * 
 * @param socket_fd Socket file descriptor
 * @param session Pointer to updated session data
 * @return true if updated successfully, false if not found
 */
bool update_session_by_socket(int socket_fd, ServerSession *session);

/**
 * @brief Get current number of active sessions.
 * @return Count of active sessions.
 */
int get_active_session_count(void);

#endif
