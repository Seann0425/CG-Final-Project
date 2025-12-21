#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

#include <GLFW/glfw3.h>
#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#undef GLAD_GL_IMPLEMENTATION
#include <glm/glm.hpp>

#include <glm/ext/matrix_transform.hpp>

#include "camera.h"
#include "context.h"
#include "gl_helper.h"
#include "isolated_viewer.h"
#include "model.h"
#include "opengl_context.h"
#include "program.h"
#include "utils.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
// https://sketchfab.com/3d-models/eva-d434dfc3cb9244fbba83407ccabdd523#download ���J�o�Ӿ����H
void initOpenGL();
void resizeCallback(GLFWwindow* window, int width, int height);
void keyCallback(GLFWwindow* window, int key, int, int action, int);

Context ctx;
IsolatedViewer g_isolatedViewer;

Material mFlatwhite;
Material mShinyred;
Material mClearblue;
Material mMirror;

class DepthProgram : public ExampleProgram {
 public:
  glm::mat4 lightSpaceMatrix;

  DepthProgram(Context* ctx) : ExampleProgram(ctx) {
    vertProgramFile = "../assets/shaders/depth.vert";
    fragProgramFIle = "../assets/shaders/depth.frag";
  }

  void doMainLoop() override {
    glUseProgram(programId);

    // global light space matrix
    GLint loc = glGetUniformLocation(programId, "lightSpaceMatrix");
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    // repeat all object
    int obj_num = (int)ctx->objects.size();
    for (int i = 0; i < obj_num; i++) {
      int modelIndex = ctx->objects[i]->modelIndex;
      glBindVertexArray(VAO[modelIndex]);
      Model* model = ctx->models[modelIndex];

      glm::mat4 finalModel = ctx->objects[i]->transformMatrix * model->modelMatrix;
      GLint mmatLoc = glGetUniformLocation(programId, "model");
      glUniformMatrix4fv(mmatLoc, 1, GL_FALSE, glm::value_ptr(finalModel));

      glDrawArrays(model->drawMode, 0, model->numVertex);
    }
    glBindVertexArray(0);
    glUseProgram(0);
  }
};

void loadMaterial() {
  mFlatwhite.ambient = glm::vec3(0.5f, 0.5f, 0.5f);
  mFlatwhite.diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
  mFlatwhite.specular = glm::vec3(0.0f, 0.0f, 0.0f);
  mFlatwhite.shininess = 10;

  mShinyred.ambient = glm::vec3(0.1985f, 0.0000f, 0.0000f);
  mShinyred.diffuse = glm::vec3(0.5921f, 0.0167f, 0.0000f);
  mShinyred.specular = glm::vec3(0.5973f, 0.2083f, 0.2083f);
  mShinyred.shininess = 100.0f;

  mClearblue.ambient = glm::vec3(0.0394f, 0.0394f, 0.3300f);
  mClearblue.diffuse = glm::vec3(0.1420f, 0.1420f, 0.9500f);
  mClearblue.specular = glm::vec3(0.1420f, 0.1420f, 0.9500f);
  mClearblue.shininess = 10;

  mMirror.ambient = glm::vec3(0.2f, 0.2f, 0.2f);
  mMirror.diffuse = glm::vec3(0.7f, 0.7f, 0.7f);
  mMirror.specular = glm::vec3(0.5f, 0.5f, 0.5f);
  mMirror.shininess = 10.0f;
  mMirror.reflectivity = 0.3f;
}

void loadPrograms() {
  ctx.programs.push_back(new SkyboxProgram(&ctx));
  ctx.programs.push_back(new LightProgram(&ctx));

  for (auto iter = ctx.programs.begin(); iter != ctx.programs.end(); iter++) {
    if (!(*iter)->load()) {
      std::cout << "Load program fail, force terminate" << std::endl;
      exit(1);
    }
  }
  glUseProgram(0);
}

Model* createBottle() {
  /* TODO#1-1: Add the bottle model
   *           1. Create a model by reading the model file "../assets/models/bottle/bottle.obj" with the object
   * loader(Model::fromObjectFile()) you write.
   *           2. Add the texture "../assets/models/bottle/bottle.jpg" to the model.
   *           3. Do transform(rotation & scale) to the model.
   *           4. Set the drawMode for this model
   * Note:
   *           You should finish implement the object loader(Model::fromObjectFile()) first.
   *           You can refer to the Model class structure in model.h.
   * Hint:
   *           Model* m = Model::fromObjectFile();
   *           m->textures.push_back();
   *           m->modelMatrix = glm::scale(m->modelMatrix, glm::vec3(0.05f, 0.05f, 0.05f));
   *           m->modelMatrix = glm::rotate(m->modelMatrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
   *           m->modelMatrix = glm::rotate(m->modelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
   *           m->drawMode =
   */
  Model* m = Model::fromObjectFile("../assets/models/bottle/bottle.obj");
  GLuint BottleTexture = createTexture("../assets/models/bottle/bottle.jpg");
  m->textures.push_back(BottleTexture);

  m->modelMatrix = glm::scale(m->modelMatrix, glm::vec3(0.05f));
  m->modelMatrix = glm::rotate(m->modelMatrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
  m->modelMatrix = glm::rotate(m->modelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

  return m;
}
Model* createRobot() {
  Model* m = Model::fromObjectFile("../assets/models/robot/robot.obj");
  if (m == NULL) {
    std::cout << "ERROR: createRobot failed to load file!" << std::endl;
    return NULL;
  }

  GLuint whiteTexture;
  glGenTextures(1, &whiteTexture);
  glBindTexture(GL_TEXTURE_2D, whiteTexture);
  unsigned char whitePixel[] = {192, 192, 192};  // sliver
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, whitePixel);

  m->textures.push_back(whiteTexture);
  m->modelMatrix = glm::scale(m->modelMatrix, glm::vec3(0.5f));

  m->drawMode = GL_TRIANGLES;
  return m;
}

Model* createPlane() {
  /* TODO#1-2: Add a plane model
   *           1. Create a model and manually set plane positions, normals, texcoords
   *           2. Add texure "../assets/models/Wood_maps/AT_Wood.jpg"
   *           3. Set m->numVertex, m->drawMode
   * Note:
   *           GL_TEXTURE_WRAP is set to GL_REPEAT in createTexture, you may need to know
   *           what this means to set m->textures correctly
   */
  Model* m = new Model();
  GLuint PlaneTexture = createTexture("../assets/models/Wood_maps/AT_Wood.jpg");
  float pos[] = {
      // (x, y, z)
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 5.12f, 8.192f, 0.0f, 5.12f, 8.192f, 0.0f, 0.0f,
  };

  float nor[]{
      0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
  };

  float tx[] = {
      0.0f, 0.0f, 0.0f, 2.0f, 2.0f, 2.0f, 2.0f, 0.0f,
  };

  for (int i = 0; i < 12; i++) {
    m->positions.push_back(pos[i]);
    m->normals.push_back(nor[i]);
  }
  for (int i = 0; i < 8; i++) {
    m->texcoords.push_back(tx[i]);
  }

  m->textures.push_back(PlaneTexture);

  m->numVertex = 4;
  m->drawMode = GL_QUADS;  // draw triangle
  return m;
}

float bezier(float t, float p0, float p1, float p2, float p3) {
  return pow(1 - t, 3) * p0 + 3 * pow(1 - t, 2) * t * p1 + 3 * (1 - t) * t * t * p2 + t * t * t * p3;
}

const float PI = 3.14159265359f;

Model* createBezierVaseModel() {
  const int segments = 36;         // Circular segments
  const int height_segments = 50;  // Height segments
  float height = 1.0f;             // Vase Height

  // Control points for the Bezier curve (you can try adjusting these to shape the vase)
  float p0 = 0.2f;  // Radius at base
  float p1 = 1.0f;  // Control point 1
  float p2 = 0.2f;  // Control point 2
  float p3 = 0.1f;  // Radius at neck

  /* TODO#1-3: Add a vase outer surface model
   *           1. Create a model and manually set vase positions, normals, texcoords
   *           2. Add texure "../assets/models/Vase/Vase.jpg"
   *           3. Set m->numVertex, m->drawMode
   * Note:
   *           You should refer to the cubic bezier curve function bezier().
   */
  Model* vase = new Model();
  for (int h = 0; h < height_segments; h++) {
    float t0 = static_cast<float>(h) / height_segments;
    float t1 = static_cast<float>(h + 1) / height_segments;

    float y0 = t0 * height;
    float y1 = t1 * height;

    float r0 = bezier(t0, p0, p1, p2, p3);
    float r1 = bezier(t1, p0, p1, p2, p3);

    for (int i = 0; i < segments; ++i) {
      float a0 = 2.0f * PI * static_cast<float>(i) / segments;
      float a1 = 2.0f * PI * static_cast<float>(i + 1) / segments;

      glm::vec3 p00(r0 * cos(a0), y0, r0 * sin(a0));  // h, i
      glm::vec3 p01(r0 * cos(a1), y0, r0 * sin(a1));  // h, i+1
      glm::vec3 p10(r1 * cos(a0), y1, r1 * sin(a0));  // h+1, i
      glm::vec3 p11(r1 * cos(a1), y1, r1 * sin(a1));  // h+1, i+1

      // face normal(to out)
      glm::vec3 n = glm::normalize(glm::cross(p10 - p00, p11 - p00));

      float u0 = static_cast<float>(i) / segments;
      float u1 = static_cast<float>(i + 1) / segments;
      float v0 = t0;
      float v1 = t1;

      glm::vec2 t00(u0, v0);
      glm::vec2 t01(u1, v0);
      glm::vec2 t10(u0, v1);
      glm::vec2 t11(u1, v1);

      // �T���� 1�Gp00, p10, p11
      vase->positions.push_back(p00.x);
      vase->positions.push_back(p00.y);
      vase->positions.push_back(p00.z);
      vase->positions.push_back(p10.x);
      vase->positions.push_back(p10.y);
      vase->positions.push_back(p10.z);
      vase->positions.push_back(p11.x);
      vase->positions.push_back(p11.y);
      vase->positions.push_back(p11.z);

      for (int k = 0; k < 3; ++k) {
        vase->normals.push_back(n.x);
        vase->normals.push_back(n.y);
        vase->normals.push_back(n.z);
      }

      vase->texcoords.push_back(t00.x);
      vase->texcoords.push_back(t00.y);
      vase->texcoords.push_back(t10.x);
      vase->texcoords.push_back(t10.y);
      vase->texcoords.push_back(t11.x);
      vase->texcoords.push_back(t11.y);

      // �T���� 2�Gp00, p11, p01
      vase->positions.push_back(p00.x);
      vase->positions.push_back(p00.y);
      vase->positions.push_back(p00.z);
      vase->positions.push_back(p11.x);
      vase->positions.push_back(p11.y);
      vase->positions.push_back(p11.z);
      vase->positions.push_back(p01.x);
      vase->positions.push_back(p01.y);
      vase->positions.push_back(p01.z);

      for (int k = 0; k < 3; ++k) {
        vase->normals.push_back(n.x);
        vase->normals.push_back(n.y);
        vase->normals.push_back(n.z);
      }

      vase->texcoords.push_back(t00.x);
      vase->texcoords.push_back(t00.y);
      vase->texcoords.push_back(t11.x);
      vase->texcoords.push_back(t11.y);
      vase->texcoords.push_back(t01.x);
      vase->texcoords.push_back(t01.y);
    }
  }

  vase->textures.push_back(createTexture("../assets/models/Vase/Vase.jpg"));
  vase->numVertex = static_cast<GLsizei>(vase->positions.size() / 3);
  vase->drawMode = GL_TRIANGLES;
  return vase;
}

Model* createBezierVaseInnerModel() {
  const int segments = 36;         // Circular segments
  const int height_segments = 50;  // Height segments
  float height = 1.0f;             // Vase Height

  // Control points for the Bezier curve (adjust these to shape the vase)
  float p0 = 0.2f;  // Radius at base
  float p1 = 1.0f;  // Control point 1
  float p2 = 0.2f;  // Control point 2
  float p3 = 0.1f;  // Radius at neck

  const float thicknessScale = 0.9f;
  /* TODO#1-4: Add a vase inner surface model
   *           1. Create a model and manually set vase positions, normals, texcoords
   *           2. Add texure "../assets/models/Vase/Vase2.jpg"
   *           3. Set m->numVertex, m->drawMode
   * Note:
   *           You should refer to the cubic bezier curve function bezier().
   */
  Model* vase = new Model();
  for (int h = 0; h < height_segments; ++h) {
    float t0 = static_cast<float>(h) / height_segments;
    float t1 = static_cast<float>(h + 1) / height_segments;

    float y0 = t0 * height;
    float y1 = t1 * height;

    float r0 = bezier(t0, p0, p1, p2, p3) * thicknessScale;
    float r1 = bezier(t1, p0, p1, p2, p3) * thicknessScale;

    for (int i = 0; i < segments; ++i) {
      float a0 = 2.0f * PI * static_cast<float>(i) / segments;
      float a1 = 2.0f * PI * static_cast<float>(i + 1) / segments;

      glm::vec3 p00(r0 * cos(a0), y0, r0 * sin(a0));
      glm::vec3 p01(r0 * cos(a1), y0, r0 * sin(a1));
      glm::vec3 p10(r1 * cos(a0), y1, r1 * sin(a0));
      glm::vec3 p11(r1 * cos(a1), y1, r1 * sin(a1));

      // ����~�����k�V�A�A���������¤�
      glm::vec3 nOuter = glm::normalize(glm::cross(p10 - p00, p11 - p00));
      glm::vec3 n = -nOuter;  //  normal �¤�

      float u0 = static_cast<float>(i) / segments;
      float u1 = static_cast<float>(i + 1) / segments;
      float v0 = t0;
      float v1 = t1;

      glm::vec2 t00(u0, v0);
      glm::vec2 t01(u1, v0);
      glm::vec2 t10(u0, v1);
      glm::vec2 t11(u1, v1);

      // front face �¤�
      // �T���� 1�Gp00, p11, p10
      vase->positions.push_back(p00.x);
      vase->positions.push_back(p00.y);
      vase->positions.push_back(p00.z);
      vase->positions.push_back(p11.x);
      vase->positions.push_back(p11.y);
      vase->positions.push_back(p11.z);
      vase->positions.push_back(p10.x);
      vase->positions.push_back(p10.y);
      vase->positions.push_back(p10.z);

      for (int k = 0; k < 3; ++k) {
        vase->normals.push_back(n.x);
        vase->normals.push_back(n.y);
        vase->normals.push_back(n.z);
      }

      vase->texcoords.push_back(t00.x);
      vase->texcoords.push_back(t00.y);
      vase->texcoords.push_back(t11.x);
      vase->texcoords.push_back(t11.y);
      vase->texcoords.push_back(t10.x);
      vase->texcoords.push_back(t10.y);

      vase->positions.push_back(p00.x);
      vase->positions.push_back(p00.y);
      vase->positions.push_back(p00.z);
      vase->positions.push_back(p01.x);
      vase->positions.push_back(p01.y);
      vase->positions.push_back(p01.z);
      vase->positions.push_back(p11.x);
      vase->positions.push_back(p11.y);
      vase->positions.push_back(p11.z);

      for (int k = 0; k < 3; ++k) {
        vase->normals.push_back(n.x);
        vase->normals.push_back(n.y);
        vase->normals.push_back(n.z);
      }

      vase->texcoords.push_back(t00.x);
      vase->texcoords.push_back(t00.y);
      vase->texcoords.push_back(t01.x);
      vase->texcoords.push_back(t01.y);
      vase->texcoords.push_back(t11.x);
      vase->texcoords.push_back(t11.y);
    }
  }

  vase->textures.push_back(createTexture("../assets/models/Vase/Vase2.jpg"));
  vase->numVertex = static_cast<GLsizei>(vase->positions.size() / 3);
  vase->drawMode = GL_TRIANGLES;
  return vase;
}

Model* createBezierVaseBottomModel() {
  /* TODO#1-5: Add a vase bottom surface model
   *           1. Create a model and manually set vase positions, normals, texcoords
   *           2. Add texure "../assets/models/Vase/Vase2.jpg"
   *           3. Set m->numVertex, m->drawMode
   * Note:
   *           You should refer to the cubic bezier curve function bezier().
   */
  const int segments = 36;
  float height = 1.0f;

  float p0 = 0.2f;
  float p1 = 1.0f;
  float p2 = 0.2f;
  float p3 = 0.1f;
  float r = bezier(0.0f, p0, p1, p2, p3);

  Model* m = new Model();

  glm::vec3 center(0.0f, 0.0f, 0.0f);
  glm::vec3 normal(0.0f, -1.0f, 0.0f);

  m->positions.push_back(center.x);
  m->positions.push_back(center.y);
  m->positions.push_back(center.z);

  m->normals.push_back(normal.x);
  m->normals.push_back(normal.y);
  m->normals.push_back(normal.z);

  m->texcoords.push_back(0.5f);
  m->texcoords.push_back(0.5f);

  for (int i = 0; i <= segments; ++i) {
    float a = 2.0f * PI * static_cast<float>(i) / segments;
    glm::vec3 p(r * cos(a), 0.0f, r * sin(a));

    m->positions.push_back(p.x);
    m->positions.push_back(p.y);
    m->positions.push_back(p.z);

    m->normals.push_back(normal.x);
    m->normals.push_back(normal.y);
    m->normals.push_back(normal.z);

    float u = 0.5f + (p.x / (2.0f * r));
    float v = 0.5f + (p.z / (2.0f * r));
    m->texcoords.push_back(u);
    m->texcoords.push_back(v);
  }

  // �� triangle fan �e�Gcenter + (i, i+1)
  m->numVertex = segments * 3;  // 1 ���� 1 �T���� = 3 ���I
  m->drawMode = GL_TRIANGLES;

  std::vector<float> pos2, nor2, tex2;
  for (int i = 0; i < segments; ++i) {
    int i1 = i + 1;
    int i2 = i + 2;

    // v0 = center
    int idx0 = 0;
    // v1 = i1
    int idx1 = i1;
    // v2 = i2
    int idx2 = i2;

    auto pushVertex = [&](int idx) {
      pos2.push_back(m->positions[idx * 3 + 0]);
      pos2.push_back(m->positions[idx * 3 + 1]);
      pos2.push_back(m->positions[idx * 3 + 2]);

      nor2.push_back(m->normals[idx * 3 + 0]);
      nor2.push_back(m->normals[idx * 3 + 1]);
      nor2.push_back(m->normals[idx * 3 + 2]);

      tex2.push_back(m->texcoords[idx * 2 + 0]);
      tex2.push_back(m->texcoords[idx * 2 + 1]);
    };

    pushVertex(idx0);
    pushVertex(idx1);
    pushVertex(idx2);
  }

  m->positions.swap(pos2);
  m->normals.swap(nor2);
  m->texcoords.swap(tex2);

  m->textures.push_back(createTexture("../assets/models/Vase/Vase2.jpg"));

  return m;
}

void loadModels() {
  /* TODO#2-1: Push the model to ctx.models
   * Note:
   *    You can refer to the context class in context.h and model class in model.h
   * Hint:
        ctx.models.push_back();
   */
  ctx.models.push_back(createPlane());
  ctx.models.push_back(createBottle());
  ctx.models.push_back(createBezierVaseModel());
  ctx.models.push_back(createBezierVaseInnerModel());
  ctx.models.push_back(createBezierVaseBottomModel());
  ctx.models.push_back(createRobot());
}

float robot_x = 0.0f;
float robot_z = 0.0f;
void setupObjects() {
  /* TODO#2-2: Set up the object by the model vector
   * Note:
   *    You can refer to the context class in context.h and objects structure in model.h
   * Hint:
   *    ctx.objects.push_back(new Object(0, glm::translate(glm::identity<glm::mat4>(), glm::vec3(1.5, 0.4, 3))));
   *    (*ctx.objects.rbegin())->material = mMirror;
   */
  glm::mat4 vaseform = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 0.0f, 3.0f));
  glm::mat4 robotform = glm::translate(glm::mat4(1.0f), glm::vec3(robot_x, 0.0f, robot_z));
  ctx.objects.push_back(new Object(0, glm::translate(glm::identity<glm::mat4>(), glm::vec3(0.0, 0.0, 0.0))));
  (*ctx.objects.rbegin())->material = mFlatwhite;
  ctx.objects.push_back(new Object(1, glm::translate(glm::mat4(1.0f), glm::vec3(2.0, 0, 3.0))));
  (*ctx.objects.rbegin())->material = mFlatwhite;
  ctx.objects.push_back(new Object(2, vaseform));
  (*ctx.objects.rbegin())->material = mMirror;
  ctx.objects.push_back(new Object(3, vaseform));
  (*ctx.objects.rbegin())->material = mFlatwhite;
  ctx.objects.push_back(new Object(4, vaseform));
  (*ctx.objects.rbegin())->material = mFlatwhite;
  ctx.objects.push_back(new Object(5, robotform));
  (*ctx.objects.rbegin())->material = mFlatwhite;
}

int main() {
  initOpenGL();
  GLFWwindow* window = OpenGLContext::getWindow();
<<<<<<< HEAD
  glfwSetWindowTitle(window, "CG-Final-Project");

  g_isolatedViewer.init(window);
  glfwSetWindowTitle(window, "CGFinalProject");
=======
  /* TODO#0: Change window title to "HW2 - `your student id`"
   *         Ex. HW2 - 311550000
   */
  glfwSetWindowTitle(window, "HW2 - 112550002");
>>>>>>> 91df080 (Based main code, add a new slider for time control)

  // Init Camera helper
  Camera camera(glm::vec3(0, 2, 5));
  camera.initialize(OpenGLContext::getAspectRatio());
  // Store camera as glfw global variable for callbacks use
  glfwSetWindowUserPointer(window, &camera);
  ctx.camera = &camera;
  ctx.window = window;

  loadMaterial();
  loadModels();
  loadPrograms();
  setupObjects();

  // bouns start
  const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
  unsigned int depthMapFBO;
  glGenFramebuffers(1, &depthMapFBO);
  // depth map
  unsigned int depthMap;
  glGenTextures(1, &depthMap);
  glBindTexture(GL_TEXTURE_2D, depthMap);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
               NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

  glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  DepthProgram simpleDepthShader(&ctx);
  simpleDepthShader.load();
  // bound end

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330 core");
  // Main rendering loop
  while (!glfwWindowShouldClose(window)) {
    // Polling events.
    glfwPollEvents();
    // Update camera position and view
    camera.move(window);
    // GL_XXX_BIT can simply "OR" together to use.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    /// TO DO Enable DepthTest
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearDepth(1.0f);

    // bouns start
    Object* robot = ctx.objects[5];
    glm::mat4 newTransform = glm::translate(glm::mat4(1.0f), glm::vec3(robot_x, 0.0f, robot_z));
    // newTransform = glm::scale(newTransform, glm::vec3(0.5f));
    robot->transformMatrix = newTransform;
    // bouns end

    // bonus start
    // Shadow Map Generation
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    GLfloat near_plane = 0.1f, far_plane = 50.0f;
    glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);

    glm::vec3 lightPos = -glm::normalize(ctx.directionLightDirection) * 20.0f;
    glm::vec3 target = glm::vec3(0.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 lightView = glm::lookAt(lightPos, target, up);

    glm::mat4 lightSpaceMatrix = lightProjection * lightView;
    simpleDepthShader.lightSpaceMatrix = lightSpaceMatrix;

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    // doing draw
    simpleDepthShader.doMainLoop();
    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // bonus end

    int scrWidth, scrHeight;
    glfwGetFramebufferSize(window, &scrWidth, &scrHeight);
    glViewport(0, 0, scrWidth, scrHeight);

    // clean up all
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearDepth(1.0f);

    for (size_t i = 0; i < ctx.programs.size(); i++) {
      Program* program = ctx.programs[i];
      program->use();  // add program.h

      // compute shadow coord
      GLint loc = glGetUniformLocation(program->getHandle(), "lightSpaceMatrix");
      if (loc >= 0) {
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
      }

      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, depthMap);

      GLint shadowLoc = glGetUniformLocation(program->getHandle(), "shadowMap");
      if (shadowLoc >= 0) {
        glUniform1i(shadowLoc, 1);
      }

      program->doMainLoop();
    }

    if (g_isolatedViewer.isActive()) {
      if (!ctx.programs.empty()) {
        g_isolatedViewer.render(*ctx.programs[1]);
      }
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    // Lights control panel
    {
      ImGui::Begin("Lights Control");

      // --- Directional Light ---
      ImGui::Text("Directional Light");
      {
        ImGui::SameLine();
        bool enable = (ctx.directionLightEnable != 0);
        if (ImGui::Checkbox("Enable##dir", &enable)) ctx.directionLightEnable = enable ? 1 : 0;
        ImGui::SliderFloat3("Dir X/Y/Z##dir", &ctx.directionLightDirection.x, -50.0f, 50.0f);
        ImGui::ColorEdit3("Color##dir", &ctx.directionLightColor[0]);
      }
      ImGui::Separator();

      // --- Point Light ---
      ImGui::Text("Point Light");
      {
        ImGui::SameLine();
        bool enable = (ctx.pointLightEnable != 0);
        if (ImGui::Checkbox("Enable##point", &enable)) ctx.pointLightEnable = enable ? 1 : 0;
        ImGui::SliderFloat3("Pos X/Y/Z##point", &ctx.pointLightPosition.x, -10.0f, 10.0f);
        ImGui::ColorEdit3("Color##point", &ctx.pointLightColor[0]);
      }
      ImGui::Separator();

      // --- Spot Light ---
      ImGui::Text("Spot Light");
      {
        ImGui::SameLine();
        bool enable = (ctx.spotLightEnable != 0);
        if (ImGui::Checkbox("Enable##spot", &enable)) ctx.spotLightEnable = enable ? 1 : 0;
        ImGui::SliderFloat3("Pos X/Y/Z##spot", &ctx.spotLightPosition.x, -10.0f, 10.0f);
        ImGui::ColorEdit3("Color##spot", &ctx.spotLightColor[0]);
      }
      ImGui::Separator();

      // Time
      ImGui::Text("Time");
      {
        ImGui::SameLine();
        bool enable = (ctx.directionLightEnable != 0);
        if (ImGui::Checkbox("Enable##dir", &enable)) ctx.directionLightEnable = enable ? 1 : 0;
       
        static float time = 12.0f;
       
        if (ImGui::SliderFloat("Time of Day##dir", &time, 6.0f, 18.0f, "%.1f:00")) {
          time = round(time * 6.0f) / 6.0f;  // 四捨五入到 10 分鐘
         
          float angle = (time - 12.0f) * 15.0f;
          float radians = angle * 3.14159f / 180.0f;
         
          ctx.directionLightDirection.x = sin(radians);
          ctx.directionLightDirection.y = -cos(radians);
          ctx.directionLightDirection.z = 0.0f;
         
          // (6:00-11:00): 黃色 -> 白色
          // (11:00-13:00): 白色
          // (13:00-18:00): 白色 -> 橘黃色
         
          if (time <= 11.0f) {
            // 早晨
            float t = (time - 6.0f) / 5.0f;
            ctx.directionLightColor[0] = 0.85f;                  
            ctx.directionLightColor[1] = 0.60f + 0.20f * t;      // G: 0.60 -> 0.80
            ctx.directionLightColor[2] = 0.35f + 0.40f * t;      // B: 0.35 -> 0.75
          }
          else if (time >= 13.0f) {
            // 傍晚
            float t = (time - 13.0f) / 5.0f;
            ctx.directionLightColor[0] = 0.85f;                  
            ctx.directionLightColor[1] = 0.80f - 0.25f * t;      // G: 0.80 -> 0.55
            ctx.directionLightColor[2] = 0.75f - 0.40f * t;      // B: 0.75 -> 0.35
          }
          else {
            // 中午
            ctx.directionLightColor[0] = 0.85f;
            ctx.directionLightColor[1] = 0.80f;
            ctx.directionLightColor[2] = 0.75f;
          }
        }
       
        int hour = (int)time;
        int minute = (int)((time - hour) * 60);
        ImGui::Text("Current: %02d:%02d", hour, minute);
       
        ImGui::ColorEdit3("Color##dir", &ctx.directionLightColor[0]);
      }
      ImGui::Separator();
      
      {
        const char* hint = "Use F1 to toggle cursor";
        ImGui::Separator();
        ImVec2 winSize = ImGui::GetWindowSize();
        ImVec2 txtSize = ImGui::CalcTextSize(hint);
        float y = winSize.y - txtSize.y - ImGui::GetStyle().FramePadding.y - ImGui::GetStyle().ItemSpacing.y;
        if (y > ImGui::GetCursorPosY()) ImGui::SetCursorPosY(y);
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.6f, 1.0f), "%s", hint);
      }
      ImGui::End();

      ImGui::Begin("Isolated Viewer Inspector");
      ImGui::Text("Click an object to open in NEW WINDOW:");

      for (size_t i = 0; i < ctx.objects.size(); ++i) {
        std::string label = "Object " + std::to_string(i);
        if (ImGui::Button(label.c_str())) {
          Model* rawPtr = ctx.models[ctx.objects[i]->modelIndex];
          std::shared_ptr<Model> sPtr(rawPtr, [](Model*) {});
          g_isolatedViewer.setTarget(sPtr);
        }
      }

      ImGui::Separator();
      if (ImGui::Button("Close Viewer / Clear Target")) {
        g_isolatedViewer.clearTarget();
      }

      if (g_isolatedViewer.isActive()) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Status: Active");
      } else {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Status: Inactive");
      }

      ImGui::End();
    }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

#ifdef __APPLE__
    // Some platform need explicit glFlush
    glFlush();
#endif
    glfwSwapBuffers(window);
  }
  // Cleanup ImGui
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  return 0;
}

void keyCallback(GLFWwindow* window, int key, int, int action, int) {
  // Press ESC to close the window.
  if (key == GLFW_KEY_ESCAPE) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
    return;
  }
  if (action == GLFW_PRESS) {
    switch (key) {
      case GLFW_KEY_F1: {
        // Toggle cursor
        Camera* cam = static_cast<Camera*>(glfwGetWindowUserPointer(window));
        if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
          glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
          int ww, hh;
          glfwGetWindowSize(window, &ww, &hh);
          glfwSetCursorPos(window, static_cast<double>(ww) / 2.0, static_cast<double>(hh) / 2.0);
          if (cam) cam->setLastMousePos(window);
        } else {
          glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
          if (cam) cam->setLastMousePos(window);
        }
        break;
      }
      default:
        break;
    }
  }

  if (action == GLFW_PRESS || action == GLFW_REPEAT) {
    switch (key) {
      case GLFW_KEY_UP:
        std::cout << "Up Key Pressed" << std::endl;
        robot_z = robot_z + 0.2f;
        if (robot_z >= 5.12f) robot_z = 5.12f;
        break;

      case GLFW_KEY_DOWN:
        std::cout << "Down Key Pressed" << std::endl;
        robot_z = robot_z - 0.2f;
        if (robot_z <= 0.0f) robot_z = 0.0f;
        break;

      case GLFW_KEY_LEFT:
        std::cout << "Left Key Pressed" << std::endl;
        robot_x = robot_x - 0.2f;
        if (robot_x <= 0.0f) robot_x = 0.0f;
        break;

      case GLFW_KEY_RIGHT:
        std::cout << "Right Key Pressed" << std::endl;
        robot_x = robot_x + 0.2f;
        if (robot_x >= 8.192f) robot_x = 8.192;
        break;
    }
  }
}

void resizeCallback(GLFWwindow* window, int width, int height) {
  OpenGLContext::framebufferResizeCallback(window, width, height);
  auto ptr = static_cast<Camera*>(glfwGetWindowUserPointer(window));
  if (ptr) {
    ptr->updateProjectionMatrix(OpenGLContext::getAspectRatio());
  }
}

void initOpenGL() {
  // Initialize OpenGL context, details are wrapped in class.
#ifdef __APPLE__
  // MacOS need explicit request legacy support
  OpenGLContext::createContext(21, GLFW_OPENGL_ANY_PROFILE);
#else
  OpenGLContext::createContext(21, GLFW_OPENGL_ANY_PROFILE);
//  OpenGLContext::createContext(43, GLFW_OPENGL_COMPAT_PROFILE);
#endif
  GLFWwindow* window = OpenGLContext::getWindow();
  glfwSetKeyCallback(window, keyCallback);
  glfwSetFramebufferSizeCallback(window, resizeCallback);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
#ifndef NDEBUG
  OpenGLContext::printSystemInfo();
  // This is useful if you want to debug your OpenGL API calls.
  OpenGLContext::enableDebugCallback();
#endif
}