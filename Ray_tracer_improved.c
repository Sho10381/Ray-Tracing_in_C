#include <SDL2/SDL.h>
#include <stdio.h>
#include <math.h>

#define WIDTH 900
#define HEIGHT 600
#define COLOR_WHITE 0xffffffffu
#define COLOR_BLACK  0x00000000u
#define COLOR_GREY   0xefefefefu
#define RAY_NUMBER 720 /* more rays for smoothness */

/*
  Improved ray tracer (simple 2D radial rays) using direct pixel access
  and a few micro-optimizations compared to the original file you sent.

  - Precompute sin/cos for each ray angle (avoid calling trig in inner loop)
  - Write pixels directly into the surface's pixel buffer (faster than SDL_FillRect per pixel)
  - Use squared distances to avoid expensive sqrt/pow in the hot loop
  - Keep human-like comments and readable structure, like your original file

  Compile on Windows (PowerShell) with SDL2 (adjust include/lib paths):
  gcc Ray_tracer_improved.c -I "C:/Libraries/SDL2/include" -L "C:/Libraries/SDL2/lib" -lSDL2 -lSDL2main -o program.exe -D SDL_MAIN_HANDLED

  Note: replace paths above with your actual SDL2 SDK paths.
*/

struct Circle {
    double x, y, r;
};

/* simple struct to hold precomputed ray directions */
struct RayDir {
    double dx, dy; /* step vector (unit) */
};

/* Draw a filled circle by direct pixel writes. This is more efficient than
   calling SDL_FillRect per pixel because we write into the surface buffer
   while the surface is locked. */
static void fill_circle_pixels(SDL_Surface *surf, const struct Circle *c, Uint32 color) {
    int x0 = (int)floor(c->x);
    int y0 = (int)floor(c->y);
    int r = (int)ceil(c->r);
    int w = surf->w;
    int h = surf->h;

    Uint32 *pixels = (Uint32 *)surf->pixels;
    double r2 = c->r * c->r;

    for (int dy = -r; dy <= r; ++dy) {
        int py = y0 + dy;
        if (py < 0 || py >= h) continue;
        /* compute dx range for this row using circle equation */
        double ydist2 = (double)dy * (double)dy;
        double remain = r2 - ydist2;
        if (remain < 0) continue;
        int dxmax = (int)floor(sqrt(remain));
        int startx = x0 - dxmax;
        int endx = x0 + dxmax;
        if (startx < 0) startx = 0;
        if (endx >= w) endx = w - 1;
        Uint32 *row = pixels + py * w;
        for (int px = startx; px <= endx; ++px) {
            row[px] = color;
        }
    }
}

/* Generate normalized direction vectors for rays. We precompute these once
   per frame (or when ray count changes) so the inner ray loop is cheap. */
static void generate_ray_dirs(struct RayDir dirs[RAY_NUMBER]) {
    for (int i = 0; i < RAY_NUMBER; ++i) {
        double angle = ((double)i / (double)RAY_NUMBER) * (2.0 * M_PI);
        dirs[i].dx = cos(angle);
        dirs[i].dy = sin(angle);
    }
}

/* Draw radial rays from a source point until they leave the screen or hit
   the obstacle circle. Instead of stepping tiny fractional amounts, we step
   by a small integer 'step' which is tuned for speed vs. accuracy. */
static void draw_rays(SDL_Surface *surf, const struct RayDir dirs[RAY_NUMBER],
                      double sx, double sy, const struct Circle *obstacle, Uint32 color) {
    int w = surf->w;
    int h = surf->h;
    Uint32 *pixels = (Uint32 *)surf->pixels;
    double obs_x = obstacle->x;
    double obs_y = obstacle->y;
    double obs_r2 = obstacle->r * obstacle->r;

    /* step controls ray sampling density: 1 gives pixel-perfect rays */
    const double step = 1.0;

    for (int i = 0; i < RAY_NUMBER; ++i) {
        double x = sx;
        double y = sy;
        double dx = dirs[i].dx;
        double dy = dirs[i].dy;

        /* march along the ray */
        for (;;) {
            x += dx * step;
            y += dy * step;
            int ix = (int)floor(x + 0.5);
            int iy = (int)floor(y + 0.5);
            if (ix < 0 || ix >= w || iy < 0 || iy >= h) break;
            pixels[iy * w + ix] = color;

            /* hit test against obstacle using squared distances */
            double ddx = x - obs_x;
            double ddy = y - obs_y;
            if (ddx * ddx + ddy * ddy < obs_r2) {
                break; /* stop ray when it enters the obstacle */
            }
        }
    }
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Ray Tracer Improved",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          WIDTH, HEIGHT, 0);
    if (!window) {
        fprintf(stderr, "CreateWindow error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Surface *surface = SDL_GetWindowSurface(window);
    if (!surface) {
        fprintf(stderr, "GetWindowSurface error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    struct Circle source = {200.0, 200.0, 8.0};
    struct Circle obstacle = {600.0, 200.0, 100.0};

    struct RayDir dirs[RAY_NUMBER];
    generate_ray_dirs(dirs);

    int running = 1;
    SDL_Event ev;

    /* Main loop: handle events, lock surface, write pixels directly, unlock */
    while (running) {
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = 0;
            if (ev.type == SDL_MOUSEMOTION && (ev.motion.state & SDL_BUTTON_LMASK)) {
                source.x = ev.motion.x;
                source.y = ev.motion.y;
            }
            if (ev.type == SDL_MOUSEBUTTONDOWN) {
                /* quick tweak: left button moves source, right button moves obstacle */
                if (ev.button.button == SDL_BUTTON_RIGHT) {
                    obstacle.x = ev.button.x;
                    obstacle.y = ev.button.y;
                }
            }
        }

        /* lock surface so we can write pixels efficiently */
        if (SDL_MUSTLOCK(surface)) SDL_LockSurface(surface);

        /* clear to black quickly using memset (works for 32-bit surfaces) */
        Uint32 *pixels = (Uint32 *)surface->pixels;
        int total = surface->w * surface->h;
        for (int i = 0; i < total; ++i) pixels[i] = COLOR_BLACK;

        /* draw source (small filled circle) */
        fill_circle_pixels(surface, &source, COLOR_WHITE);

        /* draw obstacle */
        fill_circle_pixels(surface, &obstacle, COLOR_GREY);

        /* cast and draw rays */
        draw_rays(surface, dirs, source.x, source.y, &obstacle, COLOR_WHITE);

        if (SDL_MUSTLOCK(surface)) SDL_UnlockSurface(surface);

        SDL_UpdateWindowSurface(window);
        SDL_Delay(10); /* small delay to avoid 100% CPU; tune as desired */
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
