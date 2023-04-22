#include <SDL2/SDL.h>
#include <SDL2/SDL_log.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_main.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_events.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define NUM_EVENTS 20
#define NUM_RECTS 3
#define PADDLE_HEIGHT 80
#define PADDLE_WIDTH 20
#define PADDLE_SPEED 6
#define BALL_DIAMETER 10
#define INIT_BALL_SPEED 5
#define BALL_SPEED_INCREASE .08
#define BALL_CONTROL_SPEED .1 
#define FPS 60
#define MAX_ANGLE 100

#define SET_RED() SDL_SetRenderDrawColor(g_render, 255, 0, 0, 255)
#define SET_GREEN() SDL_SetRenderDrawColor(g_render, 0, 255, 0, 255) 
#define SET_BLUE() SDL_SetRenderDrawColor(g_render, 0, 0, 255, 255)
#define SET_BLACK() SDL_SetRenderDrawColor(g_render, 0, 0, 0, 255)
#define SET_WHITE() SDL_SetRenderDrawColor(g_render, 255, 255, 255, 255)

int w = 0;
int h = 0;

SDL_Texture *menu_screen;
SDL_Texture *game_background;
SDL_Texture *scores;

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// ENTITY FUNCTIONS ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void recalc_rects(int rectc, SDL_FRect *rects) {
    for (int i = 0; i < rectc; i++) {
        // catch ball case
        if (i == 2) {
            rects[i].x = (int)(w / 2) - (int)(BALL_DIAMETER / 2);
            rects[i].y = (int)(h / 2) - (int)(BALL_DIAMETER / 2);
            rects[i].w = BALL_DIAMETER;
            rects[i].h = BALL_DIAMETER;
            continue;
        }

        // setup paddles
        rects[i].x = ((int)(w) - PADDLE_WIDTH) * i; // put 0 at 0 and 1 to the right border
        rects[i].y = (int)(h / 2) - (int)(PADDLE_HEIGHT / 2); // put them in the vertical middle
        rects[i].w = PADDLE_WIDTH;
        rects[i].h = PADDLE_HEIGHT;
    }
}

bool check_collision(SDL_FRect *rect1, SDL_FRect *rect2) {
    bool right = (rect1->x + rect1->w) > rect2->x;
    bool left = rect1->x < (rect2->x + rect2->w);
    bool bottom = (rect1->y + rect1->h) > rect2->y;
    bool top = rect1->y < (rect2->y + rect2->w);
    return right && left && bottom && top;
}

void create_random_direction(float *x, float *y, int xdir, 
                             float seed, float length) {
    // TODO: maybe put randomness in at some point

    *y = -seed / ((float)PADDLE_HEIGHT / 2);
    float x_squared = powf(length, 2) - powf(*y, 2);
    *x = sqrtf(x_squared) * (float)xdir;
    SDL_Log("%f : %f", *x, *y);
}

/// returns 0 if no actions are necessary;
/// if the ball hit the left wall it returns 1;
/// if the ball hit the right wall it returns 2;
int update_rects(int rectc, SDL_FRect *rects, int velc, float *vels,
                 float *speed, int *p3_controls) {
    // update ball vel from player input
    if (p3_controls[0] == 1) {
        vels[3] -= BALL_CONTROL_SPEED;
    }
    else if (p3_controls[1] == 1) {
        vels[3] += BALL_CONTROL_SPEED;
    }

    // update ball
    rects[2].x += vels[2];
    rects[2].y += vels[3];
    
    // check bounds
    if (rects[2].y <= 0 || rects[2].y >= h - BALL_DIAMETER) {
        vels[3] *= -1;
    }

    // check bounce
    if (vels[2] < 0 && check_collision(&(rects[0]), &(rects[2]))) {
        create_random_direction(
                &(vels[2]), 
                &(vels[3]), 
                1,
                (rects[0].y + (float)PADDLE_HEIGHT / 2) - 
                (rects[2].y + (float)BALL_DIAMETER / 2),
                *speed
        );
        *speed += BALL_SPEED_INCREASE;
    }
    else if (check_collision(&(rects[1]), &(rects[2]))) {
        create_random_direction(
                &(vels[2]), 
                &(vels[3]), 
                -1,
                (rects[1].y + (float)PADDLE_HEIGHT / 2) - 
                (rects[2].y + (float)BALL_DIAMETER / 2),
                *speed
        );
        *speed += BALL_SPEED_INCREASE;
    }
    
    // check win
    if (rects[2].x <= 0) {
        return 1;
    }
    if (rects[2].x >= w - BALL_DIAMETER) {
        return 2;
    }

    // update paddles
    rects[0].y += vels[0];
    rects[1].y += vels[1];

    // check paddle bounds
    for (int i = 0; i < 2; i++) {
        // if paddle rect is out of bounds
        // check current direction and snap to proper bound
        if (rects[i].y <= 0 || rects[i].y >= h - PADDLE_HEIGHT) {
            if (vels[i] < 0) {
                rects[i].y = 0;
            }
            else if (vels[i] > 0) {
                rects[i].y = h - PADDLE_HEIGHT;
            }
        }
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// TEXTURE FUNCTIONS //////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void create_textures(SDL_Renderer *render) {
    SDL_Surface *background = SDL_CreateRGBSurface(0, w, h, 32, 0, 0, 0, 0);
    SDL_Rect line;
    line.x = (int)(w / 2) - 1 - (w % 2);
    line.y = 0;
    line.h = h;
    line.w = 2 + (w % 2);
    SDL_FillRect(background, &line, 0x696969);
    game_background = SDL_CreateTextureFromSurface(render, background);
    free(background);
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// WINDOW FUNCTIONS ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void clearwindow(SDL_Renderer *g_render) {
    SET_BLACK();
    SDL_RenderClear(g_render);
}

void draw_scene(SDL_Window *g_window, SDL_Renderer *g_render, 
        int rectc, SDL_FRect *rects) {
    SET_BLACK();
    SDL_RenderClear(g_render);
    SDL_RenderCopy(g_render, game_background, NULL, NULL);
    SET_WHITE();

    for (int i = 0; i < rectc; i++) {
        SDL_RenderFillRectF(g_render, &rects[i]);
    }
    SDL_RenderPresent(g_render);
}

void handlewindowevent(SDL_Renderer *g_render, SDL_WindowEvent *we, 
                       SDL_FRect *rects) {
    switch (we->event) {
        case SDL_WINDOWEVENT_RESIZED:
            SDL_GetRendererOutputSize(g_render, &w, &h);
            SDL_Log("New Window Dimensions: w = %d, h = %d", w, h);
            recalc_rects(NUM_RECTS, rects);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////// KEYBOARD FUNCTIONS //////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/// returns: 0 normally, 1 for quit, -1 on error
int handlekeypressevent(SDL_Event *event, int velc, float *vels, int *keys, 
        int *p3_controls) {
    switch ((*event).key.keysym.sym) {
        case 'd':
            vels[0] = -PADDLE_SPEED;
            keys[0] = 1;
            break;
        case 'f':
            vels[0] = PADDLE_SPEED;
            keys[1] = 1;
            break;
        case 'k':
            vels[1] = -PADDLE_SPEED;
            keys[2] = 1;
            break;
        case 'j':
            vels[1] = PADDLE_SPEED;
            keys[3] = 1;
            break;
        case SDLK_UP:
            p3_controls[0] = 1;
            break;
        case SDLK_DOWN:
            p3_controls[1] = 1;
            break;
        case 'q':
            return 1;
    }
    return 0;
}

int update_paddle_vels(SDL_Event *event, int *keys, float *vels) {
    switch ((*event).key.keysym.sym) {
        case 'd':
            keys[0] = 0;
            if (keys[1] == 0) { vels[0] = 0; }
            break;
        case 'f':
            keys[1] = 0;
            if (keys[0] == 0) { vels[0] = 0; }
            break;
        case 'k':
            keys[2] = 0;
            if (keys[3] == 0) { vels[1] = 0; }
            break;
        case 'j':
            keys[3] = 0;
            if (keys[2] == 0) { vels[1] = 0; }
            break;
    }
    return 0;
}
int update_p3_flips(SDL_Event *event, int *keys) {
    switch ((*event).key.keysym.sym) {
        case SDLK_UP:
            keys[0] = 0;
            break;
        case SDLK_DOWN:
            keys[1] = 0;
            break;
    }
    return 0;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// MAIN PART ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void loop(SDL_Window *g_window, SDL_Renderer *g_render) {
    SDL_Event *events = malloc(sizeof(SDL_Event) * NUM_EVENTS);
    SDL_FRect *rects = malloc(sizeof(SDL_FRect) * NUM_RECTS);
    float vels[4] = { 0, 0, INIT_BALL_SPEED, 0 };
    bool exit_game;
    int scores[2] = { 0, 0 };
    int frame_delay = (int)(1000 / FPS);
    Uint32 start;
    int frametime;
    int move_control[4] = { 0, 0, 0, 0 };
    float current_speed = INIT_BALL_SPEED;
    int p3_keys_pressed[] = { 0, 0 };

    recalc_rects(NUM_RECTS, rects);

    while (!exit_game) {

        start = SDL_GetTicks();

        SDL_PumpEvents();
        int eventc = SDL_PeepEvents(
                events, NUM_EVENTS, SDL_GETEVENT, 
                SDL_FIRSTEVENT, SDL_LASTEVENT
        );
        
        if (eventc < 0) {
            fprintf(stderr, "ERROR when peeping events");
            SDL_DestroyWindow(g_window);
            SDL_Quit();
            exit(1);
        }
        
        for (int i = 0; i < eventc; i++) {
            switch (events[i].type) {
                case SDL_QUIT:
                    exit_game = true;
                    break;
                case SDL_WINDOWEVENT:
                    handlewindowevent(g_render, &(events[i]).window, rects);
                    break;
                case SDL_KEYDOWN:
                    if (handlekeypressevent(&(events[i]), NUM_RECTS, vels, 
                            move_control, p3_keys_pressed) == 1) { 
                        exit_game = true; 
                    }
                    break;
                case SDL_KEYUP:
                    update_paddle_vels(&(events[i]), move_control, vels);
                    update_p3_flips(&(events[i]), p3_keys_pressed);
                    break;
            }
        }
        int result = update_rects(NUM_RECTS, rects, sizeof(vels), vels, 
                                  &current_speed, p3_keys_pressed);
        draw_scene(g_window, g_render, NUM_RECTS, rects);

        // reset
        if (result > 0 && result < 3) {
            scores[2 - result]++;
            SDL_Log("Player 1: %d - Player 2: %d", scores[0], scores[1]);
            recalc_rects(NUM_RECTS, rects);
            vels[0] = 0;
            vels[1] = 0;
            vels[2] = INIT_BALL_SPEED;
            vels[3] = 0;
            move_control[0] = 0;
            move_control[1] = 0;
            move_control[2] = 0;
            move_control[3] = 0;
            current_speed = INIT_BALL_SPEED;
        }

        // steady fps
        frametime = SDL_GetTicks() - start;
        if (frame_delay > frametime) {
            SDL_Delay(frame_delay - frametime);
        }
        else {
            SDL_LogWarn(1, "SKIPPED FRAME!");
        }
    }

    free(events);
    return;
}

int main(int argc, char *argv[])
{
    // init video
    int init_successful = SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER);
    if (init_successful == -1) {
        return 1;
    }

    // create window and surface for the game
    SDL_Window *g_window = SDL_CreateWindow(
            "Game", 
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
            600, 400, 
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN
    );
    SDL_Renderer *g_render = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_GetRendererOutputSize(g_render, &w, &h);
    create_textures(g_render);
    
    loop(g_window, g_render);
    
    SDL_DestroyRenderer(g_render);
    SDL_DestroyWindow(g_window);
    SDL_Quit();
    return 0;
}

// NOTE: Gamemodes: 1-Player, 2-Players, 3-Players
//       (3rd player controls the direction of the ball in some way)
