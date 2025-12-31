#include "session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "config.h"
#include "hash.h"
#include "users.h"
#include "users_io.h"
#include "db_schema.h"  // For FILE_USERS and function declarations
#include "dc.c"

/* Global session manager with mutex for thread safety */
static SessionManager session_mgr = {NULL, 0};

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
    // khong can thiet
    // if (match_id <= 0) {
    //     /* Fallback */
    //     match_id = find_current_match_by_username(session->username);
    //     if (match_id > 0) {
    //         session->current_match_id = match_id; 
    //     }
    // }
    
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
    int user_id = (int)hashFunc(session->username);
    if (user_team->creator_id != user_id) {
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
    
    // 12. Update session with new match ID
    session->current_match_id = new_match->match_id;
    update_session_by_socket(session->socket_fd, session);
    
    // 13. Success
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


// void process_fire_request(int attacker_id, int target_id, int weapon_id) {
//     Ship* target = find_ship_by_id(target_id);
//     Ship* attacker = find_ship_by_id(attacker_id);
//     int result_code = 0;

//     // Kiểm tra tính hợp lệ của mục tiêu và đồng đội
//     if (!target || !attacker || target->team_id == attacker->team_id) {
//         send_error_response(attacker->socket_fd, ERROR_INVALID_TARGET, "Invalid target ID or friendly fire.");
//         return; 
//     }

//     result_code = calculate_and_update_damage(attacker, target, weapon_id);

//     // Xử lý báo lỗi nếu có
//     if (result_code != 0) {
//         char* error_msg = "Fire Failed";
//         if (result_code == ERROR_OUT_OF_AMMO) error_msg = "Out of Ammo";
//         else if (result_code == ERROR_WEAPON_NOT_EQUIPPED) error_msg = "Weapon Not Equipped (You dont have this ID)";
//         else if (result_code == ERROR_TARGET_DESTROYED) error_msg = "Target Already Destroyed";
        
//         send_error_response(attacker->socket_fd, result_code, error_msg);
//     }
// }

// Tính toán sát thương và trừ đạn
// int calculate_and_update_damage(Ship* attacker, Ship* target, int weapon_id) {
    
//     // TÌM VŨ KHÍ THEO ID
//     int index = -1;
//     for (int i = 0; i < MAX_WEAPONS; i++) {
//         // Tìm thấy ID vũ khí trùng khớp trong các slot
//         if (attacker->weapons[i].weapon_id == weapon_id) {
//             index = i;
//             break; 
//         }
//     }

//     // Nếu không tìm thấy vũ khí ID này trên tàu
//     if (index == -1) {
//         return ERROR_WEAPON_NOT_EQUIPPED; 
//     }
    
//     EquippedWeapon* eq_weapon = &attacker->weapons[index];

//     // KIỂM TRA ĐẠN 
//     if (eq_weapon->current_ammo <= 0) {
//         return ERROR_OUT_OF_AMMO; 
//     }

//     // LẤY THÔNG SỐ SÁT THƯƠNG
//     WeaponTemplate* template = get_weapon_template(eq_weapon->weapon_id);
//     if (!template) return 500; // Lỗi hệ thống 

//     int base_damage = template->damage;
    
//     // TÍNH TOÁN
//     eq_weapon->current_ammo--; // Trừ 1 viên đạn
    
//     int damage_to_deal = base_damage;
//     int total_damage_dealt = 0;
//     int hp_loss = 0;
    
//     // Armor Logic
//     if (target->armor > 0) {
//         if (damage_to_deal <= target->armor) {
//             target->armor -= damage_to_deal; 
//             total_damage_dealt = damage_to_deal;
//         } else {
//             int damage_spillover = damage_to_deal - target->armor;
//             target->armor = 0; // Giáp vỡ
            
//             hp_loss = damage_spillover;
//             target->health -= hp_loss; // Trừ vào máu phần dư
//             total_damage_dealt = damage_to_deal;
//         }
//     } else {
//         hp_loss = damage_to_deal;
//         target->health -= hp_loss;
//         total_damage_dealt = damage_to_deal;
//     }

//     if (target->health < 0) {
//         target->health = 0;
//     }

//     // UPDATE
//     update_ship_state(attacker); // Lưu số đạn mới
//     update_ship_state(target);   // Lưu HP/Armor mới

   
//     send_fire_ok(attacker->socket_fd, target->ship_id, total_damage_dealt, target->health, target->armor);
//     broadcast_fire_event(attacker->ship_id, target->ship_id, total_damage_dealt, target->health, target->armor);
    
//     return 0; 
// }

int server_handle_send_challenge(ServerSession *session, int target_team_id, int *new_challenge_id) {
    if (!session || !session->isLoggedIn) return 315; // RESP_NOT_LOGGED

    
    Team *my_team = find_team_by_id(session->current_team_id);
    if (!my_team || strcmp(my_team->creator_username, session->username) != 0) {
        return 316; // RESP_NOT_CREATOR
    }
    

    int id = create_challenge_record(session->current_team_id, target_team_id);
    if (id == -1) return 500; // RESP_INTERNAL_ERROR

    *new_challenge_id = id;
    return RESP_CHALLENGE_SENT;
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
    

    ch->status = CHALLENGE_ACCEPTED;
    
     create_match(ch->sender_team_id, ch->target_team_id);
    
    return RESP_CHALLENGE_ACCEPTED;
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

// //Tạo rương 
// int server_spawn_chest(int match_id) {
//     srand(time(NULL));//Sinh ngẫu nhiên = nowtime
//     int idx = match_id % MAX_TEAMS;
    
//     active_chests[idx].chest_id = rand() % 1000 + 1; // (1->1000)
//     active_chests[idx].match_id = match_id;
    
//     // Ngẫu nhiên "loại rương" theo tỷ lệ
//     int r = rand() % 100;//tạo ngẫu nhiên từ 0->99
//     if (r < 60) active_chests[idx].type = CHEST_BRONZE;      // 100 coin
//     else if (r < 90) active_chests[idx].type = CHEST_SILVER; // 500 coin
//     else active_chests[idx].type = CHEST_GOLD;               // 2000 coin

//     active_chests[idx].position_x = MAP_WIDTH / 2; 
//     active_chests[idx].position_y = MAP_HEIGHT / 2;
//     active_chests[idx].is_collected = false;


//     return active_chests[idx].chest_id;
// }

// //Sinh rương và tb all
// void broadcast_chest_drop(int match_id) {
//     int c_id = server_spawn_chest(match_id);
//     TreasureChest *chest = &active_chests[match_id % MAX_TEAMS];

//     char notify[BUFF_SIZE];
//     // Protocol: 140 <chest_id> <type> <x> <y>
//     snprintf(notify, sizeof(notify), "%d %d %d %d %d\r\n", 
//              RESP_CHEST_DROP_OK, c_id, (int)chest->type, chest->position_x, chest->position_y);
//     // Duyệt session (login, match_id)
  
//     printf("[SERVER INFO] Chest %d dropped in match %d\n", c_id, match_id);
// }

// // Hàm lấy câu hỏi dựa trên loại rương
// void get_chest_puzzle(ChestType type, char *q_out, char *a_out) {
//     strcpy(q_out, puzzles[(int)type].question);
//     strcpy(a_out, puzzles[(int)type].answer);
// }

// //Tìm rương trong trận đấu theo id
// TreasureChest* find_chest_by_id_in_match(int match_id, int chest_id) {
//     int idx = match_id % MAX_TEAMS;
//     if (active_chests[idx].chest_id == chest_id && active_chests[idx].match_id == match_id) {
//         return &active_chests[idx];
//     }
//     return NULL;
// }

// int server_handle_open_chest(ServerSession *session, int chest_id, const char *answer) {
//     if (!session || !session->isLoggedIn) return 315;

//     TreasureChest *chest = find_chest_by_id_in_match(session->current_match_id, chest_id);
//     if (!chest) return 332; // CHEST_NOT_FOUND
//     if (chest->is_collected) return 339; // OPEN_CHEST_FAIL

//     char q[256], a[64];
//     get_chest_puzzle(chest->type, q, a);
    
//     if (strcasecmp(answer, a) != 0) {
//         return 442; // RESP_WRONG_ANSWER
//     }

//     // Trả lời đúng
//     chest->is_collected = true;
//     int reward = (chest->type == CHEST_BRONZE) ? 100 : (chest->type == CHEST_SILVER ? 500 : 2000);
//     //Cập nhập coin
//     session->coins += reward; 
//     int current_player_coins = session->coins;

//     // Gửi phản hồi thành công cho người mở rương 
//     char res[BUFF_SIZE];
//     snprintf(res, sizeof(res), "127 CHEST_OK %d %d %d 2\n{}\r\n", chest_id, reward, current_player_coins);
    

//     // Thông báo cho tất cả mọi người trong trận đấu (Broadcast)
//     char notify[BUFF_SIZE];
//     snprintf(notify, sizeof(notify), "210 CHEST_COLLECTED %s %d\r\n", session->username, chest_id);

//     return 127;


// }
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


