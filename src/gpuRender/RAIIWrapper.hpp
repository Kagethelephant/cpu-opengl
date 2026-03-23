#pragma once
#include <glad/glad.h>

// RAII wrappers are used to maintain OpenGL state. Doing this manually in the code can add
// a lot of lines when you are changing alot of states, saving previous states and resetting those states.
// this also increases the likelyhood of state leaks. using RAII wrappers for each state change ensures that
// state changes are only oersistant for the scope they are created in.

//---------------------- VIEWPORT RAII ----------------------
struct GLScopedViewport {
   // This pattern is used for all RAII wrappers so only this function is commented
   GLint oldViewport[4];
   // Upon creation store the previous GL state and set the new GL state
   explicit GLScopedViewport(GLint x, GLint y, GLsizei w, GLsizei h) {
      glGetIntegerv(GL_VIEWPORT, oldViewport);
      glViewport(x, y, w, h);
   }
   // Make the struct non-copyable so it cannot leave the scope
   GLScopedViewport(const GLScopedViewport&) = delete;
   GLScopedViewport& operator=(const GLScopedViewport&) = delete;

   // Make the struct non-movable so it cannot leave the scope
   GLScopedViewport(GLScopedViewport&&) = delete;
   GLScopedViewport& operator=(GLScopedViewport&&) = delete;

   // Upon deletion reset the state. This ensures the state is only changed for the scope the wrapper was created
   ~GLScopedViewport() { glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]); }
};


//---------------------- VAO RAII ----------------------
struct GLScopedVAO {
   GLint oldVAO;
   explicit GLScopedVAO(GLuint vao) {
      glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &oldVAO);
      glBindVertexArray(vao);
   }

   GLScopedVAO(const GLScopedVAO&) = delete;
   GLScopedVAO& operator=(const GLScopedVAO&) = delete;

   GLScopedVAO(GLScopedVAO&&) = delete;
   GLScopedVAO& operator=(GLScopedVAO&&) = delete;

   ~GLScopedVAO() { glBindVertexArray(oldVAO); }
};


//---------------------- VBO RAII ----------------------
struct GLScopedVBO {
   GLint oldVBO;
   explicit GLScopedVBO(GLuint vbo) {
      glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &oldVBO);
      glBindBuffer(GL_ARRAY_BUFFER, vbo);
   }

   GLScopedVBO(const GLScopedVBO&) = delete;
   GLScopedVBO& operator=(const GLScopedVBO&) = delete;

   GLScopedVBO(GLScopedVBO&&) = delete;
   GLScopedVBO& operator=(GLScopedVBO&&) = delete;

   ~GLScopedVBO() { glBindBuffer(GL_ARRAY_BUFFER, oldVBO); }
};


//---------------------- EBO RAII ----------------------
struct GLScopedEBO {
   GLint oldEBO;
   explicit GLScopedEBO(GLuint ebo) {
      glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &oldEBO);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
   }

   GLScopedEBO(const GLScopedEBO&) = delete;
   GLScopedEBO& operator=(const GLScopedEBO&) = delete;

   GLScopedEBO(GLScopedEBO&&) = delete;
   GLScopedEBO& operator=(GLScopedEBO&&) = delete;

   ~GLScopedEBO() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, oldEBO); }
};


//---------------------- FBO RAII ----------------------
struct GLScopedFBO {
   GLint oldFBO;
   explicit GLScopedFBO(GLuint fbo) {
      glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFBO);
      glBindFramebuffer(GL_FRAMEBUFFER, fbo);
   }

   GLScopedFBO(const GLScopedFBO&) = delete;
   GLScopedFBO& operator=(const GLScopedFBO&) = delete;

   GLScopedFBO(GLScopedFBO&&) = delete;
   GLScopedFBO& operator=(GLScopedFBO&&) = delete;

   ~GLScopedFBO() { glBindFramebuffer(GL_FRAMEBUFFER, oldFBO); }
};


//---------------------- CULL-FACE RAII ----------------------
struct GLScopedCullFace {
   GLint previous;
   explicit GLScopedCullFace(GLenum mode) {
      glGetIntegerv(GL_CULL_FACE_MODE, &previous);
      glCullFace(mode);
   }

   GLScopedCullFace(const GLScopedCullFace&) = delete;
   GLScopedCullFace& operator=(const GLScopedCullFace&) = delete;

   GLScopedCullFace(GLScopedCullFace&&) = delete;
   GLScopedCullFace& operator=(GLScopedCullFace&&) = delete;

   ~GLScopedCullFace() { glCullFace(previous); }
};


//---------------------- SHADER PROGRAM RAII ----------------------
struct GLScopedProgram {
   GLint previous = 0;
   explicit GLScopedProgram(GLuint program) {
      glGetIntegerv(GL_CURRENT_PROGRAM, &previous);
      glUseProgram(program);
   }

   GLScopedProgram(const GLScopedProgram&) = delete;
   GLScopedProgram& operator=(const GLScopedProgram&) = delete;

   GLScopedProgram(GLScopedProgram&&) = delete;
   GLScopedProgram& operator=(GLScopedProgram&&) = delete;

   ~GLScopedProgram() { glUseProgram(previous); }
};


//---------------------- 2D TEXTURE RAII ----------------------
struct GLScopedTexture2D {
   GLint previous = 0;
   explicit GLScopedTexture2D(GLuint tex) {
      glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous);
      glBindTexture(GL_TEXTURE_2D, tex);
   }

   GLScopedTexture2D(const GLScopedTexture2D&) = delete;
   GLScopedTexture2D& operator=(const GLScopedTexture2D&) = delete;

   GLScopedTexture2D(GLScopedTexture2D&&) = delete;
   GLScopedTexture2D& operator=(GLScopedTexture2D&&) = delete;

   ~GLScopedTexture2D() { glBindTexture(GL_TEXTURE_2D, previous); }
};


//---------------------- CAPABILITY RAII ----------------------
struct GLScopedCapability
{
   GLenum cap;
   GLboolean wasEnabled;
   explicit GLScopedCapability(GLenum capability, bool enable) : cap(capability){
      wasEnabled = glIsEnabled(cap);
      if (enable) glEnable(cap);
      else glDisable(cap);
   }

   GLScopedCapability(const GLScopedCapability&) = delete;
   GLScopedCapability& operator=(const GLScopedCapability&) = delete;

   GLScopedCapability(GLScopedCapability&&) = delete;
   GLScopedCapability& operator=(GLScopedCapability&&) = delete;

   ~GLScopedCapability(){
      if (wasEnabled) glEnable(cap);
      else glDisable(cap);
   }
};


//---------------------- BLEND FUNCTION RAII ----------------------
struct GLScopedBlendFunc{
   GLint srcRGB, dstRGB;
   GLint srcAlpha, dstAlpha;
   explicit GLScopedBlendFunc(GLenum src, GLenum dst){
      glGetIntegerv(GL_BLEND_SRC_RGB,   &srcRGB);
      glGetIntegerv(GL_BLEND_DST_RGB,   &dstRGB);
      glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcAlpha);
      glGetIntegerv(GL_BLEND_DST_ALPHA, &dstAlpha);

      glBlendFunc(src, dst);
   }

   GLScopedBlendFunc(const GLScopedBlendFunc&) = delete;
   GLScopedBlendFunc& operator=(const GLScopedBlendFunc&) = delete;

   GLScopedBlendFunc(GLScopedBlendFunc&&) = delete;
   GLScopedBlendFunc& operator=(GLScopedBlendFunc&&) = delete;

   ~GLScopedBlendFunc(){ glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha); }
};


//---------------------- ACTIVE TEXTURE RAII ----------------------
struct GLScopedActiveTexture {
   GLint previous;
   explicit GLScopedActiveTexture(GLenum newUnit) {
      glGetIntegerv(GL_ACTIVE_TEXTURE, &previous);
      glActiveTexture(newUnit);
   }

   GLScopedActiveTexture(const GLScopedActiveTexture&) = delete;
   GLScopedActiveTexture& operator=(const GLScopedActiveTexture&) = delete;

   GLScopedActiveTexture(GLScopedActiveTexture&&) = delete;
   GLScopedActiveTexture& operator=(GLScopedActiveTexture&&) = delete;

   ~GLScopedActiveTexture() { glActiveTexture(previous); }
};


//---------------------- UNPACK ALIGNMENT RAII ----------------------
struct GLScopedUnpackAlignment {
   GLint previous;

   explicit GLScopedUnpackAlignment(GLint alignment) {
      glGetIntegerv(GL_UNPACK_ALIGNMENT, &previous);
      glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
   }

   GLScopedUnpackAlignment(const GLScopedUnpackAlignment&) = delete;
   GLScopedUnpackAlignment& operator=(const GLScopedUnpackAlignment&) = delete;

   GLScopedUnpackAlignment(GLScopedUnpackAlignment&&) = delete;
   GLScopedUnpackAlignment& operator=(GLScopedUnpackAlignment&&) = delete;

   ~GLScopedUnpackAlignment() { glPixelStorei(GL_UNPACK_ALIGNMENT, previous); }
};
