#version 460

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D glyphBitmap;
uniform vec4 color;

void main()
{
    FragColor = texture(glyphBitmap, TexCoord);
    FragColor.x = color.x;
    FragColor.y = color.y;
    FragColor.z = color.z;
    FragColor.w *= color.w;
}
