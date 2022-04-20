#include <stdio.h>
#include <X11/Xlib.h>
#include <stdlib.h>
#include <stdint-gcc.h>
#include <unistd.h>
#include <math.h>
#include <complex.h>
#include <X11/extensions/Xdbe.h>


#define GRID_WIDTH 7600
#define GRID_HEIGHT 4000
#define GRID_SIZE GRID_WIDTH * GRID_HEIGHT

#define PIX_BUF_WIDTH 1900
#define PIX_BUF_HEIGHT 1000
#define PIX_BUF_SIZE PIX_BUF_HEIGHT * PIX_BUF_WIDTH

#define MANDEL_SCALING_X 4.0f
#define MANDEL_SCALING_X_OFFSET 2.3f
#define MANDEL_SCALING_Y 2.0f
#define MANDEL_SCALING_Y_OFFSET 1.0f
#define MANDEL_MAX_DEPTH 25
#define MANDEL_DIMENSIONS 2
#define MANDEL_UNZOOM 1.0f

#define ARROW_CMD_SPEED 8

typedef uint32_t RGBA32;

void initialize_memory(RGBA32 ** element, unsigned int size);
RGBA32 make_rgba32(float r, float g, float b);
void insert_grid(RGBA32 ** grid, int x, int y, int i, int max_iter);
void draw_pixel(RGBA32 ** grid_px, int x, int y, RGBA32 pixel);
void generate_mandelbrot(RGBA32 ** grid);
RGBA32 get_pix_from_grid(RGBA32 ** grid, int x, int y);
void render_mandelbrot(RGBA32 ** grid_px, RGBA32 ** grid, int move_x, int move_y);

int main() {

    RGBA32 * grid;
    RGBA32 * grid_px;

    initialize_memory(&grid, GRID_SIZE);
    initialize_memory(&grid_px, PIX_BUF_SIZE);

    Display *display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "ERROR: could not open the default display\n");
        exit(1);
    }

    int major_version_return, minor_version_return;
    if(XdbeQueryExtension(display, &major_version_return, &minor_version_return)) {
        printf("XDBE version %d.%d\n", major_version_return, minor_version_return);
    } else {
        fprintf(stderr, "XDBE is not supported!!!1\n");
        exit(1);
    }

    Window window = XCreateSimpleWindow(
            display, XDefaultRootWindow(display),
            0, 0, 800, 600, 0, 0, 0);

    XdbeBackBuffer back_buffer = XdbeAllocateBackBufferName(display, window, 0);
    printf("back_buffer ID: %lu\n", back_buffer);

    XWindowAttributes wa = {0};
    XGetWindowAttributes(display, window, &wa);

    XImage *image = XCreateImage(display, wa.visual, wa.depth, ZPixmap, 0,
                                 (char*) grid_px,
                                 PIX_BUF_WIDTH, PIX_BUF_HEIGHT,
                                 32,
                                 PIX_BUF_WIDTH * sizeof(RGBA32));

    GC gc = XCreateGC(display, window, 0, NULL);

    Atom wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wm_delete_window, 1);

    XSelectInput(display, window, ExposureMask);
    XSelectInput(display, window, KeyPressMask);
    XMapWindow(display, window);

    int move_x = 0;
    int move_y = 500;

    generate_mandelbrot(&grid);

    int quit = 0;
    while (!quit) {
        XPutImage(display, back_buffer, gc, image,
                  0, 0,
                  0, 0,
                  PIX_BUF_WIDTH, PIX_BUF_HEIGHT);
        XdbeSwapInfo swap_info;
        swap_info.swap_window = window;
        swap_info.swap_action = 0;
        XdbeSwapBuffers(display, &swap_info, 1);
        render_mandelbrot(&grid_px, &grid, move_x, move_y);

        while(XPending(display) > 0) {
            XEvent event = {};
            XNextEvent(display, &event);
            switch (event.type) {
                case ClientMessage: {
                    if ((Atom) event.xclient.data.l[0] == wm_delete_window) {
                        quit = 1;
                    }
                }
                case KeyPress: {
                    switch (XLookupKeysym(&event.xkey, 0)) {
                        case 'w':
                            move_y -= ARROW_CMD_SPEED;
                            break;
                        case 's':
                            move_y += ARROW_CMD_SPEED;
                            break;
                        case 'a':
                            move_x -= ARROW_CMD_SPEED;
                            break;
                        case 'd':
                            move_x += ARROW_CMD_SPEED;
                            break;
                        default:
                        {}
                    }
                } break;
            }
        }
    }

    XCloseDisplay(display);
    free(grid);
    free(grid_px);
    return 0;
}

void initialize_memory(RGBA32 ** element, unsigned int size) {
    size_t grid_px_size_bytes = size * sizeof(RGBA32);
    *element = malloc(grid_px_size_bytes);
    for (int i = 0; i < size; i++){
        (*element)[i] = 0;
    }
}

RGBA32 make_rgba32(float r, float g, float b)
{
    return (((uint32_t) (b * 255.0)) << 16) |
           (((uint32_t) (g * 255.0)) << 8) |
           (uint32_t) (r * 255.0) |
           0xFF000000;
}

void insert_grid(RGBA32 ** grid, int x, int y, int i, int max_iter) {
    double scaled_color = ((double)i/(double)max_iter);

    if (i == max_iter) {
        (*grid)[y * GRID_WIDTH + x] = 0;
        return;
    }

    float normalized_color = (float)scaled_color;

    float other = 0;
    if (normalized_color-0.25f > 0) {
        other = normalized_color - 0.25f;
    }

    (*grid)[y * GRID_WIDTH + x] = make_rgba32(normalized_color, other, other);
}

void draw_pixel(RGBA32 ** grid_px, int x, int y, RGBA32 pixel) {
    (*grid_px)[y * PIX_BUF_WIDTH + x] = pixel;
}

void generate_mandelbrot(RGBA32 ** grid) {
    for (int y_pixel = 0; y_pixel < GRID_HEIGHT; y_pixel++){
        for (int x_pixel = 0; x_pixel < GRID_WIDTH; x_pixel++) {

            double scaled_x = ((double)MANDEL_SCALING_X / GRID_WIDTH * x_pixel - MANDEL_SCALING_X_OFFSET) * MANDEL_UNZOOM;
            double scaled_y = ((double)MANDEL_SCALING_Y / GRID_HEIGHT * y_pixel - MANDEL_SCALING_Y_OFFSET) * MANDEL_UNZOOM;

            double complex c = scaled_x + (scaled_y*I);
            double complex z = 0;
            int i = 0;
            int max_iter = MANDEL_MAX_DEPTH;
            while (i < max_iter) {
                z = cpow(z, MANDEL_DIMENSIONS) + c;
                i++;

                if (isinf(creal(z))) break;
            }

            insert_grid(grid, x_pixel, y_pixel, i, max_iter);
        }
    }
}

RGBA32 get_pix_from_grid(RGBA32 ** grid, int x, int y) {
    return (*grid)[y * GRID_WIDTH + x];
}

void render_mandelbrot(RGBA32 ** grid_px, RGBA32 ** grid, int move_x, int move_y) {
    for (int y_pixel = 0; y_pixel < PIX_BUF_HEIGHT; y_pixel++) {
        for (int x_pixel = 0; x_pixel < PIX_BUF_WIDTH; x_pixel++) {
            int relative_pixel_x = x_pixel + move_x;
            int relative_pixel_y = y_pixel + move_y;
            if (relative_pixel_x < GRID_WIDTH  && relative_pixel_y < GRID_HEIGHT) {
                draw_pixel(grid_px, x_pixel, y_pixel, get_pix_from_grid(grid, relative_pixel_x, relative_pixel_y));
            }
        }
    }
}