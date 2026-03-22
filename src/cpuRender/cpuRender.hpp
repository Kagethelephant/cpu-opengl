#pragma once
// Standard Libraries
#include <cstdint>
// Program Headers
#include "utils/matrix.hpp"
#include "gpuRender/window.hpp"
#include "app/object.hpp"



/// @brief: Takes mesh data from 3D objects, projects the 3D polygons to a 2D view in the 
/// form of a pixel buffer to be drawn to an OpenGL FBO.
class cpuRenderObject {

public:
   /// @brief: Create new CPU rendering engine
   /// @param cam: Reference to the window to be rendered to. Used to find resolution and SFML render target
   cpuRenderObject(camera& cam);

   /// @brief: Add object to draw when render function is called
   /// @param obj: object to load into mesh buffer
   void bindObject(const object& obj){ m_objects.push_back(obj);}

   /// @brief: Add light to use for rendering
   /// @param newLight: light to add to light buffer
   void addLight(const light& newLight){ m_lights.push_back(newLight);}

   /// @brief: Render 3D vertex data given information from 3D object. This is where most of the
   /// 3D graphics pipeline is executed: vertex shader, vertex post processing (triangle clipping)
   /// @param object: Object that provides position, orientation, scale, color and model index information
   void render();


private:

   /// @brief: Bundles 3 vertices together. This is created at the primitive assembly stage of rendering
   struct triangle3d {

      vertex v[3];

      triangle3d() : v{vertex(),vertex(),vertex()}{};
      triangle3d(vertex v0, vertex v1, vertex v2) : v{v0,v1,v2}{};

      /// @brief: Performs perspective divide (x,y,z /= w) converting from clip space to NDC. This introduces perspective 
      /// (non-linear in screen space), which is why attributes must later be interpolated using 1/w correction.
      void perspectiveDivide() {v[0].screenPos.perspectiveDivide(); v[1].screenPos.perspectiveDivide(); v[2].screenPos.perspectiveDivide();}
   };

   /// @brief: Reference to the GLFW window (contains FBO drawn to in rasterization)
   const window& m_window;
   /// @brief: Camera defining the view via the view matrix
   camera& m_camera;
   /// @brief: Resolution of the window 
   vec2 m_resolution;

   /// @brief: Pixel array for 32 bit true color + alpha (8 bits for r,g,b and alpha) used to raster triangles
   std::vector<std::uint8_t> m_pixelBuffer;
   /// @brief: Buffer to clear the pixelBuffer with a background color
   std::vector<std::uint8_t> m_clearBuffer;

   /// @brief: Vector array of light objects to use for rendering
   std::vector<light> m_lights;
   /// @brief: Vector array of 3d objects to render (added with bindObject)
   std::vector<object> m_objects;

   /// @brief: Stores the vertices loaded from the model
   std::vector<vertex> m_vertices;
   /// @brief: Stores the triangles created from the current submesh indices (rendered per submesh)
   std::vector<triangle3d> m_triangles;

   /// @brief: virtual planes that represent the edge of the view frustum in 3d space
   vec4 m_planes[6];
   /// @brief: Depth buffer storing normalized device depth (after perspective divide), used for visibility testing (smaller = closer).
   std::vector<float> m_zBuffer;


   /// @brief Rasterization stage of the CPU rendering pipeline. This stage evaluates
   /// the fragment shader and is the most complex part of the renderer.
   ///
   /// The rasterizer first computes an axis-aligned bounding box (AABB) around the
   /// triangle in screen space. It then iterates over each pixel within the box and
   /// uses barycentric coordinates to determine whether the pixel lies inside the
   /// triangle and to interpolate vertex attributes (uv, screenPos, fragPos, etc.).
   ///
   /// Note: Barycentric interpolation in screen space is linear. Attributes such as UVs
   /// and world position are NOT linearly interpolated after projection, so perspective correction
   /// (interpolation in 1/w space) is required.
   ///
   /// The interpolated attributes are used to compute shading, texture color,
   /// fragment color, and depth. If the fragment depth passes the z-buffer test,
   /// the pixel buffer and depth buffer are updated.
   ///
   /// Barycentric Coordinates:
   /// Barycentric coordinates express a point inside a triangle as a weighted
   /// combination of the triangle's three vertices (A, B, C):
   ///
   ///   P = α*A + β*B + γ*C
   ///
   /// where α, β, γ are the barycentric weights and satisfy: α + β + γ = 1.
   /// The weights represent how close the point is to each vertex. For example,
   /// if α is large, the point lies closer to vertex A.
   ///
   /// The weights can be computed using edge function areas. Each weight equals
   /// twice the signed area of the sub-triangle opposite the vertex divided by
   /// twice the signed area of the full triangle. Cross product returns twice the signed
   /// area enclosed by 2 vectors (triangle), so the equations for barycentric weights are:
   ///
   ///   α = cross(B - A, P - A) / cross(B - A, C - A)
   ///   β = cross(C - B, P - B) / cross(C - B, A - B)
   ///   γ = cross(A - C, P - C) / cross(A - C, B - C)
   ///
   /// These barycentric weights change linearly in screenspace for a change in x and y,
   /// so instead of calculating per pixel we can compute the rate of change or
   /// partial derivative of the barycentric weights with respect to screen x and y.
   ///
   /// Partial derivative with respect to x (∂F/∂x) can be approximated by:
   ///   ∂F/∂x ≈ F(x+1, y) - F(x, y)
   /// 
   /// For example, the partial derivative approximation of α with respect to x (∂α/∂x):
   /// starts with the function for α:
   ///
   ///   α(x, y) = (AB × AP) / (AB × AC)
   ///
   /// Expanding the cross products:
   ///
   ///   α(x, y) = [(Bx - Ax)*(Py - Ay) - (By - Ay)*(Px - Ax)] / area
   ///
   /// Add 1 to x for α(x+1, y):
   ///                                                  Px       + 1
   ///   α(x+1, y) = [(Bx - Ax)*(Py - Ay) - (By - Ay)*((Px - Ax) + 1)] / area
   ///
   /// Distribute -(By - Ay) over the sum:
   ///
   ///   α(x+1, y) = [(Bx - Ax)*(Py - Ay) - (By - Ay)*(Px - Ax) - (By - Ay)*1] / area
   ///
   /// Separate the original α(x, y) term:
   //                [                      a(x, y)                    ] 
   ///   α(x+1, y) = [(Bx - Ax)*(Py - Ay) - (By - Ay)*(Px - Ax)] / area - (By - Ay) / area
   ///
   /// Recognize the first fraction as α(x, y):
   ///
   ///   α(x+1, y) = α(x, y) - (By - Ay) / area
   ///
   /// Therefore:
   ///
   ///   alphaDx = α(x+1, y) - α(x, y) 
   ///   alphaDx = - (By - Ay) / area
   ///
   /// Now that we have the partial derivative of each barycentric weight, we can use this to
   /// compute the partial derivative of any vertex attribute with respect to screen x and y.
   /// This is used to compute the fragPos derivative by plugging the vertex attributes into
   /// the barycentric formula with the partial derivative weights, e.g.:
   ///
   ///   vec3 dFdx = (v0.fragPos * alphaDx + v1.fragPos * betaDx + v2.fragPos * gammaDx).xyz();
   ///
   /// This is done outside of the fragment loop because it is very expensive to do per fragment on
   /// CPU rasterizer. Since it does not interpolate vertex normals this will give flat shading
   ///
   /// Other vertex attributes use the stepped barycentric weights to find the exact
   /// interpolated value within the fragment loop.
   ///
   /// @param tri Pixel array for 32-bit true color + alpha (8 bits per r,g,b,alpha)
   /// @param res The resolution of the window
   void raster(const triangle3d& p, const object& obj, const model::subMesh& mesh);
   
   /// @brief: In-place clipping of triangles in m_triangleAttribs against all 6 planes 
   /// planes in clip space (after projection, before perspective division)
   void clipTriangles();

   /// @brief: Checks the given triangle for winding in screen space
   /// @param tri: triangle to check for culling
   bool backFaceCulling(const triangle3d& tri);

   /// @brief: Used to get the position on a line between 2 3d points where 
   /// that line intersects the given plane
   /// @param p1: Point in 3d space
   /// @param p2: 2nd point in 3d space to create a theoretical line with the 1st point
   /// @param plane: Plane in 3d space that intersects the line
   /// @return: float t value representing percent of the line where intersected [0 - 1]
   float planeIntersect(const vec4& a, const vec4& b, const vec4& plane);

   /// @brief: Wraps number from given max to 0
   /// @param n: Number to wrap
   /// @param max: Exclusive max
   int wrap(int n, int max);
};
