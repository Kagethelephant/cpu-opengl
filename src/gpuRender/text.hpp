#pragma once
// OpenGl Library
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// Standard Libraries
#include <vector>
#include <map>
// Program Headers
#include "utils/data.hpp"
#include "window.hpp"
#include "shaders/shader.hpp"
#include "utils/matrix.hpp"
// Freetype Library
#include <ft2build.h>
#include FT_FREETYPE_H


/// @brief: Simple text engine used to render text to window. After engine is created text can be printed
/// to the screen using the RenderText function
class textRenderEngine {

public:
   
   /// @brief: Create new text engine
   /// @param win: Window to draw to (using FBO)
   /// @param fontFile: filepath to the font to use for this engine 
   textRenderEngine(window& win, const char* fontFile);
   ~textRenderEngine();

   /// @brief: Render text to the window attached to this text engine
   /// @param text: String to be rendered to the screen
   /// @param x: X position on screen to render text
   /// @param y: Y position on screen to render text (top-left origin)
   /// @param col: Color to draw text (default: White)
   void RenderText(std::string text, float x, float y, Color col = Color::White);


private:

   window& m_window;
   FT_Library m_library;

   /// @brief: Stores the font face for the engine 
   /// (engine is intended to be simple and does not support multiple font faces)
   FT_Face m_fontFace;

   /// @brief: Groups the OpenGL texture its size and the character bearing and advance
   struct character{
      GLuint textureID;
      vec2 size;
      vec2 bearing;  // Position from the top-left of the theoretical pen position
      int advance;   // Distance between this character and the next
   };

   /// @brief: Maps ASCII chars (from 32-128) to their corresponding character struct / texture
   std::map<char, character> m_characters;


   /// @brief: Vertices that make up the quad used to draw characters (updated per character)
   std::vector<GLfloat> m_vertices;

   /// @brief: Shader program made specifically to draw characters to a quad
   GLuint m_shaderProgramText;

   GLuint m_vao;
   GLuint m_vbo;
};
