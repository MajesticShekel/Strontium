#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/ShadingPrimatives.h"

namespace Strontium
{
  struct Plane
  {
    float d;
    glm::vec3 point;
    glm::vec3 normal;
  };

  struct BoundingBox
  {
    glm::vec3 center;
    glm::vec3 extents;
  };

  struct Frustum
  {
    glm::vec3 corners[8];
    Plane sides[6];
    glm::vec3 center;
    glm::vec3 min;
    glm::vec3 max;
    float bSphereRadius;
  };

  // Builds a bounding box given the min+max coordinates of an object (in local space).
  BoundingBox buildBoundingBox(const glm::vec3 &min, const glm::vec3 &max);
  // Builds an AABB given the min+max coordinates of an object plus the localspace to worldspace transformation matrix.
  BoundingBox buildBoundingBox(const glm::vec3& min, const glm::vec3& max, const glm::mat4 &modelMatrix);

  // Builds a camera frustum given a camera struct.
  Frustum buildCameraFrustum(const Camera &camera);

  // Builds a camera frustum given the view and projection matrices plus the front vector of the camera. 
  Frustum buildCameraFrustum(const glm::mat4 &viewProj, const glm::vec3 &viewVec);

  float signedPlaneDistance(const Plane &plane, const glm::vec3 &point);
  bool boundingBoxOnPlane(const Plane& plane, const BoundingBox &box);

  bool sphereInFrustum(const Frustum &frustum, const glm::vec3 center, float radius);
  bool boundingBoxInFrustum(const Frustum &frustum, const glm::vec3 min, const glm::vec3 max);
  bool boundingBoxInFrustum(const Frustum& frustum, const glm::vec3 min, const glm::vec3 max, 
                            const glm::mat4 &transform);
}
