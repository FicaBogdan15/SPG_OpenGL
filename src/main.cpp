#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <vector>
#include <cmath>

#include "Shader.h"
#include "glm/glm/ext/matrix_clip_space.hpp"
#include "glm/glm/ext/matrix_transform.hpp"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
unsigned int loadTexture(const char* path);
std::vector<float> generateHill(float radius, float height, int segments);
std::vector<float> generateDonutRoad(float innerRadius, float outerRadius, int segments);
std::vector<float> generateCylinder(float radius, float height, int segments);
std::vector<float> generateSphere(float radius, int stacks, int slices);


constexpr float kSceneScale     = 15.0f;
constexpr float kBaseCameraSpeed = 50.0f;
constexpr float kBaseFarPlane   = 10000.0f;
constexpr float kBoxScale       = 200.0f * kSceneScale;
constexpr float kRoadInnerRadius = 12.0f * kSceneScale;
constexpr float kRoadOuterRadius = 18.0f * kSceneScale;


constexpr float kTreeRingRadius = 22.0f * kSceneScale;   
constexpr int   kTreeCount      = 14;                    

glm::vec3 cameraPos   = glm::vec3(0.0f, 2.0f, 14.0f * kSceneScale);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);

float deltaTime = 0.0f;
float lastFrame = 0.0f;

float lastX = 400.0f;
float lastY = 300.0f;
float yaw   = -90.0f;
float pitch =   0.0f;
bool firstMouse = true;





std::vector<float> generateDonutRoad(float innerRadius, float outerRadius, int segments) {
    std::vector<float> vertices;
    for (int i = 0; i <= segments; i++) {
        float angle = (2.0f * 3.1415926f * i) / segments;
        float cosA  = cos(angle);
        float sinA  = sin(angle);

        vertices.push_back(innerRadius * cosA);
        vertices.push_back(0.01f);
        vertices.push_back(innerRadius * sinA);
        vertices.push_back(static_cast<float>(i) / (segments / 10.0f));
        vertices.push_back(0.0f);

        vertices.push_back(outerRadius * cosA);
        vertices.push_back(0.01f);
        vertices.push_back(outerRadius * sinA);
        vertices.push_back(static_cast<float>(i) / (segments / 10.0f));
        vertices.push_back(1.0f);
    }
    return vertices;
}



std::vector<float> generateCylinder(float radius, float height, int segments) {
    std::vector<float> vertices;

    for (int i = 0; i < segments; i++) {
        float angle1 = (2.0f * 3.1415926f * i)       / segments;
        float angle2 = (2.0f * 3.1415926f * (i + 1)) / segments;

        float x1 = cos(angle1) * radius,  z1 = sin(angle1) * radius;
        float x2 = cos(angle2) * radius,  z2 = sin(angle2) * radius;

        float u1 = static_cast<float>(i)       / segments;
        float u2 = static_cast<float>(i + 1)   / segments;

        
        
        vertices.insert(vertices.end(), { x1, 0.0f,   z1, u1, 0.0f });
        vertices.insert(vertices.end(), { x2, 0.0f,   z2, u2, 0.0f });
        vertices.insert(vertices.end(), { x2, height, z2, u2, 1.0f });
        
        vertices.insert(vertices.end(), { x2, height, z2, u2, 1.0f });
        vertices.insert(vertices.end(), { x1, height, z1, u1, 1.0f });
        vertices.insert(vertices.end(), { x1, 0.0f,   z1, u1, 0.0f });
    }
    return vertices;
}


std::vector<float> generateSphere(float radius, int stacks, int slices) {
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

            float u1 = static_cast<float>(j) / slices;
            float v1 = static_cast<float>(i) / stacks;
            float u2 = static_cast<float>(j + 1) / slices;
            float v2 = static_cast<float>(i + 1) / stacks;

            
            vertices.insert(vertices.end(), { p1.x, p1.y, p1.z, u1, v1 });
            vertices.insert(vertices.end(), { p3.x, p3.y, p3.z, u1, v2 });
            vertices.insert(vertices.end(), { p2.x, p2.y, p2.z, u2, v1 });
            
            vertices.insert(vertices.end(), { p2.x, p2.y, p2.z, u2, v1 });
            vertices.insert(vertices.end(), { p3.x, p3.y, p3.z, u1, v2 });
            vertices.insert(vertices.end(), { p4.x, p4.y, p4.z, u2, v2 });
        }
    }
    return vertices;
}


int main() {

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Test", NULL, NULL);
    if (window == NULL) {
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

    unsigned int grassTex = loadTexture("D:/CLion_OpenGL/deps/textures/iarba.png");
    unsigned int skyTex   = loadTexture("D:/CLion_OpenGL/deps/textures/cer.jpg");
    unsigned int roadTex  = loadTexture("D:/CLion_OpenGL/deps/textures/asfalt.jpg");
    unsigned int barkTex  = loadTexture("D:/CLion_OpenGL/deps/textures/bark.jpg");   
    unsigned int leafTex  = loadTexture("D:/CLion_OpenGL/deps/textures/leaves.jpg"); 

    
    
    
    float vertices[] = {
        
        -0.5f,-0.5f,-0.5f, 0.0f,0.5f,
         0.5f,-0.5f,-0.5f, 1.0f,0.5f,
         0.5f, 0.5f,-0.5f, 1.0f,1.0f,
         0.5f, 0.5f,-0.5f, 1.0f,1.0f,
        -0.5f, 0.5f,-0.5f, 0.0f,1.0f,
        -0.5f,-0.5f,-0.5f, 0.0f,0.5f,

        -0.5f,-0.5f,0.5f, 0.0f,0.5f,
         0.5f,-0.5f,0.5f, 1.0f,0.5f,
         0.5f, 0.5f,0.5f, 1.0f,1.0f,
         0.5f, 0.5f,0.5f, 1.0f,1.0f,
        -0.5f, 0.5f,0.5f, 0.0f,1.0f,
        -0.5f,-0.5f,0.5f, 0.0f,0.5f,

        -0.5f,0.5f,0.5f,1.0f,1.0f,
        -0.5f,0.5f,-0.5f,0.0f,1.0f,
        -0.5f,-0.5f,-0.5f,0.0f,0.5f,
        -0.5f,-0.5f,-0.5f,0.0f,0.5f,
        -0.5f,-0.5f,0.5f,1.0f,0.5f,
        -0.5f,0.5f,0.5f,1.0f,1.0f,

         0.5f,0.5f,0.5f,1.0f,1.0f,
         0.5f,0.5f,-0.5f,0.0f,1.0f,
         0.5f,-0.5f,-0.5f,0.0f,0.5f,
         0.5f,-0.5f,-0.5f,0.0f,0.5f,
         0.5f,-0.5f,0.5f,1.0f,0.5f,
         0.5f,0.5f,0.5f,1.0f,1.0f,

        -0.5f,0.5f,-0.5f,0.0f,1.0f,
         0.5f,0.5f,-0.5f,1.0f,1.0f,
         0.5f,0.5f,0.5f,1.0f,1.0f,
         0.5f,0.5f,0.5f,1.0f,1.0f,
        -0.5f,0.5f,0.5f,0.0f,1.0f,
        -0.5f,0.5f,-0.5f,0.0f,1.0f,

        
        -0.5f,-0.5f,-0.5f,0.0f,0.0f,
         0.5f,-0.5f,-0.5f,1.0f,0.0f,
         0.5f,-0.5f,0.5f,1.0f,1.0f,
         0.5f,-0.5f,0.5f,1.0f,1.0f,
        -0.5f,-0.5f,0.5f,0.0f,1.0f,
        -0.5f,-0.5f,-0.5f,0.0f,0.0f
    };

    std::vector<float> hillVertices = generateHill(1.0f, 1.0f, 64);
    std::vector<float> roadVertices = generateDonutRoad(kRoadInnerRadius, kRoadOuterRadius, 100);

    
    
    
    std::vector<float> trunkVertices = generateCylinder(1.0f, 1.0f, 12);
    std::vector<float> crownVertices = generateSphere(1.0f, 16, 32);

    
    
    
    auto makeVAO = [](const float* data, size_t byteSize) -> unsigned int {
        unsigned int VAO, VBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, byteSize, data, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        return VAO;
    };

    unsigned int VAO1     = makeVAO(vertices, sizeof(vertices));

    unsigned int hillVAO  = makeVAO(hillVertices.data(),  hillVertices.size()  * sizeof(float));
    unsigned int roadVAO  = makeVAO(roadVertices.data(),  roadVertices.size()  * sizeof(float));
    unsigned int trunkVAO = makeVAO(trunkVertices.data(), trunkVertices.size() * sizeof(float));
    unsigned int crownVAO = makeVAO(crownVertices.data(), crownVertices.size() * sizeof(float));

    
    
    
    
    struct TreeInstance { glm::vec3 pos; float scale; };
    std::vector<TreeInstance> trees;
    trees.reserve(kTreeCount);

    
    float radialJitter[] = { 0.0f, 2.5f * kSceneScale, -2.0f * kSceneScale,
                              1.5f * kSceneScale, -3.0f * kSceneScale,
                              0.5f * kSceneScale, -1.0f * kSceneScale };
    int jitterCount = sizeof(radialJitter) / sizeof(radialJitter[0]);

    for (int i = 0; i < kTreeCount; i++) {
        float angle  = (2.0f * 3.1415926f * i) / kTreeCount;
        float r      = kTreeRingRadius + radialJitter[i % jitterCount];
        float scaleF = 1.0f + 0.3f * static_cast<float>(i % 3); 
        trees.push_back({ glm::vec3(r * cos(angle), 0.0f, r * sin(angle)), scaleF });
    }

    
    Shader shader("D:/CLion_OpenGL/shaders/shader.vs",
                  "D:/CLion_OpenGL/shaders/shader.fs");
    shader.use();
    shader.setInt("texture1", 0);

    
    const float kTrunkRadius = 0.6f * kSceneScale;
    const float kTrunkHeight = 3.5f * kSceneScale;
    const float kCrownRadius = 3.0f * kSceneScale;

    
    
    
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        glm::mat4 projection =
            glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, kBaseFarPlane);
        shader.setMat4("projection", projection);

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        shader.setMat4("view", view);

        
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.5f * kBoxScale, 0.0f));
        model = glm::scale(model, glm::vec3(kBoxScale));
        shader.setMat4("model", model);

        glBindVertexArray(VAO1);
        glBindTexture(GL_TEXTURE_2D, skyTex);
        glDrawArrays(GL_TRIANGLES, 0, 30);
        glBindTexture(GL_TEXTURE_2D, grassTex);
        glDrawArrays(GL_TRIANGLES, 30, 6);

        
        glm::mat4 roadModel = glm::mat4(1.0f);
        roadModel = glm::translate(roadModel, glm::vec3(0.0f, 0.1f, 0.0f));
        shader.setMat4("model", roadModel);
        glBindVertexArray(roadVAO);
        glBindTexture(GL_TEXTURE_2D, roadTex);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(roadVertices.size() / 5));

        
        glm::mat4 hillModel = glm::mat4(1.0f);
        float hillSize = 11.0f * kSceneScale;
        hillModel = glm::scale(hillModel, glm::vec3(hillSize, hillSize * 0.5f, hillSize));
        shader.setMat4("model", hillModel);
        glBindVertexArray(hillVAO);
        glBindTexture(GL_TEXTURE_2D, grassTex);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(hillVertices.size() / 5));

        
        
        for (const TreeInstance& tree : trees) {

            float s = tree.scale;   

            
            glm::mat4 trunkModel = glm::mat4(1.0f);
            trunkModel = glm::translate(trunkModel, tree.pos);
            trunkModel = glm::scale(trunkModel,
                glm::vec3(kTrunkRadius * s, kTrunkHeight * s, kTrunkRadius * s));
            shader.setMat4("model", trunkModel);

            glBindVertexArray(trunkVAO);
            glBindTexture(GL_TEXTURE_2D, barkTex);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(trunkVertices.size() / 5));

            
            
            glm::vec3 crownBase = tree.pos + glm::vec3(0.0f, (kTrunkHeight + kCrownRadius * 0.7f) * s, 0.0f);
            glm::mat4 crownModel = glm::mat4(1.0f);
            crownModel = glm::translate(crownModel, crownBase);
            crownModel = glm::scale(crownModel, glm::vec3(kCrownRadius * s));
            shader.setMat4("model", crownModel);

            glBindVertexArray(crownVAO);
            
            glBindTexture(GL_TEXTURE_2D, leafTex);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(crownVertices.size() / 5));
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}





void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float cameraSpeed = kBaseCameraSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
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

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw   += xoffset;
    pitch += yoffset;

    if (pitch >  89.0f) pitch =  89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

unsigned int loadTexture(char const* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);

    if (data) {
        GLenum format;
        if      (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else                        format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    return textureID;
}

std::vector<float> generateHill(float radius, float height, int segments) {
    std::vector<float> vertices;
    glm::vec3 top(0, height, 0);

    for (int i = 0; i < segments; i++) {
        float angle1 = (2.0f * 3.1415926f * i)       / segments;
        float angle2 = (2.0f * 3.1415926f * (i + 1)) / segments;

        glm::vec3 p1(radius * cos(angle1), 0, radius * sin(angle1));
        glm::vec3 p2(radius * cos(angle2), 0, radius * sin(angle2));

        float uTop = 0.5f, vTop = 1.0f;
        float u1 = (cos(angle1) + 1) * 0.5f, v1 = (sin(angle1) + 1) * 0.5f;
        float u2 = (cos(angle2) + 1) * 0.5f, v2 = (sin(angle2) + 1) * 0.5f;

        vertices.insert(vertices.end(), { top.x, top.y, top.z, uTop, vTop });
        vertices.insert(vertices.end(), { p1.x,  p1.y,  p1.z,  u1,   v1  });
        vertices.insert(vertices.end(), { p2.x,  p2.y,  p2.z,  u2,   v2  });
    }
    return vertices;
}
