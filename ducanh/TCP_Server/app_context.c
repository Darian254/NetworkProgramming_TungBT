#include "app_context.h"
#include "users_io.h"
#include "session.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * @file app_context.c
 * @brief Global application state implementation
 */

// TODO: Declare global variables here
// These will be accessed by router.c and other modules
static UserTable *g_user_table = NULL;

int app_context_init(void) {
    // TODO: Step 1 - Initialize user hash table
    g_user_table = initUserTable(HASH_SIZE);
    if (!g_user_table) {
        fprintf(stderr, "[ERROR] Failed to allocate user table.\n");
        return -1;
    }

    // TODO: Step 2 - Load users from file
    UserIOStatus status = loadUsers(g_user_table, USERS_FILE);
    if (status != USER_IO_OK) {
        fprintf(stderr, "[ERROR] Failed to load users (status=%d).\n", status);
        freeUserTable(g_user_table);
        g_user_table = NULL;
        return -1;
    }
    printf("[INFO] Loaded users successfully.\n");

    // TODO: Step 3 - Initialize session manager
    init_session_manager();
    printf("[INFO] Session manager initialized.\n");

    return 0;
}

void app_context_cleanup(void) {
    // TODO: Save users to file before shutdown (optional)
    // Uncomment if you want to persist changes:
    // if (g_user_table) {
    //     saveUsers(g_user_table, USERS_FILE);
    // }

    // TODO: Cleanup session manager
    cleanup_session_manager();

    // TODO: Free user table
    if (g_user_table) {
        freeUserTable(g_user_table);
        g_user_table = NULL;
    }

    printf("[INFO] Application context cleaned up.\n");
}

UserTable* app_context_get_user_table(void) {
    // TODO: Return global user table
    return g_user_table;
}
