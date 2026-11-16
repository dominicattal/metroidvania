#version 460

layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec2 inOffset;
layout (location = 2) in float inRadius;

layout (std140) uniform Window {
    vec2 resolution;
} window;

out vec2 TexCoord;

void main()
{
    TexCoord = inPosition;
    gl_Position = vec4(((inOffset + 2 * inPosition * inRadius) / window.resolution - 0.5f) * 2.0f, 0.0f, 1.0f);
};
