typedef struct vec2
{
    double x;
    double y;
} vec2;

typedef struct vec3
{
    double x;
    double y;
    double z;
} vec3;

typedef struct vec4
{
    double x;
    double y;
    double z;
    double w;
} vec4;

double length_vec2(vec2 a)
{
    return sqrt(pow(a.x, 2) + pow(a.y, 2));
}

double length_vec3(vec3 a)
{
    return sqrt(pow(a.x, 2) + pow(a.y, 2) + pow(a.z, 2));
}

double length_vec4(vec4 a)
{
    return sqrt(pow(a.x, 2) + pow(a.y, 2) + pow(a.z, 2) + pow(a.w, 2));
}

void print_vec2(vec2 a)
{
    printf("x: %f, y: %f\n", a.x, a.y);
}

void print_vec3(vec3 a)
{
    printf("x: %f, y: %f, z: %f\n", a.x, a.y, a.z);
}

void print_vec4(vec4 a)
{
    printf("x: %f, y: %f, z: %f, w: %f\n", a.x, a.y, a.z, a.w);
}

vec2 scale_vec2(vec2 a, double b)
{
    vec2 result;
    result.x = a.x * b;
    result.y = a.y * b;
    return result;
}

vec3 scale_vec3(vec3 a, double b)
{
    vec3 result;
    result.x = a.x * b;
    result.y = a.y * b;
    result.z = a.z * b;
    return result;
}

vec4 scale_vec4(vec4 a, double b)
{
    vec4 result;
    result.x = a.x * b;
    result.y = a.y * b;
    result.z = a.z * b;
    result.w = a.w * b;
    return result;
}

vec2 add_vec2(vec2 a, vec2 b)
{
    vec2 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

vec3 add_vec3(vec3 a, vec3 b)
{
    vec3 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}

vec4 add_vec4(vec4 a, vec4 b)
{
    vec4 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;
    return result;
}

vec2 subtract_vec2(vec2 a, vec2 b)
{
    vec2 neg_b = scale_vec2(b, -1);
    return add_vec2(a, neg_b);
}

vec3 subtract_vec3(vec3 a, vec3 b)
{
    vec3 neg_b = scale_vec3(b, -1);
    return add_vec3(a, neg_b);
}

vec4 subtract_vec4(vec4 a, vec4 b)
{
    vec4 neg_b = scale_vec4(b, -1);
    return add_vec4(a, neg_b);
}
