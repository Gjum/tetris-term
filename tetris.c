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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
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
	{ // I
		{ 1,  5,  9, 13},
		{ 8,  9, 10, 11},
		{ 1,  5,  9, 13},
		{ 8,  9, 10, 11},
	},
	{ // O
		{ 5,  6,  9, 10},
		{ 5,  6,  9, 10},
		{ 5,  6,  9, 10},
		{ 5,  6,  9, 10},
	},
	{ // T
		{ 9,  8,  5, 10},
		{ 9,  5, 10, 13},
		{ 9, 10, 13,  8},
		{ 9, 13,  8,  5},
	},
	{ // S
		{ 9, 10, 12, 13},
		{ 5,  9, 10, 14},
		{ 9, 10, 12, 13},
		{ 5,  9, 10, 14},
	},
	{ // Z
		{ 8,  9, 13, 14},
		{ 5,  8,  9, 12},
		{ 8,  9, 13, 14},
		{ 5,  8,  9, 12},
	},
	{ // J
		{ 5,  9, 12, 13},
		{ 4,  8,  9, 10},
		{ 5,  6,  9, 13},
		{ 8,  9, 10, 14},
	},
	{ // L
		{ 5,  9, 13, 14},
		{ 8,  9, 10, 12},
		{ 4,  5,  9, 13},
		{ 6,  8,  9, 10},
	},
};
// }}}

typedef struct { // FallingBrick {{{
	unsigned char type, rotation, color;
	int x, y;
} FallingBrick; // }}}

typedef struct { // TetrisGame {{{
	unsigned int width, height, size; // of the board
	unsigned char *board; // indices of pattern
	FallingBrick brick, nextBrick;
	unsigned char isRunning, isPaused;
	clock_t sleepClocks, sleepsBeforeFast, nextTick;
	unsigned long score;
} TetrisGame; // }}}

void dieIfOutOfMemory(void *pointer) { // {{{
	if (pointer == NULL) {
		printf("Error: Out of memory\n");
		exit(1);
	}
} // }}}

void nextBrick(TetrisGame *game) { // {{{
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
	game->sleepClocks = CLOCKS_PER_SEC / 2 / 150; // TODO fit with usleep() in main loop
	game->sleepsBeforeFast = game->sleepClocks;
	game->nextTick = clock() + game->sleepClocks;
	nextBrick(game); // fill preview
	nextBrick(game); // put into game
	return game;
} // }}}

void destroyTetrisGame(TetrisGame *game) { // {{{
	if (game == NULL) return;
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

void printBoard(TetrisGame *game) { // {{{
	int width = game->width;
	char line[width * 2 + 1];
	memset(line, '-', width * 2);
	line[width * 2] = 0;
	printf("\e[%iA", game->height + 2); // move to above the board
	printf("/%s+--------\\\n", line);
	int foo = 0;
	for (int y = 0; y < game->height; y++) {
		printf("|");
		for (int x = 0; x < game->width; x++) {
			char c = game->board[x + y * game->width];
			if (c == 0) // empty? try falling brick
				c = colorOfBrickAt(&game->brick, x, y);
			printf("\e[3%i;4%im  ", c, c);
		}
		if (y == 4) printf("\e[39;49m|  \e[1mScore\e[0m |\n");
		else if (y == 5) printf("\e[39;49m| %6i |\n", game->score);
		else if (y == 6) printf("\e[39;49m+--------/\n");
		else {
			if (y < 4) {
				printf("\e[39;49m|");
				for (int x = 0; x < 4; x++) {
					char c = colorOfBrickAt(&game->nextBrick, x, y);
					printf("\e[3%i;4%im  ", c, c);
				}
				foo++;
			}
			printf("\e[39;49m|\n");
		}
	}
	printf("\\%s/\n", line);
} // }}}

char brickCollides(TetrisGame *game) { // {{{
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

void landBrick(TetrisGame *game) { // {{{
	if (game->brick.type < 0) return;
	for (int i = 0; i < 4; i++) {
		int p = bricks[game->brick.type][game->brick.rotation][i];
		int x = p % 4 + game->brick.x;
		int y = p / 4 + game->brick.y;
		p = x + y * game->width;
		game->board[p] = game->brick.color;
	}
	game->sleepClocks = game->sleepsBeforeFast;
	nextBrick(game);
} // }}}

void clearFullRows(TetrisGame *game) { // {{{
	// TODO optimize, only look at landed brick
	int width = game->width;
	int rowsCleared = 0;
	for (int y = 0; y < game->height; y++) {
		char clearRow = 1;
		for (int x = 0; x < width; x++) {
			if (0 == game->board[x + y * width]) {
				clearRow = 0;
				break;
			}
		}
		if (clearRow) {
			for (int d = y; d > 0; d--) {
				memcpy(game->board + width*d, game->board + width*(d-1), width);
			}
			bzero(game->board, width);
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
	game->nextTick += game->sleepClocks;
	game->brick.y++;
	if (brickCollides(game)) {
		game->brick.y--;
		landBrick(game);
		clearFullRows(game);
		if (brickCollides(game))
			game->isRunning = 0;
	}
	printBoard(game);
} // }}}

void pauseUnpause(TetrisGame *game) { // {{{
	if (game->isPaused)
		game->nextTick = clock();
	game->isPaused ^= 1;
} // }}}

void moveBrick(TetrisGame *game, char x, char y) { // {{{
	game->brick.x += x;
	game->brick.y += y;
	if (brickCollides(game)) {
		game->brick.x -= x;
		game->brick.y -= y;
	}
	printBoard(game);
} // }}}

void rotateBrick(TetrisGame *game, char direction) { // {{{
	unsigned char oldRotation = game->brick.rotation;
	game->brick.rotation += 4 + direction; // 4: keep it positive
	game->brick.rotation %= 4;
	if (brickCollides(game))
		game->brick.rotation = oldRotation;
	printBoard(game);
} // }}}

void dropBrick(TetrisGame *game) { // {{{
	if (game->sleepsBeforeFast != game->sleepClocks) return; // only speed up once
	game->sleepsBeforeFast = game->sleepClocks;
	game->sleepClocks /= 5;
	game->nextTick = clock() + game->sleepClocks;
} // }}}

void processInputs(TetrisGame *game) { // {{{
	switch (getchar()) {
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
} // }}}

void welcome() { // {{{
	printf("tetris-term  Copyright (C) 2014  Gjum\n");
	printf("\n");
	printf("This program comes with ABSOLUTELY NO WARRANTY.\n");
	printf("This is free software, and you are welcome to redistribute it\n");
	printf("under certain conditions; see `LICENSE' for details.\n");
	printf("\n");
	// Tetris logo
	printf("\e[30;40m  \e[31;41m  \e[30;40m  \e[34;44m  \e[34;44m  \e[34;44m  \e[33;43m  \e[30;40m  \e[30;40m  \e[30;40m  \e[37;47m  \e[35;45m  \e[35;45m  \e[35;45m  \e[39;49m\n");
	printf("\e[31;41m  \e[31;41m  \e[31;41m  \e[34;44m  \e[30;40m  \e[35;45m  \e[33;43m  \e[33;43m  \e[33;43m  \e[30;40m  \e[37;47m  \e[35;45m  \e[30;40m  \e[30;40m  \e[39;49m\n");
	printf("\e[30;40m  \e[36;46m  \e[30;40m  \e[35;45m  \e[35;45m  \e[35;45m  \e[32;42m  \e[30;40m  \e[31;41m  \e[31;41m  \e[37;47m  \e[34;44m  \e[34;44m  \e[34;44m  \e[39;49m\n");
	printf("\e[30;40m  \e[36;46m  \e[30;40m  \e[34;44m  \e[30;40m  \e[30;40m  \e[32;42m  \e[30;40m  \e[31;41m  \e[30;40m  \e[37;47m  \e[30;40m  \e[30;40m  \e[34;44m  \e[39;49m\n");
	printf("\e[30;40m  \e[36;46m  \e[36;46m  \e[34;44m  \e[34;44m  \e[34;44m  \e[32;42m  \e[32;42m  \e[31;41m  \e[30;40m  \e[35;45m  \e[35;45m  \e[35;45m  \e[35;45m  \e[39;49m\n");
	printf("\n");
	printf("\e[1mControls:\e[0m\n");
	printf("<Left>  move brick left\n");
	printf("<Right> move brick right\n");
	printf("<Up>    rotate brick clockwise\n");
	printf("<Down>  rotate brick counter-clockwise\n");
	//printf("<?????> drop brick down\n");
	printf("<Space> move brick down by one step\n");
	printf("<p>     pause game\n");
	printf("<q>     quit game\n");
	printf("\n");
} // }}}

int main(int argc, char **argv) { // {{{
	srand(getpid());
	welcome();
	TetrisGame *game = newTetrisGame(10, 20);
	// Init terminal for non-blocking & noecho getchar()
	struct termios term_orig, term;
	tcgetattr(STDIN_FILENO, &term_orig);
	tcgetattr(STDIN_FILENO, &term);
	term.c_lflag &= ~(ICANON|ECHO);
	term.c_cc[VTIME] = 0;
	term.c_cc[VMIN] = 0;
	tcsetattr(STDIN_FILENO, TCSANOW, &term);
	// create space for the board
	for (int i = 0; i < game->height + 2; i++) printf("\n");
	printBoard(game);
	while (game->isRunning) {
		if (game->nextTick < clock() && !game->isPaused)
			tick(game);
		usleep(10000);
		processInputs(game);
	}
	tcsetattr(STDIN_FILENO, TCSANOW, &term_orig);
	destroyTetrisGame(game);
	return 0;
} // }}}

