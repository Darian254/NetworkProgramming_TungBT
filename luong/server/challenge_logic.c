#include "challenge.h"
#include <string.h>
#include <stdio.h>

extern Team* find_team_by_id(int team_id);
extern Match* create_match(int team1_id, int team2_id);

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