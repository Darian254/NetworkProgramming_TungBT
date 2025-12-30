/**
 * ============================================================================
 * USERS I/O MODULE - IMPLEMENTATION
 * ============================================================================
 */

#include "users_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * FILE OPERATIONS
 * ============================================================================ */

UserIOStatus loadUsers(UserTable *ut, const char *filename) {
    if (!ut || !filename) return USER_IO_FILE_ERROR;
    
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Error opening users file");
        return USER_IO_FILE_ERROR;
    }
    
    char line[1024];
    char username_buf[MAX_USERNAME];
    char password_hash_buf[MAX_PASSWORD_HASH];
    int status;
    long coin;
    long created_at, updated_at;
    
    while (fgets(line, sizeof(line), fp)) {
        // Skip empty lines and comments
        if (line[0] == '\n' || line[0] == '#') continue;
        
        // Check if line is too long (no newline found)
        if (!strchr(line, '\n') && !feof(fp)) {
            int ch;
            while ((ch = fgetc(fp)) != '\n' && ch != EOF);
            fclose(fp);
            return USER_IO_FORMAT_ERROR;
        }
        
        // Try to parse full format first (with coin and timestamps)
        int parsed = sscanf(line, "%63s %127s %d %ld %ld %ld",
            username_buf, password_hash_buf, &status, &coin, &created_at, &updated_at);
        
        if (parsed < 3) {
            // Invalid format
            fclose(fp);
            return USER_IO_FORMAT_ERROR;
        }
        
        // Allocate user
        User *user = malloc(sizeof(User));
        if (!user) {
            fclose(fp);
            return USER_IO_MEMORY_ERROR;
        }
        
        // Copy username
        strncpy(user->username, username_buf, MAX_USERNAME - 1);
        user->username[MAX_USERNAME - 1] = '\0';
        
        // Copy password hash
        strncpy(user->password_hash, password_hash_buf, MAX_PASSWORD_HASH - 1);
        user->password_hash[MAX_PASSWORD_HASH - 1] = '\0';
        
        user->status = (UserStatus)status;
        
        // Set coin and timestamps (use defaults if not in file)
        if (parsed >= 4) {
            user->coin = coin;
        } else {
            user->coin = USER_DEFAULT_COIN;
        }
        
        if (parsed >= 6) {
            user->created_at = (time_t)created_at;
            user->updated_at = (time_t)updated_at;
        } else {
            user->created_at = time(NULL);
            user->updated_at = time(NULL);
        }
        
        user->next = NULL;
        
        // Insert into hash table
        if (!insertUser(ut, user)) {
            free(user);
            // Continue loading other users even if one fails
        }
    }
    
    fclose(fp);
    return USER_IO_OK;
}

UserIOStatus saveUsers(UserTable *ut, const char *filename) {
    if (!ut || !filename) return USER_IO_FILE_ERROR;
    
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Error opening users file for writing");
        return USER_IO_FILE_ERROR;
    }
    
    // Write header comment
    fprintf(fp, "# Users Database\n");
    fprintf(fp, "# Format: username password_hash status coin created_at updated_at\n");
    fprintf(fp, "# status: 0 = banned, 1 = active\n");
    fprintf(fp, "#\n");
    
    // Write all users
    for (size_t i = 0; i < ut->size; i++) {
        User *curr = ut->table[i];
        while (curr) {
            fprintf(fp, "%s %s %d %ld %ld %ld\n",
                curr->username,
                curr->password_hash,
                curr->status,
                curr->coin,
                (long)curr->created_at,
                (long)curr->updated_at);
            curr = curr->next;
        }
    }
    
    fclose(fp);
    return USER_IO_OK;
}

const char* getUserIOStatusMessage(UserIOStatus status) {
    switch (status) {
        case USER_IO_OK:
            return "Operation completed successfully";
        case USER_IO_FILE_ERROR:
            return "Failed to open file";
        case USER_IO_MEMORY_ERROR:
            return "Memory allocation failure";
        case USER_IO_FORMAT_ERROR:
            return "Invalid data format in file";
        default:
            return "Unknown error";
    }
}
