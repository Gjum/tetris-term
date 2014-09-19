APPNAME=tetris-term
CC=gcc
CFLAGS=--std=gnu99 -O3

.PHONY: run
run: ${APPNAME}
	@./${APPNAME}

${APPNAME}: main.c tetris.o
	${CC} -o $@ $^ ${CFLAGS}

tetris.o: tetris.c tetris.h
	${CC} -c $^ ${CFLAGS}

.PHONY: clean
clean:
	rm -f ${APPNAME} tetris.o

