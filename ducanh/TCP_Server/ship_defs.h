#ifndef SHIP_DEFS_H
#define SHIP_DEFS_H

#include <pthread.h>

// Constants
#define MAX_WEAPONS 4
#define MAX_ARMOR_SLOTS 2

// Protocol Codes
#define PROTOCOL_FIRE           130 
#define PROTOCOL_FIRE_OK        131 
#define PROTOCOL_FIRE_EVENT     200 

// Error Codes

#define ERROR_WEAPON_NOT_EQUIPPED 337//Vũ khí chưa được trang bị
#define ERROR_OUT_OF_AMMO         338//Hết đạn
#define ERROR_INVALID_TARGET      343//Mục tiêu không hợp lệ
#define ERROR_TARGET_DESTROYED    344//bị hủy
#define ERROR_NOT_YOUR_TURN       335//Chưa đến lượt bắn


// Cấu trúc Vũ khí (Template - Dữ liệu tĩnh)
typedef struct {
    int weapon_id;
    char name[30];
    int damage;
    int cost;
    int ammo_capacity;
} WeaponTemplate;

// Cấu trúc Trang bị (Trạng thái động)
typedef struct {
    int weapon_id;      // ID
    int slot_number;    // 1-3
    int current_ammo;   // Số đạn
} EquippedWeapon;

// Cấu trúc Tàu 
typedef struct {
    int ship_id;
    int player_id;
    int team_id;
    int socket_fd;          
    int health;
    int armor;
    int coins;
    pthread_mutex_t ship_lock; // MUTEX: Khóa khi đọc/ghi trạng thái
    EquippedWeapon weapons[MAX_WEAPONS]; 
    int equipped_armor_ids[MAX_ARMOR_SLOTS]; 
    int is_destroyed;
} Ship;



void load_all_ships();


#endif // SHIP_DEFS_H