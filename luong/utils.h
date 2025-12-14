// utils.h
#ifndef UTILS_H
#define UTILS_H

#include "data_struct.h" 
#include <unistd.h> // Cần cho getpid()

// ID Generator
void load_initial_ids();
int generate_user_id();
int generate_team_id();
int generate_request_id(); 
void generate_session_id(char *sid); //

// Time Helper
long get_current_timestamp();
void get_current_time_string(char *time_str);

#endif // UTILS_H