roboarm: roboarm.c
	cc -ggdb -rdynamic -o roboarm roboarm.c -lm -lSDL2 -lSDL2_image

clean:
	rm roboarm

.PHONY: clean
