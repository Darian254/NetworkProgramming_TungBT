#include "ui.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "../TCP_Server/config.h"

void displayMenu() {

    printf("\n==================================\n");
    printf("           MAIN MENU              \n");
    printf("==================================\n");
    printf(" 0. Register\n");
    printf(" 1. Log in\n");
    printf(" 2. Logout\n");
    printf(" 3. Who am I?\n");
    printf(" 4. Exit\n");
    printf("----------------------------------\n");
    printf(" [PERSONAL]\n");
    printf(" 5. Check my coin\n");
    printf(" 6. Check my armor\n");
    printf(" 7. Buy armor\n");
    
    printf("----------------------------------\n");
    printf(" [TEAM OPERATIONS]\n");
    printf(" 8. Create Team\n");
    printf(" 9. Delete Team\n");
    printf("10. List All Teams\n");
    printf("11. Join Team\n");
    printf("12. Leave Team\n");
    printf("13. Team Members\n");
    printf("14. Kick Member (Captain only)\n");
    printf("15. Approve Join (Captain only)\n");
    printf("16. Reject Join (Captain only)\n");
    printf("17. Invite Member (Captain only)\n");
    printf("18. Accept Invite\n");
    printf("19. Reject Invite\n");
    printf("20. Start Match\n");
    printf("21. Get Match Result\n");
    printf("22. End Match\n");
    printf("23. Repair HP\n");
    printf("----------------------------------\n");
    printf(" [QUICK LOGIN]\n");
    printf("24. Login as test1\n");
    printf("25. Login as test2\n");
    printf("26. Login as test3\n");
    printf("27. Login as test4\n");
    printf("28. Login as test5\n");
    printf("29. Login as test6\n");
    printf("----------------------------------\n");
    printf(" [QUICK TEAM SETUP]\n");
    printf("30. test1: Create team ABC & invite test2,3\n");
    printf("31. test4: Create team DEF & invite test5,6\n");
    printf("32. test2/3: Accept invite to team ABC\n");
    printf("33. test5/6: Accept invite to team DEF\n");
    printf("34. Login / Register Menu\n");
    printf("35. View Match Info\n");
    printf("36. Shop Menu\n");
    printf("37. Buy Weapon\n");
    printf("38. Get Weapon Info\n");
    printf("39. Home Menu (Team Management)\n");
    printf("40. Team Menu (Detailed Team Management)\n");
    printf("==================================\n");
    printf("Select an option: ");
}

#ifdef USE_NCURSES
#include <ncurses.h>
// Use item price/value constants from server schema for consistency
#include "../TCP_Server/db_schema.h"

static void get_input_field(WINDOW *win, int y, int x, char *buffer, size_t size, int echo) {
    int pos = 0;
    int ch;
    
    wmove(win, y, x);
    wclrtoeol(win);
    buffer[0] = '\0';
    
    while (1) {
        ch = wgetch(win);
        
        if (ch == '\n' || ch == KEY_ENTER) {
            break;
        } else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
            if (pos > 0) {
                pos--;
                buffer[pos] = '\0';
                wmove(win, y, x + pos);
                waddch(win, ' ');
                wmove(win, y, x + pos);
            }
        } else if (ch == 27) {  /* ESC key */
            buffer[0] = '\0';
            break;
        } else if (isprint(ch) && pos < (int)(size - 1)) {
            buffer[pos++] = ch;
            buffer[pos] = '\0';
            if (echo) {
                waddch(win, ch);
            } else {
                waddch(win, '*');
            }
        }
    }
}

int register_ui_ncurses(char *username, size_t username_size, char *password, size_t password_size) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);
    erase();
    refresh();
    
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int win_h = 10;
    int win_w = 50;
    int start_y = (max_y - win_h) / 2;
    int start_x = (max_x - win_w) / 2;
    
    WINDOW *win = newwin(win_h, win_w, start_y, start_x);
    keypad(win, TRUE);
    box(win, 0, 0);
    
    mvwprintw(win, 1, (win_w - 15) / 2, "REGISTER");
    mvwprintw(win, 3, 2, "Username: ");
    mvwprintw(win, 5, 2, "Password: ");
    mvwprintw(win, 7, (win_w - 30) / 2, "Press Enter to submit");
    mvwprintw(win, 8, (win_w - 25) / 2, "Press ESC to cancel");
    
    wrefresh(win);
    
    noecho();
    curs_set(1);
    wmove(win, 3, 12);
    wrefresh(win);
    get_input_field(win, 3, 12, username, username_size, 1);
    
    if (strlen(username) == 0) {
        delwin(win);
        clear();
        refresh();
        endwin();
        return 0;
    }
    
    noecho();
    wmove(win, 5, 12);
    wrefresh(win);
    get_input_field(win, 5, 12, password, password_size, 0);
    
    int result = (strlen(username) > 0 && strlen(password) > 0) ? 1 : 0;
    
    delwin(win);
    clear();
    refresh();
    endwin();
    return result;
}

int login_ui_ncurses(char *username, size_t username_size, char *password, size_t password_size) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);
    erase();
    refresh();
    
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int win_h = 10;
    int win_w = 50;
    int start_y = (max_y - win_h) / 2;
    int start_x = (max_x - win_w) / 2;
    
    WINDOW *win = newwin(win_h, win_w, start_y, start_x);
    keypad(win, TRUE);
    box(win, 0, 0);
    
    mvwprintw(win, 1, (win_w - 8) / 2, "LOGIN");
    mvwprintw(win, 3, 2, "Username: ");
    mvwprintw(win, 5, 2, "Password: ");
    mvwprintw(win, 7, (win_w - 30) / 2, "Press Enter to submit");
    mvwprintw(win, 8, (win_w - 25) / 2, "Press ESC to cancel");
    
    wrefresh(win);
    
    noecho();
    curs_set(1);
    wmove(win, 3, 12);
    wrefresh(win);
    get_input_field(win, 3, 12, username, username_size, 1);
    
    if (strlen(username) == 0) {
        delwin(win);
        clear();
        refresh();
        endwin();
        return 0;
    }
    
    noecho();
    wmove(win, 5, 12);
    wrefresh(win);
    get_input_field(win, 5, 12, password, password_size, 0);
    
    int result = (strlen(username) > 0 && strlen(password) > 0) ? 1 : 0;
    
    delwin(win);
    clear();
    refresh();
    endwin();
    return result;
}

int logout_ui_ncurses(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    erase();
    refresh();
    
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int win_h = 7;
    int win_w = 40;
    int start_y = (max_y - win_h) / 2;
    int start_x = (max_x - win_w) / 2;
    
    WINDOW *win = newwin(win_h, win_w, start_y, start_x);
    box(win, 0, 0);
    
    mvwprintw(win, 1, (win_w - 10) / 2, "LOGOUT");
    mvwprintw(win, 3, (win_w - 25) / 2, "Are you sure?");
    mvwprintw(win, 4, (win_w - 20) / 2, "Y - Yes, N - No");
    
    wrefresh(win);
    
    int ch;
    int result = 0;
    
    while (1) {
        ch = getch();
        if (ch == 'y' || ch == 'Y') {
            result = 1;
            break;
        } else if (ch == 'n' || ch == 'N' || ch == 27) {  /* ESC key */
            result = 0;
            break;
        }
    }
    
    delwin(win);
    clear();
    refresh();
    endwin();
    return result;
}

void whoami_ui_ncurses(const char *response) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    erase();
    refresh();
    
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int code;
    char username[128] = "";
    sscanf(response, "%d %127s", &code, username);
    
    int win_h = 8;
    int win_w = 50;
    int start_y = (max_y - win_h) / 2;
    int start_x = (max_x - win_w) / 2;
    
    WINDOW *win = newwin(win_h, win_w, start_y, start_x);
    box(win, 0, 0);
    
    mvwprintw(win, 1, (win_w - 10) / 2, "WHO AM I");
    
    if (code == 201 && strlen(username) > 0) {
        mvwprintw(win, 3, 2, "You are logged in as:");
        mvwprintw(win, 4, 4, "%s", username);
    } else {
        char msg[256];
        snprintf(msg, sizeof(msg), "Error: %s", response);
        mvwprintw(win, 3, 2, "%.*s", win_w - 4, msg);
    }
    
    mvwprintw(win, 6, (win_w - 20) / 2, "Press any key...");
    
    wrefresh(win);
    getch();
    
    delwin(win);
    clear();
    refresh();
    endwin();
}

void show_message_ncurses(const char *title, const char *message) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    erase();
    refresh();
    
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int win_h = 8;
    int win_w = 60;
    int start_y = (max_y - win_h) / 2;
    int start_x = (max_x - win_w) / 2;
    
    WINDOW *win = newwin(win_h, win_w, start_y, start_x);
    box(win, 0, 0);
    
    mvwprintw(win, 1, (win_w - strlen(title)) / 2, "%s", title);
    
    int msg_len = strlen(message);
    int msg_y = 3;
    int msg_x = 2;
    int max_msg_w = win_w - 4;
    
    if (msg_len <= max_msg_w) {
        mvwprintw(win, msg_y, msg_x, "%.*s", max_msg_w, message);
    } else {
        char temp[256];
        strncpy(temp, message, max_msg_w);
        temp[max_msg_w] = '\0';
        mvwprintw(win, msg_y, msg_x, "%s", temp);
    }
    
    mvwprintw(win, 6, (win_w - 20) / 2, "Press any key...");
    
    wrefresh(win);
    getch();
    
    delwin(win);
    clear();
    refresh();
    endwin();
}

int display_menu_ncurses(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int win_h = 48;
    int win_w = 75;
    
    if (win_h > max_y - 2) win_h = max_y - 2;
    if (win_w > max_x - 2) win_w = max_x - 2;
    
    int start_y = (max_y - win_h) / 2;
    int start_x = (max_x - win_w) / 2;
    
    WINDOW *win = newwin(win_h, win_w, start_y, start_x);
    keypad(win, TRUE);
    box(win, 0, 0);
    
    mvwprintw(win, 1, (win_w - 12) / 2, "MAIN MENU");
    
    int y = 3;
    mvwprintw(win, y++, 2, " 0. Register");
    mvwprintw(win, y++, 2, " 1. Log in");
    mvwprintw(win, y++, 2, " 2. Logout");
    mvwprintw(win, y++, 2, " 3. Who am I?");
    mvwprintw(win, y++, 2, " 4. Exit");
    y++;
    mvwprintw(win, y++, 2, " [PERSONAL]");
    mvwprintw(win, y++, 2, " 5. Check my coin");
    mvwprintw(win, y++, 2, " 6. Check my armor");
    mvwprintw(win, y++, 2, " 7. Buy armor");
    y++;
    mvwprintw(win, y++, 2, " [TEAM OPERATIONS]");
    mvwprintw(win, y++, 2, " 8. Create Team");
    mvwprintw(win, y++, 2, " 9. Delete Team");
    mvwprintw(win, y++, 2, "10. List All Teams");
    mvwprintw(win, y++, 2, "11. Join Team");
    mvwprintw(win, y++, 2, "12. Leave Team");
    mvwprintw(win, y++, 2, "13. Team Members");
    mvwprintw(win, y++, 2, "14. Kick Member (Captain only)");
    mvwprintw(win, y++, 2, "15. Approve Join (Captain only)");
    mvwprintw(win, y++, 2, "16. Reject Join (Captain only)");
    mvwprintw(win, y++, 2, "17. Invite Member (Captain only)");
    mvwprintw(win, y++, 2, "18. Accept Invite");
    mvwprintw(win, y++, 2, "19. Reject Invite");
    mvwprintw(win, y++, 2, "20. Start Match");
    mvwprintw(win, y++, 2, "21. Get Match Result");
    mvwprintw(win, y++, 2, "22. End Match");
    mvwprintw(win, y++, 2, "23. Repair HP");
    y++;
    mvwprintw(win, y++, 2, " [QUICK LOGIN]");
    mvwprintw(win, y++, 2, "24. Login as test1");
    mvwprintw(win, y++, 2, "25. Login as test2");
    mvwprintw(win, y++, 2, "26. Login as test3");
    mvwprintw(win, y++, 2, "27. Login as test4");
    mvwprintw(win, y++, 2, "28. Login as test5");
    mvwprintw(win, y++, 2, "29. Login as test6");
    y++;
    mvwprintw(win, y++, 2, " [QUICK SETUP]");
    mvwprintw(win, y++, 2, "30. test1: Team ABC+invite");
    mvwprintw(win, y++, 2, "31. test4: Team DEF+invite");
    mvwprintw(win, y++, 2, "32. test2/3: Accept ABC");
    mvwprintw(win, y++, 2, "33. test5/6: Accept DEF");
    y++;
    mvwprintw(win, y++, 2, " [AUTHENTICATION MENU]");
    mvwprintw(win, y++, 2, "34. Login / Register Menu");
    y++;
    mvwprintw(win, y++, 2, " [MATCH INFO]");
    mvwprintw(win, y++, 2, "35. View Match Info");
    y++;
    mvwprintw(win, y++, 2, " [SHOP]");
    mvwprintw(win, y++, 2, "36. Shop Menu");
    
    mvwprintw(win, win_h - 2, 2, "Enter option number (0-36) or ESC to exit: ");
    
    wrefresh(win);
    
    char input[64] = {0};
    int pos = 0;
    int ch;
    int result = -1;
    
    int input_x = 50;
    if (input_x + 10 > win_w - 2) input_x = win_w - 12;
    
    wmove(win, win_h - 2, input_x);
    wrefresh(win);
    
    while (1) {
        ch = wgetch(win);
        
        if (ch == '\n' || ch == KEY_ENTER) {
            if (pos > 0) {
                input[pos] = '\0';
                int choice = atoi(input);
                if (choice >= 0 && choice <= 36) {
                    result = choice;
                    break;
                } else {
                    mvwprintw(win, win_h - 1, 2, "Invalid option! Enter 0-36: ");
                    wclrtoeol(win);
                    wmove(win, win_h - 2, input_x);
                    pos = 0;
                    input[0] = '\0';
                    wrefresh(win);
                }
            }
        } else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
            if (pos > 0) {
                pos--;
                input[pos] = '\0';
                wmove(win, win_h - 2, input_x + pos);
                waddch(win, ' ');
                wmove(win, win_h - 2, input_x + pos);
                mvwprintw(win, win_h - 1, 2, "                                    ");
                wrefresh(win);
            }
        } else if (ch == 27) {  /* ESC key */
            result = FUNC_EXIT;
            break;
        } else if (isdigit(ch) && pos < 3) {
            input[pos++] = ch;
            input[pos] = '\0';
            waddch(win, ch);
            mvwprintw(win, win_h - 1, 2, "                                    ");
            wrefresh(win);
        }
    }
    
    delwin(win);
    clear();
    refresh();
    endwin();
    
    return result;
}

int login_register_menu_ncurses(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    erase();
    refresh();
    
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int win_h = 12;
    int win_w = 60;
    int start_y = (max_y - win_h) / 2;
    int start_x = (max_x - win_w) / 2;
    
    WINDOW *win = newwin(win_h, win_w, start_y, start_x);
    keypad(win, TRUE);
    box(win, 0, 0);
    
    mvwprintw(win, 1, (win_w - 20) / 2, "LOGIN / REGISTER");
    
    int selected = 0;  // 0 = Login, 1 = Register
    int result = -1;
    
    while (1) {
        // Clear button area
        for (int i = 3; i < 8; i++) {
            for (int j = 2; j < win_w - 2; j++) {
                mvwaddch(win, i, j, ' ');
            }
        }
        
        // Draw Login button (left)
        int login_x = 8;
        int login_y = 5;
        int login_w = 18;
        int login_h = 3;
        
        if (selected == 0) {
            wattron(win, A_REVERSE);
        }
        for (int i = 0; i < login_h; i++) {
            for (int j = 0; j < login_w; j++) {
                mvwaddch(win, login_y + i, login_x + j, ' ');
            }
        }
        mvwprintw(win, login_y + 1, login_x + (login_w - 5) / 2, "LOGIN");
        if (selected == 0) {
            wattroff(win, A_REVERSE);
        }
        
        // Draw Register button (right)
        int reg_x = 34;
        int reg_y = 5;
        int reg_w = 18;
        int reg_h = 3;
        
        if (selected == 1) {
            wattron(win, A_REVERSE);
        }
        for (int i = 0; i < reg_h; i++) {
            for (int j = 0; j < reg_w; j++) {
                mvwaddch(win, reg_y + i, reg_x + j, ' ');
            }
        }
        mvwprintw(win, reg_y + 1, reg_x + (reg_w - 8) / 2, "REGISTER");
        if (selected == 1) {
            wattroff(win, A_REVERSE);
        }
        
        // Instructions
        mvwprintw(win, 9, (win_w - 40) / 2, "Arrow keys: Select | Enter: Confirm | ESC: Cancel");
        
        wrefresh(win);
        
        int ch = wgetch(win);
        
        if (ch == KEY_LEFT || ch == KEY_RIGHT) {
            selected = 1 - selected;  // Toggle between 0 and 1
        } else if (ch == '\n' || ch == KEY_ENTER) {
            result = selected;  // 0 = Login, 1 = Register
            break;
        } else if (ch == 27) {  // ESC key
            result = -1;
            break;
        } else if (ch == 'l' || ch == 'L') {
            result = 0;  // Login
            break;
        } else if (ch == 'r' || ch == 'R') {
            result = 1;  // Register
            break;
        }
    }
    
    delwin(win);
    clear();
    refresh();
    endwin();
    
    return result;
}

// Shop main menu: choose Buy Armor or Buy Weapon
int shop_menu_ncurses(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    erase();
    refresh();

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int win_h = 14;
    int win_w = 70;
    int start_y = (max_y - win_h) / 2;
    int start_x = (max_x - win_w) / 2;

    WINDOW *win = newwin(win_h, win_w, start_y, start_x);
    keypad(win, TRUE);
    box(win, 0, 0);

    mvwprintw(win, 1, (win_w - 9) / 2, "SHOP MENU");

    int selected = 0; // 0 = Buy Armor, 1 = Buy Weapon, 2 = Repair Ship
    int result = -1;

    while (1) {
        // Clear area
        for (int i = 3; i < 10; i++) {
            for (int j = 2; j < win_w - 2; j++) {
                mvwaddch(win, i, j, ' ');
            }
        }

        // Buy Armor button (left)
        int armor_x = 6, armor_y = 5, armor_w = 18, armor_h = 3;
        if (selected == 0) wattron(win, A_REVERSE);
        for (int i = 0; i < armor_h; i++) {
            for (int j = 0; j < armor_w; j++) {
                mvwaddch(win, armor_y + i, armor_x + j, ' ');
            }
        }
        mvwprintw(win, armor_y + 1, armor_x + (armor_w - 10) / 2, "BUY ARMOR");
        if (selected == 0) wattroff(win, A_REVERSE);

        // Buy Weapon button (middle)
        int weapon_x = 26, weapon_y = 5, weapon_w = 18, weapon_h = 3;
        if (selected == 1) wattron(win, A_REVERSE);
        for (int i = 0; i < weapon_h; i++) {
            for (int j = 0; j < weapon_w; j++) {
                mvwaddch(win, weapon_y + i, weapon_x + j, ' ');
            }
        }
        mvwprintw(win, weapon_y + 1, weapon_x + (weapon_w - 11) / 2, "BUY WEAPON");
        if (selected == 1) wattroff(win, A_REVERSE);

        // Repair Ship button (right)
        int repair_x = 46, repair_y = 5, repair_w = 18, repair_h = 3;
        if (selected == 2) wattron(win, A_REVERSE);
        for (int i = 0; i < repair_h; i++) {
            for (int j = 0; j < repair_w; j++) {
                mvwaddch(win, repair_y + i, repair_x + j, ' ');
            }
        }
        mvwprintw(win, repair_y + 1, repair_x + (repair_w - 11) / 2, "REPAIR SHIP");
        if (selected == 2) wattroff(win, A_REVERSE);

        // Instructions
        mvwprintw(win, 9, 2, "Arrow Keys: Navigate | Enter: Confirm | ESC: Back");
        mvwprintw(win, 10, 2, "Shortcuts: A = Armor | W = Weapon | R = Repair");
        wrefresh(win);

        int ch = wgetch(win);
        if (ch == KEY_LEFT) {
            if (selected > 0) selected--;
        } else if (ch == KEY_RIGHT) {
            if (selected < 2) selected++;
        } else if (ch == '\n' || ch == KEY_ENTER) {
            result = selected; // 0 = Buy Armor, 1 = Buy Weapon, 2 = Repair
            break;
        } else if (ch == 27) { // ESC
            result = -1;
            break;
        } else if (ch == 'a' || ch == 'A') {
            result = 0;
            break;
        } else if (ch == 'w' || ch == 'W') {
            result = 1;
            break;
        } else if (ch == 'r' || ch == 'R') {
            result = 2;
            break;
        }
    }

    delwin(win);
    clear();
    refresh();
    endwin();
    return result;
}

// Armor selection: show types and prices
int shop_armor_menu_ncurses(int coin) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    erase();
    refresh();

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int win_h = 14;
    int win_w = 60;
    int start_y = (max_y - win_h) / 2;
    int start_x = (max_x - win_w) / 2;

    WINDOW *win = newwin(win_h, win_w, start_y, start_x);
    keypad(win, TRUE);
    box(win, 0, 0);

    mvwprintw(win, 1, (win_w - 17) / 2, "BUY ARMOR (SHOP)");
    // Show coin at upper-right corner
    char coin_str[32];
    snprintf(coin_str, sizeof(coin_str), "Coin: %d", coin);
    int coin_x = win_w - 2 - (int)strlen(coin_str);
    if (coin_x < 2) coin_x = 2;
    mvwprintw(win, 1, coin_x, "%s", coin_str);

    // Info lines
    mvwprintw(win, 3, 2, "Available Armor Types:");
    mvwprintw(win, 5, 4, "1) BASIC   - Price: %d, Armor: %d", ARMOR_BASIC_PRICE, ARMOR_BASIC_VALUE);
    mvwprintw(win, 6, 4, "2) ENHANCED- Price: %d, Armor: %d", ARMOR_ENHANCED_PRICE, ARMOR_ENHANCED_VALUE);
    mvwprintw(win, 8, 2, "Use Up/Down to select, Enter to confirm, ESC to cancel.");

    int selected = 0; // 0 = BASIC, 1 = ENHANCED

    // Draw selection markers
    while (1) {
        // Clear selection markers
        mvwprintw(win, 5, 2, " ");
        mvwprintw(win, 6, 2, " ");
        // Mark selected
        mvwprintw(win, selected == 0 ? 5 : 6, 2, ">");
        wrefresh(win);

        int ch = wgetch(win);
        if (ch == KEY_UP || ch == 'k' || ch == 'K') {
            selected = 0;
        } else if (ch == KEY_DOWN || ch == 'j' || ch == 'J') {
            selected = 1;
        } else if (ch == '\n' || ch == KEY_ENTER) {
            break;
        } else if (ch == 27) { // ESC
            selected = -1;
            break;
        } else if (ch == '1') {
            selected = 0;
            break;
        } else if (ch == '2') {
            selected = 1;
            break;
        }
    }

    int result = selected; // -1 for cancel, 0 BASIC, 1 ENHANCED
    delwin(win);
    clear();
    refresh();
    endwin();
    return result;
}

// Weapon selection: show types and prices/details
int shop_weapon_menu_ncurses(int coin) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    erase();
    refresh();

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int win_h = 16;
    int win_w = 64;
    int start_y = (max_y - win_h) / 2;
    int start_x = (max_x - win_w) / 2;

    WINDOW *win = newwin(win_h, win_w, start_y, start_x);
    keypad(win, TRUE);
    box(win, 0, 0);

    mvwprintw(win, 1, (win_w - 18) / 2, "BUY WEAPON (SHOP)");
    // Show coin at upper-right corner
    char coin_str[32];
    snprintf(coin_str, sizeof(coin_str), "Coin: %d", coin);
    int coin_x = win_w - 2 - (int)strlen(coin_str);
    if (coin_x < 2) coin_x = 2;
    mvwprintw(win, 1, coin_x, "%s", coin_str);
    mvwprintw(win, 3, 2, "Available Weapon Types:");
    mvwprintw(win, 5, 4, "1) CANNON AMMO  - Price: %d per %d ammo", CANNON_AMMO_PRICE, CANNON_AMMO_PER_PURCHASE);
    mvwprintw(win, 6, 4, "2) LASER        - Price: %d", LASER_PRICE);
    mvwprintw(win, 7, 4, "3) MISSILE      - Price: %d", MISSILE_PRICE);
    mvwprintw(win, 9, 2, "Use Up/Down to select, Enter to confirm, ESC to cancel.");

    int selected = 0; // 0 = CANNON, 1 = LASER, 2 = MISSILE

    while (1) {
        // Clear markers
        mvwprintw(win, 5, 2, " ");
        mvwprintw(win, 6, 2, " ");
        mvwprintw(win, 7, 2, " ");
        // Mark
        int mark_y = selected == 0 ? 5 : (selected == 1 ? 6 : 7);
        mvwprintw(win, mark_y, 2, ">");
        wrefresh(win);

        int ch = wgetch(win);
        if (ch == KEY_UP || ch == 'k' || ch == 'K') {
            if (selected > 0) selected--;
        } else if (ch == KEY_DOWN || ch == 'j' || ch == 'J') {
            if (selected < 2) selected++;
        } else if (ch == '\n' || ch == KEY_ENTER) {
            break;
        } else if (ch == 27) { // ESC
            selected = -1;
            break;
        } else if (ch == '1') {
            selected = 0; break;
        } else if (ch == '2') {
            selected = 1; break;
        } else if (ch == '3') {
            selected = 2; break;
        }
    }

    int result = selected; // -1 cancel, 0 cannon, 1 laser, 2 missile
    delwin(win);
    clear();
    refresh();
    endwin();
    return result;
}

// Repair ship: input HP amount to repair
int shop_repair_menu_ncurses(int current_hp, int max_hp, int coin) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);  // Show cursor for input
    erase();
    refresh();

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int win_h = 18;
    int win_w = 70;
    int start_y = (max_y - win_h) / 2;
    int start_x = (max_x - win_w) / 2;

    WINDOW *win = newwin(win_h, win_w, start_y, start_x);
    keypad(win, TRUE);
    box(win, 0, 0);

    mvwprintw(win, 1, (win_w - 18) / 2, "REPAIR SHIP (SHOP)");
    
    // Show coin at upper-right corner
    char coin_str[32];
    snprintf(coin_str, sizeof(coin_str), "Coin: %d", coin);
    int coin_x = win_w - 2 - (int)strlen(coin_str);
    if (coin_x < 2) coin_x = 2;
    mvwprintw(win, 1, coin_x, "%s", coin_str);

    // Ship status
    mvwprintw(win, 3, 2, "=== SHIP STATUS ===");
    if (current_hp >= 0 && max_hp > 0) {
        mvwprintw(win, 4, 2, "Current HP: %d / %d", current_hp, max_hp);
        int missing_hp = max_hp - current_hp;
        if (missing_hp > 0) {
            mvwprintw(win, 5, 2, "Missing HP: %d", missing_hp);
        } else {
            mvwprintw(win, 5, 2, "Ship is at full health!");
        }
    } else {
        mvwprintw(win, 4, 2, "HP Status: Unknown (fetch HP first)");
    }

    // Repair cost info
    mvwprintw(win, 7, 2, "=== REPAIR PRICING ===");
    mvwprintw(win, 8, 2, "Cost: 5 coin per 1 HP repaired");
    
    // Input field
    mvwprintw(win, 10, 2, "Enter HP amount to repair:");
    mvwprintw(win, 11, 2, "> ");
    
    // Instructions
    mvwprintw(win, 13, 2, "Tips:");
    mvwprintw(win, 14, 2, "- Max repair limited by coin and missing HP");
    mvwprintw(win, 15, 2, "- Press Enter to confirm, ESC to cancel");
    
    wrefresh(win);

    // Get input
    char input[16] = "";
    int result = -1;
    
    get_input_field(win, 11, 4, input, sizeof(input), 1);
    
    if (strlen(input) > 0) {
        result = atoi(input);
        if (result <= 0) {
            result = -1; // Invalid amount
        }
    } else {
        result = -1; // Cancelled or empty input
    }

    delwin(win);
    clear();
    refresh();
    endwin();
    return result; // Returns HP amount to repair, or -1 if cancelled
}

// Home menu: team management hub with user info
int home_menu_ncurses(const char *username, long coin, const char *team_name, int hp, int armor) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    erase();
    refresh();

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int win_h = 26;
    int win_w = 75;
    int start_y = (max_y - win_h) / 2;
    int start_x = (max_x - win_w) / 2;

    WINDOW *win = newwin(win_h, win_w, start_y, start_x);
    keypad(win, TRUE);
    box(win, 0, 0);

    // Title
    mvwprintw(win, 1, (win_w - 9) / 2, "HOME MENU");

    // User Information Section
    mvwprintw(win, 3, 2, "=== USER INFORMATION ===");
    if (username && strlen(username) > 0) {
        mvwprintw(win, 4, 4, "Username: %s", username);
    } else {
        mvwprintw(win, 4, 4, "Username: Unknown");
    }
    
    if (coin >= 0) {
        mvwprintw(win, 5, 4, "Coin:     %ld", coin);
    } else {
        mvwprintw(win, 5, 4, "Coin:     --");
    }
    
    if (team_name && strlen(team_name) > 0) {
        mvwprintw(win, 6, 4, "Team:     %s", team_name);
    } else {
        mvwprintw(win, 6, 4, "Team:     (No team)");
    }
    
    if (hp >= 0) {
        mvwprintw(win, 7, 4, "HP:       %d", hp);
    } else {
        mvwprintw(win, 7, 4, "HP:       --");
    }
    
    if (armor >= 0) {
        mvwprintw(win, 8, 4, "Armor:    %d", armor);
    } else {
        mvwprintw(win, 8, 4, "Armor:    --");
    }

    mvwprintw(win, 10, 2, "=== TEAM MANAGEMENT ===");

    // Menu options
    const char *menu_items[] = {
        "1. Create Team",
        "2. Join Team Request",
        "3. List All Teams",
        "4. View My Invites",
        "5. Back to Main Menu"
    };
    int menu_count = 5;
    int selected = 0;

    while (1) {
        // Draw menu items
        for (int i = 0; i < menu_count; i++) {
            if (i == selected) {
                wattron(win, A_REVERSE);
            }
            mvwprintw(win, 12 + i * 2, 4, "%-65s", menu_items[i]);
            if (i == selected) {
                wattroff(win, A_REVERSE);
            }
        }

        // Instructions
        mvwprintw(win, 22, 2, "Navigation:");
        mvwprintw(win, 23, 2, "  Up/Down: Navigate | Enter: Select | ESC: Back");
        mvwprintw(win, 24, 2, "  Number Keys (1-5): Quick select");
        
        wrefresh(win);

        int ch = wgetch(win);
        if (ch == KEY_UP || ch == 'k' || ch == 'K') {
            if (selected > 0) selected--;
        } else if (ch == KEY_DOWN || ch == 'j' || ch == 'J') {
            if (selected < menu_count - 1) selected++;
        } else if (ch == '\n' || ch == KEY_ENTER) {
            break;
        } else if (ch == 27) { // ESC
            selected = -1;
            break;
        } else if (ch >= '1' && ch <= '5') {
            selected = ch - '1';
            break;
        }
    }

    int result = selected; // 0=Create, 1=Join, 2=List, 3=ViewInvites, 4=Back, -1=Cancel
    delwin(win);
    clear();
    refresh();
    endwin();
    return result;
}

// Create team: input team name
int home_create_team_ncurses(char *team_name, size_t team_name_size) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);  // Show cursor
    erase();
    refresh();

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int win_h = 12;
    int win_w = 60;
    int start_y = (max_y - win_h) / 2;
    int start_x = (max_x - win_w) / 2;

    WINDOW *win = newwin(win_h, win_w, start_y, start_x);
    keypad(win, TRUE);
    box(win, 0, 0);

    mvwprintw(win, 1, (win_w - 11) / 2, "CREATE TEAM");
    mvwprintw(win, 3, 2, "Enter the name for your new team:");
    mvwprintw(win, 5, 2, "Team Name: ");
    mvwprintw(win, 7, 2, "Tips:");
    mvwprintw(win, 8, 2, "- Team name should be unique");
    mvwprintw(win, 9, 2, "- Press Enter to create, ESC to cancel");
    
    wrefresh(win);

    // Get input
    team_name[0] = '\0';
    get_input_field(win, 5, 13, team_name, team_name_size, 1);
    
    int result = (strlen(team_name) > 0) ? 1 : 0; // 1=success, 0=cancelled
    
    delwin(win);
    clear();
    refresh();
    endwin();
    return result;
}

// Join team request: input team name
int home_join_team_ncurses(char *team_name, size_t team_name_size) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);  // Show cursor
    erase();
    refresh();

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int win_h = 12;
    int win_w = 60;
    int start_y = (max_y - win_h) / 2;
    int start_x = (max_x - win_w) / 2;

    WINDOW *win = newwin(win_h, win_w, start_y, start_x);
    keypad(win, TRUE);
    box(win, 0, 0);

    mvwprintw(win, 1, (win_w - 17) / 2, "JOIN TEAM REQUEST");
    mvwprintw(win, 3, 2, "Enter the team name you want to join:");
    mvwprintw(win, 5, 2, "Team Name: ");
    mvwprintw(win, 8, 2, "Press Enter to send request, ESC to cancel");
    
    wrefresh(win);

    // Get input
    team_name[0] = '\0';
    get_input_field(win, 5, 13, team_name, team_name_size, 1);
    
    int result = (strlen(team_name) > 0) ? 1 : 0; // 1=success, 0=cancelled
    
    delwin(win);
    clear();
    refresh();
    endwin();
    return result;
}

// View invites: display list and allow accept/reject
// Returns: -1=back, 0=no invites, 1=action taken (accept/reject)
// team_name_out: the team name that was acted upon (if result=1)
// action_out: 0=reject, 1=accept (if result=1)
int home_view_invites_ncurses(const char *invites_data, char *team_name_out, size_t team_name_out_size, int *action_out) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    erase();
    refresh();

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int win_h = 24;
    int win_w = 70;
    int start_y = (max_y - win_h) / 2;
    int start_x = (max_x - win_w) / 2;

    WINDOW *win = newwin(win_h, win_w, start_y, start_x);
    keypad(win, TRUE);
    box(win, 0, 0);

    mvwprintw(win, 1, (win_w - 13) / 2, "MY INVITES");
    
    // Parse invites_data (format: "teamName1\nteamName2\n...")
    char invites_copy[2048];
    strncpy(invites_copy, invites_data ? invites_data : "", sizeof(invites_copy) - 1);
    invites_copy[sizeof(invites_copy) - 1] = '\0';
    
    // Count and store team names
    char team_names[20][128];
    int team_count = 0;
    char *line = strtok(invites_copy, "\n");
    while (line != NULL && team_count < 20) {
        // Trim whitespace
        while (*line && isspace(*line)) line++;
        if (strlen(line) > 0) {
            strncpy(team_names[team_count], line, 127);
            team_names[team_count][127] = '\0';
            team_count++;
        }
        line = strtok(NULL, "\n");
    }

    if (team_count == 0) {
        mvwprintw(win, 3, 2, "No pending invites.");
        mvwprintw(win, 5, 2, "Press any key to go back...");
        wrefresh(win);
        wgetch(win);
        
        delwin(win);
        clear();
        refresh();
        endwin();
        return 0; // No invites
    }

    // Display invites
    mvwprintw(win, 3, 2, "You have %d pending invite(s):", team_count);
    
    int selected = 0;
    int result = -1;
    
    while (1) {
        // Clear list area
        for (int i = 5; i < 17; i++) {
            for (int j = 2; j < win_w - 2; j++) {
                mvwaddch(win, i, j, ' ');
            }
        }
        
        // Display teams
        int display_start = (selected / 10) * 10;
        int display_count = (team_count - display_start > 10) ? 10 : (team_count - display_start);
        
        for (int i = 0; i < display_count; i++) {
            int idx = display_start + i;
            if (idx == selected) {
                wattron(win, A_REVERSE);
            }
            mvwprintw(win, 5 + i, 4, "%2d. %s", idx + 1, team_names[idx]);
            if (idx == selected) {
                wattroff(win, A_REVERSE);
            }
        }
        
        // Instructions
        mvwprintw(win, 18, 2, "Navigation:");
        mvwprintw(win, 19, 2, "  Up/Down: Navigate | A: Accept | R: Reject | ESC: Back");
        mvwprintw(win, 20, 2, "Selected: %s", team_names[selected]);
        
        wrefresh(win);

        int ch = wgetch(win);
        if (ch == KEY_UP || ch == 'k' || ch == 'K') {
            if (selected > 0) selected--;
        } else if (ch == KEY_DOWN || ch == 'j' || ch == 'J') {
            if (selected < team_count - 1) selected++;
        } else if (ch == 'a' || ch == 'A') {
            // Accept
            strncpy(team_name_out, team_names[selected], team_name_out_size - 1);
            team_name_out[team_name_out_size - 1] = '\0';
            *action_out = 1; // Accept
            result = 1;
            break;
        } else if (ch == 'r' || ch == 'R') {
            // Reject
            strncpy(team_name_out, team_names[selected], team_name_out_size - 1);
            team_name_out[team_name_out_size - 1] = '\0';
            *action_out = 0; // Reject
            result = 1;
            break;
        } else if (ch == 27) { // ESC
            result = -1;
            break;
        }
    }

    delwin(win);
    clear();
    refresh();
    endwin();
    return result;
}

// Team menu: detailed team management
// Returns: 0=Leave, 1=Invite, 2=Accept Join Request, 3=Challenge, 4=Back, -1=Cancel
int team_menu_ncurses(int team_id, const char *team_name, const char *captain, int member_count, const char *members_list) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    erase();
    refresh();

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int win_h = 28;
    int win_w = 75;
    int start_y = (max_y - win_h) / 2;
    int start_x = (max_x - win_w) / 2;

    WINDOW *win = newwin(win_h, win_w, start_y, start_x);
    keypad(win, TRUE);
    box(win, 0, 0);

    // Title
    mvwprintw(win, 1, (win_w - 9) / 2, "TEAM MENU");

    // Team Information Section
    mvwprintw(win, 3, 2, "=== TEAM INFORMATION ===");
    mvwprintw(win, 4, 4, "Team ID:      %d", team_id);
    mvwprintw(win, 5, 4, "Team Name:    %s", team_name ? team_name : "(Unknown)");
    mvwprintw(win, 6, 4, "Captain:      %s", captain ? captain : "(Unknown)");
    mvwprintw(win, 7, 4, "Member Count: %d", member_count);

    // Members list
    mvwprintw(win, 9, 2, "=== TEAM MEMBERS ===");
    if (members_list && strlen(members_list) > 0) {
        // Parse and display members (format: "member1\nmember2\n...")
        char members_copy[1024];
        strncpy(members_copy, members_list, sizeof(members_copy) - 1);
        members_copy[sizeof(members_copy) - 1] = '\0';
        
        int line = 10;
        char *member = strtok(members_copy, "\n");
        while (member != NULL && line < 15) {
            // Trim whitespace
            while (*member && isspace(*member)) member++;
            if (strlen(member) > 0) {
                mvwprintw(win, line++, 4, "- %s", member);
            }
            member = strtok(NULL, "\n");
        }
    } else {
        mvwprintw(win, 10, 4, "(No members)");
    }

    mvwprintw(win, 16, 2, "=== TEAM ACTIONS ===");

    // Menu options
    const char *menu_items[] = {
        "1. Leave Team",
        "2. Invite Member",
        "3. Accept Join Request",
        "4. Challenge Another Team",
        "5. Back to Main Menu"
    };
    int menu_count = 5;
    int selected = 0;

    while (1) {
        // Draw menu items
        for (int i = 0; i < menu_count; i++) {
            if (i == selected) {
                wattron(win, A_REVERSE);
            }
            mvwprintw(win, 18 + i, 4, "%-65s", menu_items[i]);
            if (i == selected) {
                wattroff(win, A_REVERSE);
            }
        }

        // Instructions
        mvwprintw(win, 24, 2, "Navigation:");
        mvwprintw(win, 25, 2, "  Up/Down: Navigate | Enter: Select | ESC: Back");
        mvwprintw(win, 26, 2, "  Number Keys (1-5): Quick select");
        
        wrefresh(win);

        int ch = wgetch(win);
        if (ch == KEY_UP || ch == 'k' || ch == 'K') {
            if (selected > 0) selected--;
        } else if (ch == KEY_DOWN || ch == 'j' || ch == 'J') {
            if (selected < menu_count - 1) selected++;
        } else if (ch == '\n' || ch == KEY_ENTER) {
            break;
        } else if (ch == 27) { // ESC
            selected = -1;
            break;
        } else if (ch >= '1' && ch <= '5') {
            selected = ch - '1';
            break;
        }
    }

    int result = selected;
    delwin(win);
    clear();
    refresh();
    endwin();
    return result;
}

// Invite member: input username
int team_invite_member_ncurses(char *username, size_t username_size) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);
    erase();
    refresh();

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int win_h = 12;
    int win_w = 60;
    int start_y = (max_y - win_h) / 2;
    int start_x = (max_x - win_w) / 2;

    WINDOW *win = newwin(win_h, win_w, start_y, start_x);
    keypad(win, TRUE);
    box(win, 0, 0);

    mvwprintw(win, 1, (win_w - 13) / 2, "INVITE MEMBER");
    mvwprintw(win, 3, 2, "Enter the username to invite to your team:");
    mvwprintw(win, 5, 2, "Username: ");
    mvwprintw(win, 8, 2, "Press Enter to send invite, ESC to cancel");
    
    wrefresh(win);

    username[0] = '\0';
    get_input_field(win, 5, 12, username, username_size, 1);
    
    int result = (strlen(username) > 0) ? 1 : 0;
    
    delwin(win);
    clear();
    refresh();
    endwin();
    return result;
}

// Accept join request: display list and allow accept/reject
int team_accept_join_request_ncurses(const char *requests_data, char *username_out, size_t username_out_size, int *action_out) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    erase();
    refresh();

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int win_h = 24;
    int win_w = 70;
    int start_y = (max_y - win_h) / 2;
    int start_x = (max_x - win_w) / 2;

    WINDOW *win = newwin(win_h, win_w, start_y, start_x);
    keypad(win, TRUE);
    box(win, 0, 0);

    mvwprintw(win, 1, (win_w - 18) / 2, "JOIN REQUESTS");
    
    // Parse requests (format: "username1\nusername2\n...")
    char requests_copy[2048];
    strncpy(requests_copy, requests_data ? requests_data : "", sizeof(requests_copy) - 1);
    requests_copy[sizeof(requests_copy) - 1] = '\0';
    
    char usernames[20][128];
    int user_count = 0;
    char *line = strtok(requests_copy, "\n");
    while (line != NULL && user_count < 20) {
        while (*line && isspace(*line)) line++;
        if (strlen(line) > 0) {
            strncpy(usernames[user_count], line, 127);
            usernames[user_count][127] = '\0';
            user_count++;
        }
        line = strtok(NULL, "\n");
    }

    if (user_count == 0) {
        mvwprintw(win, 3, 2, "No pending join requests.");
        mvwprintw(win, 5, 2, "Press any key to go back...");
        wrefresh(win);
        wgetch(win);
        
        delwin(win);
        clear();
        refresh();
        endwin();
        return 0;
    }

    mvwprintw(win, 3, 2, "You have %d pending join request(s):", user_count);
    
    int selected = 0;
    int result = -1;
    
    while (1) {
        for (int i = 5; i < 17; i++) {
            for (int j = 2; j < win_w - 2; j++) {
                mvwaddch(win, i, j, ' ');
            }
        }
        
        int display_start = (selected / 10) * 10;
        int display_count = (user_count - display_start > 10) ? 10 : (user_count - display_start);
        
        for (int i = 0; i < display_count; i++) {
            int idx = display_start + i;
            if (idx == selected) wattron(win, A_REVERSE);
            mvwprintw(win, 5 + i, 4, "%2d. %s", idx + 1, usernames[idx]);
            if (idx == selected) wattroff(win, A_REVERSE);
        }
        
        mvwprintw(win, 18, 2, "Navigation:");
        mvwprintw(win, 19, 2, "  Up/Down: Navigate | A: Approve | R: Reject | ESC: Back");
        mvwprintw(win, 20, 2, "Selected: %s", usernames[selected]);
        
        wrefresh(win);

        int ch = wgetch(win);
        if (ch == KEY_UP || ch == 'k' || ch == 'K') {
            if (selected > 0) selected--;
        } else if (ch == KEY_DOWN || ch == 'j' || ch == 'J') {
            if (selected < user_count - 1) selected++;
        } else if (ch == 'a' || ch == 'A') {
            strncpy(username_out, usernames[selected], username_out_size - 1);
            username_out[username_out_size - 1] = '\0';
            *action_out = 1; // Approve
            result = 1;
            break;
        } else if (ch == 'r' || ch == 'R') {
            strncpy(username_out, usernames[selected], username_out_size - 1);
            username_out[username_out_size - 1] = '\0';
            *action_out = 0; // Reject
            result = 1;
            break;
        } else if (ch == 27) {
            result = -1;
            break;
        }
    }

    delwin(win);
    clear();
    refresh();
    endwin();
    return result;
}

// Challenge team: input team ID or name
int team_challenge_ncurses(char *target_team, size_t target_team_size) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);
    erase();
    refresh();

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int win_h = 14;
    int win_w = 65;
    int start_y = (max_y - win_h) / 2;
    int start_x = (max_x - win_w) / 2;

    WINDOW *win = newwin(win_h, win_w, start_y, start_x);
    keypad(win, TRUE);
    box(win, 0, 0);

    mvwprintw(win, 1, (win_w - 14) / 2, "CHALLENGE TEAM");
    mvwprintw(win, 3, 2, "Enter the Team ID to challenge:");
    mvwprintw(win, 5, 2, "Team ID: ");
    mvwprintw(win, 7, 2, "Tips:");
    mvwprintw(win, 8, 2, "- You can find team IDs in the team list");
    mvwprintw(win, 9, 2, "- Both teams must have 3 members");
    mvwprintw(win, 10, 2, "Press Enter to send challenge, ESC to cancel");
    
    wrefresh(win);

    target_team[0] = '\0';
    get_input_field(win, 5, 11, target_team, target_team_size, 1);
    
    int result = (strlen(target_team) > 0) ? 1 : 0;
    
    delwin(win);
    clear();
    refresh();
    endwin();
    return result;
}

#endif
