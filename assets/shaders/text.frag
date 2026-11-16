#version 460

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D glyphBitmap;

void main()
{
    FragColor = texture(glyphBitmap, TexCoord);
}
