#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D charTexture;
uniform vec3 textColor;

void main()
{
    float alpha = texture(charTexture, TexCoords).r;
    FragColor = vec4(textColor, alpha);
}
