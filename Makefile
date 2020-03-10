roboarm: roboarm.c
	cc -ggdb -rdynamic -o roboarm roboarm.c -lSDL2 -lSDL2_image

clean:
	rm roboarm

.PHONY: clean
