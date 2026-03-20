#include "object.hpp"
// OpenGL
#include <glad/glad.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
// Standard Libraries
#include <iostream>
#include <math.h>
#include "utils/matrix.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <string>
#include <unordered_map>
// STB_Image
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>




//---------------------- CAMERA MOVEMENT ----------------------
void camera::move(float x, float y, float z) {
   // Transform the world-space up vector by the camera's rotation
   vec3 up = (matrix_transform(0, 0, 0, m_rotation[0], m_rotation[1], m_rotation[2]) * vec4(0,1,0,1)).xyz();
   // Move along camera's right vector
   // use cross product of up and forward vector to get the the right vector
   m_position += (m_direction.cross(up) * x);
   // Movement in z and y is the same: not world axis but relative to camera direction 
   m_position += m_direction * z;
   m_position += up * y;

   // Updated view matrix will be referenced by renderer
   m_viewMatrix = matrix_view(matrix_pointAt(m_position, m_direction, up));
}

void camera::rotate(float u, float v, float w) {
   m_rotation += vec3(u, v, w);
   vec3 up = (matrix_transform(0, 0, 0, m_rotation[0], m_rotation[1], m_rotation[2]) * vec4(0,1,0,1)).xyz();
   m_direction = (matrix_transform(0, 0, 0, m_rotation[0], m_rotation[1],m_rotation[2]) * vec3(0,0,-1)).xyz() ;
   // Updated view matrix will be referenced by renderer
   m_viewMatrix = matrix_view(matrix_pointAt(m_position, m_direction, up));
}


//---------------------- OBJECT TRANSFORMATIONS ----------------------

void object::scale(float sx, float sy, float sz){
   m_scale = vec3(sx,sy,sz);
   m_scaleMatrix = matrix_scale(m_scale[0], m_scale[1], m_scale[2]);
}


void object::move(float x, float y, float z){
   m_position += vec3(x,y,z);
   m_transformMatrix = matrix_transform(m_position[0], m_position[1], m_position[2], m_rotation[0], m_rotation[1], m_rotation[2]);
}


void object::rotate(float u, float v, float w){
   m_rotation += vec3(u,v,w);
   m_transformMatrix = matrix_transform(m_position[0], m_position[1], m_position[2], m_rotation[0], m_rotation[1], m_rotation[2]);
}




//---------------------- LOAD MODEL FROM OBJ FILE ----------------------

model::model(const std::string& filename, bool cwWinding) {

   std::ifstream obj(filename);
   if (!obj) { throw std::runtime_error("Failed to open OBJ file");}

   // dir is used to find other files within the same directory
   std::string dir = getDirectory(filename);

   // Raw OBJ attribute streams
   std::vector<vec3> objPositions;
   std::vector<vec2> objTexcoords;

   // hash table mapping a combination of position and texture indexes to a vertex object 
   // in the vertex vector arry (vertices) vertexKeyHash is the callable struct to generate hash
   std::unordered_map<vertexKey, uint32_t, vertexKeyHash> vertexCache;

   subMesh* currentSubmesh;

   std::string line;
   while (std::getline(obj, line)) {

      std::stringstream stream(line);
      std::string type;
      stream >> type;

      // ---------- POSITION
      if (type == "v") {
         vec3 p;
         stream >> p.x >> p.y >> p.z;
         objPositions.push_back(p);
      }

      // ---------- TEXCOORD
      else if (type == "vt") {
         vec2 uv;
         stream >> uv.x >> uv.y;
         objTexcoords.push_back(uv);
      }

      // ---------- MTL FILE
      else if (type == "mtllib") {
         std::string mtlfile;
         stream >> mtlfile;
         // Maps "mtlName" below to its corresponding texture data in "textureMap"
         loadMTL(dir + mtlfile);
      }

      // ---------- MTL MATERIAL
      else if (type == "usemtl") {
         std::string mtlName;
         stream >> mtlName;
         // loadMTL should already be called at this point so we just need to create a new
         // submesh and tie the texture associated with this mtlName to that submesh
         subMesh newSubmesh;
         newSubmesh.tex = m_textureMap[mtlName];
         m_subMeshes.push_back(newSubmesh);
      }

      // ---------- FACE
      else if (type == "f") {
        
         // ---------- COLLECT INDICES

         // Create an initial submesh for the current submesh. This ensures that if there is no
         // material (untextured model) than we can still create a submesh
         if(m_subMeshes.size() == 0){
            subMesh newSubmesh;
            newSubmesh.textured = false;
            m_subMeshes.push_back(newSubmesh);
         }
         
         // Vertices associated with a texture should follow "usemtl" call for that texture / material
         // so make the last submesh aded the current submesh for this face
         subMesh& currentMesh = m_subMeshes.back();
         // Assume this mesh has no texture until a texture indice is found below
         currentMesh.textured = false;

         // Temporary storage for this polygons position and texture indices
         std::vector<int> vIdx;
         std::vector<int> tIdx;


         std::string vert;
         while (stream >> vert) {
            int v = 0, t = 0;
            // Find the slashes in the vertex string "v/t/n" or "v//n" or "v/t"
            size_t p1 = vert.find('/');         // first slash
            size_t p2 = vert.find('/', p1 + 1); // Second slash (if exists)

            // Extract position index (before first slash) and convert to 0-based
            v = std::stoi(vert.substr(0, p1)) - 1;

            // Extract texture index (between first and second slash) and convert to 0-based, if present 
            // If no slash, this vertex has no texture coordinates
            if (p1 != std::string::npos) {
               t = std::stoi(vert.substr(p1 + 1, p2 - p1 - 1)) - 1; 
               currentMesh.textured = true;
            }
            // Store indices for this face/polygon
            vIdx.push_back(v);
            tIdx.push_back(t);
         }
         // Need at least a triangle to generate face/polygon
         if (vIdx.size() < 3) continue;


         // ---------- FAN TRIANGULATION
         
         for (int i = 1; i < vIdx.size()-1; ++i) {
            // Start with the first triangle (should be {0,1,2} then {0,2,3} for triangle fan)
            int tri[3] = {0, i, i+1};
            // Triangle winding swapped if indicated in model constructor call to ensure normals face outward
            if (cwWinding) std::swap(tri[1], tri[2]);

            for (int k = 0; k < 3; ++k) {
               // Create a new key containing the index of the vertex and 
               vertexKey key{vIdx[tri[k]],tIdx[tri[k]]};
               // "it" is an iterator (similiar to pointer) to the map element (not the value but the key-value pair itself)
               auto it = vertexCache.find(key);

               // If the key is not found, 'it' equals vertexCache.end()
               // If found, it->first is the key, and it->second is the value (here, the vertex index)
               // If vertex has already been created simply add it submesh indices
               if (it != vertexCache.end()) currentMesh.indices.push_back(it->second);
               else {
                  // Correct negative indices for OBJ (negative = relative to end of array)
                  int correctedKeyV = (key.v < 0) ? objPositions.size() + key.v + 1 : key.v;
                  int correctedKeyT = (key.t < 0) ? objTexcoords.size() + key.t + 1 : key.t;

                  // vertex has not been created, so create it and add the position and texture data if it exists
                  vertex vertOut;
                  vertOut.screenPos = vec4(objPositions[correctedKeyV],1.0f);
                  if (currentMesh.textured){
                     vertOut.uv = objTexcoords[correctedKeyT];
                  }

                  // Add new vertex to verices vector array and map the location to the 
                  // current submesh and the vertexCache hashtable for use on other faces if applicable
                  uint32_t newIndex = static_cast<uint32_t>(m_vertices.size());
                  m_vertices.push_back(vertOut);
                  vertexCache[key] = newIndex;
                  currentMesh.indices.push_back(newIndex);
               }
            }
         }
      }
   }
   // Allocate memory matching the size of the vertices vector array
   // 3 pos + 2 uv = 5 floats per vertex
   m_verticesRaw.reserve(m_vertices.size() * 5); 

   // Create a tightliy packed vertice array for GPU
   for (const auto& vert : m_vertices) {
      // position
      m_verticesRaw.push_back(vert.screenPos.c[0]);
      m_verticesRaw.push_back(vert.screenPos.c[1]);
      m_verticesRaw.push_back(vert.screenPos.c[2]);
      // uv
      m_verticesRaw.push_back(vert.uv.c[0]);
      m_verticesRaw.push_back(vert.uv.c[1]);
   }
}




//---------------------- MODEL LOADING HELPERS ----------------------

std::string model::getDirectory(const std::string& filepath) {
   // find the location of the last "/" or "\\" this should be the directoy
   size_t pos = filepath.find_last_of("/\\");

   if (pos == std::string::npos) return "";
   return filepath.substr(0, pos + 1); // include trailing slash
}



model::texture model::loadTexture(const std::string& filepath) {
   // Directory converted from "dir\\file" to "dir/file"
   std::string correctedPath = filepath;
   std::replace(correctedPath.begin(), correctedPath.end(), '\\', '/');

   texture tex;
   stbi_set_flip_vertically_on_load(true);

   tex.data = stbi_load(correctedPath.c_str(), &tex.w, &tex.h, &tex.channels, 0);
   if (!tex.data) throw std::runtime_error("Failed to load texture: " + correctedPath);

   // Only 1, 3, or 4 channel images are supported in CPU and GPU raterizer
   if (!(tex.channels == 1 || tex.channels == 3 || tex.channels == 4)){
      stbi_image_free(tex.data);
      throw std::runtime_error("Unsupported image channel count: " + correctedPath);
   }
   return tex;
}



void model::loadMTL(const std::string& filepath) {
   std::ifstream file(filepath);
   if (!file) throw std::runtime_error("Failed to open MTL: " + filepath);

   // Texture locations will be relative to .mtl directory so we need to capture this
   std::string dir = getDirectory(filepath);
   texture* current = nullptr;
   std::string line;

   // itterate through the entire mtl file
   while (std::getline(file, line)) {
      std::stringstream ss(line);
      std::string type;
      ss >> type;

      // Load the name of the material into the map
      // This works because the "newmtl" will come before the "map_Kd" so current will be set before "map_Kd"
      if (type == "newmtl") {
         std::string name;
         ss >> name;
         m_textureMap[name] = {}; // Creates another element in the map if it does not exist
         current = &m_textureMap[name];
      } 
      // Load the texture into the material map
      else if (type == "map_Kd" && current) {
         std::string texPath;
         ss >> texPath;
         // Load texture from the path
         *current = loadTexture(dir + texPath);
      }
   }
}
