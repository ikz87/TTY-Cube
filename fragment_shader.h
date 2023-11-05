// Shader that gets applied to every face of the cube
// Change it to whatever you want
// Your resolution is SIDE_LENGTH by SIDE_LENGTH
vec4 fragment_shader(vec2 fragcoord)
{
    vec4 pixel = (vec4){fragcoord.x/SIDE_LENGTH, fragcoord.y/SIDE_LENGTH, 1, 0.8};
    return pixel;
}
