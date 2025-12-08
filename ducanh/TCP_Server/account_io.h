#ifndef ACCOUNT_IO_H
#define ACCOUNT_IO_H

#include "account.h"
#include <stdbool.h>

/**
 * @enum AccountIOStatus
 * @brief Status codes returned by account file operations.
 */
typedef enum {
  ACCOUNT_IO_OK = 0, /**< Operation completed successfully. */
    ACCOUNT_IO_FILE_ERROR, /**< Failed to open the account file. */
    ACCOUNT_IO_MEMORY_ERROR, /**< Memory allocation failure while loading accounts. */
    ACCOUNT_IO_FORMAT_ERROR /**< Invalid data format in the account file. */
}
AccountIOStatus;

/**
 * @brief Load accounts from a text file into the hash table.
 * 
 * Each line in the file must follow the format:
 * ```
 * <username> <status>
 * ```
 * where `status` is an integer (e.g., 0 = banned, 1 = active).
 * 
 * - If the username length exceeds `MAX_USERNAME - 1`, it will be truncated.  
 * - If memory allocation fails, all previously loaded accounts will be freed 
 *   to avoid memory leaks.  
 * - If the file contains an invalid line format, the function will stop, 
 *   free loaded accounts, and return an error status.  
 * 
 * @param ht Pointer to the hash table where accounts will be stored.  
 * @param filename Path to the account file.  
 * @return An `AccountIOStatus` code indicating success or type of failure.  
 */
AccountIOStatus loadAccounts(HashTable * ht,
  const char * filename);

/**
 * @brief Save accounts to a text file
 * @param ht Pointer to the hash table
 * @param filename Path to the account file
 * @return An `AccountIOStatus` code indicating success or type of failure
 */
AccountIOStatus saveAccounts(HashTable * ht,
  const char * filename);

#endif
