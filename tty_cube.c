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
    sigaction(SIGTERM, &action, NULL);
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
        light = (light3){(vec3){1,1,1},
            (vec3){cos(time_cyclic*PI)*SIDE_LENGTH,
                SIDE_LENGTH*sin(2*time_cyclic*PI)/2,
                sin(time_cyclic*PI)*SIDE_LENGTH}};


        for (int j = 0; j < vinfo.yres; j++)
        {
            for (int i = 0; i < vinfo.xres; i++)
            {
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
