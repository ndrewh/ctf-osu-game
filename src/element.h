
#pragma once
#include <stdint.h>
#include <vector>

#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Level;

struct Position {
    GLfloat x, y;
};

struct Color {
    GLfloat r, g, b, a;
};

#define EXIT_THRESHOLD 60
#define SCORE_THRESHOLD 30

template <typename T>
class Element {
public:
protected:
    Level *level;
    Position pos;
    Color color;
    unsigned long entry_time;
    unsigned long exit_time;
    GLint scored_at;
    size_t idx;

public:
    Element(size_t _idx, Position _p, Color _c, long _exit_time): idx(_idx), pos(_p), color(_c), exit_time(_exit_time), scored_at(-1) {
        // radius = DEFAULT_RADIUS;
    }
    virtual T glinfo() = 0;
    virtual void tick(long time) = 0;
    bool should_render(unsigned long tick) {
        return scored_at == -1 && tick >= entry_time && tick < exit_time;
    }
    bool just_died(unsigned long tick) {
        return scored_at == -1 && tick > exit_time && tick < (exit_time + EXIT_THRESHOLD);
    }
    bool just_scored(unsigned long tick) {
        return scored_at != -1 && tick > scored_at && tick < (scored_at + EXIT_THRESHOLD);
    }
    Position position() {
        return pos;
    }
    GLint score_time() {
        return scored_at;
    }
    long points(unsigned long tick) {
        if (scored_at != -1) return 1;
        if (tick >= exit_time) return -1;
        return 0;
    }
    void kill(unsigned long tick) {
        if (tick < exit_time) {
            if (exit_time - tick <= SCORE_THRESHOLD) {
                scored_at = (GLint)tick;
            } else {
                exit_time = tick;
            }
        }
    }
    size_t id() {
        return idx;
    }
private:

};
