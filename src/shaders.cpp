
#include "shaders.h"

Shader::Shader(const char *vertex_shader_text, const char *fragment_shader_text, bool feedback) {
    setup(vertex_shader_text, fragment_shader_text, feedback);
}
bool Shader::activate() {
    glUseProgram(program);
    return true;
}
bool Shader::setup(const char *vertex_shader_text, const char *fragment_shader_text, bool feedback) {
    GLuint vertex_shader, fragment_shader;
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
    glCompileShader(vertex_shader);

    if (fragment_shader_text != NULL) {
        fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
        glCompileShader(fragment_shader);
    }
    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    if (fragment_shader_text != NULL ) {
        glAttachShader(program, fragment_shader);
    }
    if (feedback) {
        register_transform_feedback();
    }

    glLinkProgram(program);

    int link_status = 0;
    glGetProgramiv( program, GL_LINK_STATUS, &link_status);
    if (!link_status) {
        char buf[0x1000];
        int len;
        glGetShaderInfoLog(vertex_shader, 0x1000, &len, buf);
        // glGetProgramInfoLog(program, 0x100, &len, buf);
        printf("%s\n", vertex_shader_text);
        printf("link error %s\n", buf);
        exit(1);
        return false;
    }
    return true;
}
GLuint Shader::attrib(const char *name) {
    return glGetAttribLocation(program, name);
}

GLuint Shader::uniform_loc(const char *name) {
    return glGetUniformLocation(program, name);
}

// precondition: must be active shader
void Shader::setUniform(GLuint loc, GLuint v0) {
    glUniform1ui(loc, v0);
}

// precondition: must be active shader
void Shader::setTexForLoc(GLuint loc, GLint v0) {
    glUniform1i(loc, v0);
}

void Shader::register_transform_feedback() {
    static const GLchar* varyings[1] = {"out_attr"};
    glTransformFeedbackVaryings(program, 1, varyings, GL_INTERLEAVED_ATTRIBS);
    // glLinkProgram(program);

    // int link_status = 0;
    // glGetProgramiv( program, GL_LINK_STATUS, &link_status);
    // if (!link_status) {
    //     char buf[0x100];
    //     int len;
    //     // glGetShaderInfoLog(vertex_shader, 0x100, &len, buf);
    //     glGetProgramInfoLog(program, 0x100, &len, buf);
    //     printf("link2 error %s\n", buf);
    // }
}
