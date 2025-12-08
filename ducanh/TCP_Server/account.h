#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <stdbool.h>
#include <stddef.h>

#define MAX_USERNAME 64
#define MAX_PASSWORD_HASH 128

/**
 * @struct Account
 * @brief Represents a user account in the hash table.
 */
typedef struct Account {
  char username[MAX_USERNAME]; /**< Username string */
  char password_hash[MAX_PASSWORD_HASH]; /**< Hashed password */
  int status; /**< Account status: 0 = banned, 1 = active */
  struct Account * next; /**< Linked list pointer for collisions */
}
Account;

/**
 * @struct HashTable
 * @brief Hash table storing accounts.
 */
typedef struct {
  Account ** table; /**< Array of buckets */
  size_t size; /**< Current number of buckets */
  size_t count; /**< Number of accounts stored */
}
HashTable;

/**
 * @brief Initialize a hash table.
 * @param size Initial number of buckets.
 * @return Pointer to the hash table or NULL if allocation fails.
 */
HashTable * initHashTable(size_t size);

/**
 * @brief Free all memory used by the hash table.
 * @param ht Pointer to the hash table.
 */
void freeHashTable(HashTable * ht);

/**
 * @brief Insert an account into the hash table.
 * If the load factor exceeds 0.75, the table is rehashed.
 * @param ht Pointer to the hash table.
 * @param acc Pointer to account (caller allocated).
 * @return true if inserted successfully, false otherwise.
 */
bool insertAccount(HashTable * ht, Account * acc);

/**
 * @brief Find an account by username.
 * @param ht Pointer to the hash table.
 * @param username The username to search.
 * @return Pointer to Account or NULL if not found.
 */
Account * findAccount(HashTable * ht,
  const char * username);

/**
 * @brief Rehash the hash table to a new capacity.
 * @param ht Pointer to the hash table.
 * @param new_size The new capacity.
 * @return true if successful, false otherwise.
 */
bool rehash(HashTable * ht, size_t new_size);

/**
 * @brief Hash password using simple hash function
 * @param password Plain text password
 * @param output Buffer to store hash (min MAX_PASSWORD_HASH bytes)
 */
void hash_password(const char *password, char *output);

/**
 * @brief Validate username format (alphanumeric, 3-32 chars)
 * @param username Username to validate
 * @return true if valid, false otherwise
 */
bool validate_username(const char *username);

/**
 * @brief Validate password format (min 6 chars)
 * @param password Password to validate
 * @return true if valid, false otherwise
 */
bool validate_password(const char *password);

#endif
