#include <fcntl.h>      // For open
#include <linux/fb.h>   // For FBIOGET_VSCREENINFO
#include <sys/ioctl.h>  // For ioctl
#include <sys/mman.h>   // For mmap
#include <unistd.h>     // For close, and other system functions
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <linux/fb.h>
#include "config.h"
#include "vectors.h"
#include "fragment_shader.h"
#include "camera.h"


#define PI 3.14159265


int main(int argc, char *argv[])
{
    const char* fbdev = "/dev/fb0";  // Adjust the path to your framebuffer device if needed

    int fbfd = open(fbdev, O_RDWR);
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

    while (1)
    {
        time++;
        time_cyclic = (time%100)/50.0;
        camera rotate_around_origin =
        {-SIDE_LENGTH*4,
            (vec2){vinfo.xres, vinfo.yres},
            (vec3){PI/6*sin(2*time_cyclic*PI),-time_cyclic*PI-PI/2,0},
            (vec3){cos(time_cyclic*PI)*SIDE_LENGTH*4,
                SIDE_LENGTH*2*sin(2*time_cyclic*PI),
                sin(time_cyclic*PI)*SIDE_LENGTH*4},
            (vec3){1,1,1},

            (vec2){0,0},
            (vec3){0,0,0},
            (vec3){0,0,0},
            (vec3){0,0,0},
            (vec3){0,0,0},
            (vec3){0,0,0}};
        camera transformed_cam = setup_camera(rotate_around_origin);

        for (int j = 0; j < vinfo.yres; j++)
        {
            for (int i = 0; i < vinfo.xres; i++)
            {
                vec2 coords = {i, j};
                vec4 color = get_pixel_through_camera(coords, transformed_cam);
                fbp[(j*vinfo.xres+i)*4] = (unsigned int)(color.x * 255);
                fbp[(j*vinfo.xres+i)*4+1] = (unsigned int)(color.y * 255);
                fbp[(j*vinfo.xres+i)*4+2] = (unsigned int)(color.z * 255);
                fbp[(j*vinfo.xres+i)*4+3] = (unsigned int)(color.w * 255);

            }
        }
    }
    munmap(fbp, screensize);
    close(fbfd);

    return 0;
}
