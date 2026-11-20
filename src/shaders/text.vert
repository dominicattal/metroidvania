#version 460

layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec2 inTexCoord;

layout (std140) uniform Window {
    vec2 resolution;
    vec2 center;
    float time;
} window;

out vec2 TexCoord;

void main()
{
    TexCoord = inTexCoord;
    gl_Position = vec4(
            (inPosition.x / window.resolution.x - 0.5f) * 2.0f,
            (inPosition.y / window.resolution.y - 0.5f) * 2.0f,
            0.0f, 1.0f);
}
