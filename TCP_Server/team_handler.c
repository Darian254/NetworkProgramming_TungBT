/**
 * @file team_handler.c
 * @brief Implementation of team operation handlers
 */

#include "team_handler.h"
#include "db_schema.h"
#include "session.h"
#include "users.h"
#include "config.h"
#include "file_transfer.h"
#include "hash.h"
#include <string.h>
#include <stdio.h>

extern Team teams[MAX_TEAMS]; 
extern TeamMember team_members[MAX_TEAM_MEMBERS * MAX_TEAMS];
extern UserTable *g_user_table;
extern int team_member_count;
extern JoinRequest join_requests[MAX_JOIN_REQUESTS];
extern int join_request_count;
extern TeamInvite team_invites[MAX_TEAM_INVITES];
extern int team_invite_count;

/* ============================================================================
 * CREATE TEAM
 * ============================================================================ */
int handle_create_team(ServerSession *session, UserTable *ut, const char *name) {
    if (!session || !ut || !name) {
        return RESP_SYNTAX_ERROR;
    }
    
    if (!session->isLoggedIn) {
        return RESP_NOT_LOGGED;
    }
    
    int current_team = session->current_team_id;
    if (current_team > 0) {
        return RESP_ALREADY_IN_TEAM;
    }
    
    if (strlen(name) == 0 || strlen(name) >= TEAM_NAME_LEN) {
        return RESP_SYNTAX_ERROR;
    }
    
    Team *existing = find_team_by_name(name);
    if (existing) {
        return RESP_TEAM_CREATE_FAILED;
    }
    
    Team *new_team = create_team(name, session->username);
    if (!new_team) {
        return RESP_INTERNAL_ERROR;
    }
    
    session->current_team_id = new_team->team_id;
    update_session_by_socket(session->socket_fd, session);
    printf("[INFO] User '%s' created team '%s' (ID: %d)\n", 
           session->username, new_team->name, new_team->team_id);
    return RESP_TEAM_CREATED;
}

/* ============================================================================
 * DELETE TEAM
 * ============================================================================ */
int handle_delete_team(ServerSession *session) {
    if (!session) {
        return RESP_SYNTAX_ERROR;
    }
    
    if (!session->isLoggedIn) {
        return RESP_NOT_LOGGED;
    }
    
    
    
    int team_id = session->current_team_id;
    if (team_id <= 0) {
        return RESP_NOT_IN_TEAM;
    }
    
    Team *team = find_team_by_id(team_id);
    if (strcmp(team->creator_username, session->username) != 0) {
        return RESP_NOT_CREATOR;
    }
    
    if (!delete_team(team->team_id)) {
        return RESP_INTERNAL_ERROR;
    }
    
    session->current_team_id = -1;
    update_session_by_socket(session->socket_fd, session);
    
    return RESP_TEAM_DELETED;
}

/* ============================================================================
 * LIST TEAMS 
 * ============================================================================ */
int handle_list_teams(ServerSession *session, char *output_buf, size_t buf_size) {
    if (!session || !output_buf || buf_size == 0) {
        return RESP_SYNTAX_ERROR;
    }
    
    if (!session->isLoggedIn) {
        return RESP_NOT_LOGGED;
    }
    
    output_buf[0] = '\0';
    int count = 0;
    char temp[256];

    for (int i = 0; i < MAX_TEAMS; i++) {
        if (teams[i].team_id > 0 && strlen(teams[i].name) > 0 && teams[i].status == TEAM_ACTIVE) {
            
            int member_count = get_team_member_count(teams[i].team_id);
        
            snprintf(temp, sizeof(temp), "[%d] %s (%d/%d)|", 
                     teams[i].team_id, 
                     teams[i].name, 
                     member_count, 
                     MAX_TEAM_MEMBERS);
            
            if (strlen(output_buf) + strlen(temp) < buf_size - 1) {
                strcat(output_buf, temp);
                count++;
            } else {
                break; 
            }
        }
    }
    
    if (count == 0) {
        snprintf(output_buf, buf_size, "No active teams available.");
    }
    
    return RESP_LIST_TEAMS_OK; 
}


/* ============================================================================
 * TEAM MEMBER LIST
 * ============================================================================ */
int handle_team_member_list(ServerSession *session, char *output_buf, size_t buf_size) {

    if (!session || !output_buf || buf_size == 0) {
        return RESP_SYNTAX_ERROR;
    }
    
    if (!session->isLoggedIn) {
        return RESP_NOT_LOGGED;
    }


    int real_team_id = find_team_id_by_username(session->username);

    if (session->current_team_id != real_team_id) {
        session->current_team_id = real_team_id;
 
    }


    if (session->current_team_id <= 0) {
        return RESP_NOT_IN_TEAM; 
    }

    int team_id = session->current_team_id;
    Team *team = find_team_by_id(team_id);
    if (!team) {
        return RESP_TEAM_NOT_FOUND;
    }

    snprintf(output_buf, buf_size, "%s|", team->name);
    
    char temp[MAX_USERNAME + 5]; 
    int count = 0;

    for (int i = 0; i < team_member_count; i++) {
        if (team_members[i].team_id == team_id) {

            snprintf(temp, sizeof(temp), "%s|", team_members[i].username);

            if (strlen(output_buf) + strlen(temp) < buf_size - 1) {
                strcat(output_buf, temp);
                count++;
            } else {
                break; 
            }
        }
    }
    
    return RESP_TEAM_MEMBERS_LIST_OK;
}

/* ============================================================================
 * LEAVE TEAM
 * ============================================================================ */

int handle_leave_team(ServerSession *session) {
    if (!session) return RESP_SYNTAX_ERROR;
    if (!session->isLoggedIn) return RESP_NOT_LOGGED;
    
    int team_id = session->current_team_id;
    if (team_id <= 0) return RESP_NOT_IN_TEAM;

    Team *team = find_team_by_id(team_id);
    if (!team) return RESP_TEAM_NOT_FOUND;

    int member_count = get_team_member_count(team_id);

    if (member_count <= 1) {
        if (!delete_team(team_id)) {
            return RESP_INTERNAL_ERROR;
        }
        session->current_team_id = -1;
        update_session_by_socket(session->socket_fd, session);
        return RESP_TEAM_LEAVE_OK; 
    }
    
    
    
    bool is_creator = (strcmp(team->creator_username, session->username) == 0);
    int remove_index = -1;

    for (int i = 0; i < team_member_count; i++) {
        if (team_members[i].team_id == team_id && 
            strcmp(team_members[i].username, session->username) == 0) {
            remove_index = i;
            break;
        }
    }

    if (remove_index != -1) {
        for (int i = remove_index; i < team_member_count - 1; i++) {
            team_members[i] = team_members[i + 1];
        }
        team_member_count--;
        
        if (is_creator) {
            int new_captain_index = -1;
            for (int i = 0; i < team_member_count; i++) {
                if (team_members[i].team_id == team_id) {
                    new_captain_index = i;
                    break; 
                }
            }

            if (new_captain_index != -1) {
                strncpy(team->creator_username, team_members[new_captain_index].username, MAX_USERNAME - 1);
                team->creator_username[MAX_USERNAME - 1] = '\0';

                team_members[new_captain_index].role = ROLE_CREATOR; 
            }
        }
    }
    
    session->current_team_id = -1;
    update_session_by_socket(session->socket_fd, session);
    
    return RESP_TEAM_LEAVE_OK;
}

/* ============================================================================
 * KICK MEMBER
 * ============================================================================ */
int handle_kick_member(ServerSession *session, const char *username) {
    if (!session || !username) return RESP_SYNTAX_ERROR;
    if (!session->isLoggedIn) return RESP_NOT_LOGGED;
    
    int team_id = session->current_team_id;
    if (team_id <= 0) return RESP_NOT_IN_TEAM;
    
    Team *team = find_team_by_id(team_id);
    if (!team) return RESP_TEAM_NOT_FOUND;


    if (strcmp(team->creator_username, session->username) != 0) {
        return RESP_NOT_CREATOR;
    }
    
    int target_team_id = find_team_id_by_username(username);
    if (target_team_id != team_id) {
        return RESP_MEMBER_KICK_NOT_IN_TEAM;
    }
    
    if (strcmp(session->username, username) == 0) {
        return RESP_SYNTAX_ERROR;
    }
    
    
    int index = -1;
    for (int i = 0; i < team_member_count; i++) {
        if (team_members[i].team_id == team_id && 
            strcmp(team_members[i].username, username) == 0) {
            index = i;
            break;
        }
    }

    if (index != -1) {
        for (int i = index; i < team_member_count - 1; i++) {
            team_members[i] = team_members[i + 1];
        }
        team_member_count--; 
    }
    
    if (index == -1) {
        return RESP_INTERNAL_ERROR; 
    }
    // --------------------------------------

    // --- CẬP NHẬT SESSION NGƯỜI BỊ KICK (Nếu đang online) ---
    SessionNode *victim_node = find_session_by_username(username);
    if (victim_node != NULL) {
        // Reset trạng thái team của người bị kick
        victim_node->session.current_team_id = -1;
        
        // Gửi thông báo cho họ biết
        // char notify_msg[256];
        // snprintf(notify_msg, sizeof(notify_msg), "You have been kicked from team '%s'.", team->name);
        // send_line(victim_node->session.socket_fd, notify_msg);
    }

    return RESP_KICK_MEMBER_OK;
}

/* ============================================================================
 * JOIN REQUEST (Gửi yêu cầu tham gia)
 * ============================================================================ */
int handle_join_request(ServerSession *session, const char *name) {
    if (!session || !name) return RESP_SYNTAX_ERROR;
    if (!session->isLoggedIn) return RESP_NOT_LOGGED;
    
    // Kiểm tra user đã có team chưa
    int current_team = find_team_id_by_username(session->username);
    if (current_team > 0) return RESP_ALREADY_IN_TEAM;
    
    // Tìm team muốn join
    Team *team = find_team_by_name(name);
    if (!team) return RESP_TEAM_NOT_FOUND;
    
    if (get_team_member_count(team->team_id) >= MAX_TEAM_MEMBERS) {
        return RESP_TEAM_FULL;
    }

    // Kiểm tra xem đã gửi request chưa (tránh spam)
    for (int i = 0; i < join_request_count; i++) {
        if (join_requests[i].team_id == team->team_id &&
            strcmp(join_requests[i].username, session->username) == 0 && // So sánh tên
            join_requests[i].status == STATUS_PENDING) { 
            
            return RESP_JOIN_REQUEST_SENT; // Đã gửi rồi
        }
    }

    if (join_request_count >= MAX_JOIN_REQUESTS) {
        return RESP_INTERNAL_ERROR;
    }

    // TẠO REQUEST MỚI
    JoinRequest *req = &join_requests[join_request_count++];
    req->team_id = team->team_id;
    
    // [QUAN TRỌNG] Lưu tên user thay vì ID
    strncpy(req->username, session->username, MAX_USERNAME - 1);
    req->username[MAX_USERNAME - 1] = '\0';
    
    req->requested_at = time(NULL);
    req->status = STATUS_PENDING;

    return RESP_JOIN_REQUEST_SENT;
}

/* ============================================================================
 * CHECK JOIN REQUESTS (Xem danh sách yêu cầu - Dành cho Captain)
 * ============================================================================ */
int handle_check_join_requests(ServerSession *session, char *output_buf, size_t buf_size) {
    if (!session || !output_buf) return RESP_SYNTAX_ERROR;
    if (!session->isLoggedIn) return RESP_NOT_LOGGED;

    int team_id = session->current_team_id;
    if (team_id <= 0) return RESP_NOT_IN_TEAM;

    Team *team = find_team_by_id(team_id);
    if (!team) return RESP_TEAM_NOT_FOUND;

    // Chỉ Captain mới được xem
    if (strcmp(team->creator_username, session->username) != 0) {
        return RESP_NOT_CREATOR;
    }

    int count = 0;
    output_buf[0] = '\0';
    char temp[256];

    for (int i = 0; i < join_request_count; i++) {
        // Lọc request của team mình & trạng thái Pending
        if (join_requests[i].team_id == team_id && 
            join_requests[i].status == STATUS_PENDING) {
            
            // [CỰC KỲ ĐƠN GIẢN] Lấy tên trực tiếp từ struct
            snprintf(temp, sizeof(temp), "%s|", join_requests[i].username);
            
            if (strlen(output_buf) + strlen(temp) < buf_size - 1) {
                strcat(output_buf, temp);
                count++;
            }
        }
    }

    if (count == 0) {
        return RESP_NOT_FOUND_REQUEST; 
    }

    return RESP_OK;
}

/* ============================================================================
 * JOIN APPROVE (Duyệt yêu cầu)
 * ============================================================================ */
int handle_join_approve(ServerSession *session, const char *target_username, UserTable *user_table) {
    if (!session || !target_username) return RESP_SYNTAX_ERROR;
    if (!session->isLoggedIn) return RESP_NOT_LOGGED;

    int team_id = session->current_team_id;
    Team *team = find_team_by_id(team_id);
    if (!team) return RESP_TEAM_NOT_FOUND;

    if (strcmp(team->creator_username, session->username) != 0) {
        return RESP_NOT_CREATOR;
    }

    // Tìm request khớp với tên người dùng được chọn
    int req_index = -1;
    for (int i = 0; i < join_request_count; i++) {
        if (join_requests[i].team_id == team->team_id &&
            strcmp(join_requests[i].username, target_username) == 0 && // So sánh tên
            join_requests[i].status == STATUS_PENDING) { 
            
            req_index = i;
            break;
        }
    }

    if (req_index == -1) {
        return RESP_NOT_FOUND_REQUEST; 
    }

    // Check full team
    if (get_team_member_count(team->team_id) >= MAX_TEAM_MEMBERS) return RESP_TEAM_FULL;
    
    // Check nếu user đã vào team khác rồi
    if (find_team_id_by_username(target_username) > 0) {
        // Xóa request này đi vì không còn hợp lệ
        // (Logic xóa: dời các phần tử phía sau lên)
        for (int i = req_index; i < join_request_count - 1; i++) {
            join_requests[i] = join_requests[i + 1];
        }
        join_request_count--;
        return RESP_ALREADY_IN_TEAM;
    }

    // THÊM THÀNH VIÊN
    TeamMember *member = &team_members[team_member_count++];
    member->team_id = team->team_id;
    strncpy(member->username, target_username, MAX_USERNAME - 1);
    member->username[MAX_USERNAME - 1] = '\0';
    member->role = 1; // Member
    member->joined_at = time(NULL);

    // Xóa request sau khi duyệt
    for (int i = req_index; i < join_request_count - 1; i++) {
        join_requests[i] = join_requests[i + 1];
    }
    join_request_count--;

    // Cập nhật session nếu người chơi đang online
    SessionNode *target_node = find_session_by_username(target_username);
    if (target_node != NULL) {
        target_node->session.current_team_id = team->team_id;
    }

    return RESP_JOIN_APPROVED;
}

/* ============================================================================
 * JOIN REJECT (Từ chối yêu cầu)
 * ============================================================================ */
int handle_join_reject(ServerSession *session, const char *target_username) {
    if (!session || !target_username) return RESP_SYNTAX_ERROR;
    
    int team_id = session->current_team_id;
    Team *team = find_team_by_id(team_id);
    if (!team) return RESP_TEAM_NOT_FOUND;
    
    if (strcmp(team->creator_username, session->username) != 0) {
        return RESP_NOT_CREATOR;
    }
    
    int req_index = -1;
    for (int i = 0; i < join_request_count; i++) {
        if (join_requests[i].team_id == team->team_id &&
            strcmp(join_requests[i].username, target_username) == 0 && // So sánh tên
            join_requests[i].status == STATUS_PENDING) { 
            
            req_index = i;
            break;
        }
    }
    
    if (req_index == -1) {
        return RESP_NOT_FOUND_REQUEST; 
    }
    
    // Xóa request
    for (int i = req_index; i < join_request_count - 1; i++) {
        join_requests[i] = join_requests[i + 1];
    }
    join_request_count--;
    
    return RESP_JOIN_REJECTED;
}

/* ============================================================================
 * INVITE
 * ============================================================================ */
int handle_invite(ServerSession *session, const char *target_username, UserTable *user_table) {
    if (!session || !target_username) return RESP_SYNTAX_ERROR;
    if (!session->isLoggedIn) return RESP_NOT_LOGGED;
    
    int team_id = session->current_team_id;
    if (team_id <= 0) return RESP_NOT_IN_TEAM;

    Team *team = find_team_by_id(team_id);
    if (!team) return RESP_TEAM_NOT_FOUND;
    
    if (strcmp(team->creator_username, session->username) != 0) {
        return RESP_NOT_CREATOR;
    }
    
    int member_count = get_team_member_count(team->team_id);
    if (member_count >= MAX_TEAM_MEMBERS) return RESP_TEAM_FULL;
    
    User *target_user = findUser(user_table, target_username);

    if (!target_user) return RESP_PLAYER_NOT_FOUND;

    int target_team_id = find_team_id_by_username(target_username);
    if (target_team_id > 0) return RESP_ALREADY_IN_TEAM;

    int inviter_id = hashFunc(session->username);
    int invitee_id = hashFunc(target_username);

    for (int i = 0; i < team_invite_count; i++) {
        if (team_invites[i].team_id == team->team_id &&
            team_invites[i].invitee_id == invitee_id && 
            team_invites[i].status == STATUS_PENDING) {  
            
            return RESP_TEAM_INVITED; 
        }
    }

    if (team_invite_count >= MAX_TEAM_INVITES) {
        return RESP_INVITE_QUEUE_FULL;
    }
    TeamInvite *invite = &team_invites[team_invite_count++];
    
    invite->team_id = team->team_id;
    invite->inviter_id = inviter_id;
    invite->invitee_id = invitee_id; 
    invite->invited_at = time(NULL);
    invite->status = STATUS_PENDING;

    return RESP_TEAM_INVITED;
}

/* ============================================================================
 * INVITE ACCEPT 
 * ============================================================================ */
int handle_invite_accept(ServerSession *session, const char *name) {
    if (!session || !name) return RESP_SYNTAX_ERROR;
    if (!session->isLoggedIn) return RESP_NOT_LOGGED;
    
    int current_team = find_team_id_by_username(session->username);
    if (current_team > 0) return RESP_ALREADY_IN_TEAM;
    
    Team *team = find_team_by_name(name);
    if (!team) return RESP_TEAM_NOT_FOUND;
    

    if (get_team_member_count(team->team_id) >= MAX_TEAM_MEMBERS) {
        return RESP_TEAM_FULL;
    }
    
    int my_user_id = hashFunc(session->username);


    int invite_index = -1;

    for (int i = 0; i < team_invite_count; i++) {
        if (team_invites[i].team_id == team->team_id &&
            team_invites[i].invitee_id == my_user_id) { 
            invite_index = i;
            break;
        }
    }

    if (invite_index == -1) {
        return RESP_INVITE_NOT_FOUND; 
    }

    for (int i = invite_index; i < team_invite_count - 1; i++) {
        team_invites[i] = team_invites[i + 1];
    }
    team_invite_count--;

    if (team_member_count >= MAX_TEAMS * MAX_TEAM_MEMBERS) {
        return RESP_INTERNAL_ERROR;
    }

    TeamMember *member = &team_members[team_member_count++];
    member->team_id = team->team_id;
    strncpy(member->username, session->username, MAX_USERNAME - 1);
    member->username[MAX_USERNAME - 1] = '\0';
    member->role = 1; 
    member->joined_at = time(NULL);

    session->current_team_id = team->team_id;
    update_session_by_socket(session->socket_fd, session);
    
    return RESP_TEAM_INVITE_ACCEPTED;
}

/* ============================================================================
 * INVITE REJECT 
 * ============================================================================ */
int handle_invite_reject(ServerSession *session, const char *name) {
    if (!session || !name) return RESP_SYNTAX_ERROR;
    if (!session->isLoggedIn) return RESP_NOT_LOGGED;
    
    Team *team = find_team_by_name(name);
    if (!team) return RESP_TEAM_NOT_FOUND;
    
    int my_user_id = hashFunc(session->username);

    int invite_index = -1;

    for (int i = 0; i < team_invite_count; i++) {
        if (team_invites[i].team_id == team->team_id &&
            team_invites[i].invitee_id == my_user_id) {
            invite_index = i;
            break;
        }
    }

    if (invite_index == -1) {
        return RESP_INVITE_NOT_FOUND; 
    }

    for (int i = invite_index; i < team_invite_count - 1; i++) {
        team_invites[i] = team_invites[i + 1];
    }
    team_invite_count--;

    return RESP_TEAM_INVITE_REJECTED;
}

/* ============================================================================
 * CHECK MY INVITES
 * ============================================================================ */
int handle_check_invites(ServerSession *session, char *output_buf, size_t buf_size) {
    if (!session || !output_buf || buf_size == 0) return RESP_SYNTAX_ERROR;
    if (!session->isLoggedIn) return RESP_NOT_LOGGED;

    int my_id = hashFunc(session->username);
    int count = 0;
    output_buf[0] = '\0';
    char temp[256];

    // Duyệt qua tất cả lời mời
    for (int i = 0; i < team_invite_count; i++) {
        // Kiểm tra xem lời mời có phải gửi cho mình không và trạng thái Pending
        if (team_invites[i].invitee_id == my_id && 
            team_invites[i].status == STATUS_PENDING) {
            
            Team *t = find_team_by_id(team_invites[i].team_id);
            if (t) {
                // Format: "TeamName (ID: X)|"
                // Dùng dấu | làm vách ngăn để Client dễ tách
                snprintf(temp, sizeof(temp), "%s (ID: %d)|", t->name, t->team_id);
                
                if (strlen(output_buf) + strlen(temp) < buf_size - 1) {
                    strcat(output_buf, temp);
                    count++;
                }
            }
        }
    }

    if (count == 0) {
        // Không có lời mời
        return RESP_INVITE_NOT_FOUND; // Hoặc một mã riêng như 206
    }

    return RESP_OK; // Dùng mã 200 hoặc mã tương ứng
}


