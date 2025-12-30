#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

/**
 * @brief Beautify server response for human-friendly display.
 * Converts response code (number) to message using get_response_message().
 * Used by both server and client.
 *
 * @param raw     Response code as string (e.g. "212", "110")
 * @param outbuf  Output buffer
 * @param buflen  Output buffer size
 */
void beautify_result(const char *raw, char *outbuf, size_t buflen);

/**
 * @brief Clear any remaining characters from the input buffer.
 *
 * Consumes all characters left in stdin until a newline or EOF is reached.
 */
void clearInputBuffer();

/**
 * @brief Safely read a string from stdin.
 *
 * - Uses fgets to avoid buffer overflow.  
 * - Removes the trailing newline character if present.  
 * - If input fails, sets buffer to an empty string.
 *
 * @param buffer Destination buffer
 * @param size Maximum number of characters to read (including null terminator)
 */
void safeInput(char * buffer, size_t size);

#endif /* UTIL_H */

