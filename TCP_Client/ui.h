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
#endif

#endif
