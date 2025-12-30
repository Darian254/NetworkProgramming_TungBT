#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "ship_defs.h"


Ship active_ships[100];
int num_active_ships = 0;

//Template vũ khí
WeaponTemplate weapon_templates[] = {
    {1, "Pháo",    10, 1000, 50},
    {2, "Laze",   100, 1000, 10},
    {3, "Tên lửa", 800, 2000, 1}
};

#define NUM_WEAPON_TEMPLATES (sizeof(weapon_templates) / sizeof(WeaponTemplate))



//Tìm tàu theo id
Ship* find_ship_by_id(int ship_id) {
    for (int i = 0; i < num_active_ships; i++) {
        if (active_ships[i].ship_id == ship_id) {
            return &active_ships[i];
        }
    }
    return NULL;
}


//Lấy template vũ khí
WeaponTemplate* get_weapon_template(int weapon_id) {
    for (int i = 0; i < NUM_WEAPON_TEMPLATES; i++) {
        if (weapon_templates[i].weapon_id == weapon_id) {
            return &weapon_templates[i];
        }
    }
    return NULL;
}


void update_ship_state(Ship* ship) {
    if (ship->health == 0) {
        printf("[DEBUG] Tàu %d đã bị phá hủy!\n", ship->ship_id);
    }
}


// Load dữ liệu ban đầu
void load_all_ships() {

    // Tàu 1
    Ship ship1;
    memset(&ship1, 0, sizeof(Ship));

    ship1.ship_id   = 101;
    ship1.player_id = 1;
    ship1.team_id   = 1;
    ship1.socket_fd = -1;
    ship1.health    = 1000;
    ship1.armor     = 500;
    ship1.coins     = 5000;

    ship1.weapons[0] = (EquippedWeapon){1, 1, 50};  // Pháo
    ship1.weapons[1] = (EquippedWeapon){3, 2, 1};
    ship1.weapons[2] = (EquippedWeapon){0, 3, 0};
    

    pthread_mutex_init(&ship1.ship_lock, NULL);
    active_ships[0] = ship1;

    // Tàu 2
    Ship ship2;
    memset(&ship2, 0, sizeof(Ship));

    ship2.ship_id   = 102;
    ship2.player_id = 2;
    ship2.team_id   = 2;
    ship2.socket_fd = -1;
    ship2.health    = 1000;
    ship2.armor     = 1500;
    ship2.coins     = 5000;

    ship2.weapons[0] = (EquippedWeapon){2, 1, 10}; // Laze
    ship2.weapons[1] = (EquippedWeapon){3, 2, 1};  // Tên lửa
    ship2.weapons[2] = (EquippedWeapon){0, 3, 0};


    pthread_mutex_init(&ship2.ship_lock, NULL);
    active_ships[1] = ship2;

    num_active_ships = 2;

    printf("Loaded %d ships into memory.\n", num_active_ships);
}
