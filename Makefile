run: tetris
	@./tetris

tetris: tetris.c
	@gcc --std=gnu99 -o $@ $^ -g

.PHONY: clean
clean:
	@rm -f tetris

