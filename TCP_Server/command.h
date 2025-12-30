#ifndef COMMAND_H
#define COMMAND_H

#include <stddef.h>

/**
 * @file command.h
 * @brief Command parsing utilities
 * 
 * Parses raw text commands from clients into structured format.
 * Handles CRLF stripping and argument extraction.
 */

/**
 * @struct Command
 * @brief Parsed command structure
 */
typedef struct {
    const char *type;        /**< Command type (e.g., "REGISTER", "LOGIN") */
    const char *user_input;  /**< Remaining arguments as string */
} Command;

/**
 * @brief Parse raw command string into Command struct
 * 
 * Modifies input string in-place by null-terminating at space.
 * Strips trailing CRLF if present.
 * 
 * @param input Raw command string (will be modified)
 * @return Command struct with pointers into input buffer
 * 
 * Example:
 *   input: "REGISTER alice pass123\r\n"
 *   output: { type: "REGISTER", user_input: "alice pass123" }
 * 
 * TODO: This implementation is copied from phu/command.c
 * No changes needed unless you want to add validation.
 */
Command parse_command(char *input);

#endif // COMMAND_H
