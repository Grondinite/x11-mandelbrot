#include <stdio.h>
#include <X11/Xlib.h>
#include <stdlib.h>
#include <stdint-gcc.h>
#include <unistd.h>
#include <math.h>
#include <complex.h>

#define GRID_WIDTH 1000
#define GRID_HEIGHT 1000
#define GRID_SIZE GRID_HEIGHT * GRID_WIDTH

#define MANDEL_SCALING_X 2.5f
#define MANDEL_SCALING_X_OFFSET 1.5f
#define MANDEL_SCALING_Y 2.0f
#define MANDEL_SCALING_Y_OFFSET 1.0f
#define MANDEL_MAX_DEPTH 50
#define MANDEL_DIMENSIONS 2

typedef uint32_t RGBA32;
RGBA32 * grid_px;

void initialize_grid(){
    size_t grid_px_size_bytes = GRID_SIZE * sizeof(RGBA32);
    grid_px = malloc(grid_px_size_bytes);
    for (int i = 0; i < GRID_SIZE; i++){
        grid_px[i] = 0;
    }
}

void draw_pixel(int x, int y, int i, int max_iter) {
    double scaled_color = (double)i/(double)max_iter*0xFFFFFFFF;
    grid_px[y*GRID_HEIGHT + x] = 0xFFFFFFFF - (RGBA32)scaled_color;
}

void render_mandelbrot() {
    for (int y_pixel = 0; y_pixel < GRID_HEIGHT; y_pixel++){
        for (int x_pixel = 0; x_pixel < GRID_WIDTH; x_pixel++) {

            double scaled_x = (double)MANDEL_SCALING_X/GRID_WIDTH * x_pixel - MANDEL_SCALING_X_OFFSET;
            double scaled_y = (double)MANDEL_SCALING_Y/GRID_HEIGHT * y_pixel - MANDEL_SCALING_Y_OFFSET;

            double complex c = scaled_x + (scaled_y*I);
            double complex z = 0;
            int i = 0;
            int max_iter = MANDEL_MAX_DEPTH;
            while (i < max_iter) {
                z = cpow(z, MANDEL_DIMENSIONS) + c;
                i++;

                if (isinf(creal(z))) break;
            }
            draw_pixel(x_pixel, y_pixel, i, max_iter);
        }
    }
}

int main() {
    initialize_grid();

    Display *display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "ERROR: could not open the default display\n");
        exit(1);
    }

    Window window = XCreateSimpleWindow(
            display, XDefaultRootWindow(display),
            0, 0, 800, 600, 0, 0, 0);

    XWindowAttributes wa = {0};
    XGetWindowAttributes(display, window, &wa);

    XImage *image = XCreateImage(display, wa.visual, wa.depth, ZPixmap, 0,
                                 (char*) grid_px,
                                 GRID_WIDTH, GRID_HEIGHT,
                                 32,
                                 GRID_WIDTH * sizeof(RGBA32));

    GC gc = XCreateGC(display, window, 0, NULL);

    Atom wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wm_delete_window, 1);

    XSelectInput(display, window, ExposureMask);
    XSelectInput(display, window, KeyPressMask);
    XMapWindow(display, window);

    render_mandelbrot();

    int quit = 0;
    while (!quit) {
        XPutImage(display, window, gc, image,
                  0, 0,
                  0, 0,
                  GRID_WIDTH, GRID_HEIGHT);
        while(XPending(display) > 0) {
            XEvent event = {};
            XNextEvent(display, &event);
            switch (event.type) {
                case ClientMessage: {
                    if ((Atom) event.xclient.data.l[0] == wm_delete_window) {
                        quit = 1;
                    }
                }
            }
        }
    }

    XCloseDisplay(display);
    free(grid_px);
    return 0;
}
