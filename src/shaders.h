#pragma once

#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <vector>
#include "element.h"
#include "shaders.h"
// https://stackoverflow.com/a/25021520
static const char* circ_vertex_shader_text = 
#include "shaders/circle.vert"
;
static const char* circ_fragment_shader_text = 
#include "shaders/circle.frag"
;

static const char* overlay_vertex_shader_text = 
#include "shaders/overlay.vert"
;
static const char* overlay_frag_shader_text = 
#include "shaders/overlay.frag"
;

static const char* checker_vertex_shader_text = 
#include "shaders/checker.vert"
;
static const char* checker_fragment_shader_text = 
#include "shaders/checker.frag"
;

class Shader {
private:
    GLuint program;
    void register_transform_feedback();
    bool setup(const char *vertex_shader_text, const char *fragment_shader_text, bool feedback);
public:
    Shader(const char *vertex_shader_text, const char *fragment_shader_text, bool feedback = false);
    bool activate();
    GLuint attrib(const char *name);
    void setUniform(GLuint loc, GLuint v0);
    void setTexForLoc(GLuint loc, GLint v0);
    GLuint uniform_loc(const char *name);
};
