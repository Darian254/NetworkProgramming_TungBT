#include "team_dao.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

#define MAX_TEAMS 20
#define MAX_REQUESTS 20

static Team team_list[MAX_TEAMS];
static int team_count = 0;
static TeamRequest request_list[MAX_REQUESTS];
static int request_count = 0;

void TeamDAO_init_cache() {
    team_count = 0;
    request_count = 0;
    memset(team_list, 0, sizeof(team_list));
    memset(request_list, 0, sizeof(request_list));
}
//Tìm team theo ID
Team* TeamDAO_find_by_id(int team_id) {
    for (int i = 0; i < team_count; i++) {
        if (team_list[i].team_id == team_id) {
            return &team_list[i];
        }
    }
    return NULL;
}

//Tạo team mới
int TeamDAO_create(const char *team_name, int captain_id) {
    if (team_count >= MAX_TEAMS) return -1; // Team limit reached
    
    Team *t = &team_list[team_count];//trỏ vào vị trí team mới
    t->team_id = generate_team_id();
    strncpy(t->team_name, team_name, 29);
    t->captain_id = captain_id;
    t->member_ids[0] = captain_id;
    t->member_count = 1;
    
    team_count++;
    return t->team_id;
}

//Thêm thành viên vào team
int TeamDAO_add_member(int team_id, int user_id) {
    Team *t = TeamDAO_find_by_id(team_id);
    if (!t) return -1; // Không tìm thấy
    if (t->member_count >= MAX_TEAM_MEMBERS) return -1; // Team full
    
    for (int i = 0; i < t->member_count; i++) {
        if (t->member_ids[i] == user_id) return -1; // Đã là thành viên
    }

    t->member_ids[t->member_count++] = user_id;
    return 1;
}

//Tạo request
int InviteDAO_create_request(int target_user_id, int team_id, int type) {
    if (request_count >= MAX_REQUESTS) return -1; // qua giới hạn

    
    TeamRequest *req = &request_list[request_count];
    req->request_id = generate_request_id();
    req->target_user_id = target_user_id;
    req->team_id = team_id;
    req->type = (type == REQ_JOIN) ? REQ_JOIN : REQ_INVITE;
    
    request_count++;
    return req->request_id;
}

//Tìm đã có request hay chưa
TeamRequest* InviteDAO_find_pending(int target_user_id, int team_id) {
     for (int i = 0; i < request_count; i++) {
        TeamRequest *req = &request_list[i];
        if (req->target_user_id == target_user_id && req->team_id == team_id) {
            return req;
        }
    }
    return NULL;
}

//Xóa request
void InviteDAO_delete_request(int request_id) {
    for (int i = 0; i < request_count; i++) {
        if (request_list[i].request_id == request_id) {
            // Thay thế bằng phần tử cuối cùng để xóa hiệu quả
            request_list[i] = request_list[request_count - 1];
            request_count--;
            return;
        }
    }
}