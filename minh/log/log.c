#include "log.h"
#include <stdio.h>
#include <time.h>

void log_msg(const char *message) {
    FILE *f = fopen("server.log", "a");
    if (!f) return;

    time_t t = time(NULL);

    printf("[%ld] %s\n", t, message);
    fflush(stdout);

    fprintf(f, "[%ld] %s\n", t, message);
    fclose(f);
}

void log_action(const char *message) {
    FILE *f = fopen("actions.log", "a");
    if (!f) return;

    printf("%s\n", message);
    fflush(stdout);

    fprintf(f, "%s\n", message);
    fclose(f);
}
