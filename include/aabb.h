#pragma once
#include <glm/glm.hpp>
#include <vector>

class AABB {
 public:
  glm::vec3 min;
  glm::vec3 max;

  AABB();
  AABB(const glm::vec3& min, const glm::vec3& max);

  void reset();
  void fit(const glm::vec3& point);
  void fit(const std::vector<glm::vec3>& points);

  glm::vec3 getCenter() const;
  glm::vec3 getSize() const;

  // [New] 射線相交檢測
  // rayOrigin: 射線起點 (Local Space)
  // rayDir: 射線方向 (Local Space)
  // t: 如果相交，回傳相交點的距離
  bool intersect(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float& t) const;
};