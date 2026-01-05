#include "session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "config.h"
#include "hash.h"
#include <unistd.h>
#include "users.h"
#include "users_io.h"
#include <ctype.h>
#include "db_schema.h"  // For FILE_USERS and function declarations



/* Global session manager with mutex for thread safety */
static SessionManager session_mgr = {NULL, 0};

//từ db.c
extern TreasureChest active_chests[];
extern ChestPuzzle puzzles[];
extern WeaponTemplate weapon_templates[];
void initServerSession(ServerSession *s) {
    if (!s) return;
    s->isLoggedIn = false;
    s->username[0] = '\0';
    s->socket_fd = -1;
    memset(&s->client_addr, 0, sizeof(s->client_addr));
    s->current_match_id = -1;
    s->current_team_id = -1;    
}

int server_handle_login(ServerSession *session, UserTable *ut, const char *username, const char *password) {
    if (!session || !ut || !username || !password) {
        return RESP_SYNTAX_ERROR;
    }
    
    /* Check if already logged in on this session */
    if (session->isLoggedIn) {
        return RESP_ALREADY_LOGGED;
    }
    
    /* Find user */
    User *user = findUser(ut, username);
    if (!user) {
        return RESP_ACCOUNT_NOT_FOUND;
    }
    
    if (user->status == USER_BANNED) {
        return RESP_ACCOUNT_LOCKED;
    }
    
    /* Validate password */
    if (!verifyPassword(password, user->password_hash)) {
        return RESP_WRONG_PASSWORD;
    }
    
    /* Login successful - update session */
    session->isLoggedIn = true;
    strncpy(session->username, user->username, MAX_USERNAME - 1);
    session->username[MAX_USERNAME - 1] = '\0';
    
    session->current_team_id = find_team_id_by_username(session->username);
    /* Update session in manager */
    if (!update_session_by_socket(session->socket_fd, session)) {
        /* If update failed, try to add new session */
        add_session(session);
    }
    
    return RESP_LOGIN_OK;
}

int server_handle_register(UserTable *ut, const char *username, const char *password) {
    if (!ut || !username || !password) {
        return RESP_SYNTAX_ERROR;
    }
    
    /* Validate username format */
    if (!validateUsername(username)) {
        return RESP_INVALID_USERNAME;
    }
    
    /* Validate password format */
    if (!validatePassword(password)) {
        return RESP_WEAK_PASSWORD;
    }
    
    /* Check if username already exists */
    if (findUser(ut, username)) {
        return RESP_USERNAME_EXISTS;
    }
    
    /* Hash password */
    char password_hash[MAX_PASSWORD_HASH];
    hashPassword(password, password_hash);
    
    /* Create new user */
    User *user = createUser(ut, username, password_hash);
    if (!user) {
        return RESP_INTERNAL_ERROR;
    }
    
    /* Save to file for persistence */
    saveUsers(ut, FILE_USERS);
    
    return RESP_REGISTER_OK;
}

int server_handle_bye(ServerSession *session) {
    if (!session) {
        return RESP_SYNTAX_ERROR;
    }
    
    if (!session->isLoggedIn) {
        return RESP_NOT_LOGGED;
    }
    
    /* Update local session first */
    session->isLoggedIn = false;
    session->username[0] = '\0';
    
    /* Update session in manager */
    update_session_by_socket(session->socket_fd, session);
    
    return RESP_LOGOUT_OK;
}

bool server_is_logged_in(ServerSession *session) {
    return session && session->isLoggedIn;
}

int server_handle_whoami(ServerSession *session, char *username_out) {
    if (!session || !username_out) {
        return RESP_SYNTAX_ERROR;
    }
    
    if (!session->isLoggedIn) {
        return RESP_NOT_LOGGED;
    }
    
    strncpy(username_out, session->username, MAX_USERNAME - 1);
    username_out[MAX_USERNAME - 1] = '\0';
    
    return RESP_WHOAMI_OK;
}

int server_handle_buyarmor(ServerSession *session, UserTable *ut, int armor_type) {
    if (!session || !ut) {
        return RESP_SYNTAX_ERROR;
    }
    
    if (!session->isLoggedIn) {
        return RESP_NOT_LOGGED;
    }

    int match_id = session->current_match_id;    
    if (match_id <= 0) {
        return RESP_NOT_IN_MATCH;
    }

    if (armor_type < ARMOR_BASIC || armor_type > ARMOR_ENHANCED) {
        return RESP_ARMOR_NOT_FOUND;
    }
    

    Ship *ship = find_ship(match_id, session->username);
    if (!ship) {
        return RESP_INTERNAL_ERROR; 
    } 
    return ship_buy_armor(ut, ship, session->username, armor_type);
}

int server_handle_repair(ServerSession *session, UserTable *ut, int repair_amount, RepairResult *out) {
    if (!session || !ut) {
        return RESP_INTERNAL_ERROR;
    }

    if (!session->isLoggedIn) {
        return RESP_NOT_LOGGED;
    }

    if (repair_amount <= 0) {
        return RESP_SYNTAX_ERROR;
    }

    int match_id = session->current_match_id;
    if (match_id <= 0) { 
        return RESP_NOT_IN_MATCH;
    }

    Ship *ship = find_ship(session->current_match_id, session->username);
    User *user = findUser(ut, session->username);

    if (!ship || !user) {
        return RESP_INTERNAL_ERROR;
    }

    int maxHP = SHIP_DEFAULT_HP;
    int currentHP = ship->hp;

    if (currentHP >= maxHP) {
        return RESP_ALREADY_FULL_HP;
    }

    int missingHP = maxHP - currentHP;
    int actualRepair = (repair_amount < missingHP) ? repair_amount : missingHP;
    int cost = actualRepair;

    if (user->coin < cost) { 
        return RESP_NOT_ENOUGH_COIN;
    }

    /* Apply changes */
    ship->hp += actualRepair;
    user->coin -= cost;

    if (out) {
        out->hp = ship->hp;
        out->coin = user->coin;
    }

    // TODO: persist ship & user to DB
    return RESP_REPAIR_OK;
}

int server_handle_buy_weapon(ServerSession *session, UserTable *ut, int weapon_type) {
    if (!session || !ut) {
        return RESP_SYNTAX_ERROR;
    }
    
    if (!session->isLoggedIn) {
        return RESP_NOT_LOGGED;
    }
    
    int match_id = session->current_match_id;
    if (match_id <= 0) {
        /* Fallback */
        match_id = find_current_match_by_username(session->username);
        if (match_id > 0) {
            session->current_match_id = match_id; 
        }
    }
    
    if (match_id <= 0) {
        return RESP_NOT_IN_MATCH;
    }
    
    if (weapon_type < WEAPON_CANNON || weapon_type > WEAPON_MISSILE) {
        return RESP_INTERNAL_ERROR;
    }
    
    Ship *ship = find_ship(match_id, session->username);
    if (!ship) {
        return RESP_INTERNAL_ERROR; 
    }
         
    return ship_buy_weapon(ut, ship, session->username, weapon_type);
}

int server_handle_start_match(ServerSession *session, int opponent_team_id) {
    // 1. Validate input
    if (!session) {
        return RESP_SYNTAX_ERROR;
    }
    
    if (opponent_team_id <= 0) {
        return RESP_SYNTAX_ERROR;
    }
    
    // 2. Check if user is logged in
    if (!session->isLoggedIn) {
        return RESP_NOT_LOGGED;
    }
    
    // 3. Get user's current team
    int user_team_id = session->current_team_id;
    if (user_team_id <= 0) {
       return RESP_NOT_IN_TEAM;
    }
    
    // 5. Validate user's team exists
    Team *user_team = find_team_by_id(user_team_id);
    if (!user_team) {
        return RESP_TEAM_NOT_FOUND;
    }
    
    // 6. Check if user is the team creator
    
    if (strcmp(user_team->creator_username, session->username) != 0) {
        return RESP_NOT_CREATOR;
    }
    
    // 7. Check if trying to match with own team
    if (opponent_team_id == user_team_id) {
        return RESP_SYNTAX_ERROR;
    }
    
    // 8. Validate opponent team exists
    Team *opponent_team = find_team_by_id(opponent_team_id);
    if (!opponent_team) {
        return RESP_OPPONENT_NOT_FOUND;
    }
    
    // 9. Check if user's team is already in a match
    int existing_match = find_running_match_by_team(user_team_id);
    if (existing_match >= 0) {
        return RESP_TEAM_IN_MATCH;
    }
    
    // 10. Check if opponent team is already in a match
    existing_match = find_running_match_by_team(opponent_team_id);
    if (existing_match >= 0) {
        return RESP_TEAM_IN_MATCH;
    }
    
    // 11. Create the match
    Match *new_match = create_match(user_team_id, opponent_team_id);
    if (!new_match) {
        return RESP_MATCH_CREATE_FAILED;
    }
    
    // 12. Create ships for all team members
    extern TeamMember team_members[];
    extern int team_member_count;
    
    /* Create ships for all members and update their sessions' current_match_id */
    for (int i = 0; i < team_member_count; i++) {
        int tid = team_members[i].team_id;
        if (tid == new_match->team1_id || tid == new_match->team2_id) {
            const char *member_username = team_members[i].username;

            /* Create ship for member */
            create_ship(new_match->match_id, member_username);

            /* Update member session if online */
            SessionNode *node = find_session_by_username(member_username);
            if (node) {
                node->session.current_match_id = new_match->match_id;
                update_session_by_socket(node->session.socket_fd, &node->session);
            }
        }
    }
    
    // 13. Update session with new match ID
    session->current_match_id = new_match->match_id;
    update_session_by_socket(session->socket_fd, session);
    
    // 14. Cập nhật current_match_id cho tất cả players trong match TRƯỚC
    extern TeamMember team_members[];
    extern int team_member_count;
    SessionNode *current = session_mgr.head;
    while (current != NULL) {
        if (current->session.isLoggedIn) {
            int user_team_id_check = find_team_id_by_username(current->session.username);
            if (user_team_id_check == user_team_id || user_team_id_check == opponent_team_id) {
                current->session.current_match_id = new_match->match_id;
                update_session_by_socket(current->session.socket_fd, &current->session);
            }
        }
        current = current->next;
    }
    
    // 15. Không gọi broadcast_chest_drop() ở đây nữa
    // Router sẽ gọi broadcast sau khi gửi response để đảm bảo thứ tự đúng
    
    // 16. Success - trả về match_id thông qua session->current_match_id
    return RESP_START_MATCH_OK;
}

static void clear_match_from_sessions(int match_id) {
    SessionNode *cur = session_mgr.head;
    while (cur) {
        if (cur->session.current_match_id == match_id) {
            cur->session.current_match_id = -1;
        }
        cur = cur->next;
    }
}

int server_handle_end_match(ServerSession *session, int match_id) {
    if (!session) return RESP_SYNTAX_ERROR;
    if (!session->isLoggedIn) return RESP_NOT_LOGGED;
    if (match_id <= 0) return RESP_SYNTAX_ERROR;

    Match *match = find_match_by_id(match_id);
    if (!match) {
        return RESP_MATCH_NOT_FOUND;
    }

    if (match->status != MATCH_RUNNING) {
        return RESP_MATCH_FINISHED;
    }

    int user_team_id = session->current_team_id;
    if (user_team_id <= 0) {
        return RESP_NOT_IN_TEAM;
    }
    if (user_team_id != match->team1_id && user_team_id != match->team2_id) {
        return RESP_NOT_AUTHORIZED;
    }

    int winner_team_id = -1;
    if (!can_end_match(match_id, &winner_team_id)) {
        // Match cannot end yet (both teams still have alive ships)
        return RESP_MATCH_RUNNING;
    }

    // End the match and record winner (or draw if -1)
    end_match(match_id, winner_team_id);

    // Clear match_id from all sessions participating in this match
    clear_match_from_sessions(match_id);

    return RESP_END_MATCH_OK;
}

int server_handle_get_match_result(ServerSession *session, int match_id) {
    if (!session) return RESP_SYNTAX_ERROR;
    if (!session->isLoggedIn) return RESP_NOT_LOGGED;
    if (match_id <= 0) return RESP_SYNTAX_ERROR;
    
    // Find match by ID
    Match *match = find_match_by_id(match_id);
    if (!match) {
        return RESP_MATCH_NOT_FOUND;
    }
    
    // Verify user is in this match (either team1 or team2)
    int user_team_id = session->current_team_id;
    if (user_team_id <= 0) {
        return RESP_NOT_IN_TEAM;
    }
    
    if (user_team_id != match->team1_id && user_team_id != match->team2_id) {
        return RESP_NOT_AUTHORIZED;
    }
    
    // Check match status
    if (match->status == MATCH_RUNNING) {
        return RESP_MATCH_RUNNING;
    }
    
    if (match->status == MATCH_FINISHED) {
        return RESP_MATCH_RESULT_OK;
    }
    
    return RESP_INTERNAL_ERROR;
}

int server_handle_get_hp(ServerSession *session, int *hp_out, int *max_hp_out) {
    if (!session || !hp_out || !max_hp_out) return RESP_SYNTAX_ERROR;
    if (!session->isLoggedIn) return RESP_NOT_LOGGED;

    int match_id = session->current_match_id;
    if (match_id <= 0) return RESP_NOT_IN_MATCH;

    Ship *ship = find_ship(match_id, session->username);
    if (!ship) return RESP_INTERNAL_ERROR;

    *hp_out = ship->hp;
    *max_hp_out = SHIP_DEFAULT_HP;
    return RESP_HP_INFO_OK;
}


int server_handle_fire(ServerSession *session,
                       char* target_name,
                       int weapon_type,
                       FireResult *result)
{
    if (!session || !session->isLoggedIn)
        return RESP_NOT_LOGGED;

    if (session->current_match_id <= 0) {
        int found_match = find_current_match_by_username(session->username);
        if (found_match > 0) {
            session->current_match_id = found_match;
        } else {
            return RESP_NOT_IN_MATCH; 
        }
    }
    
    // Clean target name - dùng buffer an toàn
    char clean_name[128];
    int j = 0;
    for (int i = 0; target_name[i] != '\0' && j < (int)(sizeof(clean_name) - 1); i++) {
        // Chỉ giữ lại chữ cái (a-z, A-Z) và số (0-9)
        if (isalnum((unsigned char)target_name[i])) {
            clean_name[j++] = target_name[i];
        }
    }
    clean_name[j] = '\0'; // Kết thúc chuỗi
    
    if (j == 0) {
        return RESP_INVALID_TARGET; // Tên không hợp lệ
    }
    
    printf("[DEBUG] Cleaning name: '%s' -> '%s'\n", target_name, clean_name); // Log để kiểm tra
    
    Ship *attacker = find_ship(
        session->current_match_id,
        session->username
    );

    Ship *target = find_ship_by_name(clean_name);

    if (!attacker || !target) {
        return RESP_INVALID_TARGET;//343
    }
    
    // Kiểm tra target có cùng match_id với attacker không
    if (target->match_id != session->current_match_id) {
        return RESP_INVALID_TARGET; // Target không ở cùng match
    }
    
    //Lấy team của người bắn từ session
    int attacker_team_id = session->current_team_id;
    //Lấy team của mục tiêu thông qua player_id
    int target_team_id = find_team_id_by_username(target->player_username);
    //Kiểm tra bắn đồng đội
    if (attacker_team_id == target_team_id) {
        return RESP_INVALID_TARGET; // Không bắn phe mình
    }

    int rc = calculate_and_update_damage(attacker, target, weapon_type, result);

    if (rc != 0) {
        // const char *msg = "Fire Failed";
        // if (rc == RESP_OUT_OF_AMMO) msg = "Out of Ammo";
        // else if (rc == RESP_WEAPON_NOT_EQUIPPED) msg = "Weapon Not Equipped";
        // else if (rc == RESP_TARGET_DESTROYED) msg = "Target Already Destroyed";

        // send_error_response(session->socket_fd, rc, msg);
        return rc;
    }

    return RESP_FIRE_OK;
}



// Tính toán sát thương và trừ đạn
int calculate_and_update_damage(Ship* attacker, Ship* target, int weapon_type, FireResult *out) {
    
    int damage = 0;

    // Check vũ khí//dam,name,..
    switch (weapon_type) {
        case WEAPON_CANNON: // 0
            // Kiểm tra biến cannon_ammo trong struct Ship
            if (attacker->cannon_ammo <= 0) return RESP_OUT_OF_AMMO;
            
            attacker->cannon_ammo--;       // Trừ đạn trực tiếp
            damage = CANNON_DAMAGE;        // Lấy damage = 10 từ config
            break;

        case WEAPON_LASER: // 1
            // Kiểm tra biến laser_count
            if (attacker->laser_count <= 0) return RESP_OUT_OF_AMMO;
            
            attacker->laser_count--;       // Trừ số lần bắn
            damage = LASER_DAMAGE;         // Lấy damage = 100
            break;

        case WEAPON_MISSILE: // 2
            // Kiểm tra biến missile_count
            if (attacker->missile_count <= 0) return RESP_OUT_OF_AMMO;
            
            attacker->missile_count--;     // Trừ tên lửa
            damage = MISSILE_DAMAGE;       // Lấy damage = 800
            break;

        default:
            return RESP_WEAPON_NOT_EQUIPPED;
    }

    // Trừ giáp và máu
    int damage_remaining = damage;
    int total_damage_dealt = damage; 

    // Kiểm tra giáp
    if (target->armor_slot_2_value > 0) {
        // Check giáp 2
        if (target->armor_slot_2_value >= damage_remaining) {
            // Giáp chịu hết sát thương
            target->armor_slot_2_value -= damage_remaining;
            damage_remaining = 0;
        } else {
            // Giáp vỡ, sát thương dư trừ vào HP
            damage_remaining -= target->armor_slot_2_value;
            target->armor_slot_2_value = 0;
            target->armor_slot_2_type = ARMOR_NONE; // Hủy giáp
        }
    } 
    else if (target->armor_slot_1_value > 0) {
        // Không có giáp 2, check giáp 1
        if (target->armor_slot_1_value >= damage_remaining) {
            // Giáp chịu hết sát thương
            target->armor_slot_1_value -= damage_remaining;
            damage_remaining = 0;
        } else {
            // Giáp vỡ, sát thương dư trừ vào HP
            damage_remaining -= target->armor_slot_1_value;
            target->armor_slot_1_value = 0;
            target->armor_slot_1_type = ARMOR_NONE; // Hủy giáp
        }
    }
  
    // Không có giáp
    if (damage_remaining > 0) {
        target->hp -= damage_remaining;
        if (target->hp < 0) target->hp = 0;
    }
    //Ghi kết quả
    if (out) {
        out->attacker_id = hashFunc(attacker->player_username);
        out->target_id = hashFunc(target->player_username);
        out->damage_dealt = total_damage_dealt;
        out->target_remaining_hp = target->hp;
        out->target_remaining_armor = target->armor_slot_1_value + target->armor_slot_2_value;
    }

    return 0; // Thành công
}
int server_handle_send_challenge(ServerSession *session, int target_team_id, int *new_challenge_id) {
    if (!session || !session->isLoggedIn) return 315; // RESP_NOT_LOGGED
    
    
    //Lấy Tên Đội (my_team->name) điền vào tin nhắn gửi cho đối thủ
    Team *my_team = find_team_by_id(session->current_team_id);
    if (!my_team) {
        return 327; // RESP_NOT_IN_TEAM
    }
    // Kiểm tra quyền (chỉ Leader mới được thách đấu)
    if (strcmp(my_team->creator_username, session->username) != 0) {
        return 316; // RESP_NOT_CREATOR
    }

    // --- 2.Tạo bản ghi thách đấu trong database ---
    int id = create_challenge_record(session->current_team_id, target_team_id);
    if (id == -1) return 500; // RESP_INTERNAL_ERROR

    // Trả ID ra ngoài 
    if (new_challenge_id) {
        *new_challenge_id = id;
    }

    // --- 3. TÌm Đối thủ ---
    // Mục đích: Để tìm Socket của Leader đối thủ (target_session) để gửi tin
    Team *target_team = find_team_by_id(target_team_id);
    if (target_team) {
        // Tìm session của Leader đội bạn
        SessionNode *target_session = find_session_by_username(target_team->creator_username);
        
        // Nếu Leader đối thủ đang Online thì gửi thông báo
        if (target_session) {
            char msg[512];
            
            // Format tin nhắn: 150 <Tên_Team_Thách_Đấu> <ID_Team_Thách_Đấu> <Challenge_ID>
            // Ở đây dùng my_team->name để đối thủ biết ai đang thách đấu mình
            snprintf(msg, sizeof(msg), "%d CHALLENGE_RECEIVED %s %d %d\r\n", 
                     150, // Mã RESP_CHALLENGE_RECEIVED
                     my_team->name, 
                     my_team->team_id, 
                     id);
            
            // Gửi tin nhắn vào Socket của đối thủ
            write(target_session->session.socket_fd, msg, strlen(msg));
        }
    }

    return RESP_CHALLENGE_SENT; // 136
}

int server_handle_accept_challenge(ServerSession *session, int challenge_id) {
    if (!session || !session->isLoggedIn) return 315;

    Challenge *ch = find_challenge_by_id(challenge_id);
    if (!ch) return RESP_CHALLENGE_NOT_FOUND;
    if (ch->status != CHALLENGE_PENDING) return RESP_ALREADY_RESPONDED;

    // Kiểm tra Leader đội nhận thách đấu (Target Team)
    Team *target_team = find_team_by_id(ch->target_team_id);
    if (!target_team || strcmp(target_team->creator_username, session->username) != 0) {
        return 316; 
    }
    
    // Đánh dấu challenge đã được accept
    ch->status = CHALLENGE_ACCEPTED;
    
    // Tìm session của sender team leader để gọi start_match
    Team *sender_team = find_team_by_id(ch->sender_team_id);
    if (!sender_team) {
        return RESP_TEAM_NOT_FOUND;
    }
    
    SessionNode *sender_session_node = find_session_by_username(sender_team->creator_username);
    if (!sender_session_node) {
        // Nếu sender không online, vẫn tạo match nhưng không thể drop chest ngay
        // Fallback: tạo match trực tiếp
        Match *new_match = create_match(ch->sender_team_id, ch->target_team_id);
        if (!new_match) {
            return RESP_MATCH_CREATE_FAILED;
        }
        // Cập nhật match_id cho tất cả thành viên
        int match_id = new_match->match_id;
        session->current_match_id = match_id;
        update_session_by_socket(session->socket_fd, session);
        
        SessionNode *current = session_mgr.head;
        while (current != NULL) {
            if (current->session.isLoggedIn) {
                int user_team_id = find_team_id_by_username(current->session.username);
                if (user_team_id == ch->sender_team_id || user_team_id == ch->target_team_id) {
                    current->session.current_match_id = match_id;
                    update_session_by_socket(current->session.socket_fd, &current->session);
                }
            }
            current = current->next;
        }
        
        // Lưu ý: Gửi 151 MATCH_STARTED và broadcast_chest_drop sẽ được xử lý trong router.c
        // sau khi gửi response để đảm bảo thứ tự đúng: Response -> 151 -> 141
        return RESP_CHALLENGE_ACCEPTED;
    }

    int result = server_handle_start_match(&sender_session_node->session, ch->target_team_id);
    // Gọi server_handle_start_match() với session của sender team
    // Logic update sessions đã có trong server_handle_start_match()
    if (result == RESP_START_MATCH_OK) { // 126
        int match_id = sender_session_node->session.current_match_id;
        
        // --- CẬP NHẬT MATCH_ID CHO TẤT CẢ THÀNH VIÊN (đã có trong server_handle_start_match, nhưng đảm bảo chắc chắn) ---
        // server_handle_start_match đã cập nhật, nhưng cần đảm bảo cả session hiện tại (B) cũng được cập nhật
        session->current_match_id = match_id;
        update_session_by_socket(session->socket_fd, session);
        
        // Đảm bảo tất cả thành viên đều có match_id được cập nhật
        SessionNode *current = session_mgr.head;
        while (current != NULL) {
            if (current->session.isLoggedIn) {
                int user_team_id = find_team_id_by_username(current->session.username);
                if (user_team_id == ch->sender_team_id || user_team_id == ch->target_team_id) {
                    // Đảm bảo match_id đã được cập nhật
                    current->session.current_match_id = match_id;
                    update_session_by_socket(current->session.socket_fd, &current->session);
                }
            }
            current = current->next;
        }
        
        // Lưu ý: Gửi 151 MATCH_STARTED sẽ được xử lý trong router.c sau khi gửi response
        // để đảm bảo thứ tự đúng: Response -> 151 -> 141
        
        return RESP_CHALLENGE_ACCEPTED; // 131 (Trả về cho B)
    } else {
        return result; 
    
    }
}

int server_handle_decline_challenge(ServerSession *session, int challenge_id) {
    if (!session || !session->isLoggedIn) return 315;

    Challenge *ch = find_challenge_by_id(challenge_id);
    if (!ch) return RESP_CHALLENGE_NOT_FOUND;
    if (ch->status != CHALLENGE_PENDING) return RESP_ALREADY_RESPONDED;

    // Chỉ Leader team nhận lời mời mới được từ chối
    ch->status = CHALLENGE_DECLINED;
    return RESP_CHALLENGE_DECLINED;
}

int server_handle_cancel_challenge(ServerSession *session, int challenge_id) {
    if (!session || !session->isLoggedIn) return 315;

    Challenge *ch = find_challenge_by_id(challenge_id);
    if (!ch) return RESP_CHALLENGE_NOT_FOUND;
    if (ch->status != CHALLENGE_PENDING) return RESP_ALREADY_RESPONDED;

    // Kiểm tra: Chỉ đội gửi lời mời mới được hủy
    if (ch->sender_team_id != session->current_team_id) return RESP_NOT_SENDER;

    ch->status = CHALLENGE_CANCELED;
    return RESP_CHALLENGE_CANCELED;
}

//Tạo rương 
int server_spawn_chest(int match_id) {
    srand(time(NULL));//Sinh ngẫu nhiên = nowtime
    int idx = match_id % MAX_TEAMS;
    
    active_chests[idx].chest_id = rand() % 1000 + 1; // (1->1000)
    active_chests[idx].match_id = match_id;
    
    // Ngẫu nhiên "loại rương" theo tỷ lệ
    int r = rand() % 100;//tạo ngẫu nhiên từ 0->99
    if (r < 60) active_chests[idx].type = CHEST_BRONZE;      // 100 coin
    else if (r < 90) active_chests[idx].type = CHEST_SILVER; // 500 coin
    else active_chests[idx].type = CHEST_GOLD;               // 2000 coin

    active_chests[idx].position_x = MAP_WIDTH / 2; 
    active_chests[idx].position_y = MAP_HEIGHT / 2;
    active_chests[idx].is_collected = false;


    return active_chests[idx].chest_id;
}

void broadcast_match_started(int match_id) {
    if (match_id <= 0) return;
    
    char msg[512];
    snprintf(msg, sizeof(msg), "%d MATCH_STARTED %d\r\n", 
             RESP_MATCH_STARTED_NOTIFY, // 151
             match_id);
    
    // Gửi tới tất cả thành viên trong match
    SessionNode *current = session_mgr.head;
    while (current != NULL) {
        if (current->session.isLoggedIn && 
            current->session.current_match_id == match_id) {
            (void)write(current->session.socket_fd, msg, strlen(msg));
        }
        current = current->next;
    }
}

//Sinh rương và tb all
int broadcast_chest_drop(int match_id, int exclude_socket_fd) {
    // 1. Tạo rương
    int c_id = server_spawn_chest(match_id);
    TreasureChest *chest = find_chest_by_id_in_match(match_id, c_id);
    if (!chest) return -1;

    // 2. Chuẩn bị tin nhắn Broadcast (Mã 141)
    char notify[BUFF_SIZE];
    snprintf(notify, sizeof(notify), "%d %d %d %d %d\r\n", 
             RESP_CHEST_DROP_OK, c_id, (int)chest->type, chest->position_x, chest->position_y);

    // 3. Gửi cho TẤT CẢ players trong match (nếu exclude_socket_fd == -1 thì gửi cho tất cả)
    SessionNode *current = session_mgr.head;
    while (current != NULL) {
        if (current->session.isLoggedIn && 
            current->session.current_match_id == match_id) {
            // Nếu exclude_socket_fd == -1 thì gửi cho tất cả, ngược lại loại trừ exclude_socket_fd
            if (exclude_socket_fd == -1 || current->session.socket_fd != exclude_socket_fd) {
                (void)write(current->session.socket_fd, notify, strlen(notify));
            }
        }
        current = current->next;
    }

    printf("[SERVER INFO] Chest %d dropped in match %d\n", c_id, match_id);
    return c_id;
}
int server_handle_open_chest(ServerSession *session, UserTable *ut, int chest_id, const char *answer) {
    if (!session || !session->isLoggedIn) return RESP_NOT_LOGGED; // 315

    TreasureChest *chest = find_chest_by_id_in_match(session->current_match_id, chest_id);
    if (!chest) return RESP_CHEST_NOT_FOUND;       // 440
    if (chest->is_collected) return RESP_CHEST_OPEN_FAIL; // 339

    // Kiểm tra đáp án
    char q[256], a[64];
    get_chest_puzzle(chest->type, q, a); //
    
    // So sánh đáp án (không phân biệt hoa thường)
    if (strcasecmp(answer, a) != 0) {
        return 442; // RESP_WRONG_ANSWER
    }

    // --- TRẢ LỜI ĐÚNG ---
    chest->is_collected = true;
    
    // Tính thưởng và cộng vào user->coin (FIX: không dùng session->coins)
    int reward = (chest->type == CHEST_BRONZE) ? 100 : (chest->type == CHEST_SILVER ? 500 : 2000);
    if (ut) {
        updateUserCoin(ut, session->username, reward);
    }

    // Broadcast thông báo cho mọi người biết rương đã bị nhặt
    char notify[BUFF_SIZE];
    snprintf(notify, sizeof(notify), "210 CHEST_COLLECTED %s %d\r\n", session->username, chest_id);
    
    // Gửi broadcast cho tất cả players trong match
    SessionNode *current = session_mgr.head;
    while (current != NULL) {
        if (current->session.isLoggedIn && 
            current->session.current_match_id == session->current_match_id) {
            (void)write(current->session.socket_fd, notify, strlen(notify));
        }
        current = current->next;
    }

    return RESP_CHEST_OPEN_OK; // 127
}

// 3. Hàm lấy câu hỏi (Giữ nguyên như bạn viết)
int server_handle_get_chest_question(ServerSession *session, int chest_id, char *question_out) {
    if (!session || !session->isLoggedIn) return RESP_NOT_LOGGED;

    TreasureChest *chest = find_chest_by_id_in_match(session->current_match_id, chest_id);
    if (!chest) return RESP_CHEST_NOT_FOUND;       // 440
    if (chest->is_collected) return RESP_CHEST_OPEN_FAIL; // 339

    char dummy_ans[64];
    get_chest_puzzle(chest->type, question_out, dummy_ans); //
    
    return RESP_CHEST_QUESTION; // 211
}
// Hàm lấy câu hỏi dựa trên loại rương
void get_chest_puzzle(ChestType type, char *q_out, char *a_out) {
    strcpy(q_out, puzzles[(int)type].question);
    strcpy(a_out, puzzles[(int)type].answer);
}

//Tìm rương trong trận đấu theo id
TreasureChest* find_chest_by_id_in_match(int match_id, int chest_id) {
    int idx = match_id % MAX_TEAMS;
    if (active_chests[idx].chest_id == chest_id && active_chests[idx].match_id == match_id) {
        return &active_chests[idx];
    }
    return NULL;
}


  
/* ====== Session Manager Implementation ====== */

void init_session_manager(void) {
    /* Initialize session manager */
    session_mgr.head = NULL;
    session_mgr.count = 0;
}

void cleanup_session_manager(void) {
    SessionNode *current = session_mgr.head;
    while (current != NULL) {
        SessionNode *next = current->next;
        free(current);
        current = next;
    }
    session_mgr.head = NULL;
    session_mgr.count = 0;
}
 
// Hàm gửi phản hồi lỗi nhanh qua socket
void send_error_response(int socket_fd, int error_code, const char *details) {
    char buffer[512];
    // Tạo chuỗi phản hồi: "Mã_Lỗi Thông_Điệp\r\n"
    snprintf(buffer, sizeof(buffer), "%d %s\r\n", error_code, details ? details : "UNKNOWN_ERROR");
    
    if (socket_fd > 0) {
        (void)write(socket_fd, buffer, strlen(buffer));
    }
}
void broadcast_fire_event(const char* attacker_name, const char* target_name, int damage_dealt, int target_remaining_hp, int target_remaining_armor) {
    //Tìm trận đấu (match_id) dựa vào người bắn
    Ship *attacker_ship = find_ship_by_name(attacker_name);
    if (!attacker_ship) return;
    
    int match_id = attacker_ship->match_id;

    //Tạo bản tin thông báo (Protocol 131)
    char msg[512];
    snprintf(msg, sizeof(msg), "131 FIRE_EVENT %s %s %d %d %d\r\n", 
             attacker_name, target_name, damage_dealt, target_remaining_hp, target_remaining_armor);

    // Duyệt danh sách session và gửi cho những người CÙNG TRẬN ĐẤU
    // pthread_mutex_lock(&session_mutex);
    SessionNode *current = session_mgr.head;
    while (current != NULL) {
        // Chỉ gửi nếu đã login và đang ở trong cùng match_id
        if (current->session.isLoggedIn && current->session.current_match_id == match_id) {
            // Kiểm tra: Nếu là người bắn thì KHÔNG gửi broadcast (tránh trùng lặp)
            if (strcmp(current->session.username, attacker_name) != 0) {
                (void)write(current->session.socket_fd, msg, strlen(msg));
            }
        }
        current = current->next;
    }
    // pthread_mutex_unlock(&session_mutex);
}
SessionNode *find_session_by_username(const char *username) {
    if (!username) return NULL;
    
    SessionNode *current = session_mgr.head;
    SessionNode *result = NULL;
    while (current != NULL) {
        if (current->session.isLoggedIn && 
            strcmp(current->session.username, username) == 0) {
            result = current;
            break;
        }
        current = current->next;
    }
    return result;
}

int get_fd_by_username(const char *username) {
    SessionNode *node = find_session_by_username(username);

    if (node != NULL) {
        return node->session.socket_fd;
    }
    
    return -1;
}


SessionNode *find_session_by_socket(int socket_fd) {
    SessionNode *current = session_mgr.head;
    SessionNode *result = NULL;
    while (current != NULL) {
        if (current->session.socket_fd == socket_fd) {
            result = current;
            break;
        }
        current = current->next;
    }
    return result;
}

bool add_session(ServerSession *session) {
    if (!session || session->socket_fd < 0) return false;
    
    /* Check if session with this socket already exists */
    SessionNode *existing = session_mgr.head;
    while (existing != NULL) {
        if (existing->session.socket_fd == session->socket_fd) {
            return false; /* Already exists */
        }
        existing = existing->next;
    }
    
    /* Create new session node */
    SessionNode *new_node = (SessionNode *)malloc(sizeof(SessionNode));
    if (!new_node) {
        return false;
    }
    
    new_node->session = *session;
    new_node->next = session_mgr.head;
    session_mgr.head = new_node;
    session_mgr.count++;
    return true;
}

bool remove_session_by_socket(int socket_fd) {
    SessionNode *current = session_mgr.head;
    SessionNode *prev = NULL;
    
    while (current != NULL) {
        if (current->session.socket_fd == socket_fd) {
            if (prev == NULL) {
                session_mgr.head = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            session_mgr.count--;
            return true;
        }
        prev = current;
        current = current->next;
    }
    return false;
}

bool remove_session_by_username(const char *username) {
    if (!username) return false;
    
    SessionNode *current = session_mgr.head;
    SessionNode *prev = NULL;
    
    while (current != NULL) {
        if (current->session.isLoggedIn && 
            strcmp(current->session.username, username) == 0) {
            if (prev == NULL) {
                session_mgr.head = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            session_mgr.count--;
            return true;
        }
        prev = current;
        current = current->next;
    }
    
    return false;
}

bool update_session_by_socket(int socket_fd, ServerSession *session) {
    if (!session || socket_fd < 0) return false;
    
    SessionNode *current = session_mgr.head;
    while (current != NULL) {
        if (current->session.socket_fd == socket_fd) {
            current->session = *session;
            return true;
        }
        current = current->next;
    }
    
    return false;
}

int get_active_session_count(void) {
    return session_mgr.count;
}

/* ============================================================================
 * MATCH INFO HANDLER
 * ============================================================================ */

// External references to db.c arrays
extern TeamMember team_members[];
extern int team_member_count;

int server_handle_match_info(int match_id, char *output, size_t output_size, UserTable *user_table) {
    if (!output || output_size == 0) {
        return RESP_INTERNAL_ERROR;
    }

    Match *match = find_match_by_id(match_id);
    if (!match) {
        return RESP_MATCH_NOT_FOUND;
    }
    
    // Get team info
    Team *team1 = find_team_by_id(match->team1_id);
    Team *team2 = find_team_by_id(match->team2_id);
    if (!team1 || !team2) {
        return RESP_INTERNAL_ERROR;
    }
    
    // Build output string
    char buffer[4096] = {0};
    int offset = 0;
    
    // Match header
    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                      "=== MATCH #%d ===\n", match_id);
    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                      "Status: %s\n", 
                      match->status == MATCH_PENDING ? "Pending" :
                      match->status == MATCH_RUNNING ? "Running" :
                      match->status == MATCH_FINISHED ? "Finished" : "Canceled");
    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                      "Duration: %d seconds\n", match->duration);
    if (match->status == MATCH_FINISHED) {
        if (match->winner_team_id == -1) {
            offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                             "Result: DRAW\n");
        } else {
            Team *winner = find_team_by_id(match->winner_team_id);
            offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                             "Winner: Team %s (ID: %d)\n", 
                             winner ? winner->name : "Unknown", match->winner_team_id);
        }
    }
    
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\n");
    
    // Team 1 info
    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                      "--- TEAM 1: %s (ID: %d) ---\n", team1->name, team1->team_id);
    // Get team 1 members
    for (int i = 0; i < team_member_count; i++) {
        if (team_members[i].team_id == team1->team_id) {
            const char *username = team_members[i].username;
            offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                             "  Player: %s", username);
            User *user = findUser(user_table, username);
            if (user) {
            } else {
            }
            // Find ship for this player
            Ship *ship = find_ship(match_id, username);
            if (ship) {
                offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                                 " | HP: %d | Armor1: %d | Armor2: %d | Cannon: %d | Laser: %d | Missile: %d\n",
                                 ship->hp,
                                 ship->armor_slot_1_value,
                                 ship->armor_slot_2_value,
                                 ship->cannon_ammo,
                                 ship->laser_count,
                                 ship->missile_count);
        
            } else {
                offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                                 " | No ship data\n");
            }
        }
    }
    
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\n");
    
    // Team 2 info
    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                      "--- TEAM 2: %s (ID: %d) ---\n", team2->name, team2->team_id);
    // Get team 2 members
    for (int i = 0; i < team_member_count; i++) {
        if (team_members[i].team_id == team2->team_id) {
            const char *username = team_members[i].username;
            offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                             "  Player: %s", username);
                             
            User *user = findUser(user_table, username);
        

            // Find ship for this player
            Ship *ship = find_ship(match_id, username);
            if (ship) {
                offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                                 " | HP: %d | Armor1: %d | Armor2: %d | Cannon: %d | Laser: %d | Missile: %d\n",
                                 ship->hp,
                                 ship->armor_slot_1_value,
                                 ship->armor_slot_2_value,
                                 ship->cannon_ammo,
                                 ship->laser_count,
                                 ship->missile_count);
            
            } else {
                offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                                 " | No ship data\n");
            }
        }
    }
    
    // Copy to output
    strncpy(output, buffer, output_size - 1);
    output[output_size - 1] = '\0';
    return RESP_MATCH_INFO_OK;
}
