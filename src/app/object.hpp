#pragma once
// Project Libraries
#include "utils/data.hpp"
#include "utils/matrix.hpp"
#include <gpuRender/window.hpp>
// Standard Libraries
#include <vector>
#include <unordered_map>




//---------------------- VERTEX ----------------------

/// @brief: Stores all vertex attributes used during rasterization. Bundling attributes 
/// together simplifies interpolation and avoids maintaining separate arrays for position, UV, etc.
struct vertex {

   vec4 screenPos; 
   vec4 clipPos;
   vec4 fragPos;
   vec2 uv;

   // W should be initialized to 1 for perspective divide
   vertex() : screenPos{0,0,0,1} {}

   /// @brief: Linearly interpolate 2 vertices with scale t
   /// @param v2: second vertex to create line
   /// @param t: 0-1 scale for position on line
   /// @return: interpolated vertex containing blended attributes
   vertex lerp(const vertex& v2, float t) const { 
      vertex v = *this; 
      v.screenPos = v.screenPos + (v2.screenPos - v.screenPos) * t;
      v.clipPos = v.clipPos + (v2.clipPos - v.clipPos) * t;
      v.fragPos = v.fragPos + (v2.fragPos - v.fragPos) * t;
      v.uv = v.uv + (v2.uv - v.uv) * t;
      return v;
   }
};



//---------------------- LIGHTS ----------------------

/// @brief: Point light used for scene shading
struct light {
   /// @brief: Create light with starting pos and color
   /// @param pos: position of the light in world space
   /// @param col: RGB light color/intensity (0–1 range)
   light (const vec3& pos, const vec3& col = vec3(1,1,1)) : position{pos}, color{col} {}
   vec3 position;
   vec3 color;
};


//---------------------- CAMERA ----------------------

/// @brief: Represents the viewer in the rendering pipeline and defines how the 3D scene
/// is observed and projected onto the screen. Stores the camera's position, orientation,
/// and the matrices required to transform world-space geometry into screen-space.
class camera {

public:

   /// @brief: Create camera associated with window
   /// @param win: window the camera renders to (used to obtain viewport aspect ratio)
   /// @param fov: vertical field of view of the camera in degrees
   camera(window& win, float fov = 70) : m_window(win), m_fov(fov) { 
      m_aspectRatio = m_window.getAspectRatio();
      m_projectionMatrix = matrix_project(m_fov, m_aspectRatio,m_near,m_far);  
      // Call move and rotate on creation to initialize view matrix
      move(0,0,0); 
      rotate(0,0,0);
   }

   /// @brief: Moves camera according to the direction the camera is facing. ie. z moves forward
   /// and backward relative to camera point direction. x moves right and left (relative)
   /// @param x: Move sideways (right vector of camera)
   /// @param y: Move in the up direction ( up vector of camera)
   /// @param z: Move in the direction of forward vector of camera
   void move(float x, float y, float z);

   /// @brief: Rotates along right, forward and up direction vectors of camera (relative)
   /// @param u: Rotate around the right vector of the camera
   /// @param v: Rotate around the up vector of the camera
   /// @param w: Rotate around the forward vector of the camera
   void rotate(float u, float v, float w);


   void updateView();

   /// @brief: Returns const reference to camera position
   const vec3& getPosition() const { return (m_position);}
   /// @brief: Returns const reference to camera rotation
   const vec3& getRotation() const { return (m_rotation);}
   /// @brief: Returns const reference to camera direction vector
   const vec3& getDirection() const { return (m_direction);}

   /// @brief: Get camera view matrix
   const mat4x4& getViewMatrix() const {return (m_viewMatrix);};
   /// @brief: Get camera projection matrix
   const mat4x4& getProjectionMatrix() const {return (m_projectionMatrix);};
   /// @brief: Get window linked to camera
   const window& getWindow() const {return (m_window);};

   /// @brief: Get far plane position of camera
   float getFarPlane() const {return (m_far);};
   /// @brief: Get near plane position of camera
   float getNearPlane() const {return (m_near);};
   /// @brief: Set far plane position of camera, and update projection matrix
   /// @param far: far plane location as float 
   void setFarPlane(float far) {m_far = far; m_projectionMatrix = matrix_project(m_fov,m_aspectRatio,m_near,m_far); };
   /// @brief: Set near plane position of camera, and update projection matrix
   /// @param near: near plane location as float 
   void setNearPlane(float near) {m_near = near; m_projectionMatrix = matrix_project(m_fov,m_aspectRatio,m_near,m_far); };
   /// @brief: Set field of view of camera, and update projection matrix
   /// @param fov: field of view of the camera
   void setFOV(float fov) {m_fov = fov; m_projectionMatrix = matrix_project(m_fov,m_aspectRatio,m_near,m_far); };

private:

   /// @brief: Field of view of view frustum
   float m_fov;
   /// @brief: Far plane of view frustum
   float m_far = 1000.0f;
   /// @brief: Near plane of view frustum
   float m_near = 0.1f;

   float m_aspectRatio;

   /// @brief: View matrix used for both GPU and CPU rendering.
   /// Transforms vertices from world space into camera (view) space, where the camera is treated as if it were at the origin
   /// looking down the negative z-axis. Conceptually, this moves the entire scene relative to the camera rather than moving
   /// the camera itself. Essential for positioning and orienting objects correctly from the camera's point of view.
   mat4x4 m_viewMatrix;

   /// @brief: Projection matrix used for both GPU and CPU rendering. Transforms vertices from camera (view) space into 
   /// clip space, defining how the 3D scene is projected onto a 2D screen. Encodes the field of view, aspect ratio, 
   /// and near/far clipping planes, producing the foreshortening effect that makes distant objects appear smaller.
   /// Conceptually, it determines the volume of space that will be visible on the screen and maps that volume into the canonical
   /// cube used by the rasterizer (-1 to 1 in x, y, z in normalized device coordinates). Essential for accurately projecting
   /// 3D geometry onto a 2D viewport and ensuring consistent rendering between CPU and GPU pipelines.
   mat4x4 m_projectionMatrix;

   window& m_window;

   vec3 m_position = vec3(0,0,0);
   vec3 m_rotation = vec3(0,0,0);
   vec3 m_direction = vec3(0,0,-1);
};






//---------------------- MODEL ----------------------

/// @brief: This stores all of the vertex data and attributes loaded from the obj file and is 
/// referenced by an object so we dont have to load multiple models for objects with the same model
class model {

public:

   /// @brief: Loads model from obj file
   /// @param filename: filepath to the OBJ file
   /// @param ccwWinding: changes the winding on the model so the triangle normal points outwards
   model(const std::string& filename, bool cwWinding = false);

   /// @brief: Stores texture data for use in rasterization
   struct texture {
      int w;                           // Width of image
      int h;                           // Height of image
      int channels;                    // Number of channels (rgb = 3)
      unsigned char* data = nullptr;   // Pointer to begining of char array
   };

   /// @brief: Portion of mesh associated with a single texture
   struct subMesh {
      std::vector<uint32_t> indices;   // Indices making up submesh (triangles)
      bool textured = true;            // Was a texture loaded for the mesh
      texture tex;                     // Texture instance storing image data
   };

   /// @brief: Get submesh vector array
   const std::vector<subMesh>& getSubMeshes() const { return m_subMeshes; }
   /// @brief: Get raw vertice vector array
   const std::vector<float>& getVerticesRaw() const { return m_verticesRaw; }
   /// @brief: Get oop vertice vector array
   const std::vector<vertex>& getVertices() const { return m_vertices; }  


private:

   /// @brief: Contains all sub meshes that make up the model
   std::vector<subMesh> m_subMeshes;
   /// @brief: Tightly packed vertex data for GPU rendering
   std::vector<float> m_verticesRaw;
   /// @brief: Structured vertex data for ease-of-use in CPU software rendering
   std::vector<vertex> m_vertices; 

   /// @brief: Maps texture names from the .mtl file to loaded texture data.
   std::unordered_map<std::string, texture> m_textureMap;

   /// @brief: Represents a unique combination of a vertex position index (v) and texture index (t)
   /// for use as a key in unordered_map. operator== is used to compare keys that hash to the same bucket.
   struct vertexKey {
      int v;
      int t;
      bool operator==(const vertexKey& o) const {
         return v == o.v && t == o.t;
      }
   };

   /// @brief Hash functor for vertexKey used in std::unordered_map.
   /// Combines position index (v) and texture index (t) into one hash value. Each integer is hashed 
   /// with std::hash<int>(). The texture hash is shifted left by one bit (<<1) so its bits occupy 
   /// different positions, then both hashes are mixed using XOR (^).
   ///
   /// Example:
   /// hv = 10101100
   /// ht = 00110101 -> (ht<<1) = 01101010
   /// result = hv ^ (ht<<1) = 11000110 (xor operator)
   ///
   /// This improves distribution across buckets. Collisions are resolved by vertexKey::operator==.
   struct vertexKeyHash {
      size_t operator()(const vertexKey& k) const {
         return std::hash<int>()(k.v) ^ (std::hash<int>()(k.t) << 1);
      }
   };

   /// @brief: load image data into a texture object from file
   /// @param filename: filepath of texture
   texture loadTexture(const std::string& filepath);

   /// @brief: Map all textures to names in the MTL file
   /// @param filename: filepath of mtl file
   void loadMTL(const std::string& filepath);

   /// @brief: Gets the directory that the given file is located
   /// @param filename: path to file
   std::string getDirectory(const std::string& filepath);

};



//---------------------- OBJECT ----------------------

/// @brief: Object that can instantiate models and position them in world space
class object {

public:
 
   /// @brief: Create object with model reference for geometry
   /// @param model: Model object references for geometry
   object(model& model) : m_model{model} {
      m_position = vec3(0,0,0);
      m_rotation = vec3(0,0,0);
      m_scale = vec3(1,1,1);
      m_transformMatrix = matrix_transform(m_position[0], m_position[1], m_position[2], m_rotation[0], m_rotation[1], m_rotation[2]);
      m_scaleMatrix = matrix_scale(m_scale[0], m_scale[1], m_scale[2]);
   };


   /// @brief: Changes object scale (absolute)
   /// @param sx: Scale in x
   /// @param sy: Scale in y
   /// @param sz: Scale in z
   void scale(float sx, float sy, float sz);

   /// @brief: Changes object world coordinates (relative)
   /// @param x: x position
   /// @param y: y position
   /// @param z: z position
   void move(float x, float y, float z);

   /// @brief: Changes object rotation (relative)
   /// @param u: rotation in radians about x axis
   /// @param v: rotation in radians about y axis
   /// @param w: rotation in radians about z axis
   void rotate(float u, float v, float w);


   /// @brief: Set color of object that will be drawn if the model does not have a texture
   /// @param _color: color as Color enum: 4 channel hexadecimal color
   void setColor(Color col) {m_color = col;};
   /// @brief: get object color as Color enum: 4 channel hexadecimal color
   Color getColor() const {return (m_color);};
   /// @brief: get object transformation matrix
   const mat4x4& getTransformMatrix() const {return (m_transformMatrix);};
   /// @brief: get object scale matrix
   const mat4x4& getScaleMatrix() const {return (m_scaleMatrix);};
   /// @brief: Get model referenced by this object
   const model& getModel() const {return (m_model);};

private:

   /// @brief: Base color to draw the object (this will be shaded by the camera)
   Color m_color = Color::White;
   /// @brief: Coordinates of the object origin in 3D space
   vec3 m_position = vec3(0,0,0);
   /// @brief: Rotation from original orientation in radians
   vec3 m_rotation = vec3(0,0,0);
   /// @brief: Scale of the object
   vec3 m_scale = vec3(1,1,1);

   /// @brief: Transform matrix used in the vertex shader of the rendering pipeline
   mat4x4 m_transformMatrix;
   /// @brief: Scale matrix used in the vertex shader of the rendering pipeline
   mat4x4 m_scaleMatrix;

   /// @brief: Reference to object model (geometry and texture data)
   model& m_model;
};


