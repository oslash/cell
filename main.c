#include <SDL.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

const int CFG_WIDTH = 400;
const int CFG_HEIGHT = 400;
const int CFG_STATE_COUNT = 8;

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

static int process_events() {
    bool do_reset = false;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) return 0;
        if (event.type == SDL_KEYDOWN) {
            switch(event.key.keysym.sym) {
              case SDLK_ESCAPE:
                return 0;
              case SDLK_SPACE:
                do_reset = true;
                break;
              default:
                break;
            }
        }
    }
    if (do_reset) {
        return -1;
    }
    return 1;
}

static void display(int* cell, SDL_Surface* screen) {
    Uint32 colour[8] = {
      SDL_MapRGB(screen->format, 0, 0, 0),
      SDL_MapRGB(screen->format, 255, 255, 255),
      SDL_MapRGB(screen->format, 255, 0, 0),
      SDL_MapRGB(screen->format, 0, 255, 0),
      SDL_MapRGB(screen->format, 0, 0, 255),
      SDL_MapRGB(screen->format, 255, 255, 0),
      SDL_MapRGB(screen->format, 255, 0, 255),
      SDL_MapRGB(screen->format, 0, 255, 255),
    };
    if (SDL_MUSTLOCK(screen)) {
        if (SDL_LockSurface(screen) < 0) return;
    }
    for (int x = 0; x < CFG_WIDTH; x++) {
        for (int y = 0; y < CFG_HEIGHT; y++) {
            Uint32* pixPos = (Uint32 *)screen->pixels + y*screen->pitch/4 + x;
            *pixPos = colour[cell[x + 1 + (y + 1) * (CFG_WIDTH + 2)] % 8];
        }
    }
    if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
    SDL_Flip(screen);
}

static void update(int** front, int** back, int rule[9]) {
    for (int x = 1; x <= CFG_WIDTH; x++) {
        for (int y = 1; y <= CFG_HEIGHT; y++) {
            int sum = 0;
            for (int i = -1; i < 2; i++) {
                for (int j = -1; j < 2; j++) {
                    //if ((i == 0) ^ (j == 0)) {
                        sum += (*front)[x + i + (y + j) * (CFG_WIDTH + 2)];
                    //}
                }
            }
            sum -= (*front)[x + y * (CFG_WIDTH + 2)];
            if (rule[sum] >= 0) {
                (*back)[x + y * (CFG_WIDTH + 2)] = rule[sum];
            } else {
                (*back)[x + y * (CFG_WIDTH + 2)] = ((*front)[x + y * (CFG_WIDTH + 2)]
                  - rule[sum] - 1) % CFG_STATE_COUNT;
            }
        }
    }
    for (int x = 1; x <= CFG_WIDTH; x++) {
        (*back)[x] = (*back)[x + CFG_HEIGHT * (CFG_WIDTH + 2)];
        (*back)[x + (CFG_HEIGHT + 1) * (CFG_WIDTH + 2)] = (*back)[x + CFG_WIDTH + 2];
    }
    for (int y = 0; y < CFG_HEIGHT + 2; y++) {
        (*back)[y * (CFG_WIDTH + 2)] = (*back)[CFG_WIDTH + y * (CFG_WIDTH + 2)];
        (*back)[CFG_WIDTH + 1 + y * (CFG_WIDTH + 2)] = (*back)[1 + y * (CFG_WIDTH + 2)];
    }
    int* h = *front;
    *front = *back;
    *back = h;
}

static void reset(int* front, int* back, int** rule) {
    for (int x = 0; x < CFG_WIDTH + 2; x++) {
        for (int y = 0; y < CFG_HEIGHT + 2; y++) {
            front[x + y * (CFG_WIDTH + 2)] = 0;
            back[x + y * (CFG_WIDTH + 2)] = 0;
        }
    }
    static int rule_seed = -1;
    int states_used = CFG_STATE_COUNT;
    if (rule_seed == -1) {
        states_used = 2;
    }
    srand(0);
    int s = 30;
    for (int x = CFG_WIDTH / 2 - s; x < CFG_WIDTH / 2 + s; x++) {
        for (int y = CFG_HEIGHT / 2 - s; y < CFG_HEIGHT / 2 + s; y++) {
            front[x + 1 + (y + 1) * (CFG_WIDTH + 2)] = rand() % states_used; //(x + y) % states_used;
        }
    }
    if (rule_seed == -1) {
        (*rule)[0] = 0;
        (*rule)[1] = 0;
        (*rule)[2] = -1;
        (*rule)[3] = 1;
        for (int i = 4; i < 9 * (CFG_STATE_COUNT - 1); i++) {
            (*rule)[i] = 0;
        }
    } else {
        srand(rule_seed);
        for (int i = 0; i < 9 * (CFG_STATE_COUNT - 1); i++) {
            if ((rand() % 10) > 8) {
                (*rule)[i] = -1;
            } else if ((rand() % 10) > 3) {
                (*rule)[i] = 0;
            } else if (((rand() % 10) > 4) && i > 0) {
                (*rule)[i] = (*rule)[i - 1];
            } else {
                (*rule)[i] = (rand() % (CFG_STATE_COUNT * 2)) - CFG_STATE_COUNT;
            }
        }
    }
    rule_seed++;
    for (int i = 0; i < 9 * (CFG_STATE_COUNT - 1); i++) {
        printf("%i, ", (*rule)[i]);
    }
    printf("\n");
}

int main(int argc, char** argv) {
    SDL_Surface* screen;
    if (!init_sdl(&screen)) return EXIT_FAILURE;
    //int cell[CFG_WIDTH][CFG_HEIGHT]; // TODO typedef that?
    int* cell_front = malloc((CFG_WIDTH + 2) * (CFG_HEIGHT + 2) * sizeof(int));
    int* cell_back = malloc((CFG_WIDTH + 2) * (CFG_HEIGHT + 2) * sizeof(int));
    int* rule = malloc(9 * (CFG_STATE_COUNT - 1) * sizeof(int));
    reset(cell_front, cell_back, &rule);
    int running = 1;
    while ((running = process_events())) {
        if (running == -1) {
            reset(cell_front, cell_back, &rule);
        }
        display(cell_front, screen);
        update(&cell_front, &cell_back, rule);
        //usleep(100000);
    }
    free(cell_front);
    free(cell_back);
    free(rule);
    teardown_sdl();
    return EXIT_SUCCESS;
}

// http://www.argentum.freeserve.co.uk/lex_l.htm#lidka
