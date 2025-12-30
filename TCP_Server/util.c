#include "util.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>

/* Convert response code to message (used by both server and client) */
void beautify_result(const char *raw, char *outbuf, size_t buflen) {
    if (!raw || !outbuf || buflen == 0) return;

    int code = atoi(raw);
    const char *message = get_response_message((ResponseCode)code);
    
    if (message) {
        snprintf(outbuf, buflen, "%s\n", message);
    } else {
        snprintf(outbuf, buflen, "%s\n", raw);
    }
}

void clearInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

void safeInput(char *buffer, size_t size) {
    if (fgets(buffer, (int)size, stdin)) {
        // Check if newline was found before removing it
        size_t len = strlen(buffer);
        int has_newline = (len > 0 && buffer[len - 1] == '\n');
        
        // Remove newline
        buffer[strcspn(buffer, "\n")] = '\0';
        
        // If buffer was full and no newline was found, clear remaining input
        if (!has_newline && len == size - 1) {
            clearInputBuffer();
        }
    } else {
        buffer[0] = '\0';
    }
}


static const char *file_path = "server_activity.log";

static const char *level_for_code(ResponseCode code) {
    if ((int)code >= 500) return "[ERROR]";
    if ((int)code >= 300) return "[WARN]";
    return "[INFO]";
}

void log_activity(const char *action,
                  const char *username,
                  bool is_logged_in,
                  const char *user_input,
                  ResponseCode code) {
    FILE *fp = fopen(file_path, "a");
    if (!fp) return;

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char ts[32] = "";
    if (tm_info) {
        strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", tm_info);
    }

    const char *lvl = level_for_code(code);
    const char *msg = get_response_message(code);
    if (!msg) msg = "UNKNOWN";

    const char *user_field = (is_logged_in && username && username[0] != '\0') ? username : "-";
    const char *action_field = action && action[0] ? action : "-";

    /* Format: 2025-12-24 12:34:56 [info] action=LOGIN user=alice input="..." code=110 message="..." */
    fprintf(fp, "%s %s action=%s user=%s input=\"%s\" code=%d message=\"%s\"\n",
            ts, lvl, action_field, user_field, user_input, (int)code, msg);
    fclose(fp);
}
