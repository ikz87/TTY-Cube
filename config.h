#define FB_DEVICE "/dev/fb0"
#define RENDER_OVER_TEXT 0
#define RENDER_BOUNDING_BOX 0
#define BLUR_ANTIALIAS 1
#define SHADING 1
#define SPECULAR_HIGHLIGHT 1 // SHADING has to be on for this to work
#define SPEED 10
#define SIDE_LENGTH 800
#define EDGE_THICKNESS 50
#define EDGE_COLOR (vec4){1,1,1,1}
#define SHADER checker_pattern
// Supported shaders:
// solid_white
// gradient
// checker_pattern
// image
// ------------------
// Check fragment_shaders.h for more info

// To use an image on the faces of the cube:
// 1. Set SHADER to image
// 2. Run `./setup_image.sh <path_to_image>`
// 3. Uncomment the line below
// #define IMAGE "image.dat"
