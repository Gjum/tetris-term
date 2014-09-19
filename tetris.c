/* tetris-term - Classic Tetris for your terminal.
 *
 * Copyright (C) 2014 Gjum
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "tetris.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// {{{ bricks
#define numBrickTypes 7
// positions of the filled cells
//  0  1  2  3
//  4  5  6  7
//  8  9 10 11
// 12 13 14 15
// [brickNr][rotation][cellNr]
const unsigned char bricks[numBrickTypes][4][4] = {
	{ { 1,  5,  9, 13}, { 8,  9, 10, 11}, { 1,  5,  9, 13}, { 8,  9, 10, 11}, }, // I
	{ { 5,  6,  9, 10}, { 5,  6,  9, 10}, { 5,  6,  9, 10}, { 5,  6,  9, 10}, }, // O
	{ { 9,  8,  5, 10}, { 9,  5, 10, 13}, { 9, 10, 13,  8}, { 9, 13,  8,  5}, }, // T
	{ { 9, 10, 12, 13}, { 5,  9, 10, 14}, { 9, 10, 12, 13}, { 5,  9, 10, 14}, }, // S
	{ { 8,  9, 13, 14}, { 5,  8,  9, 12}, { 8,  9, 13, 14}, { 5,  8,  9, 12}, }, // Z
	{ { 5,  9, 12, 13}, { 4,  8,  9, 10}, { 5,  6,  9, 13}, { 8,  9, 10, 14}, }, // J
	{ { 5,  9, 13, 14}, { 8,  9, 10, 12}, { 4,  5,  9, 13}, { 6,  8,  9, 10}, }, // L
};
// }}}

static void dieIfOutOfMemory(void *pointer) { // {{{
	if (pointer == NULL) {
		printf("Error: Out of memory\n");
		exit(1);
	}
} // }}}

static void nextBrick(TetrisGame *game) { // {{{
	game->brick = game->nextBrick;
	game->brick.x = game->width/2 - 2;
	game->brick.y = 0;
	game->nextBrick.type = rand() % numBrickTypes;
	game->nextBrick.rotation = rand() % 4;
	game->nextBrick.color = game->brick.color % 7 + 1; // (color-1 + 1) % 7 + 1, range is 1..7
	game->nextBrick.x = 0;
	game->nextBrick.y = 0;
} // }}}

TetrisGame *newTetrisGame(unsigned int width, unsigned int height) { // {{{
	TetrisGame *game = malloc(sizeof(TetrisGame));
	dieIfOutOfMemory(game);
	game->width = width;
	game->height = height;
	game->size = width * height;
	game->board = calloc(game->size, sizeof(char));
	dieIfOutOfMemory(game->board);
	game->isRunning = 1;
	game->isPaused  = 0;
	game->sleepUsec = 500000;
	game->score = 0;
	nextBrick(game); // fill preview
	nextBrick(game); // put into game
	// init terminal for non-blocking and no-echo getchar()
	struct termios term;
	tcgetattr(STDIN_FILENO, &game->termOrig);
	tcgetattr(STDIN_FILENO, &term);
	term.c_lflag &= ~(ICANON|ECHO);
	term.c_cc[VTIME] = 0;
	term.c_cc[VMIN] = 0;
	tcsetattr(STDIN_FILENO, TCSANOW, &term);
	// init signals for timer and errors
	struct sigaction signalAction;
	sigemptyset(&signalAction.sa_mask);
	signalAction.sa_handler = signalHandler;
	signalAction.sa_flags = 0;
	sigaction(SIGINT,  &signalAction, NULL);
	sigaction(SIGTERM, &signalAction, NULL);
	sigaction(SIGSEGV, &signalAction, NULL);
	sigaction(SIGALRM, &signalAction, NULL);
	// init timer
	game->timer.it_value.tv_usec = game->sleepUsec;
	setitimer(ITIMER_REAL, &game->timer, NULL);
	return game;
} // }}}

void destroyTetrisGame(TetrisGame *game) { // {{{
	if (game == NULL) return;
	tcsetattr(STDIN_FILENO, TCSANOW, &game->termOrig);
	printf("Your score: %i\n", game->score);
	printf("Game over.\n");
	free(game->board);
	free(game);
} // }}}

unsigned char colorOfBrickAt(FallingBrick *brick, int x, int y) { // {{{
	if (brick->type < 0) return 0;
	int v = y - brick->y;
	if (v < 0 || v >= 4) return 0;
	int u = x - brick->x;
	if (u < 0 || u >= 4) return 0;
	for (int i = 0; i < 4; i++) {
		if (u + 4*v == bricks[brick->type][brick->rotation][i])
			return brick->color;
	}
	return 0;
} // }}}

static char brickCollides(TetrisGame *game) { // {{{
	for (int i = 0; i < 4; i++) {
		int p = bricks[game->brick.type][game->brick.rotation][i];
		int x = p % 4 + game->brick.x;
		if (x < 0 || x >= game->width) return 1;
		int y = p / 4 + game->brick.y;
		if (y >= game->height) return 1;
		p = x + y * game->width;
		if (p >= 0 && p < game->size && game->board[p] != 0)
			return 1;
	}
	return 0;
} // }}}

static void landBrick(TetrisGame *game) { // {{{
	if (game->brick.type < 0) return;
	for (int i = 0; i < 4; i++) {
		int p = bricks[game->brick.type][game->brick.rotation][i];
		int x = p % 4 + game->brick.x;
		int y = p / 4 + game->brick.y;
		p = x + y * game->width;
		game->board[p] = game->brick.color;
	}
} // }}}

static void clearFullRows(TetrisGame *game) { // {{{
	int width = game->width;
	int rowsCleared = 0;
	for (int y = game->brick.y; y < game->brick.y + 4; y++) {
		char clearRow = 1;
		for (int x = 0; x < width; x++) {
			if (0 == game->board[x + y * width]) {
				clearRow = 0;
				break;
			}
		}
		if (clearRow) {
			for (int d = y; d > 0; d--)
				memcpy(game->board + width*d, game->board + width*(d-1), width);
			bzero(game->board, width); // delete first line
			y--;
			rowsCleared++;
		}
	}
	if (rowsCleared > 0) {
		// apply score: 0, 1, 3, 5, 8
		game->score += rowsCleared * 2 - 1;
		if (rowsCleared >= 4) game->score++;
	}
} // }}}

void tick(TetrisGame *game) { // {{{
	if (game->isPaused) return;
	game->brick.y++;
	if (brickCollides(game)) {
		game->brick.y--;
		landBrick(game);
		clearFullRows(game);
		nextBrick(game);
		if (brickCollides(game))
			game->isRunning = 0;
	}
	printBoard(game);
} // }}}

static void pauseUnpause(TetrisGame *game) { // {{{
	if (game->isPaused) {
		// TODO de-/reactivate timer
		tick(game);
	}
	game->isPaused ^= 1;
} // }}}

static void moveBrick(TetrisGame *game, char x, char y) { // {{{
	if (game->isPaused) return;
	game->brick.x += x;
	game->brick.y += y;
	if (brickCollides(game)) {
		game->brick.x -= x;
		game->brick.y -= y;
	}
	printBoard(game);
} // }}}

static void rotateBrick(TetrisGame *game, char direction) { // {{{
	if (game->isPaused) return;
	unsigned char oldRotation = game->brick.rotation;
	game->brick.rotation += 4 + direction; // 4: keep it positive
	game->brick.rotation %= 4;
	if (brickCollides(game))
		game->brick.rotation = oldRotation;
	printBoard(game);
} // }}}

void processInputs(TetrisGame *game) { // {{{
	char c = getchar();
	do {
		switch (c) {
			case ' ': moveBrick(game, 0, 1); break;
			//case '?': dropBrick(game); break;
			case 'p': pauseUnpause(game); break;
			case 'q': game->isRunning = 0; break;
			case 27: // ESC
				getchar();
				switch (getchar()) {
					case 'A': rotateBrick(game,  1);  break; // up
					case 'B': rotateBrick(game, -1);  break; // down
					case 'C': moveBrick(game,  1, 0); break; // right
					case 'D': moveBrick(game, -1, 0); break; // left
				}
				break;
		}
	} while ((c = getchar()) != -1);
} // }}}

