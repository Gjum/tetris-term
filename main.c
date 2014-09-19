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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "tetris.h"

TetrisGame *game;

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

void signalHandler(int signal) { // {{{
	switch(signal) {
		case SIGINT:
		case SIGTERM:
		case SIGSEGV:
			game->isRunning = 0;
			break;
		case SIGALRM:
			tick(game);
			game->timer.it_value.tv_usec = game->sleepUsec;
			setitimer(ITIMER_REAL, &game->timer, NULL);
			break;
	}
	return;
} // }}}

int main(int argc, char **argv) { // {{{
	srand(time(0));
	welcome();
	game = newTetrisGame(10, 20);
	// create space for the board
	for (int i = 0; i < game->height + 2; i++) printf("\n");
	printBoard(game);
	while (game->isRunning) {
		usleep(50000);
		processInputs(game);
	}
	destroyTetrisGame(game);
	return 0;
} // }}}

