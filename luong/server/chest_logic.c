#include "chest_logic.h"
#include "connect.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

// Mảng lưu trữ rương tạm thời trong các trận đấu
static TreasureChest active_chests[MAX_TEAMS]; 
static ChestPuzzle puzzles[] = {
    {"1 + 1 = ?", "2"},             // Đồng
    {"Thủ đô của Việt Nam?", "Ha Noi"}, // Bạc
    {"Giao thức tầng giao vận nào tin cậy?", "TCP"} // Vàng
};

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

//Sinh rương và tb all
void broadcast_chest_drop(int match_id) {
    int c_id = server_spawn_chest(match_id);
    TreasureChest *chest = &active_chests[match_id % MAX_TEAMS];

    char notify[BUFF_SIZE];
    // Protocol: 140 <chest_id> <type> <x> <y>
    snprintf(notify, sizeof(notify), "%d %d %d %d %d\r\n", 
             RESP_CHEST_DROP_OK, c_id, (int)chest->type, chest->position_x, chest->position_y);
    // Duyệt session (login, match_id)
  
    printf("[SERVER INFO] Chest %d dropped in match %d\n", c_id, match_id);
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

int server_handle_open_chest(ServerSession *session, int chest_id, const char *answer) {
    if (!session || !session->isLoggedIn) return 315;

    TreasureChest *chest = find_chest_by_id_in_match(session->current_match_id, chest_id);
    if (!chest) return 332; // CHEST_NOT_FOUND
    if (chest->is_collected) return 339; // OPEN_CHEST_FAIL

    char q[256], a[64];
    get_chest_puzzle(chest->type, q, a);
    
    if (strcasecmp(answer, a) != 0) {
        return 442; // RESP_WRONG_ANSWER
    }

    // Trả lời đúng
    chest->is_collected = true;
    int reward = (chest->type == CHEST_BRONZE) ? 100 : (chest->type == CHEST_SILVER ? 500 : 2000);
    //Cập nhập coin
    session->coins += reward; 
    int current_player_coins = session->coins;

    // Gửi phản hồi thành công cho người mở rương 
    char res[BUFF_SIZE];
    snprintf(res, sizeof(res), "127 CHEST_OK %d %d %d 2\n{}\r\n", chest_id, reward, current_player_coins);
    

    // Thông báo cho tất cả mọi người trong trận đấu (Broadcast)
    char notify[BUFF_SIZE];
    snprintf(notify, sizeof(notify), "210 CHEST_COLLECTED %s %d\r\n", session->username, chest_id);

    return 127;

    /*Hỏi thầy
    Rương rơi-> click? || hiển thị luôn
    nếu click -> lock không -> công bằng không
    nếu không click -> trải nghiệm UI ? 
    */
}