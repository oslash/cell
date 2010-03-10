CC = gcc
COMPILERFLAGS = -Wall -std=c99

COMPILERFLAGS += `sdl-config --cflags`
LIBPATH = `sdl-config --libs`

All: main

%: %.c
	$(CC) $(COMPILERFLAGS) $(LIBPATH) $(FRAMEWORK) $^ -o $@

main: main.c

run: main
	./main

clean:
	rm main
