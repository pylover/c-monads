#include "meloop/io.h"
#include "meloop/tcp.h"
#include "meloop/addr.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <err.h>
#include <sys/epoll.h>


#define CHUNK_SIZE  1024
#define WORKING 9999
static volatile int status = WORKING;
static struct sigaction old_action;
static struct circuitS *worker;


void sighandler(int s) {
    printf("\nSIGINT detected: %d\n", s);
    status = EXIT_SUCCESS;
}


void catch_signal() {
    struct sigaction new_action = {sighandler, 0, 0, 0, 0};
    if (sigaction(SIGINT, &new_action, &old_action) != 0) {
        err(EXIT_FAILURE, NULL);
    }
}


void
errorcb(struct circuitS *c, struct tcpserverS *s, union any data, 
        const char *error) {
    perror(error);
    status = EXIT_FAILURE;
}


void
successcb(struct circuitS *c, struct tcpserverS *s, int out) {
    printf("Out: %d\n", out);
}


void
client_error(struct circuitS *c, struct connS *conn, struct stringS buff, 
        const char *error) {
    printf("Clinet disconnected: %s -- %s\n", meloop_addr_dump(&(conn->addr)), error);
   
    if (buff.data != NULL) {
        free(buff.data);
    }
    if (conn != NULL) {
        free(conn);
    }
}


void 
client_connected (struct circuitS *c, struct tcpserverS *s, int fd, 
        struct sockaddr *addr) {
    
    printf("Client connected: %s\n", meloop_addr_dump(addr));

    /* Will be free at tcp: client_free() */
    struct connS *conn = malloc(sizeof(struct connS));
    if (conn == NULL) {
        close(fd);
        ERROR_A(c, s, NULL, "Out of memory");
        return;
    }

    conn->rfd = fd; 
    conn->wfd = fd; 
    conn->epollflags = EPOLLET;
    conn->readsize = s->readsize; 
    memcpy(&(conn->addr), addr, sizeof(addr));

    /* Will be free at tcp.c: client_free() */
    struct stringS buff = {
        .size = 0,
        .data = malloc(s->readsize),
    };

    if (buff.data == NULL) {
        ERROR_A(c, s, NULL, "Out of memory");
        return;
    }
    conn->server = s;
    RUN_A(worker, conn, buff);
}


int main() {
    catch_signal();
    meloop_io_init(0);

    /* A circuitS to run for each new connection */
    worker = NEW_C(NULL, client_error);
    struct elementS *e = APPEND_A(worker, readA,  NULL);
                         APPEND_A(worker, writeA, NULL);
               loopA(e);

    /* Initialize TCP Server */
    static struct tcpserverS server = {
        .epollflags = EPOLLET,
        .readsize = 1024,
        .backlog = 2,
        .client_connected = client_connected,
    };

    /* Parse listen address */
    meloop_addr_parse(&(server.bind), "127.0.0.1", 9090);
    
    /* Server init -> loop circuitS */
    struct circuitS *circ = NEW_C(successcb, errorcb);

                            APPEND_A(circ, listenA, NULL);
    struct elementS *acpt = APPEND_A(circ, acceptA, NULL);
               loopA(acpt);

    /* Run server circuitS */
    RUN_A(circ, &server, NULL); 

    /* Start and wait for event loop */
    if (meloop_io_loop(&status)) {
        perror("meloop_io_loop");
        status = EXIT_SUCCESS;
    }

    meloop_io_deinit();
    freeC(worker);
    freeC(circ);
    return status;
}
