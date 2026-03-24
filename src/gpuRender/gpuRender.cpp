#include "gpuRender.hpp"
// Program headers
#include "RAIIWrapper.hpp"
#include "utils/data.hpp"
#include "utils/matrix.hpp"
#include "window.hpp"
#include "app/object.hpp"
// OpenGL
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// Std Libraries
#include <vector>




void gpuRenderEngine::bindObject(const object& obj){
 
   gpuObject gpuObject(obj);
   const model& mod = obj.getModel();

   // Create VAO and VBO. Vertices are stored per object as 3 floats for position and 2 floats for UV
   GLuint& vao = gpuObject.vao;
   GLuint& vbo = gpuObject.vbo;
   glGenBuffers(1, &vbo);
   glGenVertexArrays(1, &vao);  
   GLScopedVAO tempVAO(vao);
   GLScopedVBO tempVBO(vbo);

   // Need raw vertices from model because the VAO expects contigous memory
   glBufferData(GL_ARRAY_BUFFER, mod.getVerticesRaw().size() * sizeof(GLfloat), mod.getVerticesRaw().data(), GL_STATIC_DRAW);
   // positions at location 0
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
   glEnableVertexAttribArray(0);
   // UVs at location 1
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
   glEnableVertexAttribArray(1);


   for (const auto& mesh : mod.getSubMeshes()) {
      gpuSubMesh gpuSub;

      gpuSub.textured = mesh.textured;
      gpuSub.indiceCount = mesh.indices.size();

      // Create EBO and Texture. These are unique to object submesh
      GLuint& ebo = gpuSub.ebo;
      GLuint& tex = gpuSub.tex;
      glGenBuffers(1, &gpuSub.ebo);
      glGenTextures(1, &gpuSub.tex);
      GLScopedEBO tempEBO(ebo);
      GLScopedTexture2D tempTex(tex);

      // Create EBO from the subMesh indices (these will still point to vertices in the object VAO)
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(uint32_t), mesh.indices.data(), GL_STATIC_DRAW);

      // Format for 4 channel color or 3 channel color (alpha or no alpha)
      GLenum format = (mesh.tex.channels == 4) ? GL_RGBA : GL_RGB;
      // Create texture on the GPU and load in the submesh texture data
      glTexImage2D(GL_TEXTURE_2D, 0, format, mesh.tex.w, mesh.tex.h, 0, format, GL_UNSIGNED_BYTE, mesh.tex.data);
      glGenerateMipmap(GL_TEXTURE_2D);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      gpuObject.subMeshes.push_back(gpuSub);
   }
   m_objects.push_back(gpuObject);
}




void gpuRenderEngine::render(){

   const vec2& resolution = m_window.getFboSize();
   // Set OpenGL states that are agnostic of object or submesh 
   GLScopedFBO tempFBO(m_window.getFbo());                                // Window FBO to draw to
   GLScopedViewport tempViewPort(0, 0, resolution.x, resolution.y);  // Viewport matching FBO size
   GLScopedProgram tempProgram(m_shaderProgram3D);                   // 3D rendering shader program
   GLScopedCapability tempCullEnable(GL_CULL_FACE,true);             // Backface culling enable
   GLScopedCullFace tempCullMode(GL_BACK);                           // Ensure back face is culled rather than front
   GLScopedCapability tempDepthEnable(GL_DEPTH_TEST, true);          // Depth buffer test
   GLScopedActiveTexture tempActiveTex(GL_TEXTURE0);                 // Active texture (only texture 0 is used)

   // Clear the FBO to remove what was rendered last frame
   vec4 bgColor = hexColorToFloat(Color::Black);
   glClearColor(bgColor[0],bgColor[1],bgColor[2],bgColor[3]);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   // Tell GPU how many lights we have so it does not have to itterate to MAX_LIGHTS each fragment
   int lightCount = std::min(MAX_LIGHTS, (unsigned int)m_lightPositions.size() / 3);

   for (const gpuObject& gpuObject : m_objects){
      
      const object& objRef = gpuObject.obj;
      const GLuint& vao = gpuObject.vao;
      const GLuint& vbo = gpuObject.vbo;

      vec4 color = hexColorToFloat(objRef.getColor());

      // Setup the VBO using the VAO
      GLScopedVAO tempVAO(vao);
      GLScopedVBO tempVBO(vbo);


      // update the uniforms per fram to account for camera, object or light moves
      glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram3D, "view"),1,GL_FALSE,&m_camera.getViewMatrix().m[0][0]);
      glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram3D, "project"),1,GL_FALSE,&m_camera.getProjectionMatrix().m[0][0]);

      glUniform1i(glGetUniformLocation(m_shaderProgram3D, "lightCount"),lightCount);
      glUniform3fv(glGetUniformLocation(m_shaderProgram3D, "lightPos"),lightCount,&m_lightPositions[0]);
      glUniform3fv(glGetUniformLocation(m_shaderProgram3D, "lightCol"),lightCount,&m_lightColors[0]);

      glUniform3fv(glGetUniformLocation(m_shaderProgram3D, "objCol"),1,&color[0]);
      glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram3D, "scale"),1,GL_FALSE,&objRef.getScaleMatrix().m[0][0]);
      glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram3D, "transform"),1,GL_FALSE,&objRef.getTransformMatrix().m[0][0]);
      

      for (const gpuSubMesh& sub : gpuObject.subMeshes) {

         const GLuint& ebo = sub.ebo;
         const GLuint& tex = sub.tex;

         GLScopedEBO tempEBO(ebo);
         GLScopedTexture2D tempTexture(tex);

         int textureIntBool;
         textureIntBool = (sub.textured) ? 1 : 0;

         glUniform1ui(glGetUniformLocation(m_shaderProgram3D, "hasTexture"), textureIntBool);
         glUniform1i(glGetUniformLocation(m_shaderProgram3D, "diffuseTex"), 0);

         glDrawElements(GL_TRIANGLES,sub.indiceCount, GL_UNSIGNED_INT, 0);
      }

   }
}

