// OpenGL
#include <glad/glad.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
// Project headers
#include "object.hpp"
#include "cpuRender/cpuRender.hpp"
#include "gpuRender/text.hpp"
#include "gpuRender/window.hpp"
#include "gpuRender/gpuRender.hpp"
#include "utils/matrix.hpp"
// Standard Libraries
#include <math.h>
#include <string>



int main(int argc, char* argv[]){

   //---------------------- INITIALIZE GLFW ----------------------
   glfwInit();
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

   // Toggles between GPU and CPU rasterization
   bool useGPU = true;

   // Scope used to call destructors on GLFW objects before terminating GLFW
   {
      //------------------- INITIALIZE WINDOW RESOURCES ----------------------
      window programWindow(400);
      camera userCamera(programWindow);
      // Base camera movement speeds (scaled by frame time in the main loop)
      double posSpeed = 10.0f; // Position units / sec
      double rotSpeed = 3.0f;  // Radians / sec

      textRenderEngine text(programWindow, "../resources/font/novem___.ttf");


      //------------------- CREATE MODELS, OBJECTS AND LIGHTS ------------------------
      // Models load geometry once and can be used by many objects
      model yoshiModel("../resources/objects/yoshi/yoshi.obj");
      model arcanineModel("../resources/objects/Arcanine/Arcanine.obj");
      model teapotModel("../resources/objects/teapot.obj",true);
      model apeModel("../resources/objects/apeman/Apeman.obj");

      object apeObj(apeModel);
      apeObj.move(0,-2,-5);
      apeObj.scale(.1,.1,.1);

      object teapotObj(teapotModel);
      teapotObj.scale(0.02, 0.02, 0.02);
      teapotObj.move(5,0,-5);
      teapotObj.setColor(Color::Purple);

      object arcanineObj(arcanineModel);
      arcanineObj.move(-10,0,-10);
      arcanineObj.scale(10,10,10);

      object yoshiObj(yoshiModel);
      yoshiObj.move(0,0,-10);
      yoshiObj.rotate(0,5,0);

      // Light position and RGB color (0–1 range)
      light redLight(vec3(15,5,5),vec3(0.6,0.3,0.3));
      light blueLight(vec3(-15,5,5),vec3(0.3,0.3,0.6));

      //------------------- BIND OBJECTS AND LIGHTS TO RENDERERS ------------------------
      // Pass same camera, objects, and lights to both renderers to mirror screen output between the two
      gpuRenderEngine gpuRend(userCamera);
      gpuRend.bindObject(yoshiObj);
      gpuRend.bindObject(arcanineObj);
      gpuRend.bindObject(teapotObj);
      gpuRend.bindObject(apeObj);

      gpuRend.addLight(redLight);
      gpuRend.addLight(blueLight);

      cpuRenderEngine cpuRend(userCamera);
      cpuRend.bindObject(yoshiObj);
      cpuRend.bindObject(arcanineObj);
      cpuRend.bindObject(teapotObj);
      cpuRend.bindObject(apeObj);

      cpuRend.addLight(redLight);
      cpuRend.addLight(blueLight);


      //------------------- PROGRAM LOOP ------------------------
      while(!glfwWindowShouldClose(programWindow.win)){
         
         //------------------- USER INPUT ------------------------
         // Multiply the speed of movement and rotation by the amount of time the last frame took
         // This will make it so very high and low frame rates will move relatively the same in real time
         double posDelta = posSpeed * programWindow.frameTime;
         double rotDelta = rotSpeed * programWindow.frameTime;

         if (programWindow.checkKey(GLFW_KEY_ESCAPE)) {glfwSetWindowShouldClose(programWindow.win, true);}
         if (programWindow.checkKey(GLFW_KEY_ENTER, window::KeyMode::PressedOnce)) {useGPU = !useGPU;}
         if (programWindow.checkKey(GLFW_KEY_S)) {userCamera.move(0, 0, -posDelta);}
         if (programWindow.checkKey(GLFW_KEY_W)) {userCamera.move(0, 0, posDelta);}
         if (programWindow.checkKey(GLFW_KEY_A)) {userCamera.move(-posDelta, 0, 0);}
         if (programWindow.checkKey(GLFW_KEY_D)) {userCamera.move(posDelta, 0, 0);}
         if (programWindow.checkKey(GLFW_KEY_LEFT)) {userCamera.rotate(0, -rotDelta, 0);}
         if (programWindow.checkKey(GLFW_KEY_RIGHT)) {userCamera.rotate(0, rotDelta, 0);}
         if (programWindow.checkKey(GLFW_KEY_UP)) {userCamera.rotate(rotDelta, 0, 0);}
         if (programWindow.checkKey(GLFW_KEY_DOWN)) {userCamera.rotate(-rotDelta, 0, 0);}

         //------------------- RENDER PIPELINE ------------------------
         if(useGPU){ 
            gpuRend.render();
            text.RenderText("GPU", programWindow.fboWidth/2.0f, 10,Color::Green);
         }
         else { 
            cpuRend.render();
            text.RenderText("CPU", programWindow.fboWidth/2.0f, 10, Color::Blue);
         }
         text.RenderText("FPS: " + std::to_string(programWindow.fps), 10, 10);

         // Renders the FBO to the screen, checks for events, updates FPS, etc.
         programWindow.frameUpdate();
      }
   }
   // Called after scope so all GLFW object destructors can be called
   glfwTerminate();
   return 0;
}

