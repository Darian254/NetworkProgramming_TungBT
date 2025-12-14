#include "user_dao.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SESSIONS 100 
static Session session_cache[MAX_SESSIONS];
static int session_count = 0;


// Đọc Acc 
int read_account_by_id_file(int user_id, Account *acc) {
    FILE *fp = fopen("accounts.txt", "r");
    if (!fp) return 0;

    Account temp;
    int items[MAX_ITEMS];
    //Format file : user_id|username|password|coin|i1,i2,i3,i4,i5|is_deleted
    while (fscanf(fp,
        "%d|%[^|]|%[^|]|%d|%d,%d,%d,%d,%d|%d\n",
        &temp.user_id,
        temp.username,
        temp.password,
        &temp.coin,
        &items[0], &items[1], &items[2], &items[3], &items[4],
        &temp.is_deleted) == 10)
    {
        if (temp.user_id == user_id && temp.is_deleted == 0) {
            for (int i = 0; i < MAX_ITEMS; i++)
                temp.item_counts[i] = items[i];

            *acc = temp;//copy ra biến con trỏ acc
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}

//Ghi và cập nhập
int update_account_to_file(const Account *acc) {
    FILE *fp = fopen("accounts.txt", "r");//Doc du lieu cu
    FILE *out = fopen("accounts_tmp.txt", "w");//Ghi du lieu moi
    if (!out) return 0;

    Account temp;
    int items[MAX_ITEMS];
    int found = 0;//TIm thấy hay chưa

    if (fp) {
        while (fscanf(fp,
            "%d|%[^|]|%[^|]|%d|%d,%d,%d,%d,%d|%d\n",
            &temp.user_id,
            temp.username,
            temp.password,
            &temp.coin,
            &items[0], &items[1], &items[2], &items[3], &items[4],
            &temp.is_deleted) == 10)
        {
            // Nếu trùng ID → ghi bản mới
            if (temp.user_id == acc->user_id) {
                fprintf(out, "%d|%s|%s|%d|%d,%d,%d,%d,%d|%d\n",
                    acc->user_id,
                    acc->username,
                    acc->password,
                    acc->coin,
                    acc->item_counts[0],
                    acc->item_counts[1],
                    acc->item_counts[2],
                    acc->item_counts[3],
                    acc->item_counts[4],
                    acc->is_deleted
                );
                found = 1;
            }
            // Ghi lại bản cũ
            else {
                fprintf(out, "%d|%s|%s|%d|%d,%d,%d,%d,%d|%d\n",
                    temp.user_id,
                    temp.username,
                    temp.password,
                    temp.coin,
                    items[0], items[1], items[2], items[3], items[4],
                    temp.is_deleted
                );
            }
        }
        fclose(fp);
    }

    // Nếu là account mới thì thêm vào cuối
    if (!found) {
        fprintf(out, "%d|%s|%s|%d|%d,%d,%d,%d,%d|%d\n",
            acc->user_id,
            acc->username,
            acc->password,
            acc->coin,
            acc->item_counts[0],
            acc->item_counts[1],
            acc->item_counts[2],
            acc->item_counts[3],
            acc->item_counts[4],
            acc->is_deleted
        );
    }

    fclose(out);
    remove("accounts.txt");
    rename("accounts_tmp.txt", "accounts.txt");

    return 1;
}

// Tìm account theo Username
int find_account_by_username_file(const char *username, Account *acc) {
    FILE *fp = fopen("accounts.txt", "r");
    if (!fp) return 0;

    Account temp;
    int items[MAX_ITEMS];

    while (fscanf(fp,
        "%d|%[^|]|%[^|]|%d|%d,%d,%d,%d,%d|%d\n",
        &temp.user_id,
        temp.username,
        temp.password,
        &temp.coin,
        &items[0], &items[1], &items[2], &items[3], &items[4],
        &temp.is_deleted) == 10)
    {
        if (temp.is_deleted == 0 && strcmp(temp.username, username) == 0) {
            for (int i = 0; i < MAX_ITEMS; i++)
                temp.item_counts[i] = items[i];

            if (acc) *acc = temp;
            fclose(fp);
            return 1;//tìm thấy
        }
    }

    fclose(fp);
    return 0;
}

// Create User
int UserDAO_create(const char *username, const char *password) {
    if (find_account_by_username_file(username, NULL))
        return -1; // Username tồn tại

    Account new_acc = {0};
    new_acc.user_id = generate_user_id();
    strncpy(new_acc.username, username, MAX_USERNAME_LEN - 1);
    strncpy(new_acc.password, password, MAX_PASSWORD_LEN - 1);
    new_acc.coin = 100;
    new_acc.is_deleted = 0;

    if (!update_account_to_file(&new_acc))
        return -2;

    return new_acc.user_id;
}

// Find user
int UserDAO_find_by_username(const char *username, Account *acc) {
    return find_account_by_username_file(username, acc);
}

// Update User
int UserDAO_update_account_stats(const Account *acc) {
    return update_account_to_file(acc);
}

//Session cache khởi tạo
void UserDAO_init_session_cache() {
    session_count = 0;
    memset(session_cache, 0, sizeof(session_cache));
}

// Create cache
int UserDAO_create_session(int user_id, char *session_id_out) {
    if (session_count >= MAX_SESSIONS) return 0;

    Session *s = &session_cache[session_count];
    generate_session_id(s->session_id);
    s->user_id = user_id;
    s->login_time = get_current_timestamp();
    strncpy(session_id_out, s->session_id, 19);

    session_count++;
    return 1;
}

// Get user from cache
int UserDAO_get_user_id_by_session(const char *session_id) {
    for (int i = 0; i < session_count; i++) {
        if (strcmp(session_cache[i].session_id, session_id) == 0)
            return session_cache[i].user_id;
    }
    return 0;
}

// Xoa sesion
void UserDAO_delete_session(const char *session_id) {
    for (int i = 0; i < session_count; i++) {
        if (strcmp(session_cache[i].session_id, session_id) == 0) {
            session_cache[i] = session_cache[session_count - 1];
            session_count--;
            return;
        }
    }
}
