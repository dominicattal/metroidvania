#version 460

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D spriteTextures;

void main()
{
    FragColor = texture(spriteTextures, TexCoord);
    //FragColor = vec4(1.0f, 1.0f, 0.0f, 1.0f);
}
