#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
// File includes
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv) {

    openlog(NULL, LOG_PID, LOG_USER);

    if (argc != 3) {
        syslog(LOG_ERR, "Invalid number of arguments. Expected 2, received: %d", argc - 1);
        return 1;
    }

    const char *filename = argv[1];
    const char *writestr = argv[2];

    //syslog(LOG_DEBUG, "writestr is: %s", writestr);

    int fd;
    fd = creat(filename, 0644);
    if (fd == -1) {
        syslog(LOG_ERR, "Failed to open file for writing: %s. Error: %m.", filename);
        return 1;
    }

    syslog(LOG_DEBUG, "Writing %s to %s", writestr, filename);
    
    ssize_t nr;
    size_t count = strlen(writestr);

    nr = write(fd, writestr, count);
    if (nr == -1) {
        syslog(LOG_ERR, "Could not write to file: %m.");
        return 1;
    } else if (nr != count) {
        syslog(LOG_ERR, "Incomplete file: %zd of %zd bytes written.", nr, count);
    }

    if (close(fd) == 1) {
        syslog(LOG_ERR, "Error closing file  \"%s\": %m.", filename);
        return 1;
    }

    return 0;
}

