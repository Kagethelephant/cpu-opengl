#include "cpuRender.hpp"
// Standard Libraries
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>
#include <math.h>
// Program Headers
#include "gpuRender/window.hpp"
#include "gpuRender/RAIIWrapper.hpp"
#include "utils/data.hpp"
#include "utils/matrix.hpp"
#include "app/object.hpp"



//---------------------- INITIALIZATION ----------------------

cpuRenderObject::cpuRenderObject(camera& cam) : m_camera{cam}, m_window{cam.getWindow()}{

   m_resolution = vec2(m_window.fboWidth,m_window.fboHeight);

   // Calculate the planes in clip-space that make up the frustum (box that represents the field of view)
   m_planes[0] = vec4( 1, 0, 0, 1);  // Left
   m_planes[1] = vec4(-1, 0, 0, 1);  // Right 
   m_planes[2] = vec4( 0, 1, 0, 1);  // Bottom
   m_planes[3] = vec4( 0, -1, 0, 1); // Top 
   m_planes[4] = vec4( 0, 0, 1, 1);  // Near
   m_planes[5] = vec4( 0, 0, -1, 1); // Far

   // Create a background buffer the size of the window to clear the pixel buffer with a color
   m_clearBuffer = std::vector<std::uint8_t>(m_resolution[0] * m_resolution[1] * 4, 0);
   m_zBuffer = std::vector<float>(m_resolution[0] * m_resolution[1], 1.0f);

   // Make a clear buffer to initialize the screen (pixelBuffer) to the background color every frame
   vec4 backgroundColor(hexColorToRGB(Color::Black));

   for (int y = 0; y < m_resolution[1]; y++){
      for (int x = 0; x < m_resolution[0]; x++){

         int index = (y * m_resolution[0] + x);
         m_clearBuffer[index*4] = backgroundColor[0];
         m_clearBuffer[index*4 + 1] = backgroundColor[1];
         m_clearBuffer[index*4 + 2] = backgroundColor[2];
         m_clearBuffer[index*4 + 3] = backgroundColor[3];
      }
   }
   m_pixelBuffer = m_clearBuffer;
}



//---------------------- 3D RENDER PIPELINE ----------------------

void cpuRenderObject::render() {
   // Camera owns view and projection matrix since it also owns camera FOV, direction and position 
   mat4x4 vp = m_camera.getViewMatrix() * m_camera.getProjectionMatrix();

   for (const object& obj : m_objects){
      const model& mod = obj.getModel();
     
      // Object owns scale and transformation matrix since it also owns object position, rotation and scale
      mat4x4 m = obj.getScaleMatrix() * obj.getTransformMatrix();

      const std::vector<vertex>& modVertices = mod.getVertices();
      m_vertices.resize(modVertices.size());

      // Load and apply vertex shader to model vertices
      for(int i=0; i<modVertices.size(); i ++){
         const vertex& modVert = modVertices[i];
         const vec2& aTex = modVert.uv;
         const vec4& aPos = modVert.screenPos;
         vertex newVert;

         // ---------- VERTEX SHADER

         newVert.uv = aTex;
         newVert.fragPos = aPos * m;
         newVert.screenPos = aPos * m * vp;

         // ---------- VERTEX SHADER
        
         // Save the clip position (opengl does this automatically)
         newVert.clipPos = newVert.screenPos;
         m_vertices[i] = newVert;
      }

      
      for (const auto& mesh : mod.getSubMeshes()) {
         m_primatives.clear();
         m_primatives.reserve(mesh.indices.size()/3);

         // ---------- PRIMATIVE ASSEMBLY
         for(int i=0; i< mesh.indices.size(); i += 3){
            // Generate triangles from vertices and indices (opengl does this in background)
            int i0 = mesh.indices[i];
            int i1 = mesh.indices[i+1];
            int i2 = mesh.indices[i+2];
            m_primatives.emplace_back(m_vertices[i0],m_vertices[i1],m_vertices[i2]);
         }

         // ---------- TRIANGLE CLIPPING
         clipTriangles();

         for(int i=0; i< m_primatives.size(); i++) {
            triangle3d& p = m_primatives[i];

            // ---------- PERSPECTIVE DIVIDE
            p.perspectiveDivide();

            // ---------- VIEWPORT TRANSFORMATION
            for (int i=0; i<3; i++){
               // Projection results are between -1 and 1. So shift to the positive and scale to fit screen
               p.v[i].screenPos.x = (p.v[i].screenPos.x + 1.0f) * 0.5f * m_resolution.x;
               p.v[i].screenPos.y = (p.v[i].screenPos.y + 1.0f) * 0.5f * m_resolution.y;
            }

            // ---------- BACKFACE CULLING AND RASTERIZATION
            // Do not raster if winding is incorrect
            if(!backFaceCulling(p)){ raster(p, obj, mesh);}
         }
         m_primatives.clear();
      }
   }

   // Texture needs to be attached to the window FBO. window render call will draww it to the window
   GLScopedTexture2D tempTexture(m_window.colorTex);
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_window.fboWidth, m_window.fboHeight, GL_RGBA, GL_UNSIGNED_BYTE, m_pixelBuffer.data());
   // Clear the pixel buffer with the background color so frames dont stack
   std::memcpy(m_pixelBuffer.data(), m_clearBuffer.data(), m_pixelBuffer.size());
   // Clear the z buffer with far depth
   std::fill(m_zBuffer.begin(), m_zBuffer.end(), 1.0f);
}



//---------------------- TRIANGLE CLIPPING ----------------------

void cpuRenderObject::clipTriangles() {
   
   // Create buffer to hold triangles that need to go through the split function and 
   // a buffer of triangles that have already been through the split function
   std::vector<triangle3d> splitBuffer;
   splitBuffer.reserve(m_primatives.size() * 2);

   for (vec4& plane : m_planes) {
      for(triangle3d& t : m_primatives) {

         // For conciseness check what points are out of the current plane ahead of time
         // Dot product of plane in Ax + Bx + Cz + D form and point will tell you what side 
         // the plane it is on. negative and positive on opisite sides and 0 on the plane
         bool clipped[3] = {
            plane.dot(t.v[0].clipPos) < 0.0f,
            plane.dot(t.v[1].clipPos) < 0.0f,
            plane.dot(t.v[2].clipPos) < 0.0f};

         // If all of the points are not clipped by this plane then pass along the triangle to the next step
         if (clipped[0]+clipped[1]+clipped[2] == 0){splitBuffer.push_back(t); continue;}
         if (clipped[0]+clipped[1]+clipped[2] == 3){ continue;}

         // Cycle through all of the points to find the clipped points
         for(int i=0; i<3; i++){
            // Get the position of the next and last point clockwise from the current point
            int next = wrap(i+1,3);
            int last = wrap(i+2,3);
            // We make sure the last point is not clipped so we exclude triangles with all points out of the plane
            if (clipped[i] and not clipped[last]){
               // If there are 2 points and they are the current and next point
               if (clipped[next]){
                  // Find new points where the edges of the triangles intercect the plane
                  float t1 = planeIntersect(t.v[i].screenPos, t.v[last].screenPos, plane);
                  float t2 = planeIntersect(t.v[next].screenPos, t.v[last].screenPos, plane);
                  // Lerp all vertex attributes for new triangle points
                  vertex p1 = t.v[i].lerp(t.v[last], t1);
                  vertex p2 = t.v[next].lerp(t.v[last], t2);

                  splitBuffer.emplace_back(p1,p2,t.v[last]);
                  break;
               }
               // If there is only one point and it is the current point
               else {
                  // Find new points where the edges of the triangles intercect the plane
                  float t1 = planeIntersect(t.v[i].screenPos, t.v[last].screenPos, plane);
                  float t2 = planeIntersect(t.v[i].screenPos, t.v[next].screenPos, plane);
                  // Lerp all vertex attributes for new triangle points
                  vertex p1 = t.v[i].lerp(t.v[last], t1);
                  vertex p2 = t.v[i].lerp(t.v[next], t2);

                  splitBuffer.emplace_back(p1,p2,t.v[next]);
                  splitBuffer.emplace_back(p1,t.v[next],t.v[last]);
                  break;
               }
            }
         }
      }
      // Pass triangles from the working buffer back into the loop for the next plane (and clear working buffer)
      m_primatives.swap(splitBuffer);
      splitBuffer.clear();
   }

}


//---------------------- FILL TRIANGLE ----------------------

void cpuRenderObject::raster(const triangle3d& pr, const object& obj, const model::subMesh& mesh) {
   // Bounding box triangle filling method with barycentric coordinates to interpolate 
   // between points. This is the trickiest part of the pipeline
   const vertex& v0 = pr.v[0];
   const vertex& v1 = pr.v[1];
   const vertex& v2 = pr.v[2];

   vec2 p0(v0.screenPos.x, v0.screenPos.y);
   vec2 p1(v1.screenPos.x, v1.screenPos.y);
   vec2 p2(v2.screenPos.x, v2.screenPos.y);
   // Divide is more expensive than multiply so store the inverse w (used for perspective correction)
   float invW0 = 1.0f / v0.clipPos.w;
   float invW1 = 1.0f / v1.clipPos.w;
   float invW2 = 1.0f / v2.clipPos.w;

   float clipZ0 = v0.clipPos.z * invW0;
   float clipZ1 = v1.clipPos.z * invW1;
   float clipZ2 = v2.clipPos.z * invW2;

   vec4 fragOverW0 = v0.fragPos * invW0;
   vec4 fragOverW1 = v1.fragPos * invW1;
   vec4 fragOverW2 = v2.fragPos * invW2;

   vec2 uvOverW0 = v0.uv * invW0;
   vec2 uvOverW1 = v1.uv * invW1;
   vec2 uvOverW2 = v2.uv * invW2;

   int xmin = std::max((int)std::min({ p0.x, p1.x, p2.x }), 0);
   int xmax = std::min((int)std::max({ p0.x, p1.x, p2.x }), (int)m_resolution.x);
   int ymin = std::max((int)std::min({ p0.y, p1.y, p2.y }), 0);
   int ymax = std::min((int)std::max({ p0.y, p1.y, p2.y }), (int)m_resolution.y);


   // ---------- BARYCENTRIC CALCULATIONS (details in header)

   // Cross product of 2 edges of the triangle yields 2x the area of the triangle
   float area = (p2 - p0).cross(p1 - p0);
   if (area == 0.0f) return;
   float invArea = 1.0f / area;

   // Partial derivatives of the barycentric coordinates with respect to screen x and y
   float alphaDx = (p2.y - p1.y) * invArea;
   float alphaDy = (p1.x - p2.x) * invArea;
   float betaDx  = (p0.y - p2.y) * invArea;
   float betaDy  = (p2.x - p0.x) * invArea;
   float gammaDx = (p1.y - p0.y) * invArea;
   float gammaDy = (p0.x - p1.x) * invArea;

   // Initial barycentric coordinates at the first pixel center
   vec2 start(xmin + 0.5f, ymin + 0.5f);
   float alphaY = (start - p1).cross(p2 - p1) * invArea;
   float betaY  = (start - p2).cross(p0 - p2) * invArea;
   float gammaY = (start - p0).cross(p1 - p0) * invArea;

   // Partial derivative of world position (fragPos) with respect to screen x and y
   vec3 dFdx = (v0.fragPos * alphaDx + v1.fragPos * betaDx + v2.fragPos * gammaDx).xyz();
   vec3 dFdy = (v0.fragPos * alphaDy + v1.fragPos * betaDy + v2.fragPos * gammaDy).xyz();
   // Since these vectors are tangent to worldpace triangle face, the cross product will give triangle normal
   vec3 normal = dFdx.cross(dFdy).normal();


   // ---------- RASTER LOOP

   for (int y = ymin; y <= ymax; ++y){
      float alphaXY = alphaY;
      float betaXY  = betaY;
      float gammaXY = gammaY;

      for (int x = xmin; x <= xmax; ++x){

         if (alphaXY >= 0.0f && betaXY >= 0.0f && gammaXY >= 0.0f){

            float depth = (alphaXY * clipZ0 + betaXY  * clipZ1 + gammaXY * clipZ2) * 0.5 + 0.5;
            int index = x + y * m_resolution.x;

            if (depth <= m_zBuffer[index]){
               
               // Perspective-correct attribute reconstruction
               float invW = 1.0f / (alphaXY * invW0 + betaXY  * invW1 + gammaXY * invW2);
               vec3 fragPos = (fragOverW0 * alphaXY + fragOverW1 * betaXY + fragOverW2 * gammaXY).xyz() * invW;

               vec4 texColor;

               // Get color from texture for different channels
               if (mesh.textured) {
                  vec2 uv = (uvOverW0 * alphaXY + uvOverW1 * betaXY + uvOverW2 * gammaXY) * invW;

                  // Texture wrap
                  uv.x -= std::floor(uv.x);
                  uv.y -= std::floor(uv.y);
                  int tx = (int)(uv.x * mesh.tex.w);
                  int ty = (int)(uv.y * mesh.tex.h);
                  int texel = (ty * mesh.tex.w + tx) * mesh.tex.channels;

                  // Set channels
                  if (mesh.tex.channels == 3) {
                     texColor = vec4(mesh.tex.data[texel + 0], mesh.tex.data[texel + 1], mesh.tex.data[texel + 2], 1);
                  }
                  else if (mesh.tex.channels == 4) {
                     texColor = vec4(mesh.tex.data[texel + 0], mesh.tex.data[texel + 1], mesh.tex.data[texel + 2], mesh.tex.data[texel + 3]);
                  }
                  else { // grayscale
                     float shade = mesh.tex.data[texel] / 255.0f;
                     texColor = vec4(shade,shade,shade,1.0f);
                  }
               }

               // ---------- FRAGMENT SHADER

               float ambientStrength = 0.2;
               vec3 lightSum(0,0,0);
               vec4 sampleColor;


               for (const light& l : m_lights){
                  // vec3 normal = dFdx.cross(dFdy).normal(); --- Calculated above
                  vec3 lightDir = (l.position - fragPos).normal();

                  vec3 ambient = l.color * 0.2f;
                  vec3 diffuse = l.color * std::max(normal.dot(lightDir), 0.0f);

                  lightSum += diffuse + ambient;
               }

               if (mesh.textured) {
                  sampleColor = texColor;
               }
               else {
                  sampleColor = hexColorToRGB(obj.getColor());
               }

               vec4 fragColor = sampleColor * vec4(lightSum, 1.0f);

               // ---------- FRAGMENT SHADER


               m_pixelBuffer[index*4 + 0] = std::clamp(fragColor[0], 0.0f, 255.0f);
               m_pixelBuffer[index*4 + 1] = std::clamp(fragColor[1], 0.0f, 255.0f);
               m_pixelBuffer[index*4 + 2] = std::clamp(fragColor[2], 0.0f, 255.0f);
               m_pixelBuffer[index*4 + 3] = std::clamp(fragColor[3], 0.0f, 255.0f);

               m_zBuffer[index] = depth;
            }
         }
         alphaXY += alphaDx;
         betaXY  += betaDx;
         gammaXY += gammaDx;
      }
      alphaY += alphaDy;
      betaY  += betaDy;
      gammaY += gammaDy;
   }
}



//---------------------- HELPER FUNCTIONS ----------------------

float cpuRenderObject::planeIntersect(const vec4& a, const vec4& b, const vec4& plane) {
   return plane.dot(a) / (plane.dot(a) - plane.dot(b));
}

bool cpuRenderObject::backFaceCulling(const triangle3d& tri) {
    // Use only X/Y in screen space.
    const vec2& a = tri.v[0].screenPos.xy();
    const vec2& b = tri.v[1].screenPos.xy();
    const vec2& c = tri.v[2].screenPos.xy();
    // Compute signed area (2D cross product)
    float area = (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]);
    // Area will return negative if the triangle has CCW winding
    return area < 0.0f;
}

int cpuRenderObject::wrap(int n, int max){
   if (n >= max) n -= max;
   return n;
}
