#version 460

out vec4 FragColor;

in vec2 TexCoord;

layout (std140) uniform Window {
    vec2 resolution;
    float time;
} window;

void main()
{
    FragColor = vec4(1.0f, 1.0f, 0.0f, 1.0f);
}
