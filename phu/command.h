#pragma once
#include <stddef.h>

typedef struct {
    const char *type;
    const char *user_input;
} Command;

/**
 * @brief Parses the command from the client input.
 * @param input The input string from the client.
 * @return A Command struct containing the command type and user input.
*/
Command parse_command(char *input);
