#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ship_defs.h"
#include "game_logic.h"
#include "networking.h"


extern Ship* find_ship_by_id(int ship_id);
extern WeaponTemplate* get_weapon_template(int weapon_id);
extern void update_ship_state(Ship* ship); 

// Xử lý yêu cầu FIRE 
void process_fire_request(int attacker_id, int target_id, int weapon_id) {
    Ship* target = find_ship_by_id(target_id);
    Ship* attacker = find_ship_by_id(attacker_id);
    int result_code = 0;

    // Kiểm tra tính hợp lệ của mục tiêu và đồng đội
    if (!target || !attacker || target->team_id == attacker->team_id) {
        send_error_response(attacker->socket_fd, ERROR_INVALID_TARGET, "Invalid target ID or friendly fire.");
        return; 
    }

    pthread_mutex_lock(&target->ship_lock);

    result_code = calculate_and_update_damage(attacker, target, weapon_id);
    
    pthread_mutex_unlock(&target->ship_lock);

    // Xử lý báo lỗi nếu có
    if (result_code != 0) {
        char* error_msg = "Fire Failed";
        if (result_code == ERROR_OUT_OF_AMMO) error_msg = "Out of Ammo";
        else if (result_code == ERROR_WEAPON_NOT_EQUIPPED) error_msg = "Weapon Not Equipped (You dont have this ID)";
        else if (result_code == ERROR_TARGET_DESTROYED) error_msg = "Target Already Destroyed";
        
        send_error_response(attacker->socket_fd, result_code, error_msg);
    }
}

// Tính toán sát thương và trừ đạn
int calculate_and_update_damage(Ship* attacker, Ship* target, int weapon_id) {
    
    // TÌM VŨ KHÍ THEO ID
    int index = -1;
    for (int i = 0; i < MAX_WEAPONS; i++) {
        // Tìm thấy ID vũ khí trùng khớp trong các slot
        if (attacker->weapons[i].weapon_id == weapon_id) {
            index = i;
            break; 
        }
    }

    // Nếu không tìm thấy vũ khí ID này trên tàu
    if (index == -1) {
        return ERROR_WEAPON_NOT_EQUIPPED; 
    }
    
    EquippedWeapon* eq_weapon = &attacker->weapons[index];

    // KIỂM TRA ĐẠN 
    if (eq_weapon->current_ammo <= 0) {
        return ERROR_OUT_OF_AMMO; 
    }

    // LẤY THÔNG SỐ SÁT THƯƠNG
    WeaponTemplate* template = get_weapon_template(eq_weapon->weapon_id);
    if (!template) return 500; // Lỗi hệ thống 

    int base_damage = template->damage;
    
    // TÍNH TOÁN
    eq_weapon->current_ammo--; // Trừ 1 viên đạn
    
    int damage_to_deal = base_damage;
    int total_damage_dealt = 0;
    int hp_loss = 0;
    
    // Armor Logic
    if (target->armor > 0) {
        if (damage_to_deal <= target->armor) {
            target->armor -= damage_to_deal; 
            total_damage_dealt = damage_to_deal;
        } else {
            int damage_spillover = damage_to_deal - target->armor;
            target->armor = 0; // Giáp vỡ
            
            hp_loss = damage_spillover;
            target->health -= hp_loss; // Trừ vào máu phần dư
            total_damage_dealt = damage_to_deal;
        }
    } else {
        hp_loss = damage_to_deal;
        target->health -= hp_loss;
        total_damage_dealt = damage_to_deal;
    }

    if (target->health < 0) {
        target->health = 0;
    }

    // UPDATE
    update_ship_state(attacker); // Lưu số đạn mới
    update_ship_state(target);   // Lưu HP/Armor mới

   
    send_fire_ok(attacker->socket_fd, target->ship_id, total_damage_dealt, target->health, target->armor);
    broadcast_fire_event(attacker->ship_id, target->ship_id, total_damage_dealt, target->health, target->armor);
    
    return 0; 
}