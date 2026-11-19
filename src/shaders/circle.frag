#version 460

out vec4 FragColor;

in vec2 TexCoord;

void main()
{
    float x = length(TexCoord);
    if (x > 0.5)
        discard;
    FragColor = vec4(1.0f, 0.0f, 1.0f, 1.0f);
}
