#include "monad.h"
#include "monad_io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>


#define CHUNK_SIZE  1024

#define WORKING 9999
#define ERROR -1
#define OK 0
static volatile int status = WORKING;


void prompt(MonadContext *ctx, struct device *dev, struct conn *c) {
    ssize_t size = write(c->wfd, ">>> ", 4);
    if (size < 4) {
        monad_failed(ctx, "write");
        return;
    }
    monad_succeeded(ctx, c);
}


void readit(MonadContext *ctx, struct device *dev, struct conn *c) {
    ssize_t size = read(c->rfd, c->data, CHUNK_SIZE);

    /* Check for EOF */
    if (size == 0) {
        /* CTRL+D */
        monad_failed(ctx, "EOF");
        return;
    }
    
    /* Check for error */
    if (size < 0) {
        monad_failed(ctx, "read");
        return;
    }
    c->size = size;
    monad_succeeded(ctx, c);
}


void writeit(MonadContext *ctx, struct device *dev, struct conn *c) {
    /* Empty line */
    if (c->size == 1) {
        monad_succeeded(ctx, c);
        return;
    }

    ssize_t size = write(c->wfd, "... ", 4);
    if (size < 0) {
        monad_failed(ctx, "write");
        return;
    }

    size = write(c->wfd, c->data, c->size);
    if (size < 0) {
        monad_failed(ctx, "write");
        return;
    }
    monad_succeeded(ctx, c);
}


void cleanit(MonadContext *ctx, void *, struct conn *c) {
    c->size = 0;
    monad_succeeded(ctx, c);
}


void caseit(MonadContext *ctx, void *, struct conn *c) {
    char *s = c->data;
    size_t l = c->size;
    while (l) {
        *s = toupper((unsigned char) *s);
        s++;
        l--; 
    } 
    monad_succeeded(ctx, c);
}


void failed(MonadContext *ctx, const char *reason) {
    /* CTRL+D */
    if (strstr(reason, "EOF")) {
        printf("%s\n", reason);
        status = OK;
        return;
    }
    else {
        perror(reason);
        status = ERROR;
    }
    
    printf("\n");
    status = OK;
}


int main() {
    mio_init(0);

    struct conn c = {STDIN_FILENO, STDOUT_FILENO, 0, malloc(CHUNK_SIZE)};
    struct device dev = {false, CHUNK_SIZE};

    /* Draw circut */
    Monad *m = MONAD_RETURN(   mio_waitw, &dev );
               MONAD_APPEND(m, prompt,    &dev );
               MONAD_APPEND(m, mio_waitr, &dev );
               MONAD_APPEND(m, readit,    &dev );
               MONAD_APPEND(m, caseit,    NULL );
               MONAD_APPEND(m, mio_waitw, &dev );
               MONAD_APPEND(m, writeit,   &dev );
               MONAD_APPEND(m, cleanit,   NULL );

    /* Loop/Close it */
    monad_loop(m);

    if (mio_run(m, &c, NULL, failed)) {
        err(1, "mio_run");
    }
   
    if (c.data != NULL) {
        free(c.data);
    }
    monad_free(m);
    mio_deinit();
    return status;
}
