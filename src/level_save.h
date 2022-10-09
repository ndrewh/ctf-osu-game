#include "circle_element.h"


struct __attribute__((__packed__)) Constraint {
    size_t circ_idx[4];
    GLuint code[8];
};

class LevelSave {
private:
public:
    std::vector<CircleElement> notes;
    std::vector<Constraint> constraints;
    unsigned long max_tick;
    LevelSave(std::string &filename);
};
