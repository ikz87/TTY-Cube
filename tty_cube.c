#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <math.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <termios.h>
#include <sys/select.h>
#include <errno.h>
#include "config.h"
#include "vectors.h"
#include "fragment_shaders.h"
#include "light.h"
#include "camera.h"
#include "blur.h"

#define PI 3.14159265
#define EPSILON 1e-6f

volatile sig_atomic_t done = 0;

typedef struct {
    int w, a, s, d;
    int h, j, k, l;
    int q;
    int shift;
    int space;
} KeyState;

KeyState key_state = {0};
struct libevdev *input_dev = NULL;
int input_fd = -1;

void cleanup_input() {
    if (input_dev) libevdev_free(input_dev);
    if (input_fd >= 0) close(input_fd);
}

int setup_input(const char *device_path) {
    input_fd = open(device_path, O_RDONLY | O_NONBLOCK);
    if (input_fd < 0) {
        if (errno == EACCES) {
            fprintf(stderr, "Error opening input device '%s': %s\n", device_path, strerror(errno));
            fprintf(stderr, "You do not have permission to access this device. Try running as root or check your user permissions (e.g., add your user to the 'input' group).\n");
        } else {
            fprintf(stderr, "Error opening input device '%s': %s\n", device_path, strerror(errno));
            fprintf(stderr, "Try changing your input device in config.h.\n");
            fprintf(stderr, "For example: /dev/input/event3\n");
        }
        return 0;
    }
    int rc = libevdev_new_from_fd(input_fd, &input_dev);
    if (rc < 0) {
        fprintf(stderr, "Failed to initialize libevdev: %s\n", strerror(-rc));
        close(input_fd);
        input_fd = -1;
        return 0;
    }
    printf("Input device name: \"%s\"\n", libevdev_get_name(input_dev));
    printf("Controls:\n");
    printf("WASD = Move\n");
    printf("Space = Move upwards\n");
    printf("Shift = Move downwards\n");
    printf("HJKL = Camera (vim-like binds)\n");
    atexit(cleanup_input);
    return 1;
}

void term(int signum) { done = 1; }

void process_input_events() {
    struct input_event ev;
    int rc;
    do {
        rc = libevdev_next_event(input_dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
        if (rc == 0 && ev.type == EV_KEY) {
            int pressed = ev.value != 0;
            switch (ev.code) {
                case KEY_W: key_state.w = pressed; break;
                case KEY_A: key_state.a = pressed; break;
                case KEY_S: key_state.s = pressed; break;
                case KEY_D: key_state.d = pressed; break;
                case KEY_H: key_state.h = pressed; break;
                case KEY_J: key_state.j = pressed; break;
                case KEY_K: key_state.k = pressed; break;
                case KEY_L: key_state.l = pressed; break;
                case KEY_Q: key_state.q = pressed; break;
                case KEY_SPACE: key_state.space = pressed; break;
                case KEY_LEFTSHIFT:
                case KEY_RIGHTSHIFT: key_state.shift = pressed; break;
            }
        }
    } while (rc == 1 || rc == 0);
}

void paint_pixel(int x, int y, vec4 color, char buffer[], struct fb_var_screeninfo vinfo) {
    color.x = fmin(fmax(color.x, 0), 1);
    color.y = fmin(fmax(color.y, 0), 1);
    color.z = fmin(fmax(color.z, 0), 1);
    color.w = fmin(fmax(color.w, 0), 1);
    buffer[(y*vinfo.xres+x)*4] = (unsigned int)(color.z * 255);
    buffer[(y*vinfo.xres+x)*4+1] = (unsigned int)(color.y * 255);
    buffer[(y*vinfo.xres+x)*4+2] = (unsigned int)(color.x * 255);
    buffer[(y*vinfo.xres+x)*4+3] = (unsigned int)(87);
}

int main(int argc, char *argv[]) {
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = term;
    sigaction(SIGINT, &action, NULL);

    int fbfd = open(FB_DEVICE, O_RDWR);
    if (fbfd == -1) {
        perror("Error opening framebuffer device");
        exit(1);
    }

    struct fb_var_screeninfo vinfo;
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("Error reading variable information");
        exit(1);
    }

    struct fb_fix_screeninfo finfo;
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
        perror("Error reading fixed information");
        exit(1);
    }

    long screensize = vinfo.yres_virtual * finfo.line_length;
    char* fbp = (char*)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if ((intptr_t)fbp == -1) {
        perror("Error mapping framebuffer to memory");
        exit(1);
    }

    // Input device
    const char *input_device = INPUT_DEVICE;
    if (!setup_input(input_device)) {
        munmap(fbp, screensize);
        close(fbfd);
        return 1;
    }

    size_t buffer_size = (size_t)vinfo.xres * vinfo.yres * 4;
    char* buffer = calloc(buffer_size, 1);
    if (buffer == NULL) {
        perror("Error allocating draw buffer");
        munmap(fbp, screensize);
        close(fbfd);
        exit(1);
    }
    memcpy(buffer, fbp, 4 * vinfo.xres * vinfo.yres);

    camera cam;
    light3 light;

#ifdef IMAGE
    FILE* image_file = fopen(IMAGE, "r");
    fread(image_data, SIDE_LENGTH*SIDE_LENGTH, 3, image_file);
    fclose(image_file);
#endif

    double time = 0;
    double time_cyclic = 0;
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    unsigned int delta_us = 0;
    double delta = delta_us;

    // Camera state
    vec3 camera_position = (vec3) {0, 0, -2*SIDE_LENGTH};
    vec3 camera_rotation = (vec3) {0, 0, 0};
    int move_speed = 3000;
    int rotation_speed = PI*0.7;

    while (!done) {
        clock_gettime(CLOCK_MONOTONIC_RAW, &start);
        time += SPEED*delta*20;
        time_cyclic = ((int)time%100)/(100/2.0);

        process_input_events();
        if (key_state.q) { done = 1; continue; }

        // Camera movement
        vec3 forward = { -cos(camera_rotation.y + PI/2.0), 0, sin(camera_rotation.y + PI/2.0) };
        vec3 right = { cos(camera_rotation.y), 0, -sin(camera_rotation.y) };
        forward = normalize_vec3(forward);
        right = normalize_vec3(right);

        if (key_state.w) camera_position = add_vec3(camera_position, scale_vec3(forward, move_speed * delta));
        if (key_state.s) camera_position = subtract_vec3(camera_position, scale_vec3(forward, move_speed * delta));
        if (key_state.a) camera_position = subtract_vec3(camera_position, scale_vec3(right, move_speed * delta));
        if (key_state.d) camera_position = add_vec3(camera_position, scale_vec3(right, move_speed * delta));
        if (key_state.k) camera_rotation.x += rotation_speed * delta;
        if (key_state.j) camera_rotation.x -= rotation_speed * delta;
        if (key_state.h) camera_rotation.y -= rotation_speed * delta;
        if (key_state.l) camera_rotation.y += rotation_speed * delta;
        camera_rotation.x = fmaxf(-PI/2.0f + EPSILON, fminf(PI/2.0f - EPSILON, camera_rotation.x));
        if (key_state.space) camera_position.y -= move_speed * delta;
        if (key_state.shift) camera_position.y += move_speed * delta;

        // Camera setup
        cam = (camera){-SIDE_LENGTH,
            time,
            (vec2){vinfo.xres, vinfo.yres},
            camera_rotation,
            camera_position,
            (vec3){1,1,1},
            (vec2){0,0},
            (vec3){0,0,0},
            (vec3){0,0,0},
            (vec3){0,0,0},
            (vec3){0,0,0},
            (vec3){0,0,0}
        };
        camera transformed_cam = setup_camera(cam);

        // Light setup
        vec3 light_offset = (vec3){SIDE_LENGTH*4,-SIDE_LENGTH*5,-SIDE_LENGTH*2};
        light = (light3){
            (vec3){1,1,1},
            add_vec3(transformed_cam.center_point,
                     add_vec3(scale_vec3(transformed_cam.base_x, light_offset.x),
                              add_vec3(scale_vec3(transformed_cam.base_y, light_offset.y),
                                       scale_vec3(transformed_cam.base_z, light_offset.z))))
        };

        // Bounding box for cube
        vec3 vertices[8] = {
            (vec3){SIDE_LENGTH/2, SIDE_LENGTH/2, SIDE_LENGTH/2},
            (vec3){SIDE_LENGTH/2, SIDE_LENGTH/2, -SIDE_LENGTH/2},
            (vec3){SIDE_LENGTH/2, -SIDE_LENGTH/2, SIDE_LENGTH/2},
            (vec3){SIDE_LENGTH/2, -SIDE_LENGTH/2, -SIDE_LENGTH/2},
            (vec3){-SIDE_LENGTH/2, SIDE_LENGTH/2, SIDE_LENGTH/2},
            (vec3){-SIDE_LENGTH/2, SIDE_LENGTH/2, -SIDE_LENGTH/2},
            (vec3){-SIDE_LENGTH/2, -SIDE_LENGTH/2, SIDE_LENGTH/2},
            (vec3){-SIDE_LENGTH/2, -SIDE_LENGTH/2, -SIDE_LENGTH/2}
        };

        for (int i = 0; i < 8; i++) {
            vertices[i] = rotate_vec3_y(vertices[i], time*4*PI/1000);
        }

        vec2 min_coords = {vinfo.xres, vinfo.yres};
        vec2 max_coords = {0, 0};

        for (int i = 0; i < 8; i++) {
            vec2 screen = project_vertex_to_screen(vertices[i], transformed_cam);
            if (screen.x < min_coords.x) min_coords.x = screen.x;
            if (screen.y < min_coords.y) min_coords.y = screen.y;
            if (screen.x > max_coords.x) max_coords.x = screen.x;
            if (screen.y > max_coords.y) max_coords.y = screen.y;
        }

        // Clamp to screen
        min_coords.x = fmax(0, min_coords.x);
        min_coords.y = fmax(0, min_coords.y);
        max_coords.x = fmin(vinfo.xres-1, max_coords.x);
        max_coords.y = fmin(vinfo.yres-1, max_coords.y);

        for (int j = 0; j < vinfo.yres; j += DOWNSCALING_FACTOR) {
            for (int i = 0; i < vinfo.xres; i += DOWNSCALING_FACTOR) {
                if (i >= (int)min_coords.x && i <= (int)max_coords.x &&
                    j >= (int)min_coords.y && j <= (int)max_coords.y) {
                    if (RENDER_OVER_TEXT) {
                        vec4 color = get_pixel_through_camera(i, j, transformed_cam, light);
                        for (int dc_offset_x = 0; dc_offset_x < DOWNSCALING_FACTOR; dc_offset_x++)
                            for (int dc_offset_y = 0; dc_offset_y < DOWNSCALING_FACTOR; dc_offset_y++)
                                paint_pixel(i + dc_offset_x, j + dc_offset_y, color, buffer, vinfo);
                    } else {
                        for (int dc_offset_x = 0; dc_offset_x < DOWNSCALING_FACTOR; dc_offset_x++) {
                            for (int dc_offset_y = 0; dc_offset_y < DOWNSCALING_FACTOR; dc_offset_y++) {
                                int x_off = i + dc_offset_x;
                                int y_off = j + dc_offset_y;
                                vec4 fb_color = {buffer[(y_off*vinfo.xres+x_off)*4],
                                    buffer[(y_off*vinfo.xres+x_off)*4+1],
                                    buffer[(y_off*vinfo.xres+x_off)*4+2],
                                    buffer[(y_off*vinfo.xres+x_off)*4+3]};
                                if (fb_color.x == 0 && fb_color.y == 0 && fb_color.z == 0) {
                                    vec4 color = get_pixel_through_camera(i, j, transformed_cam, light);
                                    paint_pixel(x_off, y_off, color, buffer, vinfo);
                                } else if (fb_color.w == 87) {
                                    vec4 color = get_pixel_through_camera(i, j, transformed_cam, light);
                                    paint_pixel(x_off, y_off, color, buffer, vinfo);
                                }
                            }
                        }
                    }
                } else {
                    if (RENDER_OVER_TEXT) {
                        for (int dc_offset_x = 0; dc_offset_x < DOWNSCALING_FACTOR; dc_offset_x++)
                            for (int dc_offset_y = 0; dc_offset_y < DOWNSCALING_FACTOR; dc_offset_y++)
                                paint_pixel(i + dc_offset_x, j + dc_offset_y, (vec4){0,0,0,0}, buffer, vinfo);
                    } else {
                        for (int dc_offset_x = 0; dc_offset_x < DOWNSCALING_FACTOR; dc_offset_x++) {
                            for (int dc_offset_y = 0; dc_offset_y < DOWNSCALING_FACTOR; dc_offset_y++) {
                                int x_off = i + dc_offset_x;
                                int y_off = j + dc_offset_y;
                                vec4 fb_color = {buffer[(y_off*vinfo.xres+x_off)*4],
                                    buffer[(y_off*vinfo.xres+x_off)*4+1],
                                    buffer[(y_off*vinfo.xres+x_off)*4+2],
                                    buffer[(y_off*vinfo.xres+x_off)*4+3]};
                                if (fb_color.w == 87)
                                    paint_pixel(x_off, y_off, (vec4){0,0,0,0}, buffer, vinfo);
                            }
                        }
                    }
                }
            }
        }

        if (RENDER_BOUNDING_BOX) {
            // Draw top and bottom edges
            for (int x = (int)min_coords.x; x <= (int)max_coords.x; x++) {
                if (RENDER_OVER_TEXT) {
                    paint_pixel(x, (int)min_coords.y, (vec4){1,1,1,1}, buffer, vinfo);
                    paint_pixel(x, (int)max_coords.y, (vec4){1,1,1,1}, buffer, vinfo);
                }
                else {
                    vec4 top_color = {buffer[((int)min_coords.y*vinfo.xres+x)*4],
                        buffer[((int)min_coords.y*vinfo.xres+x)*4+1],
                        buffer[((int)min_coords.y*vinfo.xres+x)*4+2],
                        buffer[((int)min_coords.y*vinfo.xres+x)*4+3]};

                    vec4 bottom_color = {buffer[((int)max_coords.y*vinfo.xres+x)*4],
                        buffer[((int)max_coords.y*vinfo.xres+x)*4+1],
                        buffer[((int)max_coords.y*vinfo.xres+x)*4+2],
                        buffer[((int)max_coords.y*vinfo.xres+x)*4+3]};
                    if (top_color.w == 87 || (top_color.x == 0 && top_color.y == 0 && top_color.z == 0))
                    {
                        paint_pixel(x, (int)min_coords.y, (vec4){1,1,1,1}, buffer, vinfo);
                    }
                    if (bottom_color.w == 87 || (bottom_color.x == 0 && bottom_color.y == 0 && bottom_color.z == 0))
                    {
                        paint_pixel(x, (int)max_coords.y, (vec4){1,1,1,1}, buffer, vinfo);
                    }
                }
            }
            // Draw left and right edges
            for (int y = (int)min_coords.y; y <= (int)max_coords.y; y++) {
                if (RENDER_OVER_TEXT) {
                    paint_pixel((int)min_coords.x, y, (vec4){1,1,1,1}, buffer, vinfo);
                    paint_pixel((int)max_coords.x, y, (vec4){1,1,1,1}, buffer, vinfo);
                }
                else {
                    vec4 left_color = {buffer[(y*vinfo.xres+(int)min_coords.x)*4],
                        buffer[((int)y*vinfo.xres+(int)min_coords.x)*4+1],
                        buffer[((int)y*vinfo.xres+(int)min_coords.x)*4+2],
                        buffer[((int)y*vinfo.xres+(int)min_coords.x)*4+3]};

                    vec4 right_color = {buffer[(y*vinfo.xres+(int)max_coords.x)*4],
                        buffer[((int)y*vinfo.xres+(int)max_coords.x)*4+1],
                        buffer[((int)y*vinfo.xres+(int)max_coords.x)*4+2],
                        buffer[((int)y*vinfo.xres+(int)max_coords.x)*4+3]};

                    if (left_color.w == 87 || (left_color.x == 0 && left_color.y == 0 && left_color.z == 0))
                    {
                        paint_pixel(min_coords.x, y, (vec4){1,1,1,1}, buffer, vinfo);
                    }
                    if (right_color.w == 87 || (right_color.x == 0 && right_color.y == 0 && right_color.z == 0))
                    {
                        paint_pixel(max_coords.x, y, (vec4){1,1,1,1}, buffer, vinfo);
                    }
                }
            }
        }
        if (BLUR_ANTIALIAS)
            blur_pixels(buffer, min_coords, max_coords, vinfo.xres, vinfo.yres);


        printf("\r");
        fflush(stdout);
        memcpy(fbp, buffer, 4 * vinfo.xres * vinfo.yres);

        clock_gettime(CLOCK_MONOTONIC_RAW, &end);
        delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;

        if (FRAME_LIMIT > 0) {
            if (delta_us < 1000000.0 / FRAME_LIMIT) {
                usleep(1000000.0 / FRAME_LIMIT - delta_us);
                delta = 1.0 / FRAME_LIMIT;
            } else {
                delta = delta_us/1000000.0;
            }
        } else {
            delta = delta_us/1000000.0;
        }
    }
    munmap(fbp, screensize);
    close(fbfd);
    free(buffer);
    return 0;
}

