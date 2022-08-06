#include "meloop/arrow.h"
#include "meloop/io.h"

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>


#define BUFFSIZE 1024


void 
promptA(struct circuit *c, struct io *io, struct string buff) {
    struct string s = meloop_vars_string_from_ptr(c);
    memcpy(buff.data, s.data, s.size);
    buff.size = s.size;
    writeA(c, io, buff);
}


void 
echoA(struct circuit *c, struct io *io, struct string buff) {
    if ((buff.size == 1) && (buff.data[0] == '\n')) {
        buff.size = 0;
    }
    RETURN_A(c, io, buff);
}


void
errorcb(struct circuit *c, struct io *io, union any, const char *error) {
    perror(error);
}


void
successcb(struct circuit *c, struct io *io, int out) {
    printf("Out: %d\n", out);
}


int main() {
    meloop_io_init(0);

    char buff[BUFFSIZE] = "\0";
    struct io state = {
        .wfd = STDOUT_FILENO,
        .rfd = STDIN_FILENO,
        .readsize = BUFFSIZE,
    };

    struct circuit *c = NEW_C(successcb, errorcb);

    struct element *e = APPEND_A(c, promptA, meloop_atos("me@loop:~$ "));
                        APPEND_A(c, readA,   NULL);
                        APPEND_A(c, echoA,   NULL);
                        APPEND_A(c, writeA,  NULL);
    loopA(e);

    /* Run circuit */
    RUN_A(c, &state, meloop_atos(buff)); 

    /* Start and wait for event loop */
    if (meloop_io_loop(NULL)) {
        err(1, "meloop_io_loop");
    }
    
    meloop_io_deinit();
    freeC(c);
    return EXIT_SUCCESS;
}
