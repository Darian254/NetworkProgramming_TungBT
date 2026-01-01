#include "ui.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
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
    printf(" [GAME ACTIONS]\n");
    printf("39. Fire (Attack)\n");
    printf("40. Challenge Team (Send Challenge)\n");
    printf("41. Open Chest\n");
    printf("42. Accept Challenge\n");
    printf("43. Decline Challenge\n");
    printf("44. Battle Screen\n");
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

void show_message_in_battle_screen(const char *title, const char *message) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int win_h = 8;
    int win_w = 60;
    int start_y = (max_y - win_h) / 2;
    int start_x = (max_x - win_w) / 2;
    
    WINDOW *win = newwin(win_h, win_w, start_y, start_x);
    keypad(win, TRUE);
    box(win, 0, 0);
    
    mvwprintw(win, 1, (win_w - strlen(title)) / 2, "%s", title);
    
    // --- KHẮC PHỤC LỖI SEGMENTATION FAULT ---
    // Nguyên nhân: Hàm strdup có thể gây lỗi trên một số hệ thống nếu không khai báo đúng chuẩn POSIX.
    // Giải pháp: Thay thế bằng malloc + strcpy thủ công để an toàn tuyệt đối.
    char *msg_copy = NULL;
    if (message) {
        msg_copy = malloc(strlen(message) + 1);
        if (msg_copy) {
            strcpy(msg_copy, message);
        }
    }

    if (msg_copy) {
        char *line = strtok(msg_copy, "\n");
        int y_offset = 0;
        int msg_y = 3;
        int msg_x = 2;
        int max_msg_w = win_w - 4;
        
        while (line && y_offset < 3) {
            mvwprintw(win, msg_y + y_offset, msg_x, "%.*s", max_msg_w, line);
            line = strtok(NULL, "\n");
            y_offset++;
        }
        free(msg_copy); // Giải phóng bộ nhớ sau khi dùng xong
    }
    // ----------------------------------------
    
    mvwprintw(win, 6, (win_w - 20) / 2, "Press any key...");
    
    wrefresh(win);
    wgetch(win);
    
    delwin(win);
   
}
// Wrapper function để khởi tạo ncurses, hiển thị message, và đóng ncurses
void show_message_in_battle_screen_with_init(const char *title, const char *message) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    show_message_in_battle_screen(title, message);
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
    mvwprintw(win, y++, 2, "44. Battle Screen");
    
    mvwprintw(win, win_h - 2, 2, "Enter option number (0-44) or ESC to exit: ");
    
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
                if (choice >= 0 && choice <= 39) {
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

    int win_h = 12;
    int win_w = 60;
    int start_y = (max_y - win_h) / 2;
    int start_x = (max_x - win_w) / 2;

    WINDOW *win = newwin(win_h, win_w, start_y, start_x);
    keypad(win, TRUE);
    box(win, 0, 0);

    mvwprintw(win, 1, (win_w - 9) / 2, "SHOP MENU");

    int selected = 0; // 0 = Buy Armor, 1 = Buy Weapon
    int result = -1;

    while (1) {
        // Clear area
        for (int i = 3; i < 8; i++) {
            for (int j = 2; j < win_w - 2; j++) {
                mvwaddch(win, i, j, ' ');
            }
        }

        // Buy Armor button (left)
        int armor_x = 8, armor_y = 5, armor_w = 18, armor_h = 3;
        if (selected == 0) wattron(win, A_REVERSE);
        for (int i = 0; i < armor_h; i++) {
            for (int j = 0; j < armor_w; j++) {
                mvwaddch(win, armor_y + i, armor_x + j, ' ');
            }
        }
        mvwprintw(win, armor_y + 1, armor_x + (armor_w - 10) / 2, "BUY ARMOR");
        if (selected == 0) wattroff(win, A_REVERSE);

        // Buy Weapon button (right)
        int weapon_x = 34, weapon_y = 5, weapon_w = 18, weapon_h = 3;
        if (selected == 1) wattron(win, A_REVERSE);
        for (int i = 0; i < weapon_h; i++) {
            for (int j = 0; j < weapon_w; j++) {
                mvwaddch(win, weapon_y + i, weapon_x + j, ' ');
            }
        }
        mvwprintw(win, weapon_y + 1, weapon_x + (weapon_w - 11) / 2, "BUY WEAPON");
        if (selected == 1) wattroff(win, A_REVERSE);

        // Instructions
        mvwprintw(win, 9, (win_w - 40) / 2, "Arrow: Select | Enter: Confirm | ESC: Back");
        wrefresh(win);

        int ch = wgetch(win);
        if (ch == KEY_LEFT || ch == KEY_RIGHT) {
            selected = 1 - selected;
        } else if (ch == '\n' || ch == KEY_ENTER) {
            result = selected; // 0 = Buy Armor, 1 = Buy Weapon
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
        }
    }

    delwin(win);
    clear();
    refresh();
    endwin();
    return result;
}

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
// --- Hàm mới: Hiển thị popup nhập câu trả lời ---
int popup_input_ncurses(const char *title, const char *question, char *buffer, size_t size) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);
    erase();
    refresh();
    
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    int win_h = 14; int win_w = 70; // Tăng chiều cao để chứa câu hỏi dài
    WINDOW *win = newwin(win_h, win_w, (max_y - win_h)/2, (max_x - win_w)/2);
    keypad(win, TRUE); box(win, 0, 0);
    
    mvwprintw(win, 1, (win_w - strlen(title))/2, "%s", title);
    
    // In câu hỏi (tự động xuống dòng nếu dài)
    int q_len = strlen(question);
    int line_width = win_w - 4;
    int q_lines = (q_len / line_width) + 1;
    for (int i = 0; i < q_lines; i++) {
        mvwprintw(win, 3 + i, 2, "%.*s", line_width, question + (i * line_width));
    }

    mvwprintw(win, 3 + q_lines + 1, 2, "Answer: ");
    mvwprintw(win, win_h - 2, (win_w - 30)/2, "Enter: Submit | ESC: Cancel");
    wrefresh(win);
    
    curs_set(1); echo();
    wmove(win, 3 + q_lines + 1, 10);
    int ch = wgetnstr(win, buffer, size - 1);
    noecho(); curs_set(0);
    delwin(win); clear(); refresh();
    endwin();
    
    if (ch == ERR) return 0;
    return (strlen(buffer) > 0);
}
int battle_screen_ncurses(const char *my_username,
    const char **team_left, int *team_left_hp, int left_count,
    const char **team_right, int *team_right_hp, int right_count,
    int my_hp, int my_armor, int my_coin,
    int active_chest_id,
    char *out_target_username, size_t out_target_username_size,
    int *out_weapon_id) {
    
    initscr(); cbreak(); noecho(); keypad(stdscr, TRUE); curs_set(0);
    erase(); refresh();

    int max_y, max_x; getmaxyx(stdscr, max_y, max_x);
    int win_h = (max_y > 24) ? 24 : max_y - 1;
    int win_w = (max_x > 80) ? 80 : max_x - 1;
    
    if (win_h < 22 || win_w < 60) {
        endwin(); printf("Terminal too small! Resize needed.\n"); return -1;
    }

    WINDOW *win = newwin(win_h, win_w, (max_y - win_h)/2, (max_x - win_w)/2);
    keypad(win, TRUE); box(win, 0, 0);

    mvwprintw(win, 1, 2, "[S] Shop");
    mvwprintw(win, 1, (win_w - 12)/2, "BATTLE FIELD");
    mvwprintw(win, 1, win_w - 50, "HP:%d Armor:%d Coin:%d", my_hp, my_armor, my_coin); 

    int col_left = 4, col_right = win_w - 24, row_top = 4;
    mvwprintw(win, 2, col_left, "TEAM A"); mvwprintw(win, 2, col_right, "TEAM B");

    if (active_chest_id > 0) {
        wattron(win, A_BOLD | A_REVERSE);
        mvwprintw(win, win_h/2, win_w/2 - 2, "[ $ ]"); 
        wattroff(win, A_BOLD | A_REVERSE);
        mvwprintw(win, win_h/2 + 1, win_w/2 - 4, "Press 'C'");
    }

    for (int i = 0; i < 3; i++) {
        int y = row_top + i * 5;
        int has_p = (i < left_count && team_left[i]);
        int dead = (has_p && team_left_hp[i] <= 0);
        if (dead) wattron(win, A_DIM);
        
        mvwhline(win, y, col_left, '-', 16);
        mvwhline(win, y+3, col_left, '-', 16);
        mvwvline(win, y+1, col_left, '|', 2);
        mvwvline(win, y+1, col_left+15, '|', 2);
        
        if (has_p) {
            mvwprintw(win, y+1, col_left+2, "%.*s", 12, team_left[i]);
            mvwprintw(win, y+2, col_left+2, "HP:%d", team_left_hp[i]);
            if (my_username && strcmp(my_username, team_left[i]) == 0) 
                mvwprintw(win, y+2, col_left+10, "(Me)");
            if (dead) mvwprintw(win, y+1, col_left+13, "XX");
        } else mvwprintw(win, y+1, col_left+2, "[Empty]");
        if (dead) wattroff(win, A_DIM);
    }

    static int sel = 0;
    if (sel > 2) sel = 0;

    while (1) {
        for (int i = 0; i < 3; i++) {
            int y = row_top + i * 5;
            int has_p = (i < right_count && team_right[i]);
            int dead = (has_p && team_right_hp[i] <= 0);
            
            if (i == sel) wattron(win, A_REVERSE);
            
            mvwhline(win, y, col_right, '-', 16);
            mvwhline(win, y+3, col_right, '-', 16);
            mvwvline(win, y+1, col_right, '|', 2);
            mvwvline(win, y+1, col_right+15, '|', 2);
            
            if (has_p) {
                if (dead) wattron(win, A_DIM);
                mvwprintw(win, y+1, col_right+2, "%.*s", 12, team_right[i]);
                mvwprintw(win, y+2, col_right+2, "HP:%d", team_right_hp[i]);
                if (dead) {
                    mvwprintw(win, y+1, col_right+13, "XX");
                    wattroff(win, A_DIM);
                }
            } else {
                mvwprintw(win, y+1, col_right+2, "[Empty]");
            }
            
            if (i == sel) wattroff(win, A_REVERSE);
        }

        mvwprintw(win, win_h-2, 2, "Up/Down: Target | Enter: FIRE | S: Shop | C: Chest | ESC");
        wrefresh(win);

        int ch = wgetch(win);
        if (ch == KEY_UP) {
            sel--; if (sel < 0) sel = 2; 
        } else if (ch == KEY_DOWN) {
            sel++; if (sel > 2) sel = 0;
        } else if (ch == 's' || ch == 'S') { delwin(win); endwin(); return 0; }
        else if (ch == 'c' || ch == 'C') { 
            if(active_chest_id > 0) { delwin(win); endwin(); return 2; }
        }
        else if (ch == 27) { delwin(win); endwin(); return -1; }
        else if (ch == '\n' || ch == KEY_ENTER || ch == 'f' || ch == 'F') {
            if (sel >= 0 && sel < right_count && team_right[sel]) {
                if (team_right_hp[sel] <= 0) {
                    mvwprintw(win, win_h-2, 2, "TARGET DEAD! Select another.                  ");
                    wrefresh(win); napms(500);
                } else {
                    if (out_target_username) strcpy(out_target_username, team_right[sel]);
                    if (out_weapon_id) *out_weapon_id = 0;
                    delwin(win); endwin(); return 1;
                }
            } else {
                mvwprintw(win, win_h-2, 2, "INVALID TARGET! (Empty slot)                  ");
                wrefresh(win); napms(500);
            }
        }
    }
}
#endif