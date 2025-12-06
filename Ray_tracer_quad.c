#include <SDL2/SDL.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>

#define WIDTH 900
#define HEIGHT 600
/* Use a moderate number of rays: mathematically correct intersections,
   so we don't need as many rays as a cheap approximate algorithm. */
#define RAY_NUMBER 360

/* Colors and utilities ------------------------------------------------- */
static inline Uint32 rgb_to_pixel(Uint8 r, Uint8 g, Uint8 b) {
    return (Uint32)0xff000000u | ((Uint32)r << 16) | ((Uint32)g << 8) | (Uint32)b;
}

/* Convert hue [0,1) + brightness [0,1] -> RGB 0..255 (simple HSV->RGB) */
static void hue_to_rgb(double h, double v, Uint8 *out_r, Uint8 *out_g, Uint8 *out_b) {
    double s = 1.0; /* full saturation for vivid rays */
    double c = v * s;
    double hh = h * 6.0;
    double x = c * (1.0 - fabs(fmod(hh, 2.0) - 1.0));
    double r = 0, g = 0, b = 0;
    if (hh < 1.0) { r = c; g = x; b = 0; }
    else if (hh < 2.0) { r = x; g = c; b = 0; }
    else if (hh < 3.0) { r = 0; g = c; b = x; }
    else if (hh < 4.0) { r = 0; g = x; b = c; }
    else if (hh < 5.0) { r = x; g = 0; b = c; }
    else { r = c; g = 0; b = x; }
    double m = v - c;
    *out_r = (Uint8)fmin(255, fmax(0, (r + m) * 255.0));
    *out_g = (Uint8)fmin(255, fmax(0, (g + m) * 255.0));
    *out_b = (Uint8)fmin(255, fmax(0, (b + m) * 255.0));
}

/* Geometry types ------------------------------------------------------ */
struct Circle { double x, y, r; };

/* Put pixel safely into 32-bit surface buffer */
static inline void put_pixel(Uint32 *pixels, int w, int h, int x, int y, Uint32 color) {
    if ((unsigned)x >= (unsigned)w || (unsigned)y >= (unsigned)h) return;
    pixels[y * w + x] = color;
}

/* Filled circle rasterizer using scanlines (fast and precise) */
static void fill_circle(Uint32 *pixels, int w, int h, const struct Circle *c, Uint32 color) {
    int cx = (int)floor(c->x + 0.5);
    int cy = (int)floor(c->y + 0.5);
    int rr = (int)ceil(c->r);
    double r2 = c->r * c->r;
    for (int dy = -rr; dy <= rr; ++dy) {
        int y = cy + dy;
        if (y < 0 || y >= h) continue;
        double dy2 = (double)dy * (double)dy;
        double remain = r2 - dy2;
        if (remain <= 0.0) continue;
        int dxmax = (int)floor(sqrt(remain));
        int x0 = cx - dxmax;
        int x1 = cx + dxmax;
        if (x0 < 0) x0 = 0;
        if (x1 >= w) x1 = w - 1;
        for (int x = x0; x <= x1; ++x) pixels[y * w + x] = color;
    }
}

/* Solve ray-circle intersection using full quadratic equation.
   Ray: p = o + t * d  (t >= 0). Circle center c, radius r.
   Returns: smallest positive t (>= 0) if intersection exists, otherwise -1.0.
*/
static double ray_circle_intersect(double ox, double oy, double dx, double dy,
                                   const struct Circle *c) {
    double cx = c->x, cy = c->y, r = c->r;
    /* Solve: |(o + t d) - c|^2 = r^2  =>  a t^2 + b t + c0 = 0 */
    double ocx = ox - cx;
    double ocy = oy - cy;
    double a = dx*dx + dy*dy; /* if dir normalized, a==1 */
    double b = 2.0 * (dx * ocx + dy * ocy);
    double c0 = ocx*ocx + ocy*ocy - r*r;
    double disc = b*b - 4.0*a*c0;
    if (disc < 0.0) return -1.0;
    double sqrt_disc = sqrt(disc);
    double t1 = (-b - sqrt_disc) / (2.0 * a);
    double t2 = (-b + sqrt_disc) / (2.0 * a);
    /* We want the smallest t >= 0. If both negative, no forward intersection. */
    double t = -1.0;
    if (t1 >= 0.0) t = t1;
    else if (t2 >= 0.0) t = t2; /* source inside circle -> t2 is exit point */
    return t;
}

/* Draw a short anti-aliased line by sampling points along ray (cheap) */
static void draw_ray_pixels(Uint32 *pixels, int w, int h, double ox, double oy,
                            double tx, double ty, Uint8 r, Uint8 g, Uint8 b) {
    double dx = tx - ox;
    double dy = ty - oy;
    double dist = sqrt(dx*dx + dy*dy);
    if (dist <= 0.0) return;
    double steps = fmax(1.0, dist);
    double sx = dx / steps;
    double sy = dy / steps;
    for (int i = 0; i <= (int)steps; ++i) {
        double x = ox + sx * i;
        double y = oy + sy * i;
        int xi = (int)floor(x + 0.5);
        int yi = (int)floor(y + 0.5);
        Uint32 col = rgb_to_pixel((Uint8)r, (Uint8)g, (Uint8)b);
        put_pixel(pixels, w, h, xi, yi, col);
    }
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("Ray Tracer (Quadratic Intersections)",
                                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       WIDTH, HEIGHT, 0);
    if (!win) { fprintf(stderr, "CreateWindow: %s\n", SDL_GetError()); SDL_Quit(); return 1; }

    SDL_Surface *surf = SDL_GetWindowSurface(win);
    if (!surf) { fprintf(stderr, "GetSurface: %s\n", SDL_GetError()); SDL_DestroyWindow(win); SDL_Quit(); return 1; }

    struct Circle light = {200.0, 200.0, 4.0};
    struct Circle sphere = {600.0, 200.0, 100.0};

    int running = 1;
    SDL_Event ev;

    /* Precompute direction vectors for each ray (normalize them) */
    double dirx[RAY_NUMBER], diry[RAY_NUMBER];
    for (int i = 0; i < RAY_NUMBER; ++i) {
        double angle = ((double)i / (double)RAY_NUMBER) * 2.0 * M_PI;
        dirx[i] = cos(angle);
        diry[i] = sin(angle);
    }

    while (running) {
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = 0;
            if (ev.type == SDL_MOUSEMOTION && (ev.motion.state & SDL_BUTTON_LMASK)) {
                light.x = ev.motion.x; light.y = ev.motion.y;
            }
            if (ev.type == SDL_MOUSEBUTTONDOWN) {
                if (ev.button.button == SDL_BUTTON_RIGHT) {
                    sphere.x = ev.button.x; sphere.y = ev.button.y;
                }
            }
        }

        if (SDL_MUSTLOCK(surf)) SDL_LockSurface(surf);
        Uint32 *pixels = (Uint32 *)surf->pixels;

        /* Clear to dark background */
        int total = surf->w * surf->h;
        for (int i = 0; i < total; ++i) pixels[i] = 0xff000010u; /* slightly blue-black */

        /* Draw the target circle precisely */
        fill_circle(pixels, surf->w, surf->h, &sphere, 0xff444444u);

        /* Cast rays with quadratic intersection math. For each ray: */
        for (int i = 0; i < RAY_NUMBER; ++i) {
            double ox = light.x;
            double oy = light.y;
            double dx = dirx[i];
            double dy = diry[i];

            /* solve quadratic to find first intersection t >= 0 */
            double t = ray_circle_intersect(ox, oy, dx, dy, &sphere);

            /* determine ray end point: either hit point or screen edge */
            double ex, ey;
            if (t >= 0.0) {
                ex = ox + dx * t;
                ey = oy + dy * t;
            } else {
                /* find intersection with screen bounds to draw to edge */
                /* param t to exit screen: compute for four borders and pick smallest positive */
                double tmin = 1e30;
                if (dx > 1e-9) { double tx = (surf->w - 1 - ox) / dx; if (tx > 0 && tx < tmin) tmin = tx; }
                if (dx < -1e-9) { double tx = (0 - ox) / dx; if (tx > 0 && tx < tmin) tmin = tx; }
                if (dy > 1e-9) { double ty = (surf->h - 1 - oy) / dy; if (ty > 0 && ty < tmin) tmin = ty; }
                if (dy < -1e-9) { double ty = (0 - oy) / dy; if (ty > 0 && ty < tmin) tmin = ty; }
                if (tmin < 1e29) { ex = ox + dx * tmin; ey = oy + dy * tmin; }
                else { ex = ox + dx * 1000.0; ey = oy + dy * 1000.0; }
            }

            /* color: hue based on angle, brightness based on distance to hit (if any) */
            double hue = (double)i / (double)RAY_NUMBER; /* 0..1 around circle */
            double dist = hypot(ex - ox, ey - oy);
            double bright = 1.0 / (1.0 + 0.005 * dist); /* mild attenuation */
            if (t >= 0.0) bright *= 1.4; /* hits appear brighter */
            if (bright > 1.0) bright = 1.0;
            Uint8 r, g, b;
            hue_to_rgb(hue, bright, &r, &g, &b);

            /* Draw the ray from light to endpoint */
            draw_ray_pixels(pixels, surf->w, surf->h, ox, oy, ex, ey, r, g, b);
        }

        /* draw light source */
        fill_circle(pixels, surf->w, surf->h, &light, 0xffffffffu);

        if (SDL_MUSTLOCK(surf)) SDL_UnlockSurface(surf);
        SDL_UpdateWindowSurface(win);
        SDL_Delay(16); /* ~60 FPS throttle */
    }

    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
