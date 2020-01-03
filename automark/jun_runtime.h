#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#define SECOND_TO_MICRO 1000000

void ssu_runtime(struct timeval* begin_t, struct timeval* end_t)
{
    end_t -> tv_sec -= begin_t -> tv_sec;

    if(end_t -> tv_usec < begin_t -> tv_usec){
        end_t -> tv_sec--;
        end_t -> tv_usec += SECOND_TO_MICRO;
    }

    end_t -> tv_usec -= begin_t -> tv_usec;
    printf("Runtime: %ld:%06d(sec:usec)\n", end_t -> tv_sec, end_t -> tv_usec);
}
