#include "account.h"
#include "hash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

HashTable * initHashTable(size_t size) {
  HashTable * ht = malloc(sizeof(HashTable));
  if (!ht) return NULL;
  ht -> size = size;
  ht -> count = 0;
  ht -> table = calloc(size, sizeof(Account * ));
  if (!ht -> table) {
    free(ht);
    return NULL;
  }
  return ht;
}

void freeHashTable(HashTable * ht) {
  if (!ht) return;
  for (size_t i = 0; i < ht -> size; i++) {
    Account * curr = ht -> table[i];
    while (curr) {
      Account * tmp = curr;
      curr = curr -> next;
      free(tmp);
    }
  }
  free(ht -> table);
  free(ht);
}

bool rehash(HashTable * ht, size_t new_size) {
  if (!ht || new_size == 0) return false;

  Account ** new_table = calloc(new_size, sizeof(Account * ));
  if (!new_table) return false;

  // Reinsert all accounts into the new table
  for (size_t i = 0; i < ht -> size; i++) {
    Account * curr = ht -> table[i];
    while (curr) {
      Account * next = curr -> next;

      unsigned long idx = hashFunc(curr -> username) % new_size;
      curr -> next = new_table[idx];
      new_table[idx] = curr;

      curr = next;
    }
  }

  free(ht -> table);
  ht -> table = new_table;
  ht -> size = new_size;
  // count remains the same
  return true;
}

bool insertAccount(HashTable * ht, Account * acc) {
  if (!ht || !acc) return false;

  // Check load factor before inserting
  double load_factor = (double)(ht -> count + 1) / ht -> size;
  if (load_factor > 0.75) {
    size_t new_size = ht -> size * 2;
    if (!rehash(ht, new_size)) {
      return false; // failed to rehash
    }
  }

  unsigned long idx = hashFunc(acc -> username) % ht -> size;
  acc -> next = ht -> table[idx];
  ht -> table[idx] = acc;
  ht -> count++;
  return true;
}

Account * findAccount(HashTable * ht,
  const char * username) {
  if (!ht || !username) return NULL;
  unsigned long idx = hashFunc(username) % ht -> size;
  Account * curr = ht -> table[idx];
  while (curr) {
    if (strcmp(curr -> username, username) == 0)
      return curr;
    curr = curr -> next;
  }
  return NULL;
}

void hash_password(const char *password, char *output) {
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

bool validate_username(const char *username) {
  if (!username) return false;
  
  size_t len = strlen(username);
  if (len < 3 || len > 20) return false;
  
  // Check alphanumeric only (no underscore per protocol)
  for (size_t i = 0; i < len; i++) {
    if (!isalnum(username[i])) {
      return false;
    }
  }
  
  return true;
}

bool validate_password(const char *password) {
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
