#include <fcntl.h>      // For open
#include <linux/fb.h>   // For FBIOGET_VSCREENINFO
#include <sys/ioctl.h>  // For ioctl
#include <sys/mman.h>   // For mmap
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <termios.h>    // For raw terminal mode
#include <sys/select.h> // For select()
#include <errno.h>      // For error checking (optional)
#include "config.h"
#include "vectors.h"
#include "fragment_shaders.h"
#include "light.h"
#include "camera.h"
#include "blur.h"


#define PI 3.14159265
#define EPSILON 1e-6f

volatile sig_atomic_t done = 0;
struct termios original_termios; // Store original terminal settings

 
void term(int signum)
{
    done = 1;
}

// Function to restore terminal settings
void restore_terminal(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
    printf("\033[?25h"); // Show cursor again
    fflush(stdout);
}

// Function to set raw terminal mode
void set_raw_terminal(void) {
    struct termios raw;

    if (tcgetattr(STDIN_FILENO, &original_termios) == -1) {
        perror("tcgetattr");
        exit(1);
    }
    // Register the cleanup function to restore settings on exit
    atexit(restore_terminal);


    raw = original_termios; // Copy original settings
    // Modify settings for raw mode
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON); // Input modes
    raw.c_oflag &= ~(OPOST); // Keep OPOST disabled for now
    raw.c_cflag |= (CS8); // Control modes (8-bit chars)
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); // Local modes
    // VMIN = 0, VTIME = 0: read() returns immediately, 0 bytes if no input
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr");
        exit(1);
    }
    // printf("\033[?25l"); // Hide cursor
    // fflush(stdout);
}
 

void paint_pixel(int x, int y, vec4 color, char buffer[], struct fb_var_screeninfo vinfo)
{
    color.x = fmin(fmax(color.x, 0), 1);
    color.y = fmin(fmax(color.y, 0), 1);
    color.z = fmin(fmax(color.z, 0), 1);
    color.w = fmin(fmax(color.w, 0), 1);
    buffer[(y*vinfo.xres+x)*4] = (unsigned int)(color.z * 255);
    buffer[(y*vinfo.xres+x)*4+1] = (unsigned int)(color.y * 255);
    buffer[(y*vinfo.xres+x)*4+2] = (unsigned int)(color.x * 255);
    buffer[(y*vinfo.xres+x)*4+3] = (unsigned int)(87);
    return;
}


int main(int argc, char *argv[])
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = term;
    sigaction(SIGINT, &action, NULL);

    set_raw_terminal();

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

    size_t buffer_size = (size_t)vinfo.xres * vinfo.yres * 4;
    char* buffer = calloc(buffer_size, 1); // Use calloc for zero-init
    if (buffer == NULL) {
        perror("Error allocating draw buffer");
        munmap(fbp, screensize);
        close(fbfd);
        exit(1);
    }

    // Save the current state of the fb
    memcpy(buffer, fbp, 4 * vinfo.xres * vinfo.yres);

    // Declare camera and light
    camera cam;
    light3 light;

    // Get image data
    #ifdef IMAGE
    FILE* image_file = fopen(IMAGE, "r");
    fread(image_data, SIDE_LENGTH*SIDE_LENGTH, 3, image_file);
    fclose(image_file);
    #endif

    // Time for frame limiter
    double time = 0;
    double time_cyclic = 0;
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    unsigned int delta_us = 0;
    double delta = delta_us;

    // Some flags for input
    int w_pressed = 0, a_pressed = 0, s_pressed = 0, d_pressed = 0;
    int k_pressed = 0, j_pressed = 0, h_pressed = 0, l_pressed = 0;
    int W_pressed = 0, S_pressed = 0;
    int q_pressed = 0;


    // Also some vars for camera position and rotation
    vec3 camera_position = (vec3) {0, 0, -2*SIDE_LENGTH};
    vec3 camera_rotation = (vec3) {0, 0, 0};
    int move_speed = 5000;
    int rotation_speed = PI;

    while (!done)
    {
        clock_gettime(CLOCK_MONOTONIC_RAW, &start);
        time += SPEED*delta*20;
        time_cyclic = ((int)time%100)/(100/2.0);


        // Reset flags 
        w_pressed = a_pressed = s_pressed = d_pressed = 0;
        h_pressed = j_pressed = k_pressed = l_pressed = 0;
        W_pressed = S_pressed = 0;
        q_pressed = 0;


        // --- Raw Terminal Input Processing ---
        char input_buf[32]; // Buffer for potentially multiple inputs/sequences
        ssize_t total_bytes_read = 0;
        int select_ret;

        // Loop select/read to consume all available input for this frame
        do {
            struct timeval tv = { 0L, 0L }; // Check immediately
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(STDIN_FILENO, &fds);
            select_ret = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);

            if (select_ret > 0) {
                // Read available data, append to buffer if space allows
                if (total_bytes_read < sizeof(input_buf) - 1) {
                    ssize_t bytes_read = read(STDIN_FILENO,
                                              input_buf + total_bytes_read,
                                              sizeof(input_buf) - 1 - total_bytes_read);
                    if (bytes_read > 0) {
                        total_bytes_read += bytes_read;
                    } else if (bytes_read == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
                        perror("read");
                        done = 1; // Exit on read error
                        break;    // Exit inner loop
                    } else {
                        // bytes_read == 0 or EAGAIN/EWOULDBLOCK means no more data right now
                        select_ret = 0; // Force exit from inner loop
                    }
                } else {
                    // Input buffer full, process what we have and try again next frame
                    select_ret = 0; // Force exit from inner loop
                }
            } else if (select_ret == -1 && errno != EINTR) {
                perror("select");
                done = 1; // Exit on select error
            }
            // Continue looping if select found data and we read something
        } while (select_ret > 0 && !done);

        // Null-terminate the buffer
        input_buf[total_bytes_read] = '\0';

        // Process the accumulated input buffer to set state flags
        for (ssize_t i = 0; i < total_bytes_read; ++i) {
            if (input_buf[i] == 'q' || input_buf[i] == 'Q') { q_pressed = 1; }
            if (input_buf[i] == 'w') { w_pressed = 1; }
            if (input_buf[i] == 's') { s_pressed = 1; }
            if (input_buf[i] == 'W') { W_pressed = 1; }
            if (input_buf[i] == 'S') { S_pressed = 1; }
            if (input_buf[i] == 'a' || input_buf[i] == 'A') { a_pressed = 1; }
            if (input_buf[i] == 'd' || input_buf[i] == 'D') { d_pressed = 1; }
            if (input_buf[i] == 'h' || input_buf[i] == 'H') { h_pressed = 1; }
            if (input_buf[i] == 'j' || input_buf[i] == 'J') { j_pressed = 1; }
            if (input_buf[i] == 'k' || input_buf[i] == 'K') { k_pressed = 1; }
            if (input_buf[i] == 'l' || input_buf[i] == 'L') { l_pressed = 1; }
        }

        if (q_pressed) { done = 1; } // Check quit flag
        
        // Calculate basis vectors based on current rotation
        vec3 forward = { -cos(camera_rotation.y + PI/2.0), 0, sin(camera_rotation.y + PI/2.0) };
        vec3 right = { cos(camera_rotation.y), 0, -sin(camera_rotation.y) };
        forward = normalize_vec3(forward);
        right = normalize_vec3(right);

        // Apply movement based on flags
        if (w_pressed) { camera_position = add_vec3(camera_position, scale_vec3(forward, move_speed * 0.01)); }
        if (s_pressed) { camera_position = subtract_vec3(camera_position, scale_vec3(forward, move_speed * 0.01)); }
        if (a_pressed) { camera_position = subtract_vec3(camera_position, scale_vec3(right, move_speed * 0.01)); }
        if (d_pressed) { camera_position = add_vec3(camera_position, scale_vec3(right, move_speed * 0.01)); }

        // Apply rotation based on flags
        if (k_pressed) { camera_rotation.x += rotation_speed * 0.01; }
        if (j_pressed) { camera_rotation.x -= rotation_speed * 0.01; }
        if (h_pressed) { camera_rotation.y -= rotation_speed * 0.01; }
        if (l_pressed) { camera_rotation.y += rotation_speed * 0.01; }

        // Clamp pitch
        camera_rotation.x = fmaxf(-PI/2.0f + EPSILON, fminf(PI/2.0f - EPSILON, camera_rotation.x));

        // Go up and down
        if (W_pressed) { camera_position.y -= move_speed * 0.01; }
        if (S_pressed) { camera_position.y += move_speed * 0.01; }

        // Define the camera
        cam = (camera){-SIDE_LENGTH,
            (vec2){vinfo.xres, vinfo.yres},
            camera_rotation,
            camera_position,
            (vec3){1,1,1},

            (vec2){0,0},
            (vec3){0,0,0},
            (vec3){0,0,0},
            (vec3){0,0,0},
            (vec3){0,0,0},
            (vec3){0,0,0}};
        camera transformed_cam = setup_camera(cam);

        // Define the light source

        vec3 light_offset = (vec3){SIDE_LENGTH*3,-SIDE_LENGTH*2,-SIDE_LENGTH};
        light = (light3){
            (vec3){1,1,1},
            light_offset
        };

        // Find the bounding box of the cube for the frame
        vec3 vertices[8] = {(vec3){SIDE_LENGTH/2, SIDE_LENGTH/2, SIDE_LENGTH/2},
            (vec3){SIDE_LENGTH/2, SIDE_LENGTH/2, -SIDE_LENGTH/2},
            (vec3){SIDE_LENGTH/2, -SIDE_LENGTH/2, SIDE_LENGTH/2},
            (vec3){SIDE_LENGTH/2, -SIDE_LENGTH/2, -SIDE_LENGTH/2},
            (vec3){-SIDE_LENGTH/2, SIDE_LENGTH/2, SIDE_LENGTH/2},
            (vec3){-SIDE_LENGTH/2, SIDE_LENGTH/2, -SIDE_LENGTH/2},
            (vec3){-SIDE_LENGTH/2, -SIDE_LENGTH/2, SIDE_LENGTH/2},
            (vec3){-SIDE_LENGTH/2, -SIDE_LENGTH/2, -SIDE_LENGTH/2}};

        vec2 min_coords = (vec2){vinfo.xres, vinfo.yres};;
        vec2 max_coords = (vec2){0,0};

        for (int i = 0; i < 8; i++)
        {
            vec3 projection_vector = normalize_vec3(subtract_vec3(vertices[i], transformed_cam.focal_point));
            vec3 projection_point = 
                    scale_vec3(projection_vector, 
                        -transformed_cam.focal_offset/
                        dot_product_vec3(projection_vector, transformed_cam.base_z));
            max_coords.x = fmax(dot_product_vec3(projection_point, transformed_cam.base_x), max_coords.x);
            max_coords.y = fmax(dot_product_vec3(projection_point, transformed_cam.base_y), max_coords.y);
            min_coords.x = fmin(dot_product_vec3(projection_point, transformed_cam.base_x), min_coords.x);
            min_coords.y = fmin(dot_product_vec3(projection_point, transformed_cam.base_y), min_coords.y);
        }
        
        min_coords = add_vec2(min_coords, scale_vec2(transformed_cam.dimensions, 0.5));
        min_coords.x = fmax(0, min_coords.x);
        min_coords.y = fmax(0, min_coords.y);
        max_coords = add_vec2(max_coords, scale_vec2(transformed_cam.dimensions, 0.5));
        max_coords.x = fmin(vinfo.xres-1, max_coords.x);
        max_coords.y = fmin(vinfo.yres-1, max_coords.y);

        for (int j = 0; j < vinfo.yres; j++)
        {
            for (int i = 0; i < vinfo.xres; i++)
            {
                // If pixel is inside the cube's projected bounding box,
                // render ir
                if (i >=(int)min_coords.x && i <=(int)max_coords.x &&
                        j >=(int)min_coords.y && j <=(int)max_coords.y)
                {
                    if (RENDER_BOUNDING_BOX)
                    {
                        if (i == (int)min_coords.x || i == (int)max_coords.x ||
                                j == (int)min_coords.y || j == (int)max_coords.y)

                        {
                            paint_pixel(i, j, (vec4){1,1,1,1}, buffer, vinfo);
                            continue;
                        }
                    }
                    if (RENDER_OVER_TEXT)
                    {
                        vec4 color = get_pixel_through_camera(i, j, transformed_cam, light);
                        paint_pixel(i, j, color, buffer, vinfo);
                    }
                    else
                    {
                        // Don't paint over tty text
                        vec4 fb_color = {buffer[(j*vinfo.xres+i)*4],
                            buffer[(j*vinfo.xres+i)*4+1],
                            buffer[(j*vinfo.xres+i)*4+2],
                            buffer[(j*vinfo.xres+i)*4+3]};
                        if (fb_color.x == 0 &&
                                fb_color.y == 0 &&
                                fb_color.z == 0)
                        {
                            vec4 color = get_pixel_through_camera(i, j, transformed_cam, light);
                            paint_pixel(i, j, color, buffer, vinfo);
                        }
                        // Also repaint previously painted pixels
                        // (marked by w = 87)
                        else if (fb_color.w == 87)
                        {
                            vec4 color = get_pixel_through_camera(i, j, transformed_cam, light);
                            paint_pixel(i, j, color, buffer, vinfo);
                        }
                    }
                }
                // If pixel is outside the bounding box, just paint it black
                else 
                {
                    if (RENDER_OVER_TEXT)
                    {
                        paint_pixel(i, j, (vec4){0,0,0,0}, buffer, vinfo);
                    }
                    else
                    {
                        vec4 fb_color = {buffer[(j*vinfo.xres+i)*4],
                            buffer[(j*vinfo.xres+i)*4+1],
                            buffer[(j*vinfo.xres+i)*4+2],
                            buffer[(j*vinfo.xres+i)*4+3]};
                        if (fb_color.w == 87)
                        {
                            paint_pixel(i, j, (vec4){0,0,0,0}, buffer, vinfo);
                        }
                    }
                }
            }
        }

        // Somewhat remove aliasing applying a gaussian blur
        if (BLUR_ANTIALIAS)
        {
            blur_pixels(buffer, min_coords, max_coords, vinfo.xres, vinfo.yres);
        }

        // For some reason, the fb doesn't update fast enough
        // unless we print something first
        printf("\r");
        fflush(stdout);
        memcpy(fbp, buffer, 4 * vinfo.xres * vinfo.yres);
        
        clock_gettime(CLOCK_MONOTONIC_RAW, &end);
        delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;

        if (FRAME_LIMIT > 0)
        {
            if (delta_us < 1000000.0 / FRAME_LIMIT)
            {
                usleep(1000000.0 / FRAME_LIMIT - delta_us);
                delta = 1.0 / FRAME_LIMIT;
            }
            else
            {
                delta = delta_us/1000000.0;
            }
        }
        else
        {
            delta = delta_us/1000000.0;
        }
    }
    munmap(fbp, screensize);
    close(fbfd);
    free(buffer);

    return 0;
}
