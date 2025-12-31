/**
 * ============================================================================
 * USERS MODULE - IMPLEMENTATION
 * ============================================================================
 */

#include "users.h"
#include "hash.h"
#include "users_io.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ============================================================================
 * HASHTABLE OPERATIONS
 * ============================================================================ */

UserTable* initUserTable(size_t size) {
    UserTable *ut = malloc(sizeof(UserTable));
    if (!ut) return NULL;
    
    ut->size = size;
    ut->count = 0;
    ut->table = calloc(size, sizeof(User*));
    if (!ut->table) {
        free(ut);
        return NULL;
    }
    return ut;
}

void freeUserTable(UserTable *ut) {
    if (!ut) return;
    
    for (size_t i = 0; i < ut->size; i++) {
        User *curr = ut->table[i];
        while (curr) {
            User *tmp = curr;
            curr = curr->next;
            free(tmp);
        }
    }
    free(ut->table);
    free(ut);
}

bool rehashUserTable(UserTable *ut, size_t new_size) {
    if (!ut || new_size == 0) return false;
    
    User **new_table = calloc(new_size, sizeof(User*));
    if (!new_table) return false;
    
    // Reinsert all users into the new table
    for (size_t i = 0; i < ut->size; i++) {
        User *curr = ut->table[i];
        while (curr) {
            User *next = curr->next;
            
            unsigned long idx = hashFunc(curr->username) % new_size;
            curr->next = new_table[idx];
            new_table[idx] = curr;
            
            curr = next;
        }
    }
    
    free(ut->table);
    ut->table = new_table;
    ut->size = new_size;
    return true;
}

// TODO : insertUser should also write to file?
bool insertUser(UserTable *ut, User *user) {
    if (!ut || !user) return false;

    // Check load factor before inserting
    double load_factor = (double)(ut->count + 1) / ut->size;
    if (load_factor > 0.75) {
        size_t new_size = ut->size * 2;
        if (!rehashUserTable(ut, new_size)) {
            return false;
        }
    }
    
    unsigned long idx = hashFunc(user->username) % ut->size;
    user->next = ut->table[idx];
    ut->table[idx] = user;
    ut->count++;
    return true;
}


// TODO : find co can mutex ko?
User* findUser(UserTable *ut, const char *username) {
    if (!ut || !username) return NULL;
    unsigned long idx = hashFunc(username) % ut->size;
    User *curr = ut->table[idx];
    
    while (curr) {
        if (strcmp(curr->username, username) == 0) {
            return curr;
        }
        curr = curr->next;
    }
    
    return NULL;
}

char* find_username_by_id(UserTable *ut, int user_id) {
    if (!ut) return NULL;

    // Duyệt qua mảng table
    for (size_t i = 0; i < ut->size; i++) {
        User *current = ut->table[i]; 
        

        while (current) {
            if ((int)hashFunc(current->username) == user_id) {
                return current->username;
            }
            current = current->next;
        }
    }
    return NULL;
}

/* ============================================================================
 * USER OPERATIONS
 * ============================================================================ */

// TODO : should createUser also write to file?
User* createUser(UserTable *ut, const char *username, const char *password_hash) {
    if (!ut || !username || !password_hash) return NULL;
    
    // Check if user already exists
    if (findUser(ut, username)) return NULL;
    
    User *user = malloc(sizeof(User));
    if (!user) return NULL;
    
    strncpy(user->username, username, MAX_USERNAME - 1);
    user->username[MAX_USERNAME - 1] = '\0';
    
    strncpy(user->password_hash, password_hash, MAX_PASSWORD_HASH - 1);
    user->password_hash[MAX_PASSWORD_HASH - 1] = '\0';
    
    user->status = USER_ACTIVE;
    user->coin = USER_DEFAULT_COIN;
    user->created_at = time(NULL);
    user->updated_at = time(NULL);
    user->next = NULL;
    
    if (!insertUser(ut, user)) {
        free(user);
        return NULL;
    }
    
    return user;
}

int updateUserCoin(UserTable *ut, const char *username, long delta) {
    User *user = findUser(ut, username);
    if (!user) return -1;
    
    if (user->coin + delta < 0) {
        return -2;  // Insufficient coin
    }
    
    user->coin += delta;
    user->updated_at = time(NULL);
    
    // Persist to file
    saveUsers(ut, USERS_FILE);

    return 0;
}

// TODO : lockUser should also write to file?
bool lockUser(UserTable *ut, const char *username) {
    User *user = findUser(ut, username);
    if (!user) return false;
    user->status = USER_BANNED;
    user->updated_at = time(NULL);
    
    return true;
}

// TODO : unlockUser should also write to file?
bool unlockUser(UserTable *ut, const char *username) {
    User *user = findUser(ut, username);
    if (!user) return false;
    user->status = USER_ACTIVE;
    user->updated_at = time(NULL);
    
    return true;
}

/* ============================================================================
 * PASSWORD & VALIDATION
 * ============================================================================ */

void hashPassword(const char *password, char *output) {
    if (!password || !output) return;
    
    // Use djb2 hash algorithm
    unsigned long hash = 5381;
    int c;
    const char *str = password;
    
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    
    snprintf(output, MAX_PASSWORD_HASH, "%lx", hash);
}

bool validateUsername(const char *username) {
    if (!username) return false;
    
    size_t len = strlen(username);
    if (len < 3 || len > 20) return false;
    
    // Check alphanumeric only
    for (size_t i = 0; i < len; i++) {
        if (!isalnum(username[i])) {
            return false;
        }
    }
    
    return true;
}

bool validatePassword(const char *password) {
    if (!password) return false;
    
    size_t len = strlen(password);
    if (len < 8) return false;
    
    // Check for uppercase, number, and special character
    bool has_upper = false;
    bool has_number = false;
    bool has_special = false;
    
    for (size_t i = 0; i < len; i++) {
        if (isupper(password[i])) has_upper = true;
        else if (isdigit(password[i])) has_number = true;
        else if (!isalnum(password[i])) has_special = true;
    }
    
    return has_upper && has_number && has_special;
}

bool verifyPassword(const char *password, const char *stored_hash) {
    if (!password || !stored_hash) return false;
    
    char hash[MAX_PASSWORD_HASH];
    hashPassword(password, hash);
    
    return strcmp(hash, stored_hash) == 0;
}
