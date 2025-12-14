// user_dao.h
#ifndef USER_DAO_H
#define USER_DAO_H

#include "data_struct.h"

// Account Management 
int UserDAO_create(const char *username, const char *password);
int UserDAO_find_by_username(const char *username, Account *acc);
int UserDAO_update_account_stats(const Account *acc); // Dùng cho mua/sửa chữa

// Session Management (Memory)
void UserDAO_init_session_cache();
int UserDAO_create_session(int user_id, char *session_id_out);
int UserDAO_get_user_id_by_session(const char *session_id);
void UserDAO_delete_session(const char *session_id);

#endif // USER_DAO_H