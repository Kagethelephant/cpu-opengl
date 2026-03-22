#version 330 core
#define MAX_LIGHTS 32

uniform int lightCount;
uniform vec3 lightPos[MAX_LIGHTS];
uniform vec3 lightCol[MAX_LIGHTS];
uniform vec3 objCol;
uniform uint hasTexture;
uniform sampler2D diffuseTex;

in vec3 fragPos;
in vec2 TexCoord;

out vec4 FragColor;

void main()
{

   float ambientStrength = 0.2;
   vec3 lightSum = vec3(0.0);
   vec4 sampleColor = vec4(0,0,0,0);


   for(int i = 0; i < lightCount; i ++){

      vec3 normal = normalize(cross(dFdx(fragPos), dFdy(fragPos)));
      vec3 lightDir = normalize(lightPos[i] - fragPos); 

      // Ambient is basically a min light value for the fragment
      vec3 ambient = ambientStrength * lightCol[i];
      // Diffuse is scaled based on how much the triangle is pointing at the light
      vec3 diffuse = max(dot(normal, lightDir), 0.0) * lightCol[i];

      // Sum light contributions from all lights
      lightSum += diffuse + ambient;
   }

   // If the object does not have a texture than use its solid color
   if (hasTexture == 1u){
      sampleColor = texture(diffuseTex, TexCoord);
   }
   else {
      sampleColor = vec4(objCol,1.0);
   }
   
   // Multiply the light by the sample color to shade object
   FragColor = vec4(lightSum,1.0) * sampleColor;
}
