#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

// Optional: use these functions to add debug or error prints to your application
//#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define DEBUG_LOG(msg, ...) printf("[thread-%lu]: " msg "\n", \
        (unsigned long) pthread_self(), ##__VA_ARGS__); 
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)


void* threadfunc(void* thread_param)
{
    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;    
    
    struct thread_data * data = (struct thread_data *) thread_param;
    DEBUG_LOG("sleeping for %dms before mutex...", data->wto_ms);
    usleep(data->wto_ms * 1000);

    DEBUG_LOG("obtaining mutex");
    int rc;
    rc = pthread_mutex_lock(data->mutex);
    if (rc) {
        errno = rc;
        perror("pthread_mutex_lock");
        DEBUG_LOG("fatal error obtaining mutex, exiting...");
        pthread_exit(thread_param);
    }

    DEBUG_LOG("holding mutex for %dms...", data->wtr_ms);
    usleep(data->wtr_ms * 1000);

    DEBUG_LOG("releasing mutex");
    rc = pthread_mutex_unlock(data->mutex);
    if (rc) {
        errno = rc;
        perror("pthread_mutex_unlock");
        DEBUG_LOG("fatal error releasing mutex, exiting...");
        pthread_exit(thread_param);
    }

    DEBUG_LOG("completed succesfully");    
    data->thread_complete_success = true;
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */

    struct thread_data * tdata;
    tdata = (struct thread_data *) malloc(sizeof (struct thread_data));
    if (tdata == NULL) {
        perror("start_thread_obtaining_mutex");
        return false;
    }

    // Initialize the struct
    tdata->mutex = mutex;
    tdata->wto_ms = wait_to_obtain_ms;
    tdata->wtr_ms = wait_to_release_ms;
    tdata-> thread_complete_success = false;

    // debug
    //pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
    //tdata->mutex = &mtx;

    int rc;
    rc = pthread_create(thread, NULL, threadfunc, (void *) tdata);

    if (rc == 0) {
        printf("New thread started, id: [%lu]\n", (unsigned long) *thread);
        // remove later for test validation, as its supposed to not block
        // pthread_join(*thread, NULL);
        return true;
    }

    errno = rc;
    perror("pthread_create");

    return false;
}

