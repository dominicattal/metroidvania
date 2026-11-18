#version 460

out vec4 FragColor;

in vec4 Color;
in vec2 TexCoord;

uniform sampler2D spriteTextures;

void main()
{
    FragColor = texture(spriteTextures, TexCoord) * Color;
}
