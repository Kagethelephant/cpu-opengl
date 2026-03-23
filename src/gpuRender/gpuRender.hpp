#pragma once
// Program headers
#include "window.hpp"
#include "app/object.hpp"
#include "shaders/shader.hpp"
// OpenGL
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// Standard Libraries
#include <vector>



// Constant max number of lights (this needs to match 3d frag shader constant MAX_LIGHTS)
constexpr unsigned int MAX_LIGHTS = 32;


/// @brief: GPU rendering engine to handle the OpenGL calls for 3D rendering for the provided
/// models / objects in 3D space. Draws to window FBO and window class handles rendering to GLFW window
class gpuRenderEngine {

public:

   /// @brief: Create new GPU rendering engine
   /// @param cam: Camera to reference for view matrix and window size
   gpuRenderEngine(camera& cam) : m_camera{cam}, m_window{cam.getWindow()}{
      m_shaderProgram3D = createShaderProgram("../src/shaders/3d_vertex.glsl", "../src/shaders/3d_fragment.glsl");
   };

   /// @brief: Load object data to GPU and memory to render in loop with "render"
   /// @param obj: Object to load
   void bindObject(const object& obj);

   /// @brief: Load lights into engine to use for rendering
   /// @param newLight: Light to be loaded
   void addLight(const light& newLight){
      m_lightPositions.push_back(newLight.position.x);
      m_lightPositions.push_back(newLight.position.y);
      m_lightPositions.push_back(newLight.position.z);
      m_lightColors.push_back(newLight.color.x);
      m_lightColors.push_back(newLight.color.y);
      m_lightColors.push_back(newLight.color.z);
   }

   /// @brief: Render the scene with loaded objects and lights to the window FBO
   void render();

private:
   
   /// @brief: OpenGL shader program comprised of vertex and fragment shaders pulled from the .glsl files
   GLuint m_shaderProgram3D;

   /// @brief: Sub mesh parsed from object submeshes on load. Meshes are uploaded to GPU one time to reduce overhead.
   /// This struct groups the gpu data like mesh textures and ebo (indices) to reference during rendering
   struct gpuSubMesh{
      bool textured;
      GLuint tex;
      GLuint ebo;
      std::size_t indiceCount;
   };

   /// @brief: Object parsed from "bindObject" to load into the GPU one time to reduce overhead.
   /// This struct groups the gpu data like vao, vbo and submeshes for reference during rendering
   struct gpuObject {
      // Grab refernce to object on create so we can get things like object color during rendering
      gpuObject(const object& o) : obj{o} {}
      const object& obj;
      GLuint vao;
      GLuint vbo;
      std::vector<gpuSubMesh> subMeshes;
   };

   std::vector<gpuObject> m_objects;

   std::vector<float> m_lightPositions;
   std::vector<float> m_lightColors;

   const window& m_window;
   camera& m_camera;

};

