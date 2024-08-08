#ifndef TIMER_H
#define TIMER_H

#include <stdbool.h>

struct thread_data {
    int fd;
    int sock_fd;
    pthread_mutex_t *mutex;
    bool completed;
};

timer_t setup_timer_thread(struct thread_data *td, int seconds);

#endif // TIMER_H
