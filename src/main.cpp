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
unsigned int loadTexture(const char* path);
std::vector<float> generateHill(float radius, float height, int segments);

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

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    unsigned int grassTex = loadTexture("D:/CLion_OpenGL/deps/textures/iarba.png");
    unsigned int skyTex   = loadTexture("D:/CLion_OpenGL/deps/textures/cer.jpg");

    float vertices[] = {

        // Skybox faces
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

        // ground
        -0.5f,-0.5f,-0.5f,0.0f,0.0f,
         0.5f,-0.5f,-0.5f,1.0f,0.0f,
         0.5f,-0.5f,0.5f,1.0f,1.0f,
         0.5f,-0.5f,0.5f,1.0f,1.0f,
        -0.5f,-0.5f,0.5f,0.0f,1.0f,
        -0.5f,-0.5f,-0.5f,0.0f,0.0f
    };

    std::vector<float> hillVertices = generateHill(5.0f,2.0f,64);

    unsigned int VBO1,VAO1;
    glGenVertexArrays(1,&VAO1);
    glGenBuffers(1,&VBO1);

    glBindVertexArray(VAO1);
    glBindBuffer(GL_ARRAY_BUFFER,VBO1);
    glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);

    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    unsigned int hillVAO,hillVBO;

    glGenVertexArrays(1,&hillVAO);
    glGenBuffers(1,&hillVBO);

    glBindVertexArray(hillVAO);
    glBindBuffer(GL_ARRAY_BUFFER,hillVBO);
    glBufferData(GL_ARRAY_BUFFER,hillVertices.size()*sizeof(float),hillVertices.data(),GL_STATIC_DRAW);

    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    Shader shader("D:/CLion_OpenGL/shaders/shader.vs",
                  "D:/CLion_OpenGL/shaders/shader.fs");

    shader.use();
    shader.setInt("texture1",0);

    while(!glfwWindowShouldClose(window)){

        processInput(window);

        glClearColor(0.1f,0.1f,0.1f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        glm::mat4 projection =
        glm::perspective(glm::radians(45.0f),800.0f/600.0f,0.1f,100.0f);

        shader.setMat4("projection",projection);

        glm::mat4 view =
        glm::lookAt(glm::vec3(0,-19,3),
                    glm::vec3(0,-20,-5),
                    glm::vec3(0,1,0));

        shader.setMat4("view",view);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model,glm::vec3(40.0f));

        shader.setMat4("model",model);

        glBindVertexArray(VAO1);

        glBindTexture(GL_TEXTURE_2D,skyTex);
        glDrawArrays(GL_TRIANGLES,0,30);

        glBindTexture(GL_TEXTURE_2D,grassTex);
        glDrawArrays(GL_TRIANGLES,30,6);

        glBindVertexArray(hillVAO);
        glBindTexture(GL_TEXTURE_2D,grassTex);

        // primul deal
        glm::mat4 hillModel = glm::mat4(1.0f);
        hillModel = glm::translate(hillModel,glm::vec3(0,-20,-22));

        shader.setMat4("model",hillModel);

        glDrawArrays(GL_TRIANGLES,0,
        static_cast<GLsizei>(hillVertices.size()/5));

        // al doilea deal
        glm::mat4 hillModel2 = glm::mat4(1.0f);

        hillModel2 =
        glm::translate(hillModel2,glm::vec3(8,-20,-30));

        hillModel2 =
        glm::scale(hillModel2,glm::vec3(2.2f,1.0f,2.2f));

        shader.setMat4("model",hillModel2);

        glDrawArrays(GL_TRIANGLES,0,
        static_cast<GLsizei>(hillVertices.size()/5));

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window,int width,int height){
    glViewport(0,0,width,height);
}

void processInput(GLFWwindow* window){
    if(glfwGetKey(window,GLFW_KEY_ESCAPE)==GLFW_PRESS)
        glfwSetWindowShouldClose(window,true);
}

unsigned int loadTexture(char const * path){

    unsigned int textureID;
    glGenTextures(1,&textureID);

    int width,height,nrComponents;

    stbi_set_flip_vertically_on_load(true);

    unsigned char *data =
    stbi_load(path,&width,&height,&nrComponents,0);

    if(data){

        GLenum format;

        if(nrComponents==1) format = GL_RED;
        else if(nrComponents==3) format = GL_RGB;
        else format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D,textureID);

        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     format,
                     width,
                     height,
                     0,
                     format,
                     GL_UNSIGNED_BYTE,
                     data);

        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D,
                        GL_TEXTURE_WRAP_S,
                        GL_REPEAT);

        glTexParameteri(GL_TEXTURE_2D,
                        GL_TEXTURE_WRAP_T,
                        GL_REPEAT);

        glTexParameteri(GL_TEXTURE_2D,
                        GL_TEXTURE_MIN_FILTER,
                        GL_LINEAR_MIPMAP_LINEAR);

        glTexParameteri(GL_TEXTURE_2D,
                        GL_TEXTURE_MAG_FILTER,
                        GL_LINEAR);

        stbi_image_free(data);
    }

    return textureID;
}

std::vector<float> generateHill(float radius,float height,int segments){

    std::vector<float> vertices;

    glm::vec3 top(0,height,0);

    for(int i=0;i<segments;i++){

        float angle1 =
        (2.0f*3.1415926f*i)/segments;

        float angle2 =
        (2.0f*3.1415926f*(i+1))/segments;

        glm::vec3 p1(radius*cos(angle1),0,radius*sin(angle1));
        glm::vec3 p2(radius*cos(angle2),0,radius*sin(angle2));

        float uTop=0.5f;
        float vTop=1.0f;

        float u1=(cos(angle1)+1)*0.5f;
        float v1=(sin(angle1)+1)*0.5f;

        float u2=(cos(angle2)+1)*0.5f;
        float v2=(sin(angle2)+1)*0.5f;

        vertices.push_back(top.x);
        vertices.push_back(top.y);
        vertices.push_back(top.z);
        vertices.push_back(uTop);
        vertices.push_back(vTop);

        vertices.push_back(p1.x);
        vertices.push_back(p1.y);
        vertices.push_back(p1.z);
        vertices.push_back(u1);
        vertices.push_back(v1);

        vertices.push_back(p2.x);
        vertices.push_back(p2.y);
        vertices.push_back(p2.z);
        vertices.push_back(u2);
        vertices.push_back(v2);
    }

    return vertices;
}