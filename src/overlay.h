#pragma once
#include "shaders.h"

#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <vector>
#include <string>
#include "circle_element.h"

class Overlay {
private:
    GLuint vpos_location, vtexcoords_location, mode_loc;
protected:
    // 
    GLuint vertex_buf;
    std::vector<GLfloat> vertex_vec;
    Shader overlay_shader;
    GLuint texture;


public:
    Overlay(char *element_data) : overlay_shader(overlay_vertex_shader_text, overlay_frag_shader_text)  {
        // exit_time = _entry_time + DEFAULT_DURATION;
        // radius = DEFAULT_RADIUS;
        //
        // setup();
    }
    ~Overlay() {
        // free circle info buffer
        glDeleteBuffers(1, &vertex_buf);
        glDeleteTextures(1, &texture);
    }
    void render(unsigned long tick, long score, std::vector<CircleElement> &circles);
    void setup();
    void print_jank_text(const std::string &text, float x, float y);
private:
    void render_circle_text(unsigned long tick, std::vector<CircleElement> &circles);
};
