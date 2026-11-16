#version 460

layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec2 inOffset;
layout (location = 2) in vec2 inSize;
layout (location = 3) in float inRotation;

layout (std140) uniform Window {
    vec2 resolution;
    float time;
} window;

out vec2 TexCoord;

void main()
{
    TexCoord = inPosition;
    float x = inSize.x * inPosition.x * cos(inRotation) - inSize.y * inPosition.y * sin(inRotation);
    float y = inSize.x * inPosition.x * sin(inRotation) + inSize.y * inPosition.y * cos(inRotation);
    gl_Position = vec4(
            ((inOffset.x + x) / window.resolution.x - 0.5f) * 2.0f,
            ((inOffset.y + y) / window.resolution.y - 0.5f) * 2.0f,
            0.0f, 1.0f);
};
