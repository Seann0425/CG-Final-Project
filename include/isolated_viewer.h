#ifndef ISOLATED_VIEWER_H
#define ISOLATED_VIEWER_H

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <optional>
#include <string>
#include "gl_helper.h"
#include "model.h"
#include "program.h"

class IsolatedViewer {
 public:
  IsolatedViewer();
  ~IsolatedViewer();

  void init(GLFWwindow* shareWindow);

  void setTarget(std::shared_ptr<Model> target);
  void clearTarget();
  bool isActive() const;
  std::shared_ptr<Model> getCurrentTarget() const;
  void render(Program& shader);

 private:
  std::shared_ptr<Model> currentTarget;
  bool active;

  GLFWwindow* viewerWindow;
  GLFWwindow* mainWindow;

  GLuint vao;
  GLuint vboPosition;
  GLuint vboNormal;
  GLuint vboTexCoord;

  void uploadDataToGPU();
  void freeGPUResources();
};

#endif