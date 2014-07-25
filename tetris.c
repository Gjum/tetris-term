#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>

// {{{ bricks
// positions of the filled cells
//  0  1  2  3
//  4  5  6  7
//  8  9 10 11
// 12 13 14 15
// [brickNr][rotation][cellNr]
const unsigned char bricks[7][4][4] = {
	{ // I
		{ 1,  5,  9, 13},
		{ 4,  5,  6,  7},
		{ 1,  5,  9, 13},
		{ 4,  5,  6,  7},
	},
	{ // O
		{ 5,  6,  9, 10},
		{ 5,  6,  9, 10},
		{ 5,  6,  9, 10},
		{ 5,  6,  9, 10},
	},
	{ // J TODO
		{ 0,  3, 12, 15},
		{ 0,  3, 12, 15},
		{ 0,  3, 12, 15},
		{ 0,  3, 12, 15},
	},
	{ // L TODO
		{ 0,  3, 12, 15},
		{ 0,  3, 12, 15},
		{ 0,  3, 12, 15},
		{ 0,  3, 12, 15},
	},
	{ // T
		{ 5,  4,  1,  6},
		{ 5,  1,  6,  9},
		{ 5,  6,  9,  4},
		{ 5,  9,  4,  1},
	},
	{ // S
		{ 1,  5,  6, 10},
		{ 2,  3,  5,  6},
		{ 1,  5,  6, 10},
		{ 2,  3,  5,  6},
	},
	{ // Z
		{ 2,  5,  6,  9},
		{ 0,  1,  5,  6},
		{ 2,  5,  6,  9},
		{ 0,  1,  5,  6},
	},
};
// }}}

char *pattern = " #X:|EEEEEEEEEEEE";

typedef struct { // {{{
	unsigned int width, height, size; // of the board
	unsigned char *board; // indices of pattern
	unsigned char falling, next, fallingRot; // brick
	unsigned int fallingX, fallingY; // brick position
	char isRunning;
	clock_t sleepClocks, nextTick;
} TetrisGame; // }}}

TetrisGame *newTetrisGame(unsigned int boardWidth, unsigned int boardHeight) { // {{{
	TetrisGame *game = malloc(sizeof(TetrisGame));
	game->width = boardWidth;
	game->height = boardHeight;
	game->size = boardWidth * boardHeight;
	game->board = calloc(game->size, sizeof(char));
	game->falling = 1; // TODO
	game->next = -1;
	game->fallingRot = 0; // TODO
	game->fallingX = 2; // TODO
	game->fallingY = 2; // TODO
	game->isRunning = 1;
	game->sleepClocks = CLOCKS_PER_SEC / 2;
	game->nextTick = clock() + game->sleepClocks;
	return game;
} // }}}

void destroyTetrisGame(TetrisGame *game) { // {{{
	if (game == NULL) return;
	free(game->board);
	free(game);
} // }}}

// TODO rename
unsigned char getFloatingTodo(TetrisGame *game, unsigned int x, unsigned int y) { // {{{
	if (game->falling < 0) return 0;
	int v = y - game->fallingY;
	if (v < 0 || v >= 4) return 0;
	int u = x - game->fallingX;
	if (u < 0 || u >= 4) return 0;
	for (int i = 0; i < 4; i++) {
		if (u + 4*v == bricks[game->falling][game->fallingRot][i])
			return 1;
	}
	return 0;
} // }}}

void printBoard(TetrisGame *game) { // {{{
	printf("\e7\e[H"); // save position, move to 0,0
	printf("/");
	for (int x = 0; x < game->width; x++)
		printf("--");
	printf("\\\n");
	for (int y = 0; y < game->size; y += game->width) {
		printf("|");
		for (int i = y; i < y+game->width; i++) {
			char c = game->board[i];
			if (c == 0) // empty? try falling brick
				c = getFloatingTodo(game, i%game->width, i/game->width);
			c = pattern[c];
			printf("%c%c", c, c);
		}
		printf("|\n");
	}
	printf("\\");
	for (int x = 0; x < game->width; x++)
		printf("--");
	printf("/\n");
	printf("\e8"); // move back to original position
} // }}}

// TODO next brick: nrand

void tick(TetrisGame *game) { // {{{
	game->nextTick += game->sleepClocks;
	// TODO move falling brick
	printBoard(game);
} // }}}

void keyPressed(TetrisGame *game, char c) { // {{{
	switch (c) {
		case 'q': game->isRunning = 0; break;
	}
} // }}}

void init() { // {{{
	srand(getpid()); // TODO
	// Init terminal for non-blocking & noecho getchar()
	struct termios term;
	tcgetattr(STDIN_FILENO, &term);
	term.c_lflag &= ~(ICANON|ECHO);
	term.c_cc[VTIME] = 0;
	term.c_cc[VMIN] = 0;
	tcsetattr(STDIN_FILENO, TCSANOW, &term);
} // }}}

int main(int argc, char **argv) { // {{{
	init();
	TetrisGame *game = newTetrisGame(10, 20);
	while (game->isRunning) {
		tick(game);
		while (game->nextTick > clock() && game->isRunning)
			keyPressed(game, getchar()); // non-blocking
	}
	printf("Game over\n");
	destroyTetrisGame(game);
	return 0;
} // }}}

