

#include <iostream>
#include "level.h"
#include "circle_element.h"
#include <random>

#define CHECKER_SIZE 0x1000

void Level::render() {
  tick++;
  if (tick <= last_tick) {
    render_circles();
    update_score();
    overlay.render(tick, _score, circle_elements);
    // invoke the checker
    checker();
  } else {
    // invoke the checker
    checker();
    std::string msg("that wasnt right :{");
    if (std::all_of(state.begin(), state.end(), [](GLint v) { return v == 1; })) {
      msg = std::string();
      for (int i=0; i < log.size(); i += 8) {
        unsigned char c = 0;
        for (int j=0; j<8; j++) {
          c |= (log[i+j] & 1) << j;
        }
        msg.push_back(c);
      }
    } else {
      // int count = 0;
      // for (GLint v : state) {
      //   if (v == 1) {
      //     count += 1;
      //   }
      // }
      // fprintf(stderr, "succ count: %d\n", count);
      // pick the first four
      // fprintf(stderr, "regs: %ld %ld %ld\n", constraint_inputs[0], constraint_inputs[1], constraint_inputs[2]);


    }
    overlay.print_jank_text(msg, -0.9, 0.0);
  }

}
void Level::update_mouse(double x, double y, bool clicked) {
  _mouse_x = (float)x;
  _mouse_y = (float)y;
  if (!_mouse_down && clicked) {
    handle_click();
  }
  _mouse_down = clicked;
}

void Level::emulate_mouse_for_circle(size_t idx) {
  if (idx != -1) {
    auto circle = circle_elements[idx];
    update_mouse(circle.position().x, circle.position().y, true);
  } else {
    update_mouse(_mouse_x, _mouse_y, false);
  }
}

void Level::render_mouse(float x, float y) {
  // std::cout << x << " " << y << std::endl;
  circ_element_shader.setUniform(mode_loc, 4);

  glBindBuffer(GL_ARRAY_BUFFER, circle_info_buffer);
  GLCircleElementInfo info = {0.0, 1.0, 0.0, x, y, 0.1};
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLCircleElementInfo) * 1, &info, GL_STATIC_DRAW);
  // Draw mesh
  glBindBuffer(GL_ARRAY_BUFFER, circle_mesh);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * circle_inside_mesh_vec.size(), circle_inside_mesh_vec.data(), GL_STATIC_DRAW);

  // This draws the cursor
  glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, circle_inside_mesh_vec.size() / 2, 1);
}
void Level::render_circles() {
  // add_more_circles();

    circle_info_vec.clear();
    for (auto &e: circle_elements) {
      if (e.should_render(tick)) {
        e.tick(tick);
        circle_info_vec.insert(circle_info_vec.end(), e.glinfo());
        // GLCircleElementInfo info = e.glinfo();
        // std::cout << " " << info.x << " "<< info.y << " " << info.r << " " << info.g << " " << info.b << " " << info.radius << std::endl;
      }
    }
    circ_element_shader.activate();
    glEnableVertexAttribArray(vpos_loc);
    glEnableVertexAttribArray(vcenter_loc);
    glEnableVertexAttribArray(vradius_loc);
    glEnableVertexAttribArray(vcol_loc);


    glBindBuffer(GL_ARRAY_BUFFER, circle_info_buffer);
    glVertexAttribDivisor(vcol_loc, 1);
    glVertexAttribDivisor(vcenter_loc, 1);
    glVertexAttribDivisor(vradius_loc, 1);
    glVertexAttribPointer(vcol_loc, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6 , (void*) 0);
    glVertexAttribPointer(vcenter_loc, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6 , (void*) (3 * sizeof(GLfloat)));
    glVertexAttribPointer(vradius_loc, 1, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6 , (void*) (5 * sizeof(GLfloat)));

    glBufferData(GL_ARRAY_BUFFER, sizeof(GLCircleElementInfo) * circle_info_vec.size(), circle_info_vec.data(), GL_STATIC_DRAW);


    circ_element_shader.setUniform(mode_loc, 2);

    // Draw mesh
    glBindBuffer(GL_ARRAY_BUFFER, circle_mesh);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * circle_inside_mesh_vec.size(), circle_inside_mesh_vec.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(vpos_loc, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 2, (void*) 0);

    // This draws the inside
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, circle_inside_mesh_vec.size() / 2, circle_info_vec.size());

    circ_element_shader.setUniform(mode_loc, 3);

    // Draw mesh
    glBindBuffer(GL_ARRAY_BUFFER, circle_mesh);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * circle_border_mesh_vec.size(), circle_border_mesh_vec.data(), GL_STATIC_DRAW);

    // This draws the inside
    glDrawArraysInstanced(GL_TRIANGLES, 0, circle_border_mesh_vec.size() / 2, circle_info_vec.size());

    // Draw outer moving ring
    circ_element_shader.setUniform(mode_loc, 1);
    glBindBuffer(GL_ARRAY_BUFFER, circle_mesh);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * circle_outline_mesh_vec.size(), circle_outline_mesh_vec.data(), GL_STATIC_DRAW);

    // This draws the outline
    glDrawArraysInstanced(GL_TRIANGLES, 0, circle_outline_mesh_vec.size() / 2, circle_info_vec.size());

    // While we have all the circle stuff loaded, render the mouse
    render_mouse(_mouse_x, _mouse_y);

    glDisableVertexAttribArray(vpos_loc);
    glDisableVertexAttribArray(vcenter_loc);
    glDisableVertexAttribArray(vradius_loc);
    glDisableVertexAttribArray(vcol_loc);


    // glEnable(GL_LINE_SMOOTH);
    // glLineWidth(3);

    // glDrawArraysInstanced(GL_LINE_LOOP, 0, circle_outline_mesh_vec.size() / 2, circle_info_vec.size());
}

void Level::setup() {
    glGenBuffers(1, &circle_mesh); 
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    glGenBuffers(1, &circle_info_buffer);
    glGenBuffers(1, &checker_codebuffer);
    glGenBuffers(1, &checker_statebuffer);
    glGenBuffers(1, &feedback_buf);


    bool dirty = true;
    CircleElement::render_mesh(circle_outline_mesh_vec, circle_inside_mesh_vec, circle_border_mesh_vec);

    // glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    circ_element_shader.activate();
    vpos_loc = circ_element_shader.attrib("vPos");
    vcenter_loc = circ_element_shader.attrib("vCenter");
    vradius_loc = circ_element_shader.attrib("vRadius");
    vcol_loc = circ_element_shader.attrib("vCol1");
    mode_loc = circ_element_shader.uniform_loc("mode");

    checker_shader.activate();
    checker_r_loc = checker_shader.attrib("r");
    checker_s_loc = checker_shader.attrib("s");
    checker_code_loc1 = checker_shader.attrib("code");
    checker_code_loc2 = checker_code_loc1 + 1;
    // checker_out_loc = checker_shader.attrib("out");
    // printf("checker locs: %d %d %d\n", checker_r_loc, checker_s_loc, checker_code_loc1);

    // put code in the code buffer
    glBindBuffer(GL_ARRAY_BUFFER, checker_codebuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLuint) * code.size(), code.data(), GL_STATIC_COPY);



    // printf("locs: %d %d %d %d\n", vpos_loc, vcenter_loc, vradius_loc, vcol_loc);
    // glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE,
    //                   sizeof(vertices[0]), (void*) 0);

    // setup checker shader
    checker_shader.activate();

    constraint_inputs.resize(constraints.size() * 4, -1);
    state.resize(constraints.size(), 7);
    glBindBuffer(GL_ARRAY_BUFFER, checker_statebuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLint) * state.size(), state.data(), GL_STATIC_READ);


    overlay.setup();
}

void Level::update_score() {
  long score = 0;
  for (auto &c: circle_elements) {
    score += c.points(tick);
  }
  _score = score;
}

void Level::handle_click() {
    // TODO: is render order defined? Maybe we just avoid overlapping
    int i=0;
    for (auto &c: circle_elements) {
      if (c.should_render(tick) && pow(c.position().x - _mouse_x, 2) + pow(c.position().y - _mouse_y, 2) < pow(DEFAULT_RADIUS_LOW, 2)) {
        // delete!
        c.kill(tick);
        // update index
        for (size_t idx : index[i]) {
          constraint_inputs[idx] = c.score_time();
        }
        if (c.score_time() != -1) {
          log.push_back(c.id());
        }
        break;
      }
      i++;
    }
}

void Level::build_index() {
  int idx = 0;
  for (Constraint &c : constraints) {
    for (size_t z : c.circ_idx) {
      index[z].push_back(idx);
      idx++;
    }
  }
}

// #define GL_BUFFER_UPDATE_BARRIER_BIT 0x00000200

void Level::checker() {
  // get data for the last frame
  if (tick > 1) {
    // glBindBuffer(GL_ARRAY_BUFFER, checker_statebuffer);
    // glCopyBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, GL_ARRAY_BUFFER, 0, 0, state.size() * sizeof(GLint));

    glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, sizeof(GLint) * state.size(), state.data());
    // glMemoryBarrier(GL_ALL_BARRIER_BITS);
    for (int i=0; i<4; i++) {
      // printf("state %d: %u\n", i, state[i]);
    }
  }
  checker_shader.activate();
  // in uvec4 r;
  // in uvec4 code;
  // in uint s;
  // uint t;
  // uint skip;
  // out uint out;
  glEnableVertexAttribArray(checker_r_loc);
  glEnableVertexAttribArray(checker_s_loc);
  glEnableVertexAttribArray(checker_code_loc1);
  glEnableVertexAttribArray(checker_code_loc2);


  glVertexAttribDivisor(checker_r_loc, 0);
  glVertexAttribDivisor(checker_code_loc1, 0);
  glVertexAttribDivisor(checker_code_loc2, 0);
  glVertexAttribDivisor(checker_s_loc, 0);

  glBindBuffer(GL_ARRAY_BUFFER, checker_codebuffer);
  glVertexAttribIPointer(checker_code_loc1, 4, GL_UNSIGNED_INT, sizeof(GLuint) * 8 , (void*) (0));
  glVertexAttribIPointer(checker_code_loc2, 4, GL_UNSIGNED_INT, sizeof(GLuint) * 8 , (void*) (4 * sizeof(GLuint)));

  glBindBuffer(GL_ARRAY_BUFFER, checker_statebuffer);
  glVertexAttribIPointer(checker_s_loc, 1, GL_INT, sizeof(GLint) * 1 , (void*) 0);

  glBufferData(GL_ARRAY_BUFFER, sizeof(GLint) * state.size(), state.data(), GL_STATIC_READ);

  glBindBuffer(GL_ARRAY_BUFFER, circle_mesh);
  glVertexAttribIPointer(checker_r_loc, 4, GL_INT, sizeof(GLint) * 4 , (void*) 0);
  // glVertexAttribPointer(checker_r_loc, 4, GL_UNSIGNED_INT, GL_FALSE, sizeof(GLint) * 9 , (void*) (4 * sizeof(GLint)));
  // glVertexAttribPointer(checker_s_loc, 1, GL_INT, GL_FALSE, sizeof(GLint) * 5 , (void*) (4 * sizeof(GLint)));


  // Add register data
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLint) * constraint_inputs.size(), constraint_inputs.data(), GL_STATIC_READ);

  // Enable transform feedback
  glBindBuffer(GL_ARRAY_BUFFER, feedback_buf);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLuint) * state.size(), nullptr, GL_STATIC_READ);
  glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, feedback_buf);

  glEnable(GL_RASTERIZER_DISCARD);
  glBeginTransformFeedback(GL_POINTS);


  // Do the checks
  glDrawArrays(GL_POINTS, 0, constraints.size());
  glEndTransformFeedback();
  glDisable(GL_RASTERIZER_DISCARD);

  glDisableVertexAttribArray(checker_r_loc);
  glDisableVertexAttribArray(checker_s_loc);
  glDisableVertexAttribArray(checker_code_loc1);
  glDisableVertexAttribArray(checker_code_loc2);

  // std::cout << "Time difference 1 = " << std::chrono::duration_cast<std::chrono::microseconds>(end1 - begin).count() << "[µs]" << std::endl;
  // std::cout << "Time difference 2 = " << std::chrono::duration_cast<std::chrono::microseconds>(end2 - end1).count() << "[µs]" << std::endl;
  // std::cout << "Time difference 3 = " << std::chrono::duration_cast<std::chrono::microseconds>(end3 - end2).count() << "[µs]" << std::endl;



}


