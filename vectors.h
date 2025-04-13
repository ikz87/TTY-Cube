typedef struct vec2 {
    double x;
    double y;
} vec2;

typedef struct vec3 {
    double x;
    double y;
    double z;
} vec3;

typedef struct vec4 {
    double x;
    double y;
    double z;
    double w;
} vec4;

double length_vec2(vec2 a) {
    return sqrt(pow(a.x, 2) + pow(a.y, 2));
}

double length_vec3(vec3 a) {
    return sqrt(pow(a.x, 2) + pow(a.y, 2) + pow(a.z, 2));
}

double length_vec4(vec4 a) {
    return sqrt(pow(a.x, 2) + pow(a.y, 2) + pow(a.z, 2) + pow(a.w, 2));
}

void print_vec2(vec2 a) {
    printf("x: %f, y: %f\n", a.x, a.y);
}

void print_vec3(vec3 a) {
    printf("x: %f, y: %f, z: %f\n", a.x, a.y, a.z);
}

void print_vec4(vec4 a) {
    printf("x: %f, y: %f, z: %f, w: %f\n", a.x, a.y, a.z, a.w);
}

vec2 scale_vec2(vec2 a, double b) {
    vec2 result;
    result.x = a.x * b;
    result.y = a.y * b;
    return result;
}

vec3 scale_vec3(vec3 a, double b) {
    vec3 result;
    result.x = a.x * b;
    result.y = a.y * b;
    result.z = a.z * b;
    return result;
}

vec4 scale_vec4(vec4 a, double b) {
    vec4 result;
    result.x = a.x * b;
    result.y = a.y * b;
    result.z = a.z * b;
    result.w = a.w * b;
    return result;
}

vec2 add_vec2(vec2 a, vec2 b) {
    vec2 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

vec3 add_vec3(vec3 a, vec3 b) {
    vec3 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}

vec4 add_vec4(vec4 a, vec4 b) {
    vec4 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;
    return result;
}

vec2 subtract_vec2(vec2 a, vec2 b) {
    vec2 neg_b = scale_vec2(b, -1);
    return add_vec2(a, neg_b);
}

vec3 subtract_vec3(vec3 a, vec3 b) {
    vec3 neg_b = scale_vec3(b, -1);
    return add_vec3(a, neg_b);
}

vec4 subtract_vec4(vec4 a, vec4 b) {
    vec4 neg_b = scale_vec4(b, -1);
    return add_vec4(a, neg_b);
}

vec2 normalize_vec2(vec2 a) {
    double length = length_vec2(a);
    if (length != 0) {
        a.x /= length;
        a.y /= length;
    }
    return a;
}

vec3 normalize_vec3(vec3 a) {
    double length = length_vec3(a);
    if (length != 0) {
        a.x /= length;
        a.y /= length;
        a.z /= length;
    }
    return a;
}

vec4 normalize_vec4(vec4 a) {
    double length = length_vec4(a);
    if (length != 0) {
        a.x /= length;
        a.y /= length;
        a.z /= length;
        a.w /= length;
    }
    return a;
}

vec2 multiply_vec2(vec2 a, vec2 b) {
    a.x *= b.x;
    a.y *= b.y;
    return a;
}

vec3 multiply_vec3(vec3 a, vec3 b) {
    a.x *= b.x;
    a.y *= b.y;
    a.z *= b.z;
    return a;
}

vec4 multiply_vec4(vec4 a, vec4 b) {
    a.x *= b.x;
    a.y *= b.y;
    a.z *= b.z;
    a.w *= b.w;
    return a;
}

double dot_product_vec2(vec2 a, vec2 b) {
    return a.x * b.x + a.y * b.y;
}

double dot_product_vec3(vec3 a, vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

double dot_product_vec4(vec4 a, vec4 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

vec2 rotate_vec2(vec2 v, double angle) {
    vec2 result;
    double cos_a = cos(angle);
    double sin_a = sin(angle);
    result.x = v.x * cos_a - v.y * sin_a;
    result.y = v.x * sin_a + v.y * cos_a;
    return result;
}

vec3 rotate_vec3_x(vec3 v, double angle) {
    vec3 result;
    double cos_a = cos(angle);
    double sin_a = sin(angle);
    result.x = v.x;
    result.y = v.y * cos_a - v.z * sin_a;
    result.z = v.y * sin_a + v.z * cos_a;
    return result;
}

vec3 rotate_vec3_y(vec3 v, double angle) {
    vec3 result;
    double cos_a = cos(angle);
    double sin_a = sin(angle);
    result.x = v.x * cos_a + v.z * sin_a;
    result.y = v.y;
    result.z = -v.x * sin_a + v.z * cos_a;
    return result;
}

vec3 rotate_vec3_z(vec3 v, double angle) {
    vec3 result;
    double cos_a = cos(angle);
    double sin_a = sin(angle);
    result.x = v.x * cos_a - v.y * sin_a;
    result.y = v.x * sin_a + v.y * cos_a;
    result.z = v.z;
    return result;
}
