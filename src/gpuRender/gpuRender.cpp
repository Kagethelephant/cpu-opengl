#include "gpuRender.hpp"
#include "RAIIWrapper.hpp"
#include "utils/data.hpp"
#include "utils/matrix.hpp"
#include "window.hpp"
#include "app/object.hpp"
#include "shaders/shader.hpp"

#include <algorithm>
#include <cstdint>
#include <glad/glad.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>

#include <string>
#include <sys/types.h>
#include <vector>


//XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// INTITIALIZE THE RENDERER
//XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
gpuRenderEngine::gpuRenderEngine(camera& cam) : m_camera{cam}, m_window{cam.getWindow()}{
   shaderProgram3D = createShaderProgram("../src/shaders/3d_vertex.glsl", "../src/shaders/3d_fragment.glsl");
}


//XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// SETUP VAO FOR RENDERING
//XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

void gpuRenderEngine::bindObject(const object& obj){
  
   gpuMesh gpuObject(obj);
   const model& mod = obj.getModel();
   GLuint& vao = gpuObject.vao;
   GLuint& vbo = gpuObject.vbo;

   glGenBuffers(1, &vbo);
   glGenVertexArrays(1, &vao);  
   // Setup the VBO using the VAO
   GLScopedVAO tempVAO(vao);
   GLScopedVBO tempVBO(vbo);
   glBufferData(GL_ARRAY_BUFFER, mod.getVerticesRaw().size() * sizeof(GLfloat), mod.getVerticesRaw().data(), GL_STATIC_DRAW);
   // 1) Shader layout location, 2) Qty of vert attributes, 3) Size of attribute, 4) normaliize btwn -1 to 1, 5)span btwn verts in bytes, 6) start of buffer
   // positions at location 0
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
   glEnableVertexAttribArray(0);
   // UVs at location 1
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
   glEnableVertexAttribArray(1);


   for (const auto& mesh : mod.getSubMeshes()) {
      gpuSubMesh gpuSub;
      GLuint& ebo = gpuSub.ebo;
      GLuint& tex = gpuSub.tex;

      gpuSub.textured = mesh.textured;

      gpuSub.indiceCount = mesh.indices.size();

      glGenBuffers(1, &gpuSub.ebo);
      glGenTextures(1, &gpuSub.tex);


      GLenum format = (mesh.tex.channels == 4) ? GL_RGBA : GL_RGB;

      GLScopedTexture2D tempTex(tex);
      glTexImage2D(GL_TEXTURE_2D, 0, format, mesh.tex.w, mesh.tex.h, 0, format, GL_UNSIGNED_BYTE, mesh.tex.data);
      glGenerateMipmap(GL_TEXTURE_2D);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      GLScopedEBO tempEBO(ebo);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(uint32_t), mesh.indices.data(), GL_STATIC_DRAW);

      gpuObject.subMeshes.push_back(gpuSub);
   }
   meshes.push_back(gpuObject);
}

//XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// DRAW MODELS AT LOCATIONS DICTATED BY OBJECTS
//XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
void gpuRenderEngine::render(){

   vec4 bgColor = hexColorToFloat(Color::Black);
   GLScopedFBO tempFBO(m_window.fbo);
   GLScopedViewport tempViewPort(0, 0, m_window.fboWidth, m_window.fboHeight);
   GLScopedProgram tempProgram(shaderProgram3D);
   glClearColor(bgColor[0],bgColor[1],bgColor[2],bgColor[3]);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   GLScopedCapability tempCullEnable(GL_CULL_FACE,true);
   GLScopedCullFace tempCullMode(GL_BACK);
   GLScopedCapability tempDepthEnable(GL_DEPTH_TEST, true);

   int lightCount = std::min(MAX_LIGHTS, (unsigned int)lights.size());

   float lightPosBuffer[MAX_LIGHTS * 3];
   float lightColBuffer[MAX_LIGHTS * 3];

   for(int i = 0; i<lightCount; i ++){
      lightPosBuffer[i*3  ] = lights[i].position.x;
      lightPosBuffer[i*3+1] = lights[i].position.y;
      lightPosBuffer[i*3+2] = lights[i].position.z;

      lightColBuffer[i*3  ] = lights[i].color.x;
      lightColBuffer[i*3+1] = lights[i].color.y;
      lightColBuffer[i*3+2] = lights[i].color.z;
   }

   for (const gpuMesh& mesh : meshes){
      
      const object& obj = mesh.obj;
      const GLuint& vao = mesh.vao;
      const GLuint& vbo = mesh.vbo;

      vec4 color = hexColorToFloat(obj.getColor());

      // Setup the VBO using the VAO
      GLScopedVAO tempVAO(vao);
      GLScopedVBO tempVBO(vbo);


      // update the uniform color
      glUniformMatrix4fv(glGetUniformLocation(shaderProgram3D, "view"),1,GL_FALSE,&m_camera.getViewMatrix().m[0][0]);
      glUniformMatrix4fv(glGetUniformLocation(shaderProgram3D, "project"),1,GL_FALSE,&m_camera.getProjectionMatrix().m[0][0]);

      glUniform1i(glGetUniformLocation(shaderProgram3D, "lightCount"),lightCount);
      glUniform3fv(glGetUniformLocation(shaderProgram3D, "lightPos"),lightCount,&lightPosBuffer[0]);
      glUniform3fv(glGetUniformLocation(shaderProgram3D, "lightCol"),lightCount,&lightColBuffer[0]);

      glUniform3fv(glGetUniformLocation(shaderProgram3D, "objCol"),1,&color[0]);
      glUniformMatrix4fv(glGetUniformLocation(shaderProgram3D, "scale"),1,GL_FALSE,&obj.getScaleMatrix().m[0][0]);
      glUniformMatrix4fv(glGetUniformLocation(shaderProgram3D, "transform"),1,GL_FALSE,&obj.getTransformMatrix().m[0][0]);
      
      // Activate this texture here so we dont have to do it for every sub mesh
      GLScopedActiveTexture tempActiveTex(GL_TEXTURE0);

      for (const gpuSubMesh& sub : mesh.subMeshes) {

         const GLuint& ebo = sub.ebo;
         const GLuint& tex = sub.tex;

         GLScopedEBO tempEBO(ebo);
         GLScopedTexture2D tempTexture(tex);

         int textureIntBool;
         textureIntBool = (sub.textured) ? 1 : 0;

         glUniform1ui(glGetUniformLocation(shaderProgram3D, "hasTexture"), textureIntBool);
         glUniform1i(glGetUniformLocation(shaderProgram3D, "diffuseTex"), 0);

         glDrawElements(GL_TRIANGLES,sub.indiceCount, GL_UNSIGNED_INT, 0);
      }

   }
}

