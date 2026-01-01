#ifndef UI_H
#define UI_H

#include <stddef.h>

/**
 * @brief Display the main user menu on the console.
 */
void displayMenu();

#ifdef USE_NCURSES
/**
 * @brief Display register form using ncurses.
 * @param username Output buffer for username
 * @param username_size Size of username buffer
 * @param password Output buffer for password
 * @param password_size Size of password buffer
 * @return 1 if user submitted, 0 if cancelled
 */
int register_ui_ncurses(char *username, size_t username_size, char *password, size_t password_size);

/**
 * @brief Display login form using ncurses.
 * @param username Output buffer for username
 * @param username_size Size of username buffer
 * @param password Output buffer for password
 * @param password_size Size of password buffer
 * @return 1 if user submitted, 0 if cancelled
 */
int login_ui_ncurses(char *username, size_t username_size, char *password, size_t password_size);

/**
 * @brief Display logout confirmation dialog using ncurses.
 * @return 1 if confirmed, 0 if cancelled
 */
int logout_ui_ncurses(void);

/**
 * @brief Display whoami result using ncurses.
 * @param response Server response string
 */
void whoami_ui_ncurses(const char *response);

/**
 * @brief Display a message dialog using ncurses.
 * @param title Dialog title
 * @param message Message to display
 */
void show_message_ncurses(const char *title, const char *message);

/**
 * @brief Display main menu using ncurses and get user selection.
 * @return Selected option number (0-23) or FUNC_EXIT if cancelled
 */
int display_menu_ncurses(void);

/**
 * @brief Display login/register selection menu using ncurses.
 * @return 0 for Register, 1 for Login, -1 if cancelled
 */
int login_register_menu_ncurses(void);

/**
 * @brief Display shop main menu using ncurses.
 * @return 0 for Buy Armor, 1 for Buy Weapon, -1 if cancelled
 */
int shop_menu_ncurses(void);

/**
 * @brief Display armor selection screen (types and prices).
 * @param coin Current coin to display (upper-right)
 * @return 0 for BASIC, 1 for ENHANCED, -1 if cancelled
 */
int shop_armor_menu_ncurses(int coin);

/**
 * @brief Display weapon selection screen (types).
 * @param coin Current coin to display (upper-right)
 * @return 0 for CANNON AMMO, 1 for LASER, 2 for MISSILE, -1 if cancelled
 */
int shop_weapon_menu_ncurses(int coin);

/**
 * @brief Battle screen: show 6 ships (3 per team), HP and Shop option.
 * Renders two sides and allows selecting an enemy ship to attack.
 * After selecting target, prompts for weapon (cannon/laser/missile).
 *
 * @param my_username Current player's username (for highlighting)
 * @param team_left Array of friendly usernames (size left_count)
 * @param left_count Number of friendly ships (expected 3)
 * @param team_right Array of enemy usernames (size right_count)
 * @param right_count Number of enemy ships (expected 3)
 * @param my_hp Current player's ship HP for display
 * @param out_target_username Output: selected enemy username (buffer provided by caller)
 * @param out_target_username_size Size of output buffer
 * @param out_weapon_id Output: 0=cannon, 1=laser, 2=missile
 * @return 1 if FIRE selected, 0 if Shop requested, -1 if cancelled
 */
 int battle_screen_ncurses(const char *my_username,
    const char **team_left, int *team_left_hp, int left_count,
    const char **team_right, int *team_right_hp, int right_count,
    int my_hp,
    char *out_target_username, size_t out_target_username_size,
    int *out_weapon_id);
#endif

#endif