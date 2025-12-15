#include "config.h"
#include <stddef.h>

const char *get_response_message(ResponseCode code) {
    for (size_t i = 0; i < RESPONSE_MESSAGES_COUNT; i++) {
        if (RESPONSE_MESSAGES[i].code == code) {
            return RESPONSE_MESSAGES[i].message;
        }
    }
    return NULL;
}

