#define _CRT_SECURE_NO_WARNINGS
#include "model.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include <glm/vec3.hpp>

Model* Model::fromObjectFile(const char* obj_file) {
  Model* m = new Model();

  m->minBound = glm::vec3(std::numeric_limits<float>::max());
  m->maxBound = glm::vec3(std::numeric_limits<float>::lowest());

  std::ifstream ObjFile(obj_file);

  if (!ObjFile.is_open()) {
    std::cout << "Can't open File !" << std::endl;
    return NULL;
  }

  /* TODO#1: Load model data from OBJ file
   *         You only need to handle v, vt, vn, f
   *         Other fields you can directly ignore
   *         Fill data into m->positions, m->texcoords m->normals and m->numVertex
   *         Data format:
   *           For positions and normals
   *         | 0    | 1    | 2    | 3    | 4    | 5    | 6    | 7    | 8    | 9    | 10   | 11   | ...
   *         | face 1                                                       | face 2               ...
   *         | v1x  | v1y  | v1z  | v2x  | v2y  | v2z  | v3x  | v3y  | v3z  | v1x  | v1y  | v1z  | ...
   *         | vn1x | vn1y | vn1z | vn1x | vn1y | vn1z | vn1x | vn1y | vn1z | vn1x | vn1y | vn1z | ...
   *           For texcoords
   *         | 0    | 1    | 2    | 3    | 4    | 5    | 6    | 7    | ...
   *         | face 1                                  | face 2        ...
   *         | v1x  | v1y  | v2x  | v2y  | v3x  | v3y  | v1x  | v1y  | ...
   * Note:
   *        OBJ File Format (https://en.wikipedia.org/wiki/Wavefront_.obj_file)
   *        Vertex per face = 3 or 4
   */
  std::vector<glm::vec3> tempPoisitions;
  std::vector<glm::vec2> tempTextureCoords;
  std::vector<glm::vec3> tempNormals;

  std::string line;
  while (std::getline(ObjFile, line)) {
    if (line.empty() || line[0] == '#') continue;

    std::stringstream ss(line);
    std::string perfix;
    ss >> perfix;

    if (perfix == "v") {
      float x, y, z;
      ss >> x >> y >> z;
      tempPoisitions.emplace_back(x, y, z);

      if (x < m->minBound.x) m->minBound.x = x;
      if (y < m->minBound.y) m->minBound.y = y;
      if (z < m->minBound.z) m->minBound.z = z;

      if (x > m->maxBound.x) m->maxBound.x = x;
      if (y > m->maxBound.y) m->maxBound.y = y;
      if (z > m->maxBound.z) m->maxBound.z = z;
    }

    else if (perfix == "vt") {
      float u, v, w;
      ss >> u >> v >> w;
      tempTextureCoords.emplace_back(u, v);
    }

    else if (perfix == "vn") {
      float xn, yn, zn;
      ss >> xn >> yn >> zn;
      tempNormals.emplace_back(xn, yn, zn);
    }

    else if (perfix == "f") {
      std::vector<std::string> faceVerts;
      std::string vertStr;
      while (ss >> vertStr) {
        faceVerts.push_back(vertStr);
      }

      auto processVertex = [&](std::string& vstr) {
        int vi = 0, ti = 0, ni = 0;

        if (vstr.find("//") != std::string::npos) {
          sscanf(vstr.c_str(), "%d//%d", &vi, &ni);

          m->texcoords.push_back(0.0f);
          m->texcoords.push_back(0.0f);
        }

        else if (vstr.find('/') != std::string::npos) {
          sscanf(vstr.c_str(), "%d/%d/%d", &vi, &ti, &ni);
          glm::vec2 t = tempTextureCoords[ti - 1];
          m->texcoords.push_back(t.x);
          m->texcoords.push_back(t.y);
        }

        glm::vec3 p = tempPoisitions[vi - 1];

        glm::vec3 n = tempNormals[ni - 1];

        m->positions.push_back(p.x);
        m->positions.push_back(p.y);
        m->positions.push_back(p.z);

        m->aabb.fit(glm::vec3(p.x, p.y, p.z));

        m->normals.push_back(n.x);
        m->normals.push_back(n.y);
        m->normals.push_back(n.z);
      };

      if (faceVerts.size() == 3) {
        processVertex(faceVerts[0]);
        processVertex(faceVerts[1]);
        processVertex(faceVerts[2]);
      }

      else if (faceVerts.size() == 4) {
        processVertex(faceVerts[0]);
        processVertex(faceVerts[1]);
        processVertex(faceVerts[2]);

        processVertex(faceVerts[0]);
        processVertex(faceVerts[2]);
        processVertex(faceVerts[3]);
      }
    }
  }
  ObjFile.close();
  m->numVertex = static_cast<GLsizei>(m->positions.size() / 3);
  return m;
}
