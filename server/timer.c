#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
// for timers
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include "timer.h"

static void timer_thread(union sigval tdata) 
{    
    struct thread_data * data = (struct thread_data *) tdata.sival_ptr;
    static char format[] = "timestamp: %F %T\n";
    char date_str[64];
    struct tm * tmp;
    time_t t;
    int rc;

    rc = pthread_mutex_lock(data->mutex);
    if (rc) 
        perror("mutex_lock");

    time(&t);
    tmp = localtime(&t);
    strftime(date_str, sizeof date_str, format, tmp);

    // TODO handle partial writes
    write(data->fd, date_str, strlen(date_str));

    rc = pthread_mutex_unlock(data->mutex);
    if (rc)
        perror("mutex_unlock");
}

// Sets up and starts the timer and return it's ID, so the caller should timer_delete() it
timer_t setup_timer_thread(struct thread_data *td, int seconds) 
{
    timer_t timer;

    struct sigevent evp;
    memset(&evp, 0, sizeof evp);
    evp.sigev_notify = SIGEV_THREAD;
    evp.sigev_value.sival_ptr = td;
    evp.sigev_notify_function = timer_thread;

    int ret;

    ret = timer_create(CLOCK_MONOTONIC, &evp, &timer);
    if (ret)
        perror("timer_create");

    // Sets the timing
    struct itimerspec ts;
    ts.it_interval.tv_sec  = seconds;
    ts.it_interval.tv_nsec = 0;
    ts.it_value.tv_sec     = seconds;
    ts.it_value.tv_nsec    = 0;

    ret = timer_settime(timer, TIMER_ABSTIME, &ts, NULL);
    if (ret)
        perror("timer_settime");

    return timer;
}

