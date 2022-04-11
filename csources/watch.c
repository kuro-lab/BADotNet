#include "common.h"
#include "config.h"

double get_cputime(void) {
    struct timespec t;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
    // clock_gettime(CLOCK_THREAD_CPUTIME_ID,&t);
    return t.tv_sec + (double)t.tv_nsec * 1e-9;
}

double get_realtime(void) {
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return t.tv_sec + (double)t.tv_nsec * 1e-9;
}

char *elapsed_span(double elapsed) {
    char *timespan = NULL;
    timespan = (char *)malloc(sizeof(char) * 200);

    double f = elapsed;
    int elp = (int)f;
    f -= elp;
    int d = elp / 86400;
    elp = elp % 86400;
    int h = elp / 3600;
    elp = elp % 3600;
    int m = elp / 60;
    int s = elp % 60;
    f += s;
    if (d)
        sprintf(timespan, "%dd %02dh %02dm %02ds", d, h, m, s);
    else if (h)
        sprintf(timespan, "%dh %02dm %02ds", h, m, s);
    else if (m)
        sprintf(timespan, "%dm %06.3fs", m, f);
    else
        sprintf(timespan, "%.3fs", f);

    return timespan;
}
