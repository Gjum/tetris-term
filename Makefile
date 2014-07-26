APPNAME=tetris-term
CC=gcc
CFLAGS= --std=gnu99 -O3

run: ${APPNAME}
	@./${APPNAME}

${APPNAME}: tetris.c
	@${CC} -o $@ $^ ${CFLAGS}

.PHONY: clean
clean:
	@rm -f ${APPNAME}

