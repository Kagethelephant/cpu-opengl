#pragma once

#include "utils/matrix.hpp"
#include "window.hpp"
#include "app/object.hpp"

#include <cstddef>
#include <glad/glad.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <string>
#include <sys/types.h>
#include <vector>



/// @brief: Loads and renders the meshes to the screen
/// @param _width: Width of the rendering view (not always the same as window)
/// @param _height: Height of the rendering view (not always the same as window)
class gpuRenderEngine {

public:

   gpuRenderEngine(camera& cam);

   /// @brief: Shader program to render 3d objects
   GLuint shaderProgram3D;


   vec3 lightPos = vec3(0,0,0);
   vec3 lightPosview = vec3(0,0,0);
   vec3 lightCol = vec3(1,1,1);
   
   // Quad used to render 2d textures on to the screen (used for UI)
   std::vector<GLfloat> quadVertices;


   struct gpuSubMesh{
      bool textured;
      GLuint tex;
      GLuint ebo;
      std::size_t indiceCount;
   };

   struct gpuMesh {
      gpuMesh(const object& o) : obj{o} {}
      const object& obj;
      GLuint vao;
      GLuint vbo;
      std::vector<gpuSubMesh> subMeshes;
   };

   std::vector<gpuMesh> meshes;

   std::vector<light> lights;

   void addLight(const light& newLight){
      lights.push_back(newLight);
   }
   const window& m_window;
   camera& m_camera;

   std::vector<GLfloat> vertices; 
   std::vector<GLuint> indices; 
  
   void render();
   void bindObject(const object& obj);

private:

};

