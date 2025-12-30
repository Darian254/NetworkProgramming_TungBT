#ifndef SESSION_H
#define SESSION_H

#include <stdbool.h>
#include <netinet/in.h>
#include "users.h"
#include "db_schema.h"

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
    int current_team_id;        /**< Current team ID, -1 if not in team */
    int current_match_id;       /**< Current match ID, -1 if not in match */
    int coin; //Lượng thêm
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
 * @brief Handle REPAIR command for TCP server
 * Repairs the player's ship in the current match.
 *
 * @param session Pointer to the ServerSession
 * @param ut Pointer to the UserTable (for coin deduction)
 * @param repair_amount Amount of HP to repair (from client)
 * @param out Pointer to RepairResult to store new HP and coin (output)
 * @return Response code:
 *   - RESP_REPAIR_OK (132): Success, returns <newHP> <newCoin>
 *   - RESP_NOT_LOGGED (315): Not logged in
 *   - RESP_NOT_IN_MATCH (503): Not in any running match
 *   - RESP_ALREADY_FULL_HP (340): HP is already full
 *   - RESP_NOT_ENOUGH_COIN (521): Insufficient coins
 *   - RESP_INTERNAL_ERROR (500): Internal error (ship/user not found)
 *   - RESP_DATABASE_ERROR (501): Database error (if any DB op fails)
 */
int server_handle_repair(ServerSession *session, UserTable *ut, int repair_amount, RepairResult *out);

/**
 * @brief Handle BUYWEAPON command for TCP server
 * Purchases weapon ammo for the player's ship in current match
 * 
 * @param session Pointer to the ServerSession
 * @param ut Pointer to the UserTable (for coin deduction)
 * @param weapon_type Weapon type to purchase (0=cannon, 1=laser, 2=missile)
 * @return Response code:
 *   - RESP_BUY_ITEM_OK (334): Success
 *   - RESP_NOT_LOGGED (221): Not logged in
 *   - RESP_NOT_IN_MATCH (223): Not in any running match
 *   - RESP_INTERNAL_ERROR (500): Ship not found or other error
 *   - RESP_NOT_ENOUGH_COIN (521): Insufficient coins
 *   - RESP_BUY_ITEM_FAILED (523): Purchase failed (e.g. maxed out)
 */
int server_handle_buy_weapon(ServerSession *session, UserTable *ut, int weapon_type);

/**
 * @brief Handle START_MATCH command - creates custom match with specified opponent
 * Both teams should coordinate beforehand (via chat/external means) to exchange team IDs
 *
 * @param session Current session
 * @param opponent_team_id ID of the opponent team to play against
 * @return Response code:
 *   - RESP_SYNTAX_ERROR (301): Invalid opponent_team_id or self-match
 *   - RESP_NOT_LOGGED (315): User not logged in
 *   - RESP_NOT_CREATOR (316): User is not team creator
 *   - RESP_TEAM_NOT_FOUND (410): User's team not found
 *   - RESP_OPPONENT_NOT_FOUND (411): Opponent team not found
 *   - RESP_TEAM_IN_MATCH (412): Either team already in match
 *   - RESP_MATCH_CREATE_FAILED (413): Match creation failed
 *   - RESP_START_MATCH_OK (126): Success
 */
int server_handle_start_match(ServerSession *session, int opponent_team_id);

/**
 * @brief Handle GET_MATCH_RESULT command - retrieves match result
 * @param session Current session
 * @param match_id Match ID to query
 * @return RESP_MATCH_RESULT_OK (203) if finished, or error code
 */
int server_handle_get_match_result(ServerSession *session, int match_id);

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
