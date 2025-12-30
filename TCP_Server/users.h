/**
 * ============================================================================
 * USERS MODULE
 * ============================================================================
 * 
 * Manages user accounts with HashTable (collision chaining).
 * 
 * Features:
 *   - HashTable with automatic rehashing (load factor > 0.75)
 *   - Password hashing (djb2)
 *   - Username/password validation
 *   - Thread-safe with mutex (in users.c)
 * 
 * File: users.txt
 * Format: <username> <password_hash> <status> <coin> <created_at> <updated_at>
 * ============================================================================
 */

#ifndef USERS_H
#define USERS_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */
#define MAX_USERNAME        64
#define MAX_PASSWORD_HASH   128
#define USER_DEFAULT_COIN   500

/* ============================================================================
 * USER STATUS
 * ============================================================================ */
typedef enum {
    USER_BANNED = 0,
    USER_ACTIVE = 1
} UserStatus;

/* ============================================================================
 * USER STRUCT
 * ============================================================================ */
/**
 * @struct User
 * @brief Represents a user account in the hash table.
 */
typedef struct User {
    char        username[MAX_USERNAME];         /**< Username (unique) */
    char        password_hash[MAX_PASSWORD_HASH]; /**< Hashed password */
    UserStatus  status;                         /**< 0 = banned, 1 = active */
    long        coin;                           /**< Persistent currency */
    time_t      created_at;                     /**< Account creation time */
    time_t      updated_at;                     /**< Last update time */
    struct User *next;                          /**< Linked list for hash collision */
} User;

/* ============================================================================
 * USER HASHTABLE
 * ============================================================================ */
/**
 * @struct UserTable
 * @brief Hash table storing users with collision chaining.
 */
typedef struct {
    User      **table;      /**< Array of buckets */
    size_t      size;       /**< Current number of buckets */
    size_t      count;      /**< Number of users stored */
} UserTable;

/* ============================================================================
 * HASHTABLE OPERATIONS
 * ============================================================================ */

/**
 * @brief Initialize a user hash table.
 * @param size Initial number of buckets.
 * @return Pointer to the hash table or NULL if allocation fails.
 */
UserTable* initUserTable(size_t size);

/**
 * @brief Free all memory used by the hash table.
 * @param ut Pointer to the hash table.
 */
void freeUserTable(UserTable *ut);

/**
 * @brief Insert a user into the hash table.
 * If load factor exceeds 0.75, the table is rehashed.
 * @param ut Pointer to the hash table.
 * @param user Pointer to user (caller allocated).
 * @return true if inserted successfully, false otherwise.
 */
bool insertUser(UserTable *ut, User *user);

/**
 * @brief Find a user by username.
 * @param ut Pointer to the hash table.
 * @param username The username to search.
 * @return Pointer to User or NULL if not found.
 */
User* findUser(UserTable *ut, const char *username);

/**
 * @brief Rehash the hash table to a new capacity.
 * @param ut Pointer to the hash table.
 * @param new_size The new capacity.
 * @return true if successful, false otherwise.
 */
bool rehashUserTable(UserTable *ut, size_t new_size);

/* ============================================================================
 * USER OPERATIONS
 * ============================================================================ */

/**
 * @brief Create a new user with default values.
 * @param ut Pointer to the hash table.
 * @param username Username for the new user.
 * @param password_hash Hashed password.
 * @return Pointer to created User or NULL if failed.
 */
User* createUser(UserTable *ut, const char *username, const char *password_hash);

/**
 * @brief Update user's coin balance.
 * @param ut Pointer to the hash table.
 * @param username Username of the user.
 * @param delta Amount to add (can be negative).
 * @return 0 = success, -1 = user not found, -2 = insufficient coin.
 */
int updateUserCoin(UserTable *ut, const char *username, long delta);

/**
 * @brief Lock/ban a user account.
 * @param ut Pointer to the hash table.
 * @param username Username of the user.
 * @return true if successful, false if user not found.
 */
bool lockUser(UserTable *ut, const char *username);

/**
 * @brief Unlock a user account.
 * @param ut Pointer to the hash table.
 * @param username Username of the user.
 * @return true if successful, false if user not found.
 */
bool unlockUser(UserTable *ut, const char *username);

/* ============================================================================
 * PASSWORD & VALIDATION
 * ============================================================================ */

/**
 * @brief Hash password using djb2 algorithm.
 * @param password Plain text password.
 * @param output Buffer to store hash (min MAX_PASSWORD_HASH bytes).
 */
void hashPassword(const char *password, char *output);

/**
 * @brief Validate username format.
 * Rules: alphanumeric only, 3-20 characters.
 * @param username Username to validate.
 * @return true if valid, false otherwise.
 */
bool validateUsername(const char *username);

/**
 * @brief Validate password format.
 * Rules: min 8 chars, must have uppercase, number, special char.
 * @param password Password to validate.
 * @return true if valid, false otherwise.
 */
bool validatePassword(const char *password);

/**
 * @brief Verify password against stored hash.
 * @param password Plain text password.
 * @param stored_hash Stored password hash.
 * @return true if password matches, false otherwise.
 */
bool verifyPassword(const char *password, const char *stored_hash);

#endif // USERS_H
