#include "command.h"
#include <string.h>

Command parse_command(char *input) {
    Command cmd;
    cmd.type = NULL;
    cmd.user_input = NULL;

    size_t len = strlen(input);
    if (len >= 2 && input[len-2] == '\r' && input[len-1] == '\n') {
        input[len-2] = '\0';
    }

    char *space = strchr(input, ' ');

    if (space == NULL) {
        cmd.type = input;
        cmd.user_input = "";
    } else {
        *space = '\0';
        cmd.type = input;
        cmd.user_input = space + 1;
    }

    return cmd;
}
