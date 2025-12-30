#ifndef CHALLENGE_H
#define CHALLENGE_H

#include <stddef.h>
#include "db_schema.h"
#include "session.h"

/**
 * @brief Các mã phản hồi bổ sung cho giao thức Thách đấu (Challenge)
 * Các mã này được đánh số để tránh trùng lặp với các mã trong config.h
 */
typedef enum {
    /* Success codes - Challenge */
    RESP_CHALLENGE_SENT = 130,      /**< Challenge sent successfully */
    RESP_CHALLENGE_ACCEPTED = 131,  /**< Challenge accepted */
    RESP_CHALLENGE_DECLINED = 132,  /**< Challenge declined */
    RESP_CHALLENGE_CANCELED = 133,  /**< Challenge canceled */

    /* Error codes - Challenge */
    RESP_CHALLENGE_NOT_FOUND = 332, /**< Challenge ID does not exist */
    RESP_ALREADY_RESPONDED = 333,   /**< Challenge already accepted or declined */
    RESP_NOT_SENDER = 334,          /**< User is not the sender of this challenge */
    RESP_NOT_LOGGED = 315,
    
} ChallengeResponseCode;

/**
 * @brief Cấu trúc ánh xạ mã phản hồi sang thông điệp (đồng bộ với ResponseMessage trong config.h)
 */
typedef struct {
    int code; // Dùng int để chứa được cả ResponseCode và ChallengeResponseCode
    const char *message;
} ChallengeResponseMessage;

/**
 * @brief Danh sách các thông điệp phản hồi cho chức năng Thách đấu
 */
static const ChallengeResponseMessage CHALLENGE_MESSAGES[] = {
    {RESP_CHALLENGE_SENT,     "130 CHALLENGE_SENT successful."},
    {RESP_CHALLENGE_ACCEPTED, "131 CHALLENGE_ACCEPTED successful."},
    {RESP_CHALLENGE_DECLINED, "132 CHALLENGE_DECLINED successful."},
    {RESP_CHALLENGE_CANCELED, "133 CHALLENGE_CANCELED successful."},
    
    {RESP_CHALLENGE_NOT_FOUND, "332 CHALLENGE_NOT_FOUND error."},
    {RESP_ALREADY_RESPONDED,   "333 ALREADY_RESPONDED (Already accepted/declined/canceled)."},
    {RESP_NOT_SENDER,          "334 NOT_SENDER: Only the challenger can cancel."}
};

#define CHALLENGE_MESSAGES_COUNT (sizeof(CHALLENGE_MESSAGES) / sizeof(CHALLENGE_MESSAGES[0]))

/* ============================================================================
 * LOGIC HANDLERS (Nguyên mẫu hàm xử lý)
 * ============================================================================ */

/**
 * @brief Xử lý yêu cầu gửi lời thách đấu
 * @param session Phiên làm việc của người gửi (phải là Leader)
 * @param target_team_id ID của đội bị thách đấu
 * @param new_challenge_id Con trỏ lưu ID của challenge mới tạo
 * @return Mã phản hồi (130 nếu thành công, hoặc các mã lỗi 315, 316...)
 */
int server_handle_send_challenge(ServerSession *session, int target_team_id, int *new_challenge_id);

/**
 * @brief Xử lý yêu cầu chấp nhận lời thách đấu
 * @param session Phiên làm việc của người nhận (phải là Leader đội bị thách đấu)
 * @param challenge_id ID của bản ghi thách đấu
 * @return Mã phản hồi (131 nếu thành công)
 */
int server_handle_accept_challenge(ServerSession *session, int challenge_id);

/**
 * @brief Xử lý yêu cầu từ chối lời thách đấu
 */
int server_handle_decline_challenge(ServerSession *session, int challenge_id);

/**
 * @brief Xử lý yêu cầu hủy lời thách đấu
 */
int server_handle_cancel_challenge(ServerSession *session, int challenge_id);

#endif /* CHALLENGE_H */