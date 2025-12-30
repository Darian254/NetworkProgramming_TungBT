#include "db_schema.h"
#include <stddef.h>
#include <time.h>

static Challenge challenges[MAX_CHALLENGES];
static int challenge_count = 0;

Challenge* find_challenge_by_id(int id) {
    for (int i = 0; i < challenge_count; i++) {
        if (challenges[i].challenge_id == id) return &challenges[i];
    }
    return NULL;
}

int create_challenge_record(int sender_team_id, int target_team_id) {
    if (challenge_count >= MAX_CHALLENGES) return -1;
    
    int new_id = challenge_count + 1;
    challenges[challenge_count].challenge_id = new_id;
    challenges[challenge_count].sender_team_id = sender_team_id;
    challenges[challenge_count].target_team_id = target_team_id;
    challenges[challenge_count].status = CHALLENGE_PENDING;
    challenges[challenge_count].created_at = time(NULL);
    
    challenge_count++;
    return new_id;
}