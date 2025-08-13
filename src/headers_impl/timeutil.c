#include <stdio.h>
#include <sys/time.h>
#include "../headers/common_headers.h"

long long current_time_millis() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)(tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
}

long long current_time_microseconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);  // Fills tv with current time
    return (long long)tv.tv_sec * 1000000 + tv.tv_usec;
}