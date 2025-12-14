// team_dao.h
#ifndef TEAM_DAO_H
#define TEAM_DAO_H

#include "data_struct.h"


void TeamDAO_init_cache();
// Team Management
int TeamDAO_create(const char *team_name, int captain_id);
int TeamDAO_delete(int team_id, int user_id); // Check quyền creator
Team* TeamDAO_find_by_id(int team_id);
int TeamDAO_is_member(int team_id, int user_id);
int TeamDAO_get_member_list(int team_id, int *member_ids_out); // Trả về mảng ID thành viên
int TeamDAO_add_member(int team_id, int user_id); // Dùng khi ACCEPT_INVITE
int TeamDAO_remove_member(int team_id, int user_id); // Dùng cho LEAVE/KICK

// Request/Invite Management
int InviteDAO_create_request(int target_user_id, int team_id, int type);
TeamRequest* InviteDAO_find_pending(int target_user_id, int team_id);
void InviteDAO_delete_request(int request_id);

#endif // TEAM_DAO_H