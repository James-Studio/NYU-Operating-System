# Refer to the Makefile in Lab1

CC=gcc
CFLAGS=-fsigned-char -g -pedantic -std=gnu17 -Wall -Werror -Wextra

.PHONY: all

# compile nyush
all: nyush

nyush: nyush.o functions.o

nyush.o: nyush.c functions.h

functions.o: functions.c functions.h


.PHONY: clean
clean:
	rm -f *.o nyush