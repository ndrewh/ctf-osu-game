
#pragma once
#include <stdint.h>
#include <vector>
#include "element.h"
#include "overlay_utils.h"

#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define NUM_TRIANGLES_IN_CIRCLE 512

// OUTER_EXTRA_RADIUS will be scaled
#define OUTER_EXTRA_RADIUS 0.01f 

#define MESH_RADIUS 1.0f
#define VERTICES_PER_CIRCLE (NUM_TRIANGLES_IN_CIRCLE * 3)

#define DEFAULT_DURATION 120UL

#define DEFAULT_RADIUS_LOW 0.08f
#define DEFAULT_RADIUS_HIGH 0.25f
class Level;

struct GLCircleElementInfo {
    GLfloat r, g, b, x, y, radius;
};

class CircleElement : public Element<GLCircleElementInfo> {
private:
    GLfloat _radius;
    int _num;

public:
    CircleElement(size_t _idx, Position _p, Color _c, long _exit_time, char num): Element(_idx, _p, _c, _exit_time) {
        entry_time = _exit_time - DEFAULT_DURATION;
        _radius = DEFAULT_RADIUS_HIGH;
        _num = num;
        // radius = DEFAULT_RADIUS;
    }
    static void render_mesh(std::vector<GLfloat> &outlineBuf, std::vector<GLfloat> &insideBuf, std::vector<GLfloat> &borderBuf);
    GLCircleElementInfo glinfo();
    void tick(long time);
    GLfloat radius() {
        return _radius;
    }
    unsigned int tilemap_idx() {
        return CHAR_TO_IDX(_num);
    }
private:
};
