CC=gcc
CFLAGS=-Wall -Wextra -std=gnu11
SRCS=$(wildcard *.c)

mishell: $(SRCS) $(wildcard *.h)
	$(CC) $(CFLAGS) -o mishell $(SRCS)

clean:
	rm -f mishell

.PHONY: clean