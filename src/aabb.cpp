#include "aabb.h"
#include <algorithm>
#include <limits>

AABB::AABB() { reset(); }

AABB::AABB(const glm::vec3& min, const glm::vec3& max) : min(min), max(max) {}

void AABB::reset() {
  min = glm::vec3(std::numeric_limits<float>::max());
  max = glm::vec3(std::numeric_limits<float>::lowest());
}

void AABB::fit(const glm::vec3& point) {
  min = glm::min(min, point);
  max = glm::max(max, point);
}

void AABB::fit(const std::vector<glm::vec3>& points) {
  for (const auto& point : points) {
    fit(point);
  }
}

glm::vec3 AABB::getCenter() const { return (min + max) * 0.5f; }

glm::vec3 AABB::getSize() const { return max - min; }

bool AABB::intersect(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float& t) const {
  // Slab Method
  float tMin = 0.0f;
  float tMax = 100000.0f;  // 無限遠

  glm::vec3 bounds[2] = {min, max};

  // 針對 X, Y, Z 三個軸分別檢查
  for (int i = 0; i < 3; ++i) {
    float invD = 1.0f / rayDir[i];
    float t0 = (bounds[0][i] - rayOrigin[i]) * invD;
    float t1 = (bounds[1][i] - rayOrigin[i]) * invD;

    if (invD < 0.0f) std::swap(t0, t1);

    tMin = t0 > tMin ? t0 : tMin;
    tMax = t1 < tMax ? t1 : tMax;

    if (tMax <= tMin) return false;  // 沒有交集
  }

  t = tMin;
  return true;
}