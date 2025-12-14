#include <stdio.h>
#include <string.h>
#include "user_dao.h"
#include "team_dao.h"
#include "utils.h"

int main() {
    printf("=== KHOI TAO HE THONG ===\n");
    remove("accounts.txt");
    UserDAO_init_session_cache();
    TeamDAO_init_cache();


    // 1. TAO 7 USER
    printf("\n=== TAO 7 USER ===\n");

    int user_ids[7];
    user_ids[0] = UserDAO_create("user1", "111");
    user_ids[1] = UserDAO_create("user2", "222");
    user_ids[2] = UserDAO_create("user3", "333");
    user_ids[3] = UserDAO_create("user4", "444");
    user_ids[4] = UserDAO_create("user5", "555");
    user_ids[5] = UserDAO_create("user6", "666");
    user_ids[6] = UserDAO_create("user7", "777");

    for (int i = 0; i < 7; i++) {
        printf("User%d ID = %d\n", i + 1, user_ids[i]);
    }

  
    // 2. FIND USER BY USERNAME
    printf("\n=== TEST FIND USER ===\n");
    Account acc;
    if (UserDAO_find_by_username("user3", &acc)) {
        printf("Tim thay user3: ID = %d, Coin = %d\n", acc.user_id, acc.coin);
    }

    
    // 3. UPDATE ACCOUNT
    printf("\n=== TEST UPDATE COIN ===\n");
    acc.coin = 999;
    UserDAO_update_account_stats(&acc);
    printf("Da cap nhat coin user3 = 999\n");


    // 4. TAO SESSION
    printf("\n=== TAO SESSION ===\n");

    char sessions[7][20];
    for (int i = 0; i < 7; i++) {
        UserDAO_create_session(user_ids[i], sessions[i]);
        printf("Session user%d: %s\n", i + 1, sessions[i]);
    }


    // 5. GET USER ID FROM SESSION
    printf("\n=== GET USER FROM SESSION ===\n");
    int uid = UserDAO_get_user_id_by_session(sessions[2]);
    printf("Session %s thuoc user_id = %d\n", sessions[2], uid);

    // 6. TAO 2 TEAM
    printf("\n=== TAO 2 TEAM ===\n");

    int team1_id = TeamDAO_create("Team Alpha", user_ids[0]); // user1 captain
    int team2_id = TeamDAO_create("Team Beta", user_ids[1]);  // user2 captain

    printf("Team Alpha ID = %d\n", team1_id);
    printf("Team Beta  ID = %d\n", team2_id);

    // 7. INVITE VAO TEAM
    printf("\n=== GUI INVITE ===\n");

    InviteDAO_create_request(user_ids[2], team1_id, REQ_INVITE);
    InviteDAO_create_request(user_ids[3], team1_id, REQ_INVITE); // nguoi thu 3 hop le
    InviteDAO_create_request(user_ids[4], team1_id, REQ_INVITE); // nguoi thu 4 se BI TU CHOI

    InviteDAO_create_request(user_ids[5], team2_id, REQ_INVITE);
    InviteDAO_create_request(user_ids[6], team2_id, REQ_INVITE);

    // 8. CHAP NHAN INVITE (TEAM 1 TOI DA 3 NGUOI)
    printf("\n=== XU LY TEAM 1 ===\n");

    TeamRequest *req;

    // User 3 vao team 1 (ok)
    req = InviteDAO_find_pending(user_ids[2], team1_id);
    if (req) {
        TeamDAO_add_member(team1_id, user_ids[2]);
        InviteDAO_delete_request(req->request_id);
        printf("User3 vao Team Alpha\n");
    }

    // User 4 vao team 1 (ok)
    req = InviteDAO_find_pending(user_ids[3], team1_id);
    if (req) {
        TeamDAO_add_member(team1_id, user_ids[3]);
        InviteDAO_delete_request(req->request_id);
        printf("User4 vao Team Alpha\n");
    }

    // User 5 vao team 1 (BI TU CHOI - TEAM FULL)
    req = InviteDAO_find_pending(user_ids[4], team1_id);
    if (req) {
        int kq = TeamDAO_add_member(team1_id, user_ids[4]);
        if (kq == -2) {
            printf("User5 BI TU CHOI do TEAM Alpha da FULL!\n");
        }
        InviteDAO_delete_request(req->request_id);
    }

    // 9. CHAP NHAN INVITE TEAM 2
    printf("\n=== XU LY TEAM 2 ===\n");

    req = InviteDAO_find_pending(user_ids[5], team2_id);
    if (req) {
        TeamDAO_add_member(team2_id, user_ids[5]);
        InviteDAO_delete_request(req->request_id);
        printf("User6 vao Team Beta\n");
    }

    req = InviteDAO_find_pending(user_ids[6], team2_id);
    if (req) {
        TeamDAO_add_member(team2_id, user_ids[6]);
        InviteDAO_delete_request(req->request_id);
        printf("User7 vao Team Beta\n");
    }

    // 10. IN TEAM 1
    printf("\n=== TEAM ALPHA ===\n");
    Team *team1 = TeamDAO_find_by_id(team1_id);
    printf("So thanh vien: %d\n", team1->member_count);
    for (int i = 0; i < team1->member_count; i++) {
        printf("Thanh vien %d: UserID = %d\n", i + 1, team1->member_ids[i]);
    }

  
    // 11. IN TEAM 2
    printf("\n=== TEAM BETA ===\n");
    Team *team2 = TeamDAO_find_by_id(team2_id);
    printf("So thanh vien: %d\n", team2->member_count);
    for (int i = 0; i < team2->member_count; i++) {
        printf("Thanh vien %d: UserID = %d\n", i + 1, team2->member_ids[i]);
    }


    // 12. LOGOUT TAT CA
    printf("\n=== LOGOUT ALL ===\n");
    for (int i = 0; i < 7; i++) {
        UserDAO_delete_session(sessions[i]);
    }
    printf("Da logout toan bo user!\n");

    return 0;
}
