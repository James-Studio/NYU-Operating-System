# Refer to the Makefile in Lab1

CC=gcc
CFLAGS=-fsigned-char -g -pedantic -std=gnu17 -Wall -Werror -Wextra
LDFLAGS=-pthread

.PHONY: all

# compile nyuenc
all: nyuenc

nyuenc: nyuenc.o functions.o

nyuenc.o: nyuenc.c functions.h

functions.o: functions.c functions.h


.PHONY: clean
clean:
	rm -f *.o nyuenc