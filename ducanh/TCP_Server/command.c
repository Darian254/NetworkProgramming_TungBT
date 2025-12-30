#include "command.h"
#include <string.h>

/**
 * @file command.c
 * @brief Command parsing implementation
 * 
 * Simple parser that splits command line into type and arguments.
 * Copied from phu/command.c - proven to work.
 */

Command parse_command(char *input) {
    Command cmd;
    cmd.type = NULL;
    cmd.user_input = NULL;

    // Strip trailing CRLF if present
    size_t len = strlen(input);
    if (len >= 2 && input[len-2] == '\r' && input[len-1] == '\n') {
        input[len-2] = '\0';
    }

    // Find first space to split command type from arguments
    char *space = strchr(input, ' ');

    if (space == NULL) {
        // No arguments - entire string is command type
        cmd.type = input;
        cmd.user_input = "";
    } else {
        // Split at space: before = type, after = arguments
        *space = '\0';
        cmd.type = input;
        cmd.user_input = space + 1;
    }

    return cmd;
}
