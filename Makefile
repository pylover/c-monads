CC = gcc
CFLAGS = -I.

# Uncomment for production
CFLAGS += -g


# Internal variables
objects = $(patsubst %.c,%.o,$(wildcard *.c))


.PHONY: all
all: $(objects)


# Implicit rule for other modules
%.o: %.c %.h
	$(CC) -c $(CFLAGS) $< -o $@ 


.PHONY: clean
clean:: 
	- rm *.o
