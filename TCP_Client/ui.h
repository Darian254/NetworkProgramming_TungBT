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
 * @brief Display repair ship screen with HP input field.
 * @param current_hp Current ship HP (for display, -1 if unknown)
 * @param max_hp Maximum ship HP (for display, -1 if unknown)
 * @param coin Current coin to display (upper-right)
 * @return HP amount to repair (positive integer), or -1 if cancelled
 */
int shop_repair_menu_ncurses(int current_hp, int max_hp, int coin);

/**
 * @brief Display home menu for team management with user information.
 * @param username Current logged-in username (for display)
 * @param coin Current coin balance (-1 if unknown)
 * @param team_name Current team name (NULL or empty if no team)
 * @param hp Current HP (-1 if unknown)
 * @param armor Current armor (-1 if unknown)
 * @return 0=Create Team, 1=Join Request, 2=List Teams, 3=View Invites, 4=Back, -1=Cancel
 */
int home_menu_ncurses(const char *username, long coin, const char *team_name, int hp, int armor);

/**
 * @brief Display create team input form.
 * @param team_name Output buffer for team name
 * @param team_name_size Size of team_name buffer
 * @return 1 if team name entered, 0 if cancelled
 */
int home_create_team_ncurses(char *team_name, size_t team_name_size);

/**
 * @brief Display join team request input form.
 * @param team_name Output buffer for team name
 * @param team_name_size Size of team_name buffer
 * @return 1 if team name entered, 0 if cancelled
 */
int home_join_team_ncurses(char *team_name, size_t team_name_size);

/**
 * @brief Display invites list and allow accept/reject actions.
 * @param invites_data Newline-separated list of team names
 * @param team_name_out Output buffer for selected team name
 * @param team_name_out_size Size of team_name_out buffer
 * @param action_out Output: 0=reject, 1=accept
 * @return -1=back, 0=no invites, 1=action taken
 */
int home_view_invites_ncurses(const char *invites_data, char *team_name_out, size_t team_name_out_size, int *action_out);

/**
 * @brief Display team menu with detailed team information and management options.
 * @param team_id Team ID
 * @param team_name Team name
 * @param captain Captain username
 * @param member_count Number of members
 * @param members_list Newline-separated list of member usernames
 * @return 0=Leave, 1=Invite, 2=Accept Join, 3=Challenge, 4=Back, -1=Cancel
 */
int team_menu_ncurses(int team_id, const char *team_name, const char *captain, int member_count, const char *members_list);

/**
 * @brief Display invite member input form.
 * @param username Output buffer for username
 * @param username_size Size of username buffer
 * @return 1 if username entered, 0 if cancelled
 */
int team_invite_member_ncurses(char *username, size_t username_size);

/**
 * @brief Display join requests list and allow approve/reject.
 * @param requests_data Newline-separated list of usernames
 * @param username_out Output buffer for selected username
 * @param username_out_size Size of username_out buffer
 * @param action_out Output: 0=reject, 1=approve
 * @return -1=back, 0=no requests, 1=action taken
 */
int team_accept_join_request_ncurses(const char *requests_data, char *username_out, size_t username_out_size, int *action_out);

/**
 * @brief Display challenge team input form.
 * @param target_team Output buffer for team ID
 * @param target_team_size Size of target_team buffer
 * @return 1 if team ID entered, 0 if cancelled
 */
int team_challenge_ncurses(char *target_team, size_t target_team_size);
#endif

#endif
