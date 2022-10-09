
#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <vector>
#include "element.h"
#include "shaders.h"
#include "circle_element.h"
#include "overlay.h"
#include "level_save.h"
#include <map>
#include <string>

class Level {
protected:
    unsigned long tick;
    unsigned long last_tick;
    // circles
    GLuint circle_mesh, vertex_array, circle_info_buffer, checker_codebuffer, checker_statebuffer;
    GLuint vpos_loc, vcenter_loc, vradius_loc, vcol_loc;
    std::vector<GLfloat> circle_outline_mesh_vec;
    std::vector<CircleElement> circle_elements;
    std::vector<GLCircleElementInfo> circle_info_vec;
    Shader circ_element_shader;

    std::vector<GLfloat> circle_inside_mesh_vec, circle_border_mesh_vec;
    GLuint mode_loc;

    // text overlay
    Overlay overlay;

    // mouse
    GLfloat _mouse_x, _mouse_y;
    bool _mouse_down;

    // checker
    Shader checker_shader;
    GLuint checker_r_loc, checker_s_loc, checker_code_loc1, checker_code_loc2;
    GLuint feedback_buf;
    std::vector<GLuint> code; // does not change after loaded
    std::vector<GLint> state, constraint_inputs; // changes after loaded. state comes back from the GPU, constraint_inputs is updated when things are clicked
    std::vector<Constraint> constraints;

    std::map<size_t, std::vector<size_t>> index; // Circle index => list of constraint inputs to modify

    // score
    long _score;
    std::vector<size_t> log;

    // TODO: sliders



public:
    Level(const char *fname) : circ_element_shader(circ_vertex_shader_text, circ_fragment_shader_text), checker_shader(checker_vertex_shader_text, nullptr, true), tick(0), overlay("") {
        // exit_time = _entry_time + DEFAULT_DURATION;
        // radius = DEFAULT_RADIUS;
        //
        // for (int i=0; i<200000; i++) {
        //     code.insert(code.end(), {0x07070707, 0x07070707, 0x07070707, 0x07070707, 0x06060606, 0x06060606,0x06060606,0x06060606});
        //     checks.insert(checks.end(), { {0, 1, 1, 1}, {}, 42});
        // }
        std::string fname_s(fname);
        LevelSave save(fname_s);
        circle_elements = save.notes;
        last_tick = save.max_tick;
        constraints = save.constraints;
        for (Constraint &c : constraints) {
            code.insert(code.end(), c.code, c.code + 8);
        }
        // circle_elements.push_back(CircleElement({0.5, 0.5}, {1.0, 0.0, 0.0}, 150.0, '1'));
        // circle_elements.push_back(CircleElement({0.75, 0.5}, {1.0, 0.0, 0.0}, 150.0, '2'));
        setup();
        build_index();
    }
    ~Level() {
        // free gl stuff
        glDeleteBuffers(1, &circle_info_buffer);
        glDeleteBuffers(1, &circle_mesh);
        glDeleteVertexArrays(1, &vertex_array);
    }
    long score() {
        return _score;
    }
    void render();
    void update_mouse(double x, double y, bool clicked);
    void emulate_mouse_for_circle(size_t idx);
    unsigned long cur_tick() {
        return tick;
    }
private:
    void setup();
    void create_mesh();
    void render_circles();
    void add_more_circles();
    void render_mouse(float x, float y);
    void handle_click();
    void checker();
    void build_index();
    void update_score();
};
