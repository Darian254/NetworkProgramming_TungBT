/**
 * ============================================================================
 * USERS I/O MODULE
 * ============================================================================
 * 
 * File I/O operations for users.
 * 
 * File: users.txt
 * Format: <username> <password_hash> <status> <coin> <created_at> <updated_at>
 * ============================================================================
 */

#ifndef USERS_IO_H
#define USERS_IO_H

#include "users.h"
#include <stdbool.h>

/* ============================================================================
 * STATUS CODES
 * ============================================================================ */
typedef enum {
    USER_IO_OK = 0,             /**< Operation completed successfully */
    USER_IO_FILE_ERROR,         /**< Failed to open file */
    USER_IO_MEMORY_ERROR,       /**< Memory allocation failure */
    USER_IO_FORMAT_ERROR        /**< Invalid data format in file */
} UserIOStatus;

/* ============================================================================
 * FILE OPERATIONS
 * ============================================================================ */

/**
 * @brief Load users from a text file into the hash table.
 * 
 * File format (each line):
 *   <username> <password_hash> <status> <coin> <created_at> <updated_at>
 * 
 * Lines starting with '#' are comments and skipped.
 * 
 * @param ut Pointer to the user hash table.
 * @param filename Path to the users file.
 * @return UserIOStatus code indicating success or type of failure.
 */
UserIOStatus loadUsers(UserTable *ut, const char *filename);

/**
 * @brief Save users from hash table to a text file.
 * 
 * @param ut Pointer to the user hash table.
 * @param filename Path to the users file.
 * @return UserIOStatus code indicating success or type of failure.
 */
UserIOStatus saveUsers(UserTable *ut, const char *filename);

/**
 * @brief Get error message for UserIOStatus code.
 * 
 * @param status The status code.
 * @return Human-readable error message.
 */
const char* getUserIOStatusMessage(UserIOStatus status);

#endif // USERS_IO_H
