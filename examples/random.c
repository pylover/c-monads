#include "meloop/arrow.h"
#include "meloop/io.h"
#include "meloop/random.h"

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>


#define BUFFSIZE 32


void 
promptA(struct circuitS *c, struct ioS *io, struct stringS buff) {
    struct stringS s = meloop_vars_string_from_ptr(c);
    memcpy(buff.data, s.data, s.size);
    buff.size = s.size;
    writeA(c, io, buff);
}


void
encodeA(struct circuitS *c, struct ioS *io, struct stringS buff) {
    int i;
    unsigned int t;
    for (i = 0; i < buff.size; i++) {
        t = buff.data[i];
        buff.data[i] = (t % 26) + 97;
    }
    RETURN_A(c, io, buff);
}


void
errorcb(struct circuitS *c, struct ioS *io, struct stringS d, const char *e) {
    perror(e);
}


void
successcb(struct circuitS *c, struct ioS *io, int out) {
    printf("Out: %d\n", out);
}


int main() {
    meloop_io_init(0);

    char buff[BUFFSIZE] = "\0";
    struct randS state = {
        .wfd = STDOUT_FILENO,
        .rfd = STDIN_FILENO,
        .readsize = BUFFSIZE,
        .randfd = -1,
    };

    struct circuitS *c = NEW_C(successcb, errorcb);

                         APPEND_A(c, randopenA, NULL);
    struct elementS *e = APPEND_A(c, readA,     NULL);
                         APPEND_A(c, randreadA, NULL);
                         APPEND_A(c, encodeA,   NULL);
                         APPEND_A(c, writeA,    NULL);
              loopA(e);

    /* Run circuitS */
    RUN_A(c, &state, meloop_atos(buff)); 

    /* Start and wait for event loop */
    if (meloop_io_loop(NULL)) {
        err(1, "meloop_io_loop");
    }
    
    meloop_io_deinit();
    freeC(c);
    return EXIT_SUCCESS;
}
