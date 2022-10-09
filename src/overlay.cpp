
#define _USE_MATH_DEFINES
#include "math.h"
#include <numbers>
#include "overlay.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <iostream>
#include <sstream>
#include "tilemap.h"
#define TILEMAP_WIDTH (1.0f / 15.0f)
#define TILEMAP_HEIGHT (1.0f)
#define TILEMAP_ROW_HEIGHT (1.0f / 5.0f)

#define IDX_TO_X(idx) ((idx % 15) * TILEMAP_WIDTH)
#define IDX_TO_Y(idx) ((idx / 15) * TILEMAP_ROW_HEIGHT)

#define ANGLE1 1.07144961f
#define ANGLE2 (PIF - ANGLE1)

#define RADIUS_FOR_TEXT (DEFAULT_RADIUS_LOW * 2.0f / 3.0f)

static char const* gl_error_string(GLenum const err) noexcept;

void Overlay::render(unsigned long tick, long score, std::vector<CircleElement> &circles) {
  render_circle_text(tick, circles);
  std::stringstream ss;
  ss << "Score: " << score;
  std::string s = ss.str();
  print_jank_text(s.c_str(), -0.75, 0.9);
}

static void add_vertices_for_circle(CircleElement &e, char i, std::vector<GLfloat> &vertex_vec) {
  const float PIF = std::numbers::pi_v<float>;
  vertex_vec.insert(vertex_vec.end(), {
    e.position().x + RADIUS_FOR_TEXT * cosf(ANGLE2), e.position().y + RADIUS_FOR_TEXT * sinf(ANGLE2), IDX_TO_X(i), IDX_TO_Y(i),
    e.position().x + RADIUS_FOR_TEXT * cosf(-ANGLE2), e.position().y + RADIUS_FOR_TEXT * sinf(-ANGLE2), IDX_TO_X(i), IDX_TO_Y(i) + TILEMAP_ROW_HEIGHT,
    e.position().x + RADIUS_FOR_TEXT * cosf(-ANGLE1), e.position().y + RADIUS_FOR_TEXT * sinf(-ANGLE1), IDX_TO_X(i) + TILEMAP_WIDTH, IDX_TO_Y(i) + TILEMAP_ROW_HEIGHT,
    e.position().x + RADIUS_FOR_TEXT * cosf(-ANGLE1), e.position().y + RADIUS_FOR_TEXT * sinf(-ANGLE1), IDX_TO_X(i) + TILEMAP_WIDTH, IDX_TO_Y(i) + TILEMAP_ROW_HEIGHT,
    e.position().x + RADIUS_FOR_TEXT * cosf(ANGLE1), e.position().y + RADIUS_FOR_TEXT * sinf(ANGLE1), IDX_TO_X(i) + TILEMAP_WIDTH, IDX_TO_Y(i),
    e.position().x + RADIUS_FOR_TEXT * cosf(ANGLE2), e.position().y + RADIUS_FOR_TEXT * sinf(ANGLE2), IDX_TO_X(i), IDX_TO_Y(i),
  });

}
void Overlay::render_circle_text(unsigned long tick, std::vector<CircleElement> &circles) {
    vertex_vec.clear();
    // TODO: Optimize this? This is somewhat stupid to do all this cpu math here for the same 10 digits when it could be a mesh / computed in shader
    for (auto &e: circles) {
      if (e.should_render(tick)) {
        add_vertices_for_circle(e, e.tilemap_idx(), vertex_vec);
      }
      // std::cout << " " << info.x << " "<< info.y << " " << info.r << " " << info.g << " " << info.b << " " << info.radius << std::endl;
    }
    overlay_shader.activate();
    overlay_shader.setUniform(mode_loc, 1);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buf);
    glEnableVertexAttribArray(vpos_location);
    glEnableVertexAttribArray(vtexcoords_location);
    glVertexAttribDivisor(vpos_location, 0);
    glVertexAttribDivisor(vtexcoords_location, 0);

    glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, (void*) 0);
    glVertexAttribPointer(vtexcoords_location, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4 , (void*) (2 * sizeof(GLfloat)));

    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertex_vec.size(), vertex_vec.data(), GL_STATIC_DRAW);

    glActiveTexture(GL_TEXTURE0);

    glBindTexture(GL_TEXTURE_2D, texture);
    overlay_shader.setTexForLoc(overlay_shader.uniform_loc("tex"), 0);

    // This draws the text in most of the circles
    glDrawArrays(GL_TRIANGLES, 0, vertex_vec.size() / 4);

    // Now mark recently dead ones with an X
    vertex_vec.clear();
    for (auto &e: circles) {
      if (e.just_died(tick)) {
        char i = CHAR_TO_IDX('X');
        add_vertices_for_circle(e, i, vertex_vec);
        // std::cout << " " << info.x << " "<< info.y << " " << info.r << " " << info.g << " " << info.b << " " << info.radius << std::endl;
      }
    }
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertex_vec.size(), vertex_vec.data(), GL_STATIC_DRAW);

    overlay_shader.setUniform(mode_loc, 3);
    // This draws the text on the just died circles
    glDrawArrays(GL_TRIANGLES, 0, vertex_vec.size() / 4);

    // Now mark recently dead ones with an X
    vertex_vec.clear();
    for (auto &e: circles) {
      if (e.just_scored(tick)) {
        char i = CHECK_IDX;
        add_vertices_for_circle(e, i, vertex_vec);
        // std::cout << " " << info.x << " "<< info.y << " " << info.r << " " << info.g << " " << info.b << " " << info.radius << std::endl;
      }
    }
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertex_vec.size(), vertex_vec.data(), GL_STATIC_DRAW);

    overlay_shader.setUniform(mode_loc, 2);
    // This draws the text on the just scored circles
    glDrawArrays(GL_TRIANGLES, 0, vertex_vec.size() / 4);


    glDisableVertexAttribArray(vpos_location);
    glDisableVertexAttribArray(vtexcoords_location);
}
#define JANK_TEXT_WIDTH 0.1f / 2.0f
#define JANK_TEXT_HEIGHT 0.1833333333f / 2.0f
void Overlay::print_jank_text(const std::string &text, float x, float y) {
    vertex_vec.clear();
    overlay_shader.activate();
    overlay_shader.setUniform(mode_loc, 1);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buf);
    glEnableVertexAttribArray(vpos_location);
    glEnableVertexAttribArray(vtexcoords_location);
    glVertexAttribDivisor(vpos_location, 0);
    glVertexAttribDivisor(vtexcoords_location, 0);

    glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, (void*) 0);
    glVertexAttribPointer(vtexcoords_location, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4 , (void*) (2 * sizeof(GLfloat)));

    vertex_vec.clear();
    float orig_x = x, orig_y = y;
    int char_idx = 0;
    for (const char &c: text) {
      char i = CHAR_TO_IDX(c);
      vertex_vec.insert(vertex_vec.end(), {
        x, y, IDX_TO_X(i), IDX_TO_Y(i)+TILEMAP_ROW_HEIGHT,
        x, y+JANK_TEXT_HEIGHT, IDX_TO_X(i), IDX_TO_Y(i),
        x+JANK_TEXT_WIDTH, y+JANK_TEXT_HEIGHT, IDX_TO_X(i) + TILEMAP_WIDTH, IDX_TO_Y(i),
        x+JANK_TEXT_WIDTH, y+JANK_TEXT_HEIGHT, IDX_TO_X(i) + TILEMAP_WIDTH, IDX_TO_Y(i),
        x+JANK_TEXT_WIDTH, y, IDX_TO_X(i) + TILEMAP_WIDTH, IDX_TO_Y(i) + TILEMAP_ROW_HEIGHT,
        x, y, IDX_TO_X(i), IDX_TO_Y(i) + TILEMAP_ROW_HEIGHT,
      });
      x += JANK_TEXT_WIDTH;
      // wrap around every 16 chars
      if (char_idx > 24) {
        char_idx = 0;
        x = orig_x;
        y -= JANK_TEXT_HEIGHT;
      }
      char_idx++;
        // std::cout << " " << info.x << " "<< info.y << " " << info.r << " " << info.g << " " << info.b << " " << info.radius << std::endl;
    }

    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertex_vec.size(), vertex_vec.data(), GL_STATIC_DRAW);

    glActiveTexture(GL_TEXTURE0);

    glBindTexture(GL_TEXTURE_2D, texture);
    overlay_shader.setTexForLoc(overlay_shader.uniform_loc("tex"), 0);
    // This draws the text
    glDrawArrays(GL_TRIANGLES, 0, vertex_vec.size() / 4);

    glDisableVertexAttribArray(vpos_location);
    glDisableVertexAttribArray(vtexcoords_location);

}

static char const* gl_error_string(GLenum const err) noexcept
{
  switch (err)
  {
    // opengl 2 errors (8)
    case GL_NO_ERROR:
      return "GL_NO_ERROR";

    case GL_INVALID_ENUM:
      return "GL_INVALID_ENUM";

    case GL_INVALID_VALUE:
      return "GL_INVALID_VALUE";

    case GL_INVALID_OPERATION:
      return "GL_INVALID_OPERATION";

    case GL_STACK_OVERFLOW:
      return "GL_STACK_OVERFLOW";

    case GL_STACK_UNDERFLOW:
      return "GL_STACK_UNDERFLOW";

    case GL_OUT_OF_MEMORY:
      return "GL_OUT_OF_MEMORY";

    // case GL_TABLE_TOO_LARGE:
    //   return "GL_TABLE_TOO_LARGE";

    // opengl 3 errors (1)
    case GL_INVALID_FRAMEBUFFER_OPERATION:
      return "GL_INVALID_FRAMEBUFFER_OPERATION";

    // gles 2, 3 and gl 4 error are handled by the switch above
    default:
      assert(!"unknown error");
      return nullptr;
  }
}

void Overlay::setup() {
    glGenBuffers(1, &vertex_buf);

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buf);

    overlay_shader.activate();
    vpos_location = overlay_shader.attrib("vPos");
    vtexcoords_location = overlay_shader.attrib("vTexCoord");
    // printf("overlay locations %d %d\n", vpos_location, vtexcoords_location);

    // setup texture
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    int width, height, nrChannels;
    unsigned char *data = stbi_load_from_memory(tilemap, tilemap_len, &width, &height, &nrChannels, STBI_rgb_alpha);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    mode_loc = overlay_shader.uniform_loc("mode");
};
