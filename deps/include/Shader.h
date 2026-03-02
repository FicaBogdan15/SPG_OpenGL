//
// Created by rollo on 2/22/2026.
//

#ifndef CLION_OPENGL_SHADER_H
#define CLION_OPENGL_SHADER_H
#include <string>


class Shader {
    public:
    unsigned int ID;

    Shader(const char* vertexPath, const char* fragmentPath);
    void use();
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value)const;

};



#endif //CLION_OPENGL_SHADER_H