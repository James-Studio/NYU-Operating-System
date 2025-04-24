# Refer to the Makefile in Lab1

CC=gcc
CFLAGS=-fsigned-char -g -pedantic -std=gnu17 -Wall -Werror -Wextra

.PHONY: all

# compile nyufile
all: nyufile

nyufile: nyufile.o functions.o

nyufile.o: nyufile.c functions.h

functions.o: functions.c functions.h


.PHONY: clean
clean:
	rm -f *.o nyufile