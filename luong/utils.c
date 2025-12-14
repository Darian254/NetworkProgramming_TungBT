#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

// Bien static de giu ID tiep theo
static int next_user_id = 1;
static int next_team_id = 1;
static int next_request_id = 1;

void load_initial_ids() {
    printf("[INFO] Initial IDs loaded: User ID start at %d, Team ID start at %d\n", next_user_id, next_team_id);
}

//Tra ve ID nguoi dung tiep theo
int generate_user_id() {
    return next_user_id++;
}   

//Tra ve ID doi tiep theo
int generate_team_id() {
    return next_team_id++;
}

//Tra ve ID yeu cau/loi moi tiep theo
int generate_request_id() {
    return next_request_id++;
}

// Tao session ID ngau nhien
void generate_session_id(char *sid) {
    static int seed_initialized = 0;
    if(!seed_initialized) {//chưa khởi tạo
        srand((unsigned int)time(NULL));//khởi tạo bộ sinh số ngẫu nhiên cho rand
        seed_initialized = 1;
    }
    //ghi chuỗi vào sid, có giới hạn
    //rand() & 0xFFFF: Phép and bit 
    snprintf(sid, 17, "%04x%04x%04x%04x", rand() & 0xFFFF, rand() & 0xFFFF, rand() & 0xFFFF, rand() & 0xFFFF);

}


//Time helper
long get_current_timestamp() {
    return (long)time(NULL);
}

void get_current_time_string(char *time_str) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(time_str, 20, "%Y-%m-%d %H:%M:%S", tm_info);
}