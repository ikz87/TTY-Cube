#define PI 3.14159265

#ifdef IMAGE
unsigned char image_data[SIDE_LENGTH*SIDE_LENGTH*3];
#endif

// Shaders that can apply to every face of the cube
// Change it to whatever you want
// Your resolution is SIDE_LENGTH by SIDE_LENGTH
vec4 solid_white(vec2 fragcoord, int face)
{
    vec4 pixel = (vec4){1, 1, 1, 1};
    return pixel;
}


vec4 gradient(vec2 fragcoord, int face)
{
    vec4 pixel = (vec4){fragcoord.x/SIDE_LENGTH, fragcoord.y/SIDE_LENGTH, 1, 0.8};
    return pixel;
}

vec4 checker_pattern(vec2 fragcoord, int face)
{
    int x = fragcoord.x;
    int y = fragcoord.y;
    int n = 8;
    double value = (((((x*n)/SIDE_LENGTH)+((y*n)/SIDE_LENGTH)%2))%2);
    vec4 pixel = (vec4){0, value/2, value, 0.8+value/8};
    return pixel;
}

#ifdef IMAGE
vec4 image(vec2 fragcoord, int face)
{
    int value = (round(fragcoord.y) * SIDE_LENGTH + round(fragcoord.x))*3;
    vec4 pixel;
    pixel.x = image_data[value];
    pixel.y = image_data[value+1];
    pixel.z = image_data[value+2];
    pixel.w = 255;
    pixel = scale_vec4(pixel, 1.0/255.0);
    return pixel;
}
#endif
