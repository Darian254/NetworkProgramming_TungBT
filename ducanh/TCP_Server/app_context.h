#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include "users.h"

/**
 * @file app_context.h
 * @brief Global application state and initialization
 * 
 * Manages shared resources accessed by multiple modules:
 * - User hash table (single-threaded access)
 * - Configuration
 * - Session manager initialization
 */

/**
 * @brief Initialize global application context
 * 
 * Must be called once before server starts accepting connections.
 * Loads user database, initializes session manager.
 * 
 * @return 0 on success, -1 on failure
 * 
 * TODO: Implement this to:
 * 1. Call initUserTable(HASH_SIZE) and store in g_user_table
 * 2. Call loadUsers(g_user_table, USERS_FILE)
 * 3. Call init_session_manager() from session.h
 * 4. Return 0 if all succeed, -1 if any fail
 */
int app_context_init(void);

/**
 * @brief Cleanup global resources
 * 
 * Called on server shutdown to free memory and save state.
 * 
 * TODO: Implement this to:
 * 1. Call saveUsers(g_user_table, USERS_FILE) if needed
 * 2. Call cleanup_session_manager()
 * 3. Call freeUserTable(g_user_table)
 */
void app_context_cleanup(void);

/**
 * @brief Get global user table
 * 
 * Returns pointer to the shared user hash table.
 * Single-threaded epoll access, no locking needed.
 * 
 * @return Pointer to global UserTable
 */
UserTable* app_context_get_user_table(void);

#endif // APP_CONTEXT_H
