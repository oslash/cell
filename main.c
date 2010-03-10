#include <SDL.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

const int CFG_WIDTH = 512;
const int CFG_HEIGHT = 512;

static bool init_sdl(SDL_Surface** screen) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
        return false;
    }
    *screen = SDL_SetVideoMode(CFG_WIDTH, CFG_HEIGHT, 32, SDL_HWSURFACE);
    if (!*screen) {
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

static void display(int* cell, SDL_Surface* screen) {
    Uint32 black = SDL_MapRGB(screen->format, 0, 0, 0);
    Uint32 white = SDL_MapRGB(screen->format, 255, 255, 255);
    if (SDL_MUSTLOCK(screen)) {
        if (SDL_LockSurface(screen) < 0) return;
    }
    for (int x = 0; x < CFG_WIDTH; x++) {
        for (int y = 0; y < CFG_HEIGHT; y++) {
            Uint32* pixPos = (Uint32 *)screen->pixels + y*screen->pitch/4 + x;
            *pixPos = cell[x + y * CFG_WIDTH]? white : black;
        }
    }
    if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
    SDL_Flip(screen);
}

static void update(int** front, int** back, int rule[9]) {
    for (int x = 1; x < CFG_WIDTH - 1; x++) {
        for (int y = 1; y < CFG_HEIGHT - 1; y++) {
            int sum = 0;
            for (int i = -1; i < 2; i++) {
                for (int j = -1; j < 2; j++) {
                    //if ((i == 0) ^ (j == 0)) {
                        sum += (*front)[x + i + (y + j) * CFG_WIDTH];
                    //}
                }
            }
            sum -= (*front)[x + y * CFG_WIDTH];
            switch (rule[sum]) {
              case -1:
                (*back)[x + y * CFG_WIDTH] = (*front)[x + y * CFG_WIDTH];
                break;
              default:
                (*back)[x + y * CFG_WIDTH] = rule[sum];
            }
        }
    }
    int* h = *front;
    *front = *back;
    *back = h;
}

int main(int argc, char** argv) {
    SDL_Surface* screen;
    if (!init_sdl(&screen)) return EXIT_FAILURE;
    //int cell[CFG_WIDTH][CFG_HEIGHT]; // TODO typedef that?
    int* cell_front = malloc(CFG_WIDTH * CFG_HEIGHT * sizeof(int));
    int* cell_back = malloc(CFG_WIDTH * CFG_HEIGHT * sizeof(int));
    srand(0);
    int s = 2;
    for (int x = CFG_WIDTH / 2 - s; x < CFG_WIDTH / 2 + s; x++) {
        for (int y = CFG_HEIGHT / 2 - s; y < CFG_HEIGHT / 2 + s; y++) {
            cell_front[x + y * CFG_WIDTH] = rand() % 2; //(x + y) % 2;
        }
    }
    srand(3);
    int rule[9]; // = {0, 0, -1, 1, 0, 0, 0, 0, 0};
    for (int i = 0; i < 9; i++) {
        rule[i] = (rand() % 3) - 1;
    }
    while (process_events()) {
        display(cell_front, screen);
        update(&cell_front, &cell_back, rule);
        //usleep(8000);
    }
    free(cell_front);
    free(cell_back);
    teardown_sdl();
    return EXIT_SUCCESS;
}