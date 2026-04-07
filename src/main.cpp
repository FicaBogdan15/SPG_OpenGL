#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <vector>
#include <cmath>
#include <string>

#include "Shader.h"
#include "glm/glm/ext/matrix_clip_space.hpp"
#include "glm/glm/ext/matrix_transform.hpp"

#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif

#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

#ifndef GL_GENERATE_MIPMAP_HINT
#define GL_GENERATE_MIPMAP_HINT 0x8192
#endif

// -- forward declarations -----------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
unsigned int loadTexture(const char* path);
std::vector<float> generateHill(float radius, float height, int segments);
std::vector<float> generateDonutRoad(float innerRadius, float outerRadius, int segments);
std::vector<float> generateCylinder(float radius, float height, int segments);
std::vector<float> generateSphere(float radius, int stacks, int slices);

bool hasOpenGLExtension(const char* extensionName)
{
    GLint extensionCount = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);

    for (GLint i = 0; i < extensionCount; ++i) {
        const auto* extension = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
        if (extension != nullptr && std::string(extension) == extensionName) {
            return true;
        }
    }

    return false;
}

// -- scene constants ----------------------------------------------------------
constexpr int kWindowWidth = 800;
constexpr int kWindowHeight = 600;
constexpr float kSceneScale = 15.0f;
constexpr float kBaseCameraSpeed = 50.0f;
constexpr float kBaseFarPlane = 10000.0f;
constexpr float kBoxScale = 200.0f * kSceneScale;
constexpr float kRoadInnerRadius = 12.0f * kSceneScale;
constexpr float kRoadOuterRadius = 18.0f * kSceneScale;
constexpr float kTreeRingRadius = 22.0f * kSceneScale;
constexpr int kTreeCount = 14;
constexpr int kLampCount = 6;
constexpr unsigned int kShadowMapSize = 1024;
constexpr float kLampShadowNearPlane = 1.0f;
constexpr float kLampShadowFarPlane = 55.0f * kSceneScale;

// -- camera globals -----------------------------------------------------------
glm::vec3 cameraPos = glm::vec3(0.0f, 2.0f, 14.0f * kSceneScale);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float deltaTime = 0.0f;
float lastFrame = 0.0f;
float lastX = 400.0f;
float lastY = 300.0f;
float yaw = -90.0f;
float pitch = 0.0f;
bool firstMouse = true;

// -- main --------------------------------------------------------------------
int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(kWindowWidth, kWindowHeight, "Night Scene", NULL, NULL);
    if (!window) {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

    // -- textures -------------------------------------------------------------
    unsigned int grassTex = loadTexture("D:/CLion_OpenGL/deps/textures/iarba.png");
    unsigned int skyTex = loadTexture("D:/CLion_OpenGL/deps/textures/cer_noapte.png");
    unsigned int roadTex = loadTexture("D:/CLion_OpenGL/deps/textures/asfalt.jpg");
    unsigned int barkTex = loadTexture("D:/CLion_OpenGL/deps/textures/bark.jpg");
    unsigned int leafTex = loadTexture("D:/CLion_OpenGL/deps/textures/leaves.jpg");
    unsigned int poleTex = loadTexture("D:/CLion_OpenGL/deps/textures/stalp.jpg");
    unsigned int bulbTex = loadTexture("D:/CLion_OpenGL/deps/textures/bec.jpg");

    // -- skybox + ground  stride=8: pos(3) uv(2) normal(3) -------------------
    float vertices[] = {
        // back face   normal  0, 0, 1
        -0.5f,-0.5f,-0.5f,  0.0f,0.5f,  0.0f,0.0f,1.0f,
         0.5f,-0.5f,-0.5f,  1.0f,0.5f,  0.0f,0.0f,1.0f,
         0.5f, 0.5f,-0.5f,  1.0f,1.0f,  0.0f,0.0f,1.0f,
         0.5f, 0.5f,-0.5f,  1.0f,1.0f,  0.0f,0.0f,1.0f,
        -0.5f, 0.5f,-0.5f,  0.0f,1.0f,  0.0f,0.0f,1.0f,
        -0.5f,-0.5f,-0.5f,  0.0f,0.5f,  0.0f,0.0f,1.0f,
        // front face  normal  0, 0,-1
        -0.5f,-0.5f, 0.5f,  0.0f,0.5f,  0.0f,0.0f,-1.0f,
         0.5f,-0.5f, 0.5f,  1.0f,0.5f,  0.0f,0.0f,-1.0f,
         0.5f, 0.5f, 0.5f,  1.0f,1.0f,  0.0f,0.0f,-1.0f,
         0.5f, 0.5f, 0.5f,  1.0f,1.0f,  0.0f,0.0f,-1.0f,
        -0.5f, 0.5f, 0.5f,  0.0f,1.0f,  0.0f,0.0f,-1.0f,
        -0.5f,-0.5f, 0.5f,  0.0f,0.5f,  0.0f,0.0f,-1.0f,
        // left face   normal  1, 0, 0
        -0.5f, 0.5f, 0.5f,  1.0f,1.0f,  1.0f,0.0f,0.0f,
        -0.5f, 0.5f,-0.5f,  0.0f,1.0f,  1.0f,0.0f,0.0f,
        -0.5f,-0.5f,-0.5f,  0.0f,0.5f,  1.0f,0.0f,0.0f,
        -0.5f,-0.5f,-0.5f,  0.0f,0.5f,  1.0f,0.0f,0.0f,
        -0.5f,-0.5f, 0.5f,  1.0f,0.5f,  1.0f,0.0f,0.0f,
        -0.5f, 0.5f, 0.5f,  1.0f,1.0f,  1.0f,0.0f,0.0f,
        // right face  normal -1, 0, 0
         0.5f, 0.5f, 0.5f,  1.0f,1.0f, -1.0f,0.0f,0.0f,
         0.5f, 0.5f,-0.5f,  0.0f,1.0f, -1.0f,0.0f,0.0f,
         0.5f,-0.5f,-0.5f,  0.0f,0.5f, -1.0f,0.0f,0.0f,
         0.5f,-0.5f,-0.5f,  0.0f,0.5f, -1.0f,0.0f,0.0f,
         0.5f,-0.5f, 0.5f,  1.0f,0.5f, -1.0f,0.0f,0.0f,
         0.5f, 0.5f, 0.5f,  1.0f,1.0f, -1.0f,0.0f,0.0f,
        // top face    normal  0,-1, 0  (interior)
        -0.5f, 0.5f,-0.5f,  0.0f,1.0f,  0.0f,-1.0f,0.0f,
         0.5f, 0.5f,-0.5f,  1.0f,1.0f,  0.0f,-1.0f,0.0f,
         0.5f, 0.5f, 0.5f,  1.0f,1.0f,  0.0f,-1.0f,0.0f,
         0.5f, 0.5f, 0.5f,  1.0f,1.0f,  0.0f,-1.0f,0.0f,
        -0.5f, 0.5f, 0.5f,  0.0f,1.0f,  0.0f,-1.0f,0.0f,
        -0.5f, 0.5f,-0.5f,  0.0f,1.0f,  0.0f,-1.0f,0.0f,
        // ground face normal  0, 1, 0
        -0.5f,-0.5f,-0.5f,  0.0f,0.0f,  0.0f,1.0f,0.0f,
         0.5f,-0.5f,-0.5f,  1.0f,0.0f,  0.0f,1.0f,0.0f,
         0.5f,-0.5f, 0.5f,  1.0f,1.0f,  0.0f,1.0f,0.0f,
         0.5f,-0.5f, 0.5f,  1.0f,1.0f,  0.0f,1.0f,0.0f,
        -0.5f,-0.5f, 0.5f,  0.0f,1.0f,  0.0f,1.0f,0.0f,
        -0.5f,-0.5f,-0.5f,  0.0f,0.0f,  0.0f,1.0f,0.0f,
    };

    std::vector<float> hillVertices = generateHill(1.0f, 1.0f, 64);
    std::vector<float> roadVertices = generateDonutRoad(kRoadInnerRadius, kRoadOuterRadius, 100);
    std::vector<float> trunkVertices = generateCylinder(1.0f, 1.0f, 32);
    std::vector<float> crownVertices = generateSphere(1.0f, 16, 32);

    const GLsizei kSkyVertexCount = 30;
    const GLsizei kGroundVertexCount = 6;
    const GLsizei kRoadVertexCount = static_cast<GLsizei>(roadVertices.size() / 8);
    const GLsizei kHillVertexCount = static_cast<GLsizei>(hillVertices.size() / 8);
    const GLsizei kTrunkVertexCount = static_cast<GLsizei>(trunkVertices.size() / 8);
    const GLsizei kCrownVertexCount = static_cast<GLsizei>(crownVertices.size() / 8);

    // -- VAO stride-8  (pos3 + uv2 + normal3) ---------------------------------
    auto makeVAO8 = [](const float* data, size_t byteSize) -> unsigned int {
        unsigned int VAO, VBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, byteSize, data, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
        glEnableVertexAttribArray(2);
        return VAO;
    };

    unsigned int VAO1 = makeVAO8(vertices, sizeof(vertices));
    unsigned int hillVAO = makeVAO8(hillVertices.data(), hillVertices.size() * sizeof(float));
    unsigned int roadVAO = makeVAO8(roadVertices.data(), roadVertices.size() * sizeof(float));
    unsigned int trunkVAO = makeVAO8(trunkVertices.data(), trunkVertices.size() * sizeof(float));
    unsigned int crownVAO = makeVAO8(crownVertices.data(), crownVertices.size() * sizeof(float));

    // -- shaders ---------------------------------------------------------------
    Shader shader("D:/CLion_OpenGL/shaders/shader.vs",
                  "D:/CLion_OpenGL/shaders/shader.fs");
    Shader depthShader("D:/CLion_OpenGL/shaders/point_shadow_depth.vs",
                       "D:/CLion_OpenGL/shaders/point_shadow_depth.fs");

    shader.use();
    shader.setInt("texture1", 0);
    shader.setInt("shadowMap0", 1);
    shader.setInt("shadowMap1", 2);
    shader.setInt("shadowMap2", 3);
    shader.setInt("shadowMap3", 4);
    shader.setInt("shadowMap4", 5);
    shader.setInt("shadowMap5", 6);

    // low moonlight so the lamp shadows remain visible at night
    shader.setVec3("sunDirection", glm::vec3(-0.25f, -1.0f, -0.12f));
    shader.setVec3("sunColor", glm::vec3(0.06f, 0.09f, 0.16f));
    shader.setFloat("ambientStrength", 0.12f);
    shader.setFloat("shadowFarPlane", kLampShadowFarPlane);
    shader.setFloat("uvScale", 1.0f);
    shader.setBool("useWorldUV", false);

    // -- tree instances --------------------------------------------------------
    struct TreeInstance { glm::vec3 pos; float scale; };
    std::vector<TreeInstance> trees;
    trees.reserve(kTreeCount);

    float radialJitter[] = { 0.0f, 2.5f * kSceneScale, -2.0f * kSceneScale,
                             1.5f * kSceneScale, -3.0f * kSceneScale,
                             0.5f * kSceneScale, -1.0f * kSceneScale };
    int jitterCount = static_cast<int>(sizeof(radialJitter) / sizeof(radialJitter[0]));

    for (int i = 0; i < kTreeCount; i++) {
        float angle = (2.0f * 3.1415926f * i) / kTreeCount;
        float r = kTreeRingRadius + radialJitter[i % jitterCount];
        float scaleF = 1.0f + 0.3f * static_cast<float>(i % 3);
        trees.push_back({ glm::vec3(r * cos(angle), 0.0f, r * sin(angle)), scaleF });
    }

    const float kTrunkRadius = 0.6f * kSceneScale;
    const float kTrunkHeight = 3.5f * kSceneScale;
    const float kCrownRadius = 3.0f * kSceneScale;

    // -- lamp post instances ---------------------------------------------------
    const float kLampTrunkRadius = 0.2f * kSceneScale;
    const float kLampTrunkHeight = 5.0f * kSceneScale;
    const float kLampBulbRadius = 0.5f * kSceneScale;
    const float kLampColor_R = 2.6f;
    const float kLampColor_G = 2.3f;
    const float kLampColor_B = 1.8f;
    const float kLampTreeClearance = 2.0f * kSceneScale;

    struct LampInstance {
        glm::vec3 base;
        glm::vec3 bulbCenter;
        glm::vec3 lightPos;
    };
    std::vector<LampInstance> lamps;
    lamps.reserve(kLampCount);

    float lampAngles[3] = { 0.0f, 2.0f * 3.1415926f / 3.0f, 4.0f * 3.1415926f / 3.0f };

    auto treeClearanceAt = [&](const glm::vec3& base) {
        float bestClearance = 1e9f;
        for (const TreeInstance& tree : trees) {
            float canopyRadius = kCrownRadius * tree.scale + kLampTrunkRadius + kLampTreeClearance;
            float distanceXZ = glm::length(glm::vec2(base.x - tree.pos.x, base.z - tree.pos.z));
            float clearance = distanceXZ - canopyRadius;
            if (clearance < bestClearance) {
                bestClearance = clearance;
            }
        }
        return bestClearance;
    };

    auto buildLamp = [&](float ringRadius, float preferredAngle) {
        const float angleStep = glm::radians(4.0f);
        const int searchSteps = 45;
        const float requiredClearance = 0.0f;

        float bestAngle = preferredAngle;
        float bestClearance = -1e9f;
        bool foundSafe = false;

        for (int step = 0; step <= searchSteps && !foundSafe; ++step) {
            int variants = step == 0 ? 1 : 2;
            for (int variant = 0; variant < variants; ++variant) {
                float signedStep = step == 0 ? 0.0f : (variant == 0 ? 1.0f : -1.0f) * static_cast<float>(step);
                float angle = preferredAngle + signedStep * angleStep;
                glm::vec3 base(ringRadius * cos(angle), 0.0f, ringRadius * sin(angle));
                float clearance = treeClearanceAt(base);

                if (clearance > bestClearance) {
                    bestClearance = clearance;
                    bestAngle = angle;
                }

                if (clearance >= requiredClearance) {
                    bestAngle = angle;
                    foundSafe = true;
                    break;
                }
            }
        }

        glm::vec3 base(ringRadius * cos(bestAngle), 0.0f, ringRadius * sin(bestAngle));
        glm::vec3 bulbCenter = base + glm::vec3(0.0f, kLampTrunkHeight + kLampBulbRadius, 0.0f);
        glm::vec3 lightPos = base + glm::vec3(0.0f, kLampTrunkHeight + kLampBulbRadius * 2.0f, 0.0f);
        return LampInstance{ base, bulbCenter, lightPos };
    };

    for (int i = 0; i < 3; i++) {
        float rin = kRoadInnerRadius - 2.5f * kSceneScale;
        lamps.push_back(buildLamp(rin, lampAngles[i]));
    }
    for (int i = 0; i < 3; i++) {
        float rout = kRoadOuterRadius + 2.5f * kSceneScale;
        lamps.push_back(buildLamp(rout, lampAngles[i]));
    }

    shader.setInt("numLights", kLampCount);
    for (int i = 0; i < kLampCount; i++) {
        shader.setVec3("lightPositions[" + std::to_string(i) + "]", lamps[i].lightPos);
        shader.setVec3("lightColors[" + std::to_string(i) + "]",
                       glm::vec3(kLampColor_R, kLampColor_G, kLampColor_B));
    }

    // -- point-light shadow maps ----------------------------------------------

    //shadows
    unsigned int shadowFBO;
    glGenFramebuffers(1, &shadowFBO);

    unsigned int shadowCubeMaps[kLampCount] = {};
    glGenTextures(kLampCount, shadowCubeMaps);
    for (int i = 0; i < kLampCount; i++) {
        glBindTexture(GL_TEXTURE_CUBE_MAP, shadowCubeMaps[i]);
        for (unsigned int face = 0; face < 6; face++) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                         0,
                         GL_DEPTH_COMPONENT,
                         kShadowMapSize,
                         kShadowMapSize,
                         0,
                         GL_DEPTH_COMPONENT,
                         GL_FLOAT,
                         nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }
    // Configure framebuffer to store depth only
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Build the 6 view-projection matrices for cubemap shadow rendering
    auto buildShadowTransforms = [&](const glm::vec3& lightPos, glm::mat4* shadowTransforms) {
        glm::mat4 shadowProjection = glm::perspective(glm::radians(90.0f), 1.0f,
                                                      kLampShadowNearPlane, kLampShadowFarPlane);
        shadowTransforms[0] = shadowProjection * glm::lookAt(lightPos, lightPos + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f));
        shadowTransforms[1] = shadowProjection * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f));
        shadowTransforms[2] = shadowProjection * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f));
        shadowTransforms[3] = shadowProjection * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f));
        shadowTransforms[4] = shadowProjection * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f));
        shadowTransforms[5] = shadowProjection * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f));
    };

    // Render the scene in depth mode from the light position
    auto renderDepthScene = [&](Shader& activeShader) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.5f * kBoxScale, 0.0f));
        model = glm::scale(model, glm::vec3(kBoxScale));
        activeShader.setMat4("model", model);
        glBindVertexArray(VAO1);
        glDrawArrays(GL_TRIANGLES, kSkyVertexCount, kGroundVertexCount);

        glm::mat4 roadModel = glm::mat4(1.0f);
        roadModel = glm::translate(roadModel, glm::vec3(0.0f, 0.1f, 0.0f));
        activeShader.setMat4("model", roadModel);
        glBindVertexArray(roadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, kRoadVertexCount);

        glm::mat4 hillModel = glm::mat4(1.0f);
        float hillSize = 11.0f * kSceneScale;
        hillModel = glm::scale(hillModel, glm::vec3(hillSize, hillSize * 0.5f, hillSize));
        activeShader.setMat4("model", hillModel);
        glBindVertexArray(hillVAO);
        glDrawArrays(GL_TRIANGLES, 0, kHillVertexCount);

        for (const TreeInstance& tree : trees) {
            float s = tree.scale;

            glm::mat4 trunkModel = glm::mat4(1.0f);
            trunkModel = glm::translate(trunkModel, tree.pos);
            trunkModel = glm::scale(trunkModel, glm::vec3(kTrunkRadius * s, kTrunkHeight * s, kTrunkRadius * s));
            activeShader.setMat4("model", trunkModel);
            glBindVertexArray(trunkVAO);
            glDrawArrays(GL_TRIANGLES, 0, kTrunkVertexCount);

            glm::vec3 crownBase = tree.pos + glm::vec3(0.0f, (kTrunkHeight + kCrownRadius * 0.7f) * s, 0.0f);
            glm::mat4 crownModel = glm::mat4(1.0f);
            crownModel = glm::translate(crownModel, crownBase);
            crownModel = glm::scale(crownModel, glm::vec3(kCrownRadius * s));
            activeShader.setMat4("model", crownModel);
            glBindVertexArray(crownVAO);
            glDrawArrays(GL_TRIANGLES, 0, kCrownVertexCount);
        }

        for (const LampInstance& lamp : lamps) {
            glm::mat4 poleModel = glm::mat4(1.0f);
            poleModel = glm::translate(poleModel, lamp.base);
            poleModel = glm::scale(poleModel, glm::vec3(kLampTrunkRadius, kLampTrunkHeight, kLampTrunkRadius));
            activeShader.setMat4("model", poleModel);
            glBindVertexArray(trunkVAO);
            glDrawArrays(GL_TRIANGLES, 0, kTrunkVertexCount);
        }
    };
    // Render the final textured scene with lighting and shadows
    auto renderLitScene = [&](Shader& activeShader) {
        glActiveTexture(GL_TEXTURE0);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.5f * kBoxScale, 0.0f));
        model = glm::scale(model, glm::vec3(kBoxScale));
        activeShader.setMat4("model", model);
        glBindVertexArray(VAO1);

        activeShader.setBool("useLighting", false);
        activeShader.setFloat("emissiveStrength", 0.0f);
        activeShader.setFloat("uvScale", 1.0f);
        activeShader.setBool("useWorldUV", false);
        glBindTexture(GL_TEXTURE_2D, skyTex);
        glDrawArrays(GL_TRIANGLES, 0, kSkyVertexCount);

        activeShader.setBool("useLighting", true);
        activeShader.setFloat("emissiveStrength", 0.0f);
        activeShader.setFloat("uvScale", 1.0f);
        activeShader.setBool("useWorldUV", true);
        glBindTexture(GL_TEXTURE_2D, grassTex);
        glDrawArrays(GL_TRIANGLES, kSkyVertexCount, kGroundVertexCount);

        glm::mat4 roadModel = glm::mat4(1.0f);
        roadModel = glm::translate(roadModel, glm::vec3(0.0f, 0.1f, 0.0f));
        activeShader.setMat4("model", roadModel);
        glBindVertexArray(roadVAO);
        activeShader.setFloat("uvScale", 1.0f);
        activeShader.setBool("useWorldUV", false);
        glBindTexture(GL_TEXTURE_2D, roadTex);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, kRoadVertexCount);

        glm::mat4 hillModel = glm::mat4(1.0f);
        float hillSize = 11.0f * kSceneScale;
        hillModel = glm::scale(hillModel, glm::vec3(hillSize, hillSize * 0.5f, hillSize));
        activeShader.setMat4("model", hillModel);
        glBindVertexArray(hillVAO);
        activeShader.setFloat("uvScale", 1.0f);
        activeShader.setBool("useWorldUV", true);
        glBindTexture(GL_TEXTURE_2D, grassTex);
        glDrawArrays(GL_TRIANGLES, 0, kHillVertexCount);

        for (const TreeInstance& tree : trees) {
            float s = tree.scale;

            glm::mat4 trunkModel = glm::mat4(1.0f);
            trunkModel = glm::translate(trunkModel, tree.pos);
            trunkModel = glm::scale(trunkModel, glm::vec3(kTrunkRadius * s, kTrunkHeight * s, kTrunkRadius * s));
            activeShader.setMat4("model", trunkModel);
            activeShader.setBool("useLighting", true);
            activeShader.setFloat("emissiveStrength", 0.0f);
            activeShader.setFloat("uvScale", 2.0f);
            activeShader.setBool("useWorldUV", false);
            glBindVertexArray(trunkVAO);
            glBindTexture(GL_TEXTURE_2D, barkTex);
            glDrawArrays(GL_TRIANGLES, 0, kTrunkVertexCount);

            glm::vec3 crownBase = tree.pos + glm::vec3(0.0f, (kTrunkHeight + kCrownRadius * 0.7f) * s, 0.0f);
            glm::mat4 crownModel = glm::mat4(1.0f);
            crownModel = glm::translate(crownModel, crownBase);
            crownModel = glm::scale(crownModel, glm::vec3(kCrownRadius * s));
            activeShader.setMat4("model", crownModel);
            activeShader.setFloat("uvScale", 2.0f);
            activeShader.setBool("useWorldUV", false);
            glBindVertexArray(crownVAO);
            glBindTexture(GL_TEXTURE_2D, leafTex);
            glDrawArrays(GL_TRIANGLES, 0, kCrownVertexCount);
        }

        for (const LampInstance& lamp : lamps) {
            glm::mat4 poleModel = glm::mat4(1.0f);
            poleModel = glm::translate(poleModel, lamp.base);
            poleModel = glm::scale(poleModel, glm::vec3(kLampTrunkRadius, kLampTrunkHeight, kLampTrunkRadius));
            activeShader.setMat4("model", poleModel);
            activeShader.setBool("useLighting", true);
            activeShader.setFloat("emissiveStrength", 0.0f);
            activeShader.setFloat("uvScale", 2.0f);
            activeShader.setBool("useWorldUV", false);
            glBindVertexArray(trunkVAO);
            glBindTexture(GL_TEXTURE_2D, poleTex);
            glDrawArrays(GL_TRIANGLES, 0, kTrunkVertexCount);

            glm::mat4 bulbModel = glm::mat4(1.0f);
            bulbModel = glm::translate(bulbModel, lamp.bulbCenter);
            bulbModel = glm::scale(bulbModel, glm::vec3(kLampBulbRadius));
            activeShader.setMat4("model", bulbModel);
            activeShader.setBool("useLighting", false);
            activeShader.setFloat("emissiveStrength", 1.5f);
            activeShader.setFloat("uvScale", 2.0f);
            activeShader.setBool("useWorldUV", false);
            glBindVertexArray(crownVAO);
            glBindTexture(GL_TEXTURE_2D, bulbTex);
            glDrawArrays(GL_TRIANGLES, 0, kCrownVertexCount);
        }
    };

    // -- render loop -----------------------------------------------------------
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // 1. Render point-light shadow maps.
        glViewport(0, 0, kShadowMapSize, kShadowMapSize);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
        depthShader.use();
        depthShader.setFloat("farPlane", kLampShadowFarPlane);

        for (int lightIndex = 0; lightIndex < kLampCount; lightIndex++) {
            glm::mat4 shadowTransforms[6];
            buildShadowTransforms(lamps[lightIndex].lightPos, shadowTransforms);
            depthShader.setVec3("lightPos", lamps[lightIndex].lightPos);

            for (int face = 0; face < 6; face++) {
                glFramebufferTexture2D(GL_FRAMEBUFFER,
                                       GL_DEPTH_ATTACHMENT,
                                       GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                                       shadowCubeMaps[lightIndex],
                                       0);
                glClear(GL_DEPTH_BUFFER_BIT);
                depthShader.setMat4("shadowMatrix", shadowTransforms[face]);
                renderDepthScene(depthShader);
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
        glViewport(0, 0, framebufferWidth, framebufferHeight);

        // 2. Render the scene with night lighting + point shadows.
        glClearColor(0.01f, 0.015f, 0.03f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        float aspect = framebufferHeight > 0
                     ? static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight)
                     : static_cast<float>(kWindowWidth) / static_cast<float>(kWindowHeight);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, kBaseFarPlane);
        shader.setMat4("projection", projection);

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        shader.setMat4("view", view);
        shader.setVec3("viewPos", cameraPos);

        for (int i = 0; i < kLampCount; i++) {
            glActiveTexture(GL_TEXTURE1 + i);
            glBindTexture(GL_TEXTURE_CUBE_MAP, shadowCubeMaps[i]);
        }
        glActiveTexture(GL_TEXTURE0);

        renderLitScene(shader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

// -- implementations ----------------------------------------------------------

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    (void)window;
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float speed = kBaseCameraSpeed * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos += speed * glm::vec3(0.0f, 1.0f, 0.0f);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        cameraPos -= speed * glm::vec3(0.0f, 1.0f, 0.0f);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    (void)window;
    if (firstMouse) {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos) - lastX;
    float yoffset = lastY - static_cast<float>(ypos);
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    const float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

unsigned int loadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);

    if (data) {
        GLenum format;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        static const bool hasAnisotropicFiltering =
            hasOpenGLExtension("GL_EXT_texture_filter_anisotropic");
        if (hasAnisotropicFiltering) {
            float maxAniso = 0.0f;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
            if (maxAniso > 0.0f) {
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);
            }
        }

        stbi_image_free(data);
    } else {
        std::cout << "Failed to load texture: " << path << '\n';
    }
    return textureID;
}

std::vector<float> generateDonutRoad(float innerRadius, float outerRadius, int segments)
{
    std::vector<float> vertices;
    for (int i = 0; i <= segments; i++) {
        float angle = (2.0f * 3.1415926f * i) / segments;
        float cosA = cos(angle);
        float sinA = sin(angle);

        vertices.insert(vertices.end(), {
            innerRadius * cosA, 0.01f, innerRadius * sinA,
            static_cast<float>(i) / (segments / 4.0f), 0.0f,
            0.0f, 1.0f, 0.0f
        });

        vertices.insert(vertices.end(), {
            outerRadius * cosA, 0.01f, outerRadius * sinA,
            static_cast<float>(i) / (segments / 4.0f), 1.0f,
            0.0f, 1.0f, 0.0f
        });
    }
    return vertices;
}

std::vector<float> generateCylinder(float radius, float height, int segments)
{
    std::vector<float> vertices;
    for (int i = 0; i < segments; i++) {
        float angle1 = (2.0f * 3.1415926f * i) / segments;
        float angle2 = (2.0f * 3.1415926f * (i + 1)) / segments;

        float x1 = cos(angle1) * radius, z1 = sin(angle1) * radius;
        float x2 = cos(angle2) * radius, z2 = sin(angle2) * radius;
        glm::vec3 n1 = glm::normalize(glm::vec3(cos(angle1), 0.0f, sin(angle1)));
        glm::vec3 n2 = glm::normalize(glm::vec3(cos(angle2), 0.0f, sin(angle2)));

        float u1 = static_cast<float>(i) / segments;
        float u2 = static_cast<float>(i + 1) / segments;

        vertices.insert(vertices.end(), { x1, 0.0f,   z1, u1, 0.0f, n1.x, n1.y, n1.z });
        vertices.insert(vertices.end(), { x2, 0.0f,   z2, u2, 0.0f, n2.x, n2.y, n2.z });
        vertices.insert(vertices.end(), { x2, height, z2, u2, 1.0f, n2.x, n2.y, n2.z });

        vertices.insert(vertices.end(), { x2, height, z2, u2, 1.0f, n2.x, n2.y, n2.z });
        vertices.insert(vertices.end(), { x1, height, z1, u1, 1.0f, n1.x, n1.y, n1.z });
        vertices.insert(vertices.end(), { x1, 0.0f,   z1, u1, 0.0f, n1.x, n1.y, n1.z });
    }
    return vertices;
}

std::vector<float> generateSphere(float radius, int stacks, int slices)
{
    std::vector<float> vertices;
    for (int i = 0; i < stacks; i++) {
        float phi1 = static_cast<float>(i) / stacks * 3.1415926f;
        float phi2 = static_cast<float>(i + 1) / stacks * 3.1415926f;

        for (int j = 0; j < slices; j++) {
            float theta1 = static_cast<float>(j) / slices * 2.0f * 3.1415926f;
            float theta2 = static_cast<float>(j + 1) / slices * 2.0f * 3.1415926f;

            auto P = [&](float phi, float theta) -> glm::vec3 {
                return {
                    radius * sin(phi) * cos(theta),
                    radius * cos(phi),
                    radius * sin(phi) * sin(theta)
                };
            };

            glm::vec3 p1 = P(phi1, theta1);
            glm::vec3 p2 = P(phi1, theta2);
            glm::vec3 p3 = P(phi2, theta1);
            glm::vec3 p4 = P(phi2, theta2);
            glm::vec3 n1 = glm::normalize(p1);
            glm::vec3 n2 = glm::normalize(p2);
            glm::vec3 n3 = glm::normalize(p3);
            glm::vec3 n4 = glm::normalize(p4);

            float u1 = static_cast<float>(j) / slices, v1 = static_cast<float>(i) / stacks;
            float u2 = static_cast<float>(j + 1) / slices, v2 = static_cast<float>(i + 1) / stacks;

            vertices.insert(vertices.end(), { p1.x, p1.y, p1.z, u1, v1, n1.x, n1.y, n1.z });
            vertices.insert(vertices.end(), { p3.x, p3.y, p3.z, u1, v2, n3.x, n3.y, n3.z });
            vertices.insert(vertices.end(), { p2.x, p2.y, p2.z, u2, v1, n2.x, n2.y, n2.z });

            vertices.insert(vertices.end(), { p2.x, p2.y, p2.z, u2, v1, n2.x, n2.y, n2.z });
            vertices.insert(vertices.end(), { p3.x, p3.y, p3.z, u1, v2, n3.x, n3.y, n3.z });
            vertices.insert(vertices.end(), { p4.x, p4.y, p4.z, u2, v2, n4.x, n4.y, n4.z });
        }
    }
    return vertices;
}

std::vector<float> generateHill(float radius, float height, int segments)
{
    std::vector<float> vertices;
    glm::vec3 top(0, height, 0);

    for (int i = 0; i < segments; i++) {
        float angle1 = (2.0f * 3.1415926f * i) / segments;
        float angle2 = (2.0f * 3.1415926f * (i + 1)) / segments;

        glm::vec3 p1(radius * cos(angle1), 0, radius * sin(angle1));
        glm::vec3 p2(radius * cos(angle2), 0, radius * sin(angle2));
        glm::vec3 n1 = glm::normalize(glm::vec3(height * cos(angle1), radius, height * sin(angle1)));
        glm::vec3 n2 = glm::normalize(glm::vec3(height * cos(angle2), radius, height * sin(angle2)));
        glm::vec3 topNormal = glm::normalize(n1 + n2);

        float uTop = 0.5f, vTop = 1.0f;
        float u1 = (cos(angle1) + 1) * 0.5f, v1 = (sin(angle1) + 1) * 0.5f;
        float u2 = (cos(angle2) + 1) * 0.5f, v2 = (sin(angle2) + 1) * 0.5f;

        vertices.insert(vertices.end(), { top.x, top.y, top.z, uTop, vTop, topNormal.x, topNormal.y, topNormal.z });
        vertices.insert(vertices.end(), { p1.x,  p1.y,  p1.z,  u1,   v1, n1.x, n1.y, n1.z });
        vertices.insert(vertices.end(), { p2.x,  p2.y,  p2.z,  u2,   v2, n2.x, n2.y, n2.z });
    }
    return vertices;
}
