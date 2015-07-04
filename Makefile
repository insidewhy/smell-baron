.PHONY: debug release

flags = -Wall

debug: smell-baron.debug
release: smell-baron

smell-baron.debug: main.c
	gcc ${flags} -g $^ -o $@

smell-baron: main.c
	gcc ${flags} -DNDEBUG -O3 $^ -o $@
