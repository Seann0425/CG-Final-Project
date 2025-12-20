#include "isolated_viewer.h"
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
      vboTexCoord(0) {}

IsolatedViewer::~IsolatedViewer() {
  freeGPUResources();  // 這裡其實有風險，因為 Context 可能不對，但程式結束時通常無所謂
  if (viewerWindow) {
    glfwDestroyWindow(viewerWindow);
  }
}

void IsolatedViewer::init(GLFWwindow* shareWindow) {
  this->mainWindow = shareWindow;

  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);  // 讓它保持在最上層

  // 建立視窗，並共享資源
  viewerWindow = glfwCreateWindow(800, 600, "Isolated Viewer", NULL, shareWindow);

  if (!viewerWindow) {
    std::cerr << "Failed to create Isolated Viewer window" << std::endl;
  }
}

void IsolatedViewer::setTarget(std::shared_ptr<Model> target) {
  if (!viewerWindow) return;

  // 1. 切換 Context 到新視窗 (建立 VAO 必須在該視窗的 Context 下)
  glfwMakeContextCurrent(viewerWindow);

  // 2. 為了避免第二次打開全白，我們先清除舊的 VAO
  freeGPUResources();

  if (target) {
    currentTarget = target;
    active = true;

    // 3. 上傳資料
    uploadDataToGPU();

    // 4. 重置視窗關閉旗標 (這很重要，不然第二次打開會馬上被視為要關閉)
    glfwSetWindowShouldClose(viewerWindow, GLFW_FALSE);

    // 5. 顯示視窗
    glfwShowWindow(viewerWindow);
  } else {
    clearTarget();
  }

  // 6. 切回主視窗
  glfwMakeContextCurrent(mainWindow);
}

void IsolatedViewer::clearTarget() {
  if (viewerWindow) {
    glfwHideWindow(viewerWindow);
  }
  currentTarget = nullptr;
  active = false;
  // 不要在這裡切換 Context 清除 GPU 資源，留給下一次 setTarget 或 解構函數
}

bool IsolatedViewer::isActive() const { return active; }

void IsolatedViewer::freeGPUResources() {
  // 確保只在有資源時釋放
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

  // 此時 Context 已經是 viewerWindow 了
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  // Positions
  if (!currentTarget->positions.empty()) {
    glGenBuffers(1, &vboPosition);
    glBindBuffer(GL_ARRAY_BUFFER, vboPosition);
    glBufferData(GL_ARRAY_BUFFER, currentTarget->positions.size() * sizeof(float), currentTarget->positions.data(),
                 GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
  }

  // Normals
  if (!currentTarget->normals.empty()) {
    glGenBuffers(1, &vboNormal);
    glBindBuffer(GL_ARRAY_BUFFER, vboNormal);
    glBufferData(GL_ARRAY_BUFFER, currentTarget->normals.size() * sizeof(float), currentTarget->normals.data(),
                 GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
  }

  // TexCoords
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

  // 1. 切換 Context
  glfwMakeContextCurrent(viewerWindow);

  // 更新 Viewport
  int width, height;
  glfwGetFramebufferSize(viewerWindow, &width, &height);
  if (width == 0 || height == 0) return;
  glViewport(0, 0, width, height);

  // 2. 清除畫面
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // 3. 設定 Shader
  shader.use();

  // === 計算變換矩陣 ===
  float time = (float)glfwGetTime();

  // 建立檢視器旋轉 (自轉)
  glm::mat4 viewerTransform = glm::mat4(1.0f);
  viewerTransform = glm::rotate(viewerTransform, time * 1.5f, glm::vec3(0.0f, 1.0f, 0.0f));
  viewerTransform = glm::rotate(viewerTransform, time * 0.5f, glm::vec3(1.0f, 0.0f, 0.0f));

  // [重要] 結合原本 Model 的矩陣 (包含縮放)
  glm::mat4 model = viewerTransform * currentTarget->modelMatrix;

  // View: 相機位置
  glm::vec3 cameraPos = glm::vec3(0.0f, 5.0f, 10.0f);
  glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

  // Projection
  glm::mat4 proj = glm::perspective(glm::radians(60.0f), (float)width / height, 0.1f, 200.0f);

  // [新增] 計算 Normal Matrix (TIModelMatrix)
  // 這是給光照計算用的，如果不傳這個，光影會是錯的
  glm::mat4 tiModel = glm::transpose(glm::inverse(model));

  // === [關鍵修正] 使用正確的 Uniform 名稱 ===
  // 對應 light.vert 中的變數名

  GLint viewLoc = glGetUniformLocation(shader.getHandle(), "ViewMatrix");        // 改名了
  GLint projLoc = glGetUniformLocation(shader.getHandle(), "Projection");        // 改名了
  GLint modelLoc = glGetUniformLocation(shader.getHandle(), "ModelMatrix");      // 改名了
  GLint tiModelLoc = glGetUniformLocation(shader.getHandle(), "TIModelMatrix");  // 新增

  if (viewLoc >= 0) glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
  if (projLoc >= 0) glUniformMatrix4fv(projLoc, 1, GL_FALSE, &proj[0][0]);
  if (modelLoc >= 0) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
  if (tiModelLoc >= 0) glUniformMatrix4fv(tiModelLoc, 1, GL_FALSE, &tiModel[0][0]);

  // === 設定 Light & Material (同前一次設定) ===
  // 這裡也要注意，如果你的 fragment shader 變數名也有改，要對應修改
  // 目前假設 frag shader 沒變

  GLint viewPosLoc = glGetUniformLocation(shader.getHandle(), "viewPos");
  if (viewPosLoc >= 0) glUniform3fv(viewPosLoc, 1, &cameraPos[0]);

  // Directional Light
  glUniform1i(glGetUniformLocation(shader.getHandle(), "dl.enable"), 1);
  glUniform3f(glGetUniformLocation(shader.getHandle(), "dl.direction"), -0.5f, -1.0f, -0.5f);
  glUniform3f(glGetUniformLocation(shader.getHandle(), "dl.lightColor"), 1.0f, 1.0f, 1.0f);

  glUniform1i(glGetUniformLocation(shader.getHandle(), "pl.enable"), 0);
  glUniform1i(glGetUniformLocation(shader.getHandle(), "sl.enable"), 0);

  // Material
  glUniform3f(glGetUniformLocation(shader.getHandle(), "material.ambient"), 0.3f, 0.3f, 0.3f);
  glUniform3f(glGetUniformLocation(shader.getHandle(), "material.diffuse"), 0.8f, 0.8f, 0.8f);
  glUniform3f(glGetUniformLocation(shader.getHandle(), "material.specular"), 0.8f, 0.8f, 0.8f);
  glUniform1f(glGetUniformLocation(shader.getHandle(), "material.shininess"), 64.0f);
  glUniform1f(glGetUniformLocation(shader.getHandle(), "material.reflectivity"), 0.0f);

  // 4. 繪製
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

  // 恢復主視窗狀態
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
}