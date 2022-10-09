
#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <vector>
#include "element.h"
#include "shaders.h"
#include "level.h"
#include "audio.h"
#include "replay.h"

#include <iostream>
#include <sstream>
// Most of this file is cstatic const struct

#define STARTUP_FRAMES 155
struct {
    float x, y;
    float r, g, b;
} vertices[3] =
{
    { -0.6f, -0.4f, 1.f, 0.f, 0.f },
    {  0.6f, -0.4f, 0.f, 1.f, 0.f },
    {   0.f,  0.6f, 0.f, 0.f, 1.f }
};

void error_callback(int error, const char* description);
static int run_loop(GLFWwindow *window, const char *beatmap_name, const char *recording_name);

void GLAPIENTRY
MessageCallback( GLenum source,
                 GLenum type,
                 GLuint id,
                 GLenum severity,
                 GLsizei length,
                 const GLchar* message,
                 const void* userParam )
{
  fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
           ( "" ),
            type, severity, message );
}

int main(int argc, const char **argv) {
  if (argc < 2 || argc > 4) {
    fprintf(stderr, "Usage: ./osu <beatmap_file> [<record_file>]\n");
    return -1;
  }
    const char *beatmap_name = argv[1];
    const char *recording_name = argc > 2 ? argv[2] : nullptr;
    if (!glfwInit()) {
        // Initialization failed
        printf("glfwInit failed\n");
        return -1;
    }
    glfwSetErrorCallback(error_callback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    GLFWwindow* window = glfwCreateWindow(900, 900, "osu?", NULL, NULL);
    if (!window) {
        // Window or OpenGL context creation failed
        printf("glfwCreateWindow failed\n");
        goto cleanup;
    }
    glfwSetWindowAttrib(window, GLFW_RESIZABLE, false);
    glfwMakeContextCurrent(window);
    gladLoadGL();
    printf("%s\n", glGetString(GL_VERSION));
    run_loop(window, beatmap_name, recording_name);
cleanup:
    glfwTerminate();
    return 0;
}

void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

char const* gl_error_string(GLenum const err) noexcept
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
      return "Unknown OpenGL error";
  }
}
double clamp(float d, float min, float max) {
  const float t = d < min ? min : d;
  return t > max ? max : t;
}

double lastTime;
unsigned long nbFrames;
void showFPS(GLFWwindow *pWindow)
{
   // Measure speed
   double currentTime = glfwGetTime();
   double delta = currentTime - lastTime;
   nbFrames++;
   if ( delta >= 1.0 ){ // If last cout was more than 1 sec ago
       // std::cout << 1000.0/double(nbFrames) << std::endl;

       double fps = double(nbFrames) / delta;
       // double delay = delta - double(nbFrames) * (1.0f / 60.0f);
       std::stringstream ss;
       ss << "osu? [" << fps << " FPS]";

       glfwSetWindowTitle(pWindow, ss.str().c_str());

       nbFrames = 0;
       lastTime = currentTime;
       // return delay;
   }
   // return 0;
}
static int run_loop(GLFWwindow *window, const char *beatmap_name, const char *recording_name) {
    GLuint vertex_buffer, vertex_array, circle_info_buffer, vertex_shader, fragment_shader, program;
    GLint vpos_location, vcol_location, vcenter_location, vradius_location;
    auto level = Level(beatmap_name);
    Replay *rec = nullptr;
    if (recording_name != nullptr) {
      rec = new Replay(recording_name);
    }
    auto audio = Audio((STARTUP_FRAMES - 2) * (1000.0 / 60.0));

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_MULTISAMPLE);
    glfwSetKeyCallback(window, key_callback);
    glfwSwapInterval(1);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    unsigned long fnum = 0;
    while (!glfwWindowShouldClose(window))
    {
        // Update
        fnum++;
        float ratio;
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        // printf("width=%d height=%d\n", width, height);
        ratio = width / (float) height;

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        double x, y;
        if (rec == nullptr) {
          glfwGetWindowSize(window, &width, &height);
          glfwGetCursorPos(window, &x, &y);
          bool pressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
          level.update_mouse(clamp(2 * x / width - 1.0, -1.0, 1.0), clamp(-2 * y / height + 1.0, -1.0, 1.0), pressed);
        } else {
          if (level.cur_tick() < rec->replay.size()) {
            auto idx_to_click = rec->replay[level.cur_tick()];
            level.emulate_mouse_for_circle(idx_to_click);
          }
        }

        level.render();
        GLint err;
        while((err = glGetError()) != GL_NO_ERROR)
        {
            const char *errString = gl_error_string(err);
            printf("%s\n", errString);
          // Process/log the error.
        }


        // glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
        // glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertex_vec.size(), vertex_vec.data(), GL_STATIC_DRAW);
        // glBindBuffer(GL_ARRAY_BUFFER, circle_info_buffer);
        // glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * circle_info_vec.size(), circle_info_vec.data(), GL_STATIC_DRAW);

        // glDrawArrays(GL_TRIANGLES, 0, 3);
        // // circle info buf
        // glBindBuffer(GL_ARRAY_BUFFER, circle_info_buffer);
        // glEnableVertexAttribArray(vcol_location);
        // glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE,
        //                       sizeof(GLfloat) * 6, (void*) 0);
        // glEnableVertexAttribArray(vcenter_location);
        // glVertexAttribPointer(vcenter_location, 2, GL_FLOAT, GL_FALSE,
        //                       sizeof(GLfloat) * 6, (void*) (3 * sizeof(GLfloat)));

        // glEnableVertexAttribArray(vradius_location);
        // glVertexAttribPointer(vradius_location, 1, GL_FLOAT, GL_FALSE,
        //                       sizeof(GLfloat) * 6, (void*) (5 * sizeof(GLfloat)));

        // glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * circle_info_vec.size(), circle_info_vec.data(), GL_STATIC_DRAW);
        // buffer swap
        glfwSwapBuffers(window);
        showFPS(window);
        audio.count_frames();
        glfwPollEvents();
    }
    return 0;
}
