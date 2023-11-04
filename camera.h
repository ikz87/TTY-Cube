#include "vectors.h"

typedef struct
{
    double focal_offset; // Distance along the Z axis between the camera 
                         // center and the focal point. Use negative values
                         // This kinda works like FOV in games

    // Transformations 
    // Use these to modify the coordinate system of the camera plane
    vec3 rotations; // Rotations in radians around each axis 
                    // The camera plane rotates around 
                    // its center point, not the origin

    vec3 translations; // Translations in pixels along each axis

    vec3 deformations; // Deforms the camera. Higher values on each axis
                       // means the window will be squished on that axis

    // ---------------------------------------------------------------// 
    
    // These will be set later with setup_camera()
    vec3 base_x;
    vec3 base_y;
    vec3 base_z;
    vec3 center_point;
    vec3 focal_point;
} pinhole_camera;


