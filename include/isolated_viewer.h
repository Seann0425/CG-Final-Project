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

  bool isDragging;
  float lastMouseX;
  float lastMouseY;

  float cameraYaw;
  float cameraPitch;
  float cameraRadius;

  glm::vec3 focusPoint;

  void uploadDataToGPU();
  void freeGPUResources();

  static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
  static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
  static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

  void onMouseButton(int button, int action);
  void onCursorPos(double xpos, double ypos);
  void onScroll(double yoffset);
};

#endif