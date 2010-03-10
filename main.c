#include <SDL.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

const int CFG_WIDTH = 512;
const int CFG_HEIGHT = 512;

SDL_Surface* screen;

static bool init_sdl() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
        return false;
    }
    screen = SDL_SetVideoMode(CFG_WIDTH, CFG_HEIGHT, 32, SDL_HWSURFACE);
    if (!screen) {
        fprintf(stderr, "Unable set SDL video mode: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }
    SDL_WM_SetCaption("cell", NULL);
    return true;
}

static void teardown_sdl() {
    SDL_Quit();
}

static bool process_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) return false;
        if (event.type == SDL_KEYDOWN) {
            switch(event.key.keysym.sym) {
              case SDLK_ESCAPE:
                return false;
              default:
                break;
            }
        }
    }
    return true;
}

static void display(int cell[CFG_WIDTH][CFG_HEIGHT], SDL_Surface* screen) {
    Uint32 black = SDL_MapRGB(screen->format, 0, 0, 0);
    Uint32 white = SDL_MapRGB(screen->format, 255, 255, 255);
    if (SDL_MUSTLOCK(screen)) {
        if (SDL_LockSurface(screen) < 0) return;
    }
    for (int x = 0; x < CFG_WIDTH; x++) {
        for (int y = 0; y < CFG_HEIGHT; y++) {
            Uint32* pixPos = (Uint32 *)screen->pixels + y*screen->pitch/4 + x;
            *pixPos = cell[x][y]? black : white;
        }
    }
    if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
    SDL_Flip(screen);
}

static void update(int cell[CFG_WIDTH][CFG_HEIGHT]) {
    for (int x = 1; x < CFG_WIDTH - 1; x++) {
        for (int y = 1; y < CFG_HEIGHT - 1; y++) {
            int sum = 0;
            for (int i = -1; i < 2; i++) {
                for (int j = -1; j < 2; j++) {
                    sum += cell[x + i][y + j];
                }
            }
            if ((sum < 2) || (sum > 4)) {
                cell[x][y] = 0;
            } else if (sum == 4) {
                cell[x][y] = 1;
            }
        }
    }
}

int main(int argc, char** argv) {
    if (!init_sdl()) return EXIT_FAILURE;
    int cell[CFG_WIDTH][CFG_HEIGHT]; // TODO typedef that?
    srand(0);
    for (int x = 0; x < CFG_WIDTH; x++) {
        for (int y = 0; y < CFG_HEIGHT; y++) {
            cell[x][y] = rand() % 2; //(x + y) % 2;
        }
    }
    while (process_events()) {
        display(cell, screen);
        update(cell);
        usleep(80000);
    }
    teardown_sdl();
    return EXIT_SUCCESS;
}