#version 460

layout (location = 0) in vec2 inPos;

out vec2 TexCoords;

void main()
{
    gl_Position = vec4(inPos, 0.0f, 1.0f);
    TexCoords = inPos / 2.0f + 0.5f;
}
