#include <stdio.h>
#include <stdlib.h>
#include "ship_defs.h"
#include "game_logic.h"

extern void load_all_ships();
extern Ship* find_ship_by_id(int ship_id);


int main() {
    load_all_ships(); 

    Ship *ship101 = find_ship_by_id(101);
    Ship *ship102 = find_ship_by_id(102);

    if (!ship101 || !ship102) {
        printf("Error: Ship data not loaded.\n");
        return 1;
    }

    printf("--- MOCK LOGIC TEST START ---\n");
    
    
    ship101->socket_fd = 1; 

    
    printf("\nTEST 1: Dam=10 vs Armor=1500\n");
   
    process_fire_request(101, 102, 1);
    
    
    printf("Result: Ship 102 HP: %d | Armor: %d | Ammo 101: %d\n", ship102->health, ship102->armor, ship101->weapons[0].current_ammo);

    // Đặt lại Armor để chuẩn bị cho test tràn
    ship102->armor = 50; 
    

    printf("\nTEST 2: Dam=800 vs Armor=50 (Slot 2)\n");
   
    process_fire_request(101, 102, 2);
    
    // Kiểm tra kết quả
    printf("Result: Ship 102 HP: %d | Armor: %d | Ammo 101: %d\n", ship102->health, ship102->armor, ship101->weapons[1].current_ammo);
   

    printf("\n--- MOCK LOGIC TEST END ---\n");
    return 0;
}