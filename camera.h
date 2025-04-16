typedef struct camera
{
    double focal_offset; // Distance along the Z axis between the camera 
                         // center and the focal point. Use negative values
                         // This kinda works like FOV in games

    float time; // Self explanatory
    vec2 dimensions;

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
    vec2 center_offset;
    vec3 base_x;
    vec3 base_y;
    vec3 base_z;
    vec3 center_point;
    vec3 focal_point;
} camera;


camera setup_camera(camera camera)
{
    // Get distance from the center of the camera to its
    // top left pixel
    camera.center_offset = scale_vec2(camera.dimensions, 0.5);

    // Apply translations
    camera.focal_point = camera.translations;

    // Apply rotations 
    // We initialize our vector basis as normalized vectors
    // in each axis * our deformations vector
    camera.base_x = (vec3){camera.deformations.x, 0.0, 0.0};
    camera.base_y = (vec3){0.0, camera.deformations.y, 0.0};
    camera.base_z = (vec3){0.0, 0.0, camera.deformations.z};


    // Then we rotate them around following our rotations vector:
    // First save these values to avoid redundancy
    float cosx = cos(camera.rotations.x);
    float cosy = cos(camera.rotations.y);
    float cosz = cos(camera.rotations.z);
    float sinx = sin(camera.rotations.x);
    float siny = sin(camera.rotations.y);
    float sinz = sin(camera.rotations.z);
    
    // Declare a buffer vector we will use to apply multiple changes at once
    vec3 tmp = {0, 0, 0};

    // Rotations for base_x:
    tmp = camera.base_x;
    // X axis:
    tmp.y =  camera.base_x.y * cosx - camera.base_x.z * sinx;
    tmp.z =  camera.base_x.y * sinx + camera.base_x.z * cosx;
    camera.base_x = tmp;
    // Y axis:
    tmp.x =  camera.base_x.x * cosy + camera.base_x.z * siny;
    tmp.z = -camera.base_x.x * siny + camera.base_x.z * cosy;
    camera.base_x = tmp;
    // Z axis:
    tmp.x =  camera.base_x.x * cosz - camera.base_x.y * sinz;
    tmp.y =  camera.base_x.x * sinz + camera.base_x.y * cosz;
    camera.base_x = tmp;

    // Rotations for base_y:
    tmp = camera.base_y;
    // X axis:
    tmp.y =  camera.base_y.y * cosx - camera.base_y.z * sinx;
    tmp.z =  camera.base_y.y * sinx + camera.base_y.z * cosx;
    camera.base_y = tmp;
    // Y axis:
    tmp.x =  camera.base_y.x * cosy + camera.base_y.z * siny;
    tmp.z = -camera.base_y.x * siny + camera.base_y.z * cosy;
    camera.base_y = tmp;
    // Z axis:
    tmp.x =  camera.base_y.x * cosz - camera.base_y.y * sinz;
    tmp.y =  camera.base_y.x * sinz + camera.base_y.y * cosz;
    camera.base_y = tmp;

    // Rotations for base_z: 
    tmp = camera.base_z;
    // X axis:
    tmp.y =  camera.base_z.y * cosx - camera.base_z.z * sinx;
    tmp.z =  camera.base_z.y * sinx + camera.base_z.z * cosx;
    camera.base_z = tmp;
    // Y axis:
    tmp.x =  camera.base_z.x * cosy + camera.base_z.z * siny;
    tmp.z = -camera.base_z.x * siny + camera.base_z.z * cosy;
    camera.base_z = tmp;
    // Z axis:
    tmp.x =  camera.base_z.x * cosz - camera.base_z.y * sinz;
    tmp.y =  camera.base_z.x * sinz + camera.base_z.y * cosz;
    camera.base_z = tmp;

    // Now that we have our transformed 3d orthonormal base 
    // we can calculate our focal point 
    camera.center_point = add_vec3(camera.focal_point, scale_vec3(camera.base_z, -camera.focal_offset));

    // Return our set up camera
    return camera;
}


// Gets a pixel from the end of a ray projected to an axis
vec4 get_pixel_from_projection(
    float t, int face, camera camera, vec3 focal_vector, light3 light,
    float cube_rotation_y, vec3 local_focal_point, vec3 local_focal_vector
) {
    if (t < 1) {
        return (vec4){0, 0, 0, 0};
    }

    // Intersection in local (cube) space
    vec3 intersection_local = add_vec3(
        scale_vec3(local_focal_vector, t), local_focal_point
    );

    // Rotate intersection back to world space
    vec3 intersection = rotate_vec3_y(intersection_local, cube_rotation_y);

    // Save necessary coordinates and normal vector
    vec2 cam_coords;
    vec3 normal_local;
    switch (face) {
        case 0:
            cam_coords = (vec2){intersection_local.x, intersection_local.y};
            normal_local = (vec3){0, 0, 1};
            break;
        case 1:
            cam_coords = (vec2){intersection_local.x, intersection_local.y};
            normal_local = (vec3){0, 0, -1};
            break;
        case 2:
            cam_coords = (vec2){intersection_local.z, intersection_local.y};
            normal_local = (vec3){1, 0, 0};
            break;
        case 3:
            cam_coords = (vec2){intersection_local.z, intersection_local.y};
            normal_local = (vec3){-1, 0, 0};
            break;
        case 4:
            cam_coords = (vec2){intersection_local.z, intersection_local.x};
            normal_local = (vec3){0, 1, 0};
            break;
        case 5:
            cam_coords = (vec2){intersection_local.z, intersection_local.x};
            normal_local = (vec3){0, -1, 0};
            break;
    }
    cam_coords.x += SIDE_LENGTH / 2;
    cam_coords.y += SIDE_LENGTH / 2;

    vec4 pixel;
    if (cam_coords.x > SIDE_LENGTH - 1 ||
        cam_coords.y > SIDE_LENGTH - 1 ||
        cam_coords.x < 0 || cam_coords.y < 0) {
        return (vec4){0, 0, 0, 0};
    } else if (cam_coords.x > SIDE_LENGTH - EDGE_THICKNESS - 1 ||
               cam_coords.y > SIDE_LENGTH - EDGE_THICKNESS - 1 ||
               cam_coords.x < EDGE_THICKNESS || cam_coords.y < EDGE_THICKNESS) {
        pixel = EDGE_COLOR;
    } else {
        pixel = SHADER(cam_coords, face);
    }

    // Rotate normal to world space for shading
    vec3 normal = rotate_vec3_y(normal_local, cube_rotation_y);

    // Apply shading
    if (SHADING) {
        // Rotate light into local space
vec3 local_light_position = rotate_vec3_y(light.position, -cube_rotation_y);

// Diffuse lighting
double base_light = 0.2;
vec3 incident = normalize_vec3(subtract_vec3(local_light_position, intersection_local));
double dot = dot_product_vec3(incident, normal_local);
vec3 diffuse = scale_vec3(light.color, (fmin(dot, 0) - base_light) / (-1 - base_light));
pixel.x *= diffuse.x;
pixel.y *= diffuse.y;
pixel.z *= diffuse.z;

// Specular highlight
if (SPECULAR_HIGHLIGHT) {
    double smoothness = 0.2;
    vec3 view_dir = normalize_vec3(scale_vec3(local_focal_vector, -1));
    vec3 reflected = subtract_vec3(incident, scale_vec3(normal_local, 2 * dot));
    double spec = fmax(0, -dot_product_vec3(view_dir, reflected));
    vec3 highlight = scale_vec3(light.color, pow(spec, smoothness * 100));
    pixel.x += highlight.x;
    pixel.y += highlight.y;
    pixel.z += highlight.z;
}
pixel.w = 1;
    }

    return pixel;
}

vec4 alpha_composite(vec4 color1, vec4 color2) {
    float ar = color1.w + color2.w - (color1.w * color2.w);
    float asr = color2.w / ar;
    float a1 = 1 - asr;
    float a2 = asr * (1 - color1.w);
    float ab = asr * color1.w;
    vec4 outcolor;
    outcolor.x = color1.x * a1 + color2.x * a2 + color2.x * ab;
    outcolor.y = color1.y * a1 + color2.y * a2 + color2.y * ab;
    outcolor.z = color1.z * a1 + color2.z * a2 + color2.z * ab;
    outcolor.w = ar;
    return outcolor;
}

vec4 get_pixel_through_camera(int x, int y, camera camera, light3 light) {
    // Offset coords
    x -= camera.center_offset.x;
    y -= camera.center_offset.y;

    // Find the pixel 3d position using the camera vector basis
    vec3 pixel_3dposition = add_vec3(
        camera.center_point,
        add_vec3(
            scale_vec3(camera.base_x, x),
            scale_vec3(camera.base_y, y)
        )
    );

    // Get the vector going from the focal point to the pixel in 3d space
    vec3 focal_vector = subtract_vec3(pixel_3dposition, camera.focal_point);

    // --- CUBE ROTATION ---
    float cube_rotation_y = camera.time*4*PI/1000; // or any function of time

    // Rotate the ray into the cube's local space
    vec3 local_focal_point = rotate_vec3_y(camera.focal_point, -cube_rotation_y);
    vec3 local_focal_vector = rotate_vec3_y(focal_vector, -cube_rotation_y);

    // Cube face planes (in local space)
    static const float a[] = {0, 0, 1, 1, 0, 0};
    static const float b[] = {0, 0, 0, 0, 1, 1};
    static const float c[] = {1, 1, 0, 0, 0, 0};
    static const float d[] = {
        -SIDE_LENGTH / 2.0, SIDE_LENGTH / 2.0 - 1,
        -SIDE_LENGTH / 2.0, SIDE_LENGTH / 2.0 - 1,
        -SIDE_LENGTH / 2.0, SIDE_LENGTH / 2.0 - 1
    };

    vec2 t[6];
    vec4 projection_pixels[6];
    vec4 blended_pixels = (vec4){0, 0, 0, 0};
    int last_valid_t = 0;

    for (int i = 0; i < 6; i++) {
        t[i].x = (d[i]
                  - a[i] * local_focal_point.x
                  - b[i] * local_focal_point.y
                  - c[i] * local_focal_point.z)
                 / (a[i] * local_focal_vector.x
                    + b[i] * local_focal_vector.y
                    + c[i] * local_focal_vector.z);
        t[i].y = i;

        projection_pixels[i] = get_pixel_from_projection(
            t[i].x, round(t[i].y), camera, focal_vector, light,
            cube_rotation_y, local_focal_point, local_focal_vector
        );

        if (projection_pixels[i].w > 0) {
            if (t[i].x > t[last_valid_t].x) {
                if (!SHADING) {
                    blended_pixels = alpha_composite(
                        projection_pixels[i], projection_pixels[last_valid_t]
                    );
                } else {
                    blended_pixels = projection_pixels[last_valid_t];
                }
            } else if (t[i].x < t[last_valid_t].x) {
                if (!SHADING) {
                    blended_pixels = alpha_composite(
                        projection_pixels[last_valid_t], projection_pixels[i]
                    );
                } else {
                    blended_pixels = projection_pixels[i];
                }
            }
            last_valid_t = i;
        }
    }
    return blended_pixels;
}

vec2 project_vertex_to_screen(vec3 vertex, camera cam) {
    // Vector from focal point to vertex
    vec3 focal_vector = subtract_vec3(vertex, cam.focal_point);

    // Project onto camera plane (find t where the ray hits the camera plane)
    // The camera plane is at cam.center_point, normal is cam.base_z
    double denom = dot_product_vec3(focal_vector, cam.base_z);
    if (fabs(denom) < 1e-6) denom = 1e-6; // Avoid division by zero

    double t = -cam.focal_offset / denom;

    // Intersection point on camera plane
    vec3 intersection = add_vec3(cam.focal_point, scale_vec3(focal_vector, t));

    // Offset from center_point in camera basis
    vec3 offset = subtract_vec3(intersection, cam.center_point);

    // Project onto camera's x and y basis
    double x = dot_product_vec3(offset, cam.base_x);
    double y = dot_product_vec3(offset, cam.base_y);

    // Convert to screen coordinates (centered)
    x += cam.dimensions.x * 0.5;
    y += cam.dimensions.y * 0.5;

    return (vec2){x, y};
}

