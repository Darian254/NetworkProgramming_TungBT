#ifndef CHEST_LOGIC_H
#define CHEST_LOGIC_H

#include <stddef.h>
#include "db_schema.h"


typedef enum {
    /* Success codes - Chest */
    RESP_CHEST_DROP_OK = 140,     
    
    /* Error codes - Chest (Dự phòng cho logic mở rương) */
    RESP_CHEST_NOT_FOUND = 440,    /**< Chest ID invalid */
    RESP_CHEST_ALREADY_OPENED = 441, 
    RESP_WRONG_ANSWER = 442,       
    RESP_CHEST_OPEN_OK = 127,
    RESP_MATCH_FINISHED = 325,
    RESP_CHEST_OPEN_FAIL = 339,
    RESP_CHEST_BROADCAST = 210

} ChestResponseCode;

/**
 * @brief Cấu trúc ánh xạ mã phản hồi sang thông điệp
 */
typedef struct {
    int code;
    const char *message;
} ChestResponseMessage;

/**
 * @brief Danh sách các thông điệp phản hồi cho chức năng Rương
 */
static const ChallengeResponseMessage CHEST_MESSAGES[] = {
    {RESP_CHEST_DROP_OK,        "140 CHEST_DROP_OK: A treasure chest has appeared!"},
    {RESP_CHEST_NOT_FOUND,      "440 ERROR: Treasure chest not found."},
    {RESP_CHEST_ALREADY_OPENED, "441 ERROR: This chest has already been claimed."},
    {RESP_WRONG_ANSWER,         "442 ERROR: Incorrect answer. Try again!"},
    {RESP_CHEST_OPEN_OK,        "127 CHEST_OK: Chest opened successfully."},
    {RESP_MATCH_FINISHED,       "325 ERROR: Match has already finished."},
    {RESP_CHEST_OPEN_FAIL,      "339 ERROR: Chest has already been opened."},
    {RESP_CHEST_BROADCAST,      "210 CHEST_COLLECTED: A chest has been collected in the match."}
};

#define CHEST_MESSAGES_COUNT (sizeof(CHEST_MESSAGES) / sizeof(CHEST_MESSAGES[0]))

//LOGIC HANDLERS (Nguyên mẫu hàm xử lý)
/**
 * @brief Khởi tạo rương cho một trận đấu
 * @param match_id ID của trận đấu diễn ra
 * @return ID của rương được tạo
 */
int server_spawn_chest(int match_id);

/**
 * @brief Gửi thông báo rương rơi tới toàn bộ người chơi trong trận đấu
 * @param match_id ID trận đấu cần thông báo
 */
void broadcast_chest_drop(int match_id);

/**
 * @brief Xử lý khi người chơi thực hiện mở rương (trả lời câu hỏi)
 * @param session Phiên làm việc của người chơi
 * @param chest_id ID rương muốn mở
 * @param answer Câu trả lời trắc nghiệm hoặc ngắn
 */
int server_handle_open_chest(ServerSession *session, int chest_id, const char *answer);

typedef struct {
    char question[256];
    char answer[64];
} ChestPuzzle;

int server_handle_open_chest(ServerSession *session, int chest_id, const char *answer);
void get_chest_puzzle(ChestType type, char *q_out, char *a_out);

#endif /* CHEST_LOGIC_H */