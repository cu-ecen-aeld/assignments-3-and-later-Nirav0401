#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
// File includes
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
// Sockets
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <netdb.h>
// Thread
#include <pthread.h>
// Timer
#include "timer.h"


#define PORT        "9000"
#define BACKLOG     10 
#define WRITEFILE   "/var/tmp/aesdsocketdata"
#define EOL         '\n'

struct node_t {
    pthread_t thread_id;
    struct thread_data *tdata;
    struct node_t *next;
};

void * connection_handler(void *);
int length(struct node_t *head);

void insert(struct node_t **headref, struct thread_data * tdata) 
{
    struct node_t *aux;

    aux = (struct node_t *) malloc (sizeof(struct node_t));
    if (aux == NULL) {
        syslog(LOG_ERR, "Error allocating memory for list: %m");
        _exit(-1);
    }
    memset(aux, 0, sizeof (struct node_t));

    int rc;
    pthread_t tid;
    rc = pthread_create(&tid, NULL, connection_handler, (void *) tdata);
    if (rc != 0) {
        errno = rc;
        syslog(LOG_ERR, "Error creating thread: %m");
        _exit(-1);
    }

    syslog(LOG_DEBUG, "New thread started: %ld", (unsigned long) tid);

    aux->thread_id = tid;
    aux->tdata = tdata;
    aux->next = *headref;

    *headref = aux;
    aux = NULL;
}

int free_completed(struct node_t **headref) {

    if (*headref == NULL) {
        return -1;
    }

    int cnt = 0;
    struct node_t dummy, *current, *next;

    dummy.next = *headref;
    current = &dummy;
    next = current->next;

    while (next != NULL) {

        if (next->tdata->completed) {

            // If we're on the first element, move head
            if (next == (*headref)) {
                *headref = (*headref)->next;
            }

            current->next = next->next;

            next->next = NULL;
            pthread_join(next->thread_id, NULL);
    
            free(next->tdata);
            free(next);
            next = current->next;

            cnt = cnt + 1;
            continue;
        }

        current = current->next;
        next = next->next;
    }

    return cnt;
}

void free_list(struct node_t *head) 
{
    struct node_t *current = head;
    struct node_t *next;
    
    while (current != NULL) {
        next = current->next;

        pthread_join(current->thread_id, NULL);

        free(current->tdata);
        free(current);
        current = next;
    }
}

int length(struct node_t *head) 
{
    int len = 0;
    while (head != NULL) {
        head = head->next;
        len += 1;
    }

    return len;
}

void * connection_handler(void * tdata) 
{
    struct thread_data *data = (struct thread_data *) tdata;

    char buf[BUFSIZ];
    bool eol;
    ssize_t  bytes_r, bytes_w, total_w, bytesleft;
    ssize_t total_r;

    data->completed = false;
    eol = false;
    total_r = 0;

    if (pthread_mutex_lock(data->mutex) == -1) {
        syslog(LOG_ERR, "Error obtaining mutex: %m");
        _exit(-1);
    }

    while (!eol && (bytes_r = recv(data->sock_fd, buf, sizeof buf, 0)) > 0) {
        total_r = total_r + bytes_r;
        syslog(LOG_DEBUG, "Received %ld (%ld) bytes", bytes_r, total_r);

        // Check if EOL received
        for (bytesleft = 0; bytesleft < bytes_r; bytesleft++) {
            if (buf[bytesleft] == EOL) {
                syslog(LOG_DEBUG, "EOL found at %ld of %ld", bytesleft+1, bytes_r);
                eol = true;
                bytesleft = bytesleft + 1;
                break;
            }
        }

        total_w = 0;
        while ((bytes_w = write(data->fd, buf + total_w, bytesleft)) > 0) {
            syslog(LOG_DEBUG, "Wrote %ld/%ld bytes to %s",
                    bytes_w, bytesleft, WRITEFILE);

            total_w += bytes_w;
            bytesleft -= bytes_w;
            if (bytesleft == 0)
                break;
        }

        if (bytes_w == -1) {
            syslog(LOG_ERR, "Error writing to file: %m");
            break;
        }
    }

    if (bytes_r == -1) {
        syslog(LOG_ERR, "Error recv: %m");
    }

    if (lseek(data->fd, 0, SEEK_SET) == -1) {
        syslog(LOG_ERR, "lseek: %m");
        pthread_exit(NULL);
    }

    while ((bytes_r = read(data->fd, buf, sizeof buf)) > 0) {

        // send the msg
        syslog(LOG_DEBUG, "Read %ld bytes from file", bytes_r);

        total_w = 0;
        bytesleft = bytes_r;
        while ((bytes_w = send(data->sock_fd, buf + total_w, bytesleft, 0)) > 0) {
            syslog(LOG_DEBUG, "Sent %ld/%ld bytes to client", bytes_w, bytesleft);

            total_w += bytes_w;
            bytesleft -= bytes_w;
            if (bytesleft == 0)
                break;
        }

        if (bytes_w == -1) {
            syslog(LOG_ERR, "Error sending to socket: %m");
            break;
        }

    }
    
    if (bytes_r == -1) {
        syslog(LOG_ERR, "Error when reading back file: %m");
    }

    if (pthread_mutex_unlock(data->mutex) == -1) {
        syslog(LOG_ERR, "Error unlocking mutex: %m");
    }

    if (close(data->sock_fd) == -1) {
        syslog(LOG_ERR, "Error closing connection: %m");
    }    
    syslog(LOG_DEBUG, "Closed connection");

    data->completed = true;

    return NULL;
}


bool b_exit = false;
int fd, sockfd, new_fd;
timer_t timer;

void g_exit(int status) {

    if (b_exit)
        syslog(LOG_INFO, "Caught signal, exiting");

    if (close(sockfd) == -1) 
        syslog(LOG_ERR, "Error closing listening socket: %m");

    if (close(fd) == -1) 
        syslog(LOG_ERR, "Error closing file %s: %m", WRITEFILE);

    if (remove(WRITEFILE) == -1) {
        syslog(LOG_ERR, "Error removing %s: %m", WRITEFILE);
        status = -1;
    }

    exit(status);
}

static void signal_handler(int signal) {
    int errno_saved = errno;

    if (signal == SIGINT || signal == SIGTERM) 
        b_exit = true;

    errno = errno_saved;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char** argv) {

    openlog(NULL, LOG_PID, LOG_USER | LOG_CONS);
    syslog(LOG_DEBUG, "Starting of %s success.", argv[0]);

    struct sigaction new_action;
    memset(&new_action, 0, sizeof (struct sigaction));
    new_action.sa_handler = signal_handler;

    if (sigaction(SIGTERM, &new_action, NULL) != 0) {
        syslog(LOG_ERR, "Error %d while registering SIGTERM: %m", errno);
        g_exit(-1);
    }

    if (sigaction(SIGINT, &new_action, NULL) != 0) {
        syslog(LOG_ERR, "Error %d while registering SIGINT: %m", errno);
        g_exit(-1);
    }


    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    char ipstr[INET6_ADDRSTRLEN];
    int yes = 1;
    int rc;


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // use my IP

    rc = getaddrinfo(NULL, PORT, &hints, &servinfo);
    if (rc != 0) {
        syslog(LOG_ERR, "getaddrinfo: %s", gai_strerror(rc));
        g_exit(-1);
    }


    // loop through the linked list and bind the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {

        inet_ntop(p->ai_family, get_in_addr(p->ai_addr), ipstr, sizeof ipstr);
        syslog(LOG_DEBUG, "Trying to socket and bind %s", ipstr);            

        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            syslog(LOG_ERR, "socket: %m");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (int)) == -1) {
            syslog(LOG_ERR, "setsockopt: %m");
            g_exit(-1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            syslog(LOG_ERR, "bind: %m");
            if (close(sockfd) == -1) 
                syslog(LOG_ERR, "close sockfd: %m");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL) {
        syslog(LOG_ERR, "server: failed to bind");
        g_exit(-1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        syslog(LOG_ERR, "listen: %m");
        g_exit(-1);
    }

    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        syslog(LOG_INFO, "Will switch to daemon mode");
        if (daemon(0, 0) == -1) {
            syslog(LOG_ERR, "Failed to switch to daemon mode: %m");
            g_exit(-1);
        }
    }


    fd = open(WRITEFILE, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd == -1) {
        syslog(LOG_ERR, "Error creating %s: %m", WRITEFILE);
        return -1;
    }

    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    // Timer init
    struct thread_data timer_tdata;
    memset(&timer_tdata, 0, sizeof timer_tdata);
    timer_tdata.fd = fd;
    timer_tdata.mutex = &mutex;

    timer_t timer = setup_timer_thread(&timer_tdata, 10);   

    struct node_t *head = NULL;
    struct thread_data *tdata = NULL;

    while (!b_exit) {
        syslog(LOG_INFO, "Waiting for connections...");
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size);

        if (new_fd == -1) {
            syslog(LOG_ERR, "accept: %m");
            continue;
        }

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *) &their_addr),
                ipstr, sizeof ipstr);
        syslog(LOG_DEBUG, "Accepted connection from %s", ipstr);

        tdata = (struct thread_data *) malloc(sizeof (struct thread_data));
        if (tdata == NULL) {
            syslog(LOG_ERR, "Error allocating memory for thread: %m");
            break;
        }
        memset(tdata, 0, sizeof (struct thread_data));

        tdata->fd = fd;
        tdata->sock_fd = new_fd;
        tdata->mutex = &mutex;

        insert(&head, tdata);
        free_completed(&head);
    }

    rc = timer_delete(timer);
    if (rc) 
        perror("timer_delete");

    free_list(head);
    head = NULL;
    g_exit(0);

    return -1;
}
