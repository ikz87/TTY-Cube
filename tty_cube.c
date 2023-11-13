#include <fcntl.h>      // For open
#include <linux/fb.h>   // For FBIOGET_VSCREENINFO
#include <sys/ioctl.h>  // For ioctl
#include <sys/mman.h>   // For mmap
#include <unistd.h>     // For close, and other system functions
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <signal.h>
#include <linux/fb.h>
#include "config.h"
#include "vectors.h"
#include "fragment_shaders.h"
#include "light.h"
#include "camera.h"
#include "blur.h"


#define PI 3.14159265

volatile sig_atomic_t done = 0;
 
void term(int signum)
{
    done = 1;
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

    int time = 0;
    double time_cyclic = 0;
    char buffer[vinfo.xres * vinfo.yres * 4];

    // Save the current state of the fb
    memcpy(buffer, fbp, 4 * vinfo.xres * vinfo.yres);

    // Declare camera and light
    camera rotate_around_origin;
    light3 light;

    // Get image data
    #ifdef IMAGE
    FILE* image_file = fopen(IMAGE, "r");
    fread(image_data, SIDE_LENGTH*SIDE_LENGTH, 3, image_file);
    #endif

    while (!done)
    {
        time++;
        time_cyclic = (time%(1000/SPEED))/(1000/(SPEED)/2.0);

        // Define the camera
        rotate_around_origin = (camera){-SIDE_LENGTH,
            (vec2){vinfo.xres, vinfo.yres},
            (vec3){PI/6*sin(2*time_cyclic*PI),-time_cyclic*PI-PI/2,0},
            (vec3){cos(time_cyclic*PI)*SIDE_LENGTH*2,
                SIDE_LENGTH*sin(2*time_cyclic*PI),
                sin(time_cyclic*PI)*SIDE_LENGTH*2},
            (vec3){1,1,1},

            (vec2){0,0},
            (vec3){0,0,0},
            (vec3){0,0,0},
            (vec3){0,0,0},
            (vec3){0,0,0},
            (vec3){0,0,0}};
        camera transformed_cam = setup_camera(rotate_around_origin);

        // Define the light source

        vec3 light_offset = (vec3){SIDE_LENGTH*3,-SIDE_LENGTH*2,SIDE_LENGTH};
        light = (light3){(vec3){1,1,1},
            add_vec3(transformed_cam.center_point,
                    add_vec3(scale_vec3(transformed_cam.base_x, light_offset.x),
                        add_vec3(scale_vec3(transformed_cam.base_y, light_offset.y),
                            scale_vec3(transformed_cam.base_z, light_offset.z))))};

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

        // Somewhat remove aliasing applying a gaussian blur
        if (BLUR_ANTIALIAS)
        {
            blur_pixels(buffer, vinfo.xres, vinfo.yres); 
        }

        // For some reason, the fb doesn't update fast enough
        // unless we print something first
        printf("\r");
        fflush(stdout);
        memcpy(fbp, buffer, 4 * vinfo.xres * vinfo.yres);
    }
    munmap(fbp, screensize);
    close(fbfd);

    return 0;
}
