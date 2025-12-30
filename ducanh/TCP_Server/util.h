#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdbool.h>
#include "config.h"

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

/**
 * @brief Write a structured activity log line.
 *
 * Fields written per line:
 *   timestamp, level ([info]|[warn]|[error]), action, user, input, code, message
 *
 * - Level is derived automatically from code: <300 => info, 300-499 => warn, >=500 => error
 * - If not logged in, user will be "-".
 * - Control characters in input are sanitized to spaces.
 *
 * @param action       Operation name, e.g. "REGISTER", "LOGIN"
 * @param username     Username if known (NULL or empty if anonymous)
 * @param is_logged_in Whether the session is logged in
 * @param user_input   Raw user payload (will be sanitized)
 * @param code         Response code
 */
void log_activity(const char *action,
				  const char *username,
				  bool is_logged_in,
				  const char *user_input,
				  ResponseCode code);

/* Convenience macro when you have a ServerSession pointer available. */
/* Intentionally not including session.h here to avoid coupling.        */
#define LOG_ACTIVITY_SESSION(action, sessionPtr, input, code)                                       \
	do {                                                                                           \
		const char *_u_ = ((sessionPtr) && (sessionPtr)->isLoggedIn) ? (sessionPtr)->username : NULL; \
		bool _lg_ = ((sessionPtr) && (sessionPtr)->isLoggedIn);                                    \
		log_activity((action), _u_, _lg_, (input), (code));                                        \
	} while (0)

#endif /* UTIL_H */

