#include "util.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h> 

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

