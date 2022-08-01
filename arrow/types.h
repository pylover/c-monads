#ifndef ARROW_TYPES_H
#define ARROW_TYPES_H


#include <string.h>


struct pair {
    int left;
    int right;
};


struct string {
    size_t size;
    char *data;
};


union any {
    int sint;
    void *ptr;
    struct pair pair;
    struct string string;
};


#endif
