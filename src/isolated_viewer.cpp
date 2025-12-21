#include "isolated_viewer.h"
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

IsolatedViewer::IsolatedViewer()
    : currentTarget(nullptr),
      active(false),
      viewerWindow(nullptr),
      mainWindow(nullptr),
      vao(0),
      vboPosition(0),
      vboNormal(0),
      vboTexCoord(0),
      isDragging(false),
      lastMouseX(0.0f),
      lastMouseY(0.0f),
      cameraYaw(-90.0f),
      cameraPitch(0.0f),
      cameraRadius(10.0f),
      focusPoint(0.0f, 0.0f, 0.0f) {}

IsolatedViewer::~IsolatedViewer() {
  freeGPUResources();
  if (viewerWindow) {
    glfwDestroyWindow(viewerWindow);
  }
}

void IsolatedViewer::init(GLFWwindow* shareWindow) {
  this->mainWindow = shareWindow;

  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);

  viewerWindow = glfwCreateWindow(800, 600, "Isolated Viewer", NULL, shareWindow);

  if (!viewerWindow) {
    std::cerr << "Failed to create Isolated Viewer window" << std::endl;
    return;
  }

  glfwSetWindowUserPointer(viewerWindow, this);
  glfwSetMouseButtonCallback(viewerWindow, IsolatedViewer::mouseButtonCallback);
  glfwSetCursorPosCallback(viewerWindow, IsolatedViewer::cursorPosCallback);
  glfwSetScrollCallback(viewerWindow, IsolatedViewer::scrollCallback);
}

void IsolatedViewer::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
  IsolatedViewer* viewer = static_cast<IsolatedViewer*>(glfwGetWindowUserPointer(window));
  if (viewer) {
    viewer->onMouseButton(button, action);
  }
}

void IsolatedViewer::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
  IsolatedViewer* viewer = static_cast<IsolatedViewer*>(glfwGetWindowUserPointer(window));
  if (viewer) {
    viewer->onCursorPos(xpos, ypos);
  }
}

void IsolatedViewer::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
  IsolatedViewer* viewer = static_cast<IsolatedViewer*>(glfwGetWindowUserPointer(window));
  if (viewer) {
    viewer->onScroll(yoffset);
  }
}

void IsolatedViewer::onMouseButton(int button, int action) {
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (action == GLFW_PRESS) {
      isDragging = true;
      double xpos, ypos;
      glfwGetCursorPos(viewerWindow, &xpos, &ypos);
      lastMouseX = static_cast<float>(xpos);
      lastMouseY = static_cast<float>(ypos);
    } else if (action == GLFW_RELEASE) {
      isDragging = false;
    }
  }
}

void IsolatedViewer::onCursorPos(double xpos, double ypos) {
  if (!isDragging) return;

  float xoffset = static_cast<float>(xpos) - lastMouseX;
  float yoffset = lastMouseY - static_cast<float>(ypos);
  lastMouseX = static_cast<float>(xpos);
  lastMouseY = static_cast<float>(ypos);

  float sensitivity = 0.3f;
  xoffset *= sensitivity;
  yoffset *= sensitivity;

  cameraYaw += xoffset;
  cameraPitch -= yoffset;

  if (cameraPitch > 89.0f) cameraPitch = 89.0f;
  if (cameraPitch < -89.0f) cameraPitch = -89.0f;
}

void IsolatedViewer::onScroll(double yoffset) {
  cameraRadius -= static_cast<float>(yoffset);
  if (cameraRadius < 1.0f) cameraRadius = 1.0f;
  if (cameraRadius > 50.0f) cameraRadius = 50.0f;
}

void IsolatedViewer::setTarget(std::shared_ptr<Model> target) {
  if (!viewerWindow) return;

  glfwMakeContextCurrent(viewerWindow);
  freeGPUResources();

  if (target) {
    currentTarget = target;
    active = true;
    uploadDataToGPU();

    // Reset Camera
    cameraRadius = 10.0f;
    cameraYaw = -90.0f;
    cameraPitch = 0.0f;
    focusPoint = glm::vec3(0.0f, 0.0f, 0.0f);

    glfwSetWindowShouldClose(viewerWindow, GLFW_FALSE);
    glfwShowWindow(viewerWindow);
  } else {
    clearTarget();
  }
  glfwMakeContextCurrent(mainWindow);
}

void IsolatedViewer::clearTarget() {
  if (viewerWindow) {
    glfwHideWindow(viewerWindow);
  }
  currentTarget = nullptr;
  active = false;
}

bool IsolatedViewer::isActive() const { return active; }

void IsolatedViewer::freeGPUResources() {
  if (vao != 0) glDeleteVertexArrays(1, &vao);
  if (vboPosition != 0) glDeleteBuffers(1, &vboPosition);
  if (vboNormal != 0) glDeleteBuffers(1, &vboNormal);
  if (vboTexCoord != 0) glDeleteBuffers(1, &vboTexCoord);

  vao = 0;
  vboPosition = 0;
  vboNormal = 0;
  vboTexCoord = 0;
}

void IsolatedViewer::uploadDataToGPU() {
  if (!currentTarget) return;

  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  if (!currentTarget->positions.empty()) {
    glGenBuffers(1, &vboPosition);
    glBindBuffer(GL_ARRAY_BUFFER, vboPosition);
    glBufferData(GL_ARRAY_BUFFER, currentTarget->positions.size() * sizeof(float), currentTarget->positions.data(),
                 GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
  }

  if (!currentTarget->normals.empty()) {
    glGenBuffers(1, &vboNormal);
    glBindBuffer(GL_ARRAY_BUFFER, vboNormal);
    glBufferData(GL_ARRAY_BUFFER, currentTarget->normals.size() * sizeof(float), currentTarget->normals.data(),
                 GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
  }

  if (!currentTarget->texcoords.empty()) {
    glGenBuffers(1, &vboTexCoord);
    glBindBuffer(GL_ARRAY_BUFFER, vboTexCoord);
    glBufferData(GL_ARRAY_BUFFER, currentTarget->texcoords.size() * sizeof(float), currentTarget->texcoords.data(),
                 GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);
  }

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void IsolatedViewer::render(Program& shader) {
  if (!active || !currentTarget || !viewerWindow) return;

  if (glfwWindowShouldClose(viewerWindow)) {
    clearTarget();
    return;
  }

  glfwMakeContextCurrent(viewerWindow);

  // ... (Viewport 與 Clear 設定保持不變) ...
  int width, height;
  glfwGetFramebufferSize(viewerWindow, &width, &height);
  if (width == 0 || height == 0) return;  // 視窗最小化時直接返回
  glViewport(0, 0, width, height);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  shader.use();

  // === [新增] WASD 平移邏輯 ===
  // 計算相機的前、右、上向量，確保移動方向是相對於「目前的視角」
  // 1. 計算目前的相機方向 (從球面座標算出來的)
  glm::vec3 front;
  front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
  front.y = sin(glm::radians(cameraPitch));
  front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
  glm::vec3 cameraFront = glm::normalize(front);

  glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront, glm::vec3(0.0f, 1.0f, 0.0f)));
  glm::vec3 cameraUp = glm::normalize(glm::cross(cameraRight, cameraFront));

  float moveSpeed = 0.05f * cameraRadius * 0.5f;  // 移動速度隨距離縮放，離越遠動越快

  // W: 向上平移 (沿著相機的 Up 向量)
  if (glfwGetKey(viewerWindow, GLFW_KEY_W) == GLFW_PRESS) focusPoint += cameraUp * moveSpeed;

  // S: 向下平移
  if (glfwGetKey(viewerWindow, GLFW_KEY_S) == GLFW_PRESS) focusPoint -= cameraUp * moveSpeed;

  // A: 向左平移 (沿著相機的 Right 向量的反方向)
  if (glfwGetKey(viewerWindow, GLFW_KEY_A) == GLFW_PRESS) focusPoint -= cameraRight * moveSpeed;

  // D: 向右平移
  if (glfwGetKey(viewerWindow, GLFW_KEY_D) == GLFW_PRESS) focusPoint += cameraRight * moveSpeed;
  // ============================

  // Camera Calculation (Orbit + Pan)
  // 相機位置 = 對焦點 + 球面座標偏移量
  float camX = cameraRadius * cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
  float camY = cameraRadius * sin(glm::radians(cameraPitch));
  float camZ = cameraRadius * sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));

  // 最終相機位置要加上 focusPoint
  glm::vec3 cameraPos = focusPoint + glm::vec3(camX, camY, camZ);

  // Matrices
  glm::mat4 model = currentTarget->modelMatrix;
  // View Matrix: 相機位置 -> 看向 focusPoint -> 上方為 (0,1,0)
  // 注意：這裡的 Up 向量建議用 World Up (0,1,0)，除非你要做飛行模擬的滾轉
  glm::mat4 view = glm::lookAt(cameraPos, focusPoint, glm::vec3(0, 1, 0));
  glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 200.0f);
  glm::mat4 tiModel = glm::transpose(glm::inverse(model));

  // ... (後面的 Uniform 設定與 Drawing 保持不變) ...
  // Copy 之前的 code 即可，記得 viewPos 也要更新成新的 cameraPos

  GLint viewLoc = glGetUniformLocation(shader.getHandle(), "ViewMatrix");
  GLint projLoc = glGetUniformLocation(shader.getHandle(), "Projection");
  GLint modelLoc = glGetUniformLocation(shader.getHandle(), "ModelMatrix");
  GLint tiModelLoc = glGetUniformLocation(shader.getHandle(), "TIModelMatrix");

  if (viewLoc >= 0) glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
  if (projLoc >= 0) glUniformMatrix4fv(projLoc, 1, GL_FALSE, &proj[0][0]);
  if (modelLoc >= 0) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
  if (tiModelLoc >= 0) glUniformMatrix4fv(tiModelLoc, 1, GL_FALSE, &tiModel[0][0]);

  GLint viewPosLoc = glGetUniformLocation(shader.getHandle(), "viewPos");
  if (viewPosLoc >= 0) glUniform3fv(viewPosLoc, 1, &cameraPos[0]);

  // Light (Headlamp)
  // 讓燈光繼續跟著相機走，方向設為 (focusPoint - cameraPos) 即 -offset
  glUniform1i(glGetUniformLocation(shader.getHandle(), "dl.enable"), 1);
  glUniform3f(glGetUniformLocation(shader.getHandle(), "dl.direction"), -camX, -camY, -camZ);
  glUniform3f(glGetUniformLocation(shader.getHandle(), "dl.lightColor"), 1.0f, 1.0f, 1.0f);

  // ... 其他 Light/Material 設定 ...
  glUniform1i(glGetUniformLocation(shader.getHandle(), "pl.enable"), 0);
  glUniform1i(glGetUniformLocation(shader.getHandle(), "sl.enable"), 0);

  glUniform3f(glGetUniformLocation(shader.getHandle(), "material.ambient"), 0.3f, 0.3f, 0.3f);
  glUniform3f(glGetUniformLocation(shader.getHandle(), "material.diffuse"), 0.8f, 0.8f, 0.8f);
  glUniform3f(glGetUniformLocation(shader.getHandle(), "material.specular"), 0.8f, 0.8f, 0.8f);
  glUniform1f(glGetUniformLocation(shader.getHandle(), "material.shininess"), 64.0f);
  glUniform1f(glGetUniformLocation(shader.getHandle(), "material.reflectivity"), 0.0f);

  if (vao != 0) {
    glBindVertexArray(0);
    glBindVertexArray(vao);
    if (!currentTarget->textures.empty()) {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, currentTarget->textures[0]);
      glUniform1i(glGetUniformLocation(shader.getHandle(), "ourTexture"), 0);
    }
    glDrawArrays(currentTarget->drawMode, 0, currentTarget->numVertex);
    glBindVertexArray(0);
  }

  glfwSwapBuffers(viewerWindow);
  glfwMakeContextCurrent(mainWindow);

  glEnable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
}