typedef struct camera
{
    double focal_offset; // Distance along the Z axis between the camera 
                         // center and the focal point. Use negative values
                         // This kinda works like FOV in games

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


void insertion_sort(vec2 arr[], int n)
{
    for (int i = 1; i < n; i++)
    {
        vec2 tmp = arr[i];
        int j = i - 1;

    
        while (tmp.x < arr[j].x && j >= 0)
        {
            arr[j + 1] = arr[j];
            --j;
        }
        arr[j + 1] = tmp;
    }
}

camera setup_camera(camera camera)
{
    // Get distance from the center of the camera to its
    // top left pixel
    camera.center_offset = scale_vec2(camera.dimensions, 0.5);

    // Apply translations
    camera.center_point = add_vec3(camera.center_point, camera.translations);

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
    camera.focal_point = add_vec3(camera.center_point, scale_vec3(camera.base_z, camera.focal_offset));

    // Return our set up camera
    return camera;
}


// Gets a pixel from the end of a ray projected to an axis
vec4 get_pixel_from_projection(float t, int face, camera camera, vec3 focal_vector)
{
    // If the point we end up in is behind our camera, don't "render" it
    if (t < 1)
    {
        return (vec4){0, 0, 0, 0};
    }

    // Then we multiply our focal vector by t and add our focal point to it 
    vec3 intersection = add_vec3(scale_vec3(focal_vector, t), camera.focal_point);
    

    // Save necessary coordinates
    // (different cube faces need different coords)
    vec2 cam_coords;
    switch (face) 
    {
        case 0:
            cam_coords = (vec2){intersection.x, intersection.y};
            break;
        case 1:
            cam_coords = (vec2){intersection.x, intersection.y};
            break;
        case 2:
            cam_coords = (vec2){intersection.z, intersection.y};
            break;
        case 3:
            cam_coords = (vec2){intersection.z, intersection.y};
            break;
        case 4:
            cam_coords = (vec2){intersection.z, intersection.x};
            break;
        case 5:
            cam_coords = (vec2){intersection.z, intersection.x};
            break;
    }
    cam_coords.x += SIDE_LENGTH/2;
    cam_coords.y += SIDE_LENGTH/2;
    
    // If pixel is outside of the region occupied by the cube
    // return a completely transparent color
    if (cam_coords.x >=SIDE_LENGTH || 
        cam_coords.y >=SIDE_LENGTH ||
        cam_coords.x <0 || cam_coords.y <0)
    {
        return (vec4){0,0,0,0};
    }
    // Make edges a different color
    else if (cam_coords.x >=SIDE_LENGTH-EDGE_THICKNESS || 
            cam_coords.y >=SIDE_LENGTH-EDGE_THICKNESS ||
            cam_coords.x < EDGE_THICKNESS || cam_coords.y < EDGE_THICKNESS)
    {
        return EDGE_COLOR;
    }

    // Fetch pixel
    vec4 pixel = fragment_shader(cam_coords, face);

    return pixel;
}

// Combines colors using alpha
// Got this from https://stackoverflow.com/questions/64701745/how-to-blend-colours-with-transparency
// Not sure how it works honestly lol
vec4 alpha_composite(vec4 color1, vec4 color2)
{
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


// Gets a pixel through the camera using coords as coordinates in
// the camera plane
vec4 get_pixel_through_camera(vec2 coords, camera camera)
{
    // Offset coords
    coords = subtract_vec2(coords, camera.center_offset);

    // Find the pixel 3d position using the camera vector basis
    vec3 pixel_3dposition = add_vec3(camera.center_point, 
                            add_vec3(scale_vec3(camera.base_x, coords.x),
                                    scale_vec3(camera.base_y, coords.y)));

    // Get the vector going from the focal point to the pixel in 3d sapace
    vec3 focal_vector = subtract_vec3(pixel_3dposition, camera.focal_point);

    // We need 6 planes, one for each face of the cube, they all follow the plane EQ
    // ax + by + cz + d
    static const float a[] = {0,0,
                 1,1,
                 0,0};
    static const float b[] = {0,0,
                 0,0,
                 1,1};
    static const float c[] = {1,1,
                 0,0,
                 0,0};
    static const float d[] = {-SIDE_LENGTH/2.0,SIDE_LENGTH/2.0,
                 -SIDE_LENGTH/2.0,SIDE_LENGTH/2.0,
                 -SIDE_LENGTH/2.0,SIDE_LENGTH/2.0};

    // Then there's a line going from our focal point to each of the planes 
    // which we can describe as:
    // x(t) = focal_point.x + focal_vector.x * t
    // y(t) = focal_point.y + focal_vector.y * t
    // z(t) = focal_point.z + focal_vector.z * t
    // We substitute x, y and z with x(t), y(t) and z(t) in the plane EQ
    // Solving for t we get:
    vec2 t[6]; // we use a vec2 to also store the plane that was hit
    for (int i = 0; i < 6; i++)
    {
        t[i].x = (d[i] 
                - a[i]*camera.focal_point.x 
                - b[i]*camera.focal_point.y 
                - c[i]*camera.focal_point.z)
            / (a[i]*focal_vector.x 
                    + b[i]*focal_vector.y 
                    + c[i]*focal_vector.z);
        t[i].y = i;
    }
    
    // Bubble sort to know which intersections happen first
    /*
    for (int i = 0; i < 6; i++)
    {
        for (int j = 0; j < 5; j++)
        {
            if (t [j].x > t[j+1].x)
            {
                vec2 tmp = t[j];
                t[j] = t[j+1];
                t[j+1] = tmp;
            }
        }
    }
    */
    insertion_sort(t, 6); 


    // Then we go through each one of the intersections in order 
    // and mix pixels together using alpha
    vec4 blended_pixels = (vec4){0,0,0,0};
    for (int i = 0; i < 6; i++)
    {
        // We get the pixel through projection
        vec4 projection_pixel = get_pixel_from_projection(t[i].x, 
                                                          round(t[i].y),
                                                          camera,
                                                          focal_vector);
        // Only blend non fully transparent pixels
        if (projection_pixel.w > 0.0)
        {
            // Blend the pixel using alpha
            blended_pixels = alpha_composite(projection_pixel, blended_pixels);
        }
    }
    return blended_pixels;
}
