#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex;

out vec3 fragPos;
out vec2 TexCoord;

uniform mat4x4 transform;
uniform mat4x4 scale;
uniform mat4x4 view;
uniform mat4x4 project;

void main()
{
   TexCoord = aTex;
   fragPos = vec3( transform * scale * vec4(aPos, 1.0f));
   gl_Position = project * view * transform * scale * vec4(aPos, 1.0f);
}
