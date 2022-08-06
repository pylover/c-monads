#include <meloop/arrow.h>
#include <meloop/io.h>
#include <meloop/random.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <linux/random.h>


void 
randopenA(struct circuitS *c, struct randS *s, struct stringS d) {
    int fd = open("/dev/urandom", O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        ERROR_A(c, s, d, "open urandom");
        return;
    }
    s->randfd = fd;
    RETURN_A(c, s, d);
}


void 
randreadA(struct circuitS *c, struct randS *s, struct stringS d) {
    size_t size;

    /* Read from the file descriptor */
    size = read(s->randfd, d.data, s->readsize);

    /* Check for error */
    if (size < 0) {
        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
            WAIT_A(c, s, d, s->randfd, EPOLLIN);
        }
        else {
            ERROR_A(c, s, d, "read urandom");
        }
        return;
    }
    d.size = size;
    RETURN_A(c, s, d);
}
