CC = gcc
CFLAGS = -std=c99 -Wall -pedantic -Wno-deprecated-declarations
LDFLAGS = -lm -lgsl

wi: wi.o
	${CC} -o $@ ${LDFLAGS} $^

wi.o: wi.c
	${CC} -o $@ -c ${CFLAGS} $^
