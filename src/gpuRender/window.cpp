#include "window.hpp"
#include "gpuRender/RAIIWrapper.hpp"
#include "shaders/shader.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <iostream>
#include <vector>


window::window(int height){
   //---------------------- SET FBO AND WINDOW SIZE ----------------------
   // On start make the window the size of the display
   const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
   windowSize.x = mode->width;
   windowSize.y = mode->height;

   // FBO is the texture that we draw everything to. Lower resolution will give pixelated look
   aspectRatio = float(windowSize.x) / float(windowSize.y);
   fboSize.y = height;
   fboSize.x = int(fboSize.y * aspectRatio);


   //---------------------- SETUP GLFW ----------------------

   win = glfwCreateWindow(windowSize.x, windowSize.y, "The Game", NULL, NULL);
   glfwMakeContextCurrent(win);
   // Dont let the window height scale below FBO height
   glfwSetWindowSizeLimits(win, GLFW_DONT_CARE, fboSize.y, GLFW_DONT_CARE, GLFW_DONT_CARE);
   // This disables vsync
   glfwSwapInterval(0); 
   // Initialize GLAD (Loads functions from the GPU)
   gladLoadGL();

   // Print GPU information
   std::cout << "GPU Vendor: " << glGetString(GL_VENDOR) << std::endl;
   std::cout << "GPU Model: " << glGetString(GL_RENDERER) << std::endl;
   std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

   // Makes it so "this*" can be used to reference the GLFW window in this classes callback functions
   glfwSetWindowUserPointer(win, this); 
   glfwSetFramebufferSizeCallback(win, [](GLFWwindow* _win, int w, int h){

      auto* self = static_cast<window*>(glfwGetWindowUserPointer(_win));
      if (!self) return;

      self->windowSize.x  = w;
      self->windowSize.y = h;
      self->aspectRatio  = float(w) / float(h);

      self->resizePending = true;
   });

   // DEFINE THE VERTEX DATA QUAD
   // -----------------------------------------------------------------------------------
   quadVertices = {
      -1.0f,  1.0f,  0.0f,  1.0f, // x, y, u, v
      -1.0f, -1.0f,  0.0f,  0.0f,
       1.0f, -1.0f,  1.0f,  0.0f,

      -1.0f,  1.0f,  0.0f,  1.0f,
       1.0f, -1.0f,  1.0f,  0.0f,
       1.0f,  1.0f,  1.0f,  1.0f
   };

   // Create the VAO storing the vertice data for our full screen quad
   glGenVertexArrays(1, &UIvao);  
   glGenBuffers(1, &UIvbo);

   // Setup the VBO using the VAO
   GLScopedVAO tempVAO(UIvao);
   GLScopedVBO tempVBO(UIvbo);

   glBufferData(GL_ARRAY_BUFFER, quadVertices.size() * sizeof(GLfloat), quadVertices.data(), GL_STATIC_DRAW);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2*sizeof(GLfloat)));
   // This tells GL to use the vertex attributes defined above (it does not do this by default)
   glEnableVertexAttribArray(0);  
   glEnableVertexAttribArray(1);  
   // Shader program used to draw the quad defined above to the FBO
   shaderProgramUI = createShaderProgram("../src/shaders/ui_vertex.glsl", "../src/shaders/ui_fragment.glsl");


   // CREATE FBO
   // -----------------------------------------------------------------------------------
   glGenFramebuffers(1, &fbo);
   GLScopedFBO tempFBO(fbo);

   {
      // Color attachment
      glGenTextures(1, &colorTex);
      GLScopedTexture2D tempTexture(colorTex);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, fboSize.x, fboSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);
   }
   {
      glGenTextures(1, &depth);
      GLScopedTexture2D tempTexture(depth);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, fboSize.x, fboSize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
      // REQUIRED settings for depth textures
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      // Attach depth texture
      glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,depth,0);
   }

   GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
   glDrawBuffers(1, buffers);

   if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
      std::cerr << "FixedFBO incomplete\n";
}


void window::resize(){
   if (resizePending){
      // Recalculate FBO size (height fixed, width scales with aspect ratio)
      fboSize.x = int(fboSize.y * aspectRatio);
      {
         GLScopedTexture2D tempTexture(colorTex);
         glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, fboSize.x, fboSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
      }
      {
         GLScopedTexture2D tempDepth(depth);
         glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, fboSize.x, fboSize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
      }
      aspectRatio = (float)fboSize.x / (float)fboSize.y;
      resizePending = false;
   }
}

bool window::checkKey(int key, KeyMode mode){
    int current = glfwGetKey(win, key);
    int previous = prevKeyState[key];

    bool result = false;
    switch(mode){
        case KeyMode::Pressed:
            result = (current == GLFW_PRESS);
            break;
        case KeyMode::PressedOnce:
            result = (current == GLFW_PRESS && previous != GLFW_PRESS);
            break;
        case KeyMode::Released:
            result = (current == GLFW_RELEASE && previous == GLFW_PRESS);
            break;
    }

    // update previous state for next frame
    prevKeyState[key] = current;
    return result;
}


void window::frameUpdate(){

   // Bind the main window framebuffer to draw to the screen
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   // Set the viewport, shader program and VAO with RAII wrappers that will reset to previous
   // OpenGL states at the end of the scope where they were bound
   GLScopedViewport tempViewPort(0, 0, windowSize.x, windowSize.y);
   GLScopedProgram tempProgram(shaderProgramUI);
   GLScopedVAO tempVAO(UIvao);
   // Select texture unit 0 and bind colorTex to GL_TEXTURE_2D on that unit
   glActiveTexture(GL_TEXTURE0);
   GLScopedTexture2D tempTexture(colorTex);

   glUniform1i(glGetUniformLocation(shaderProgramUI, "screenTexture"), 0);
   glDrawArrays(GL_TRIANGLES, 0, 6);


   frameTime = glfwGetTime() - currentTime; // In seconds
   currentTime = glfwGetTime();
   frameCount++;
   // If a second has passed
   if (currentTime - lastTime >= 0.2f){
      // Calculate FPS and display it (e.g., in the window title or console)
      fps = (int)frameCount / (currentTime - lastTime);
      frameCount = 0;
      lastTime = currentTime;
   }

   glfwSwapBuffers(win);
   glfwPollEvents();

   resize();
}

window::~window(){
   // Make sure the context is current before deleting GL objects
   if (win) glfwMakeContextCurrent(win);
   // OpenGL cleanup
   if (UIvbo) glDeleteBuffers(1, &UIvbo);
   if (UIvao) glDeleteVertexArrays(1, &UIvao);
   if (colorTex) glDeleteTextures(1, &colorTex);
   if (depth) glDeleteTextures(1, &depth);
   if (fbo) glDeleteFramebuffers(1, &fbo);
   if (shaderProgramUI) glDeleteProgram(shaderProgramUI);
   // Destroy window + context
   if (win) glfwDestroyWindow(win);

}
