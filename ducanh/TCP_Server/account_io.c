#include "account_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

AccountIOStatus loadAccounts(HashTable * ht,
  const char * filename) {
  FILE * fp = fopen(filename, "r");
  if (!fp) {
    perror("Error opening file");
    return ACCOUNT_IO_FILE_ERROR;
  }

  char line[1024]; // temporary buffer for one line
  char username_buf[1024];
  char password_hash_buf[MAX_PASSWORD_HASH];
  int status;

  while (fgets(line, sizeof(line), fp)) {
    // Skip empty lines and comments
    if (line[0] == '\n' || line[0] == '#') continue;
    
    // Nếu dòng quá dài (không chứa '\n') thì flush phần còn lại
    if (!strchr(line, '\n')) {
      int ch;
      while ((ch = fgetc(fp)) != '\n' && ch != EOF);
      fclose(fp);
      return ACCOUNT_IO_FORMAT_ERROR; // coi là format lỗi
    }

    // Parse username, password_hash, and status from the line
    if (sscanf(line, "%1023s %127s %d", username_buf, password_hash_buf, & status) != 3) {
      fclose(fp);
      return ACCOUNT_IO_FORMAT_ERROR;
    }

    // Allocate account node
    Account * acc = malloc(sizeof(Account));
    if (!acc) {
      fclose(fp);
      return ACCOUNT_IO_MEMORY_ERROR;
    }

    // Truncate username if too long
    strncpy(acc -> username, username_buf, MAX_USERNAME - 1);
    acc -> username[MAX_USERNAME - 1] = '\0';
    
    // Copy password hash
    strncpy(acc -> password_hash, password_hash_buf, MAX_PASSWORD_HASH - 1);
    acc -> password_hash[MAX_PASSWORD_HASH - 1] = '\0';
    
    acc -> status = status;
    acc -> next = NULL;

    // Insert into hash table
    insertAccount(ht, acc);
  }

  fclose(fp);
  return ACCOUNT_IO_OK;
}

AccountIOStatus saveAccounts(HashTable * ht,
  const char * filename) {
  FILE * fp = fopen(filename, "w");
  if (!fp) {
    perror("Error opening file for writing");
    return ACCOUNT_IO_FILE_ERROR;
  }
  
  fprintf(fp, "# username password_hash status\n");
  
  for (size_t i = 0; i < ht->size; i++) {
    Account * curr = ht->table[i];
    while (curr) {
      fprintf(fp, "%s %s %d\n", curr->username, curr->password_hash, curr->status);
      curr = curr->next;
    }
  }
  
  fclose(fp);
  return ACCOUNT_IO_OK;
}
