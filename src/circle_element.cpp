#include "circle_element.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>


void CircleElement::render_mesh(std::vector<GLfloat> &outlineBuf, std::vector<GLfloat> &insideBuf, std::vector<GLfloat> &borderBuf) {
    // Add triangles
    GLfloat radius = MESH_RADIUS;
    GLfloat circle_radius = DEFAULT_RADIUS_LOW;
    GLfloat border_outer_radius = DEFAULT_RADIUS_LOW + 0.01;
    GLfloat twicePi = 2.0f * M_PI;

    insideBuf.insert(insideBuf.end(), {0.0, 0.0});

    // // Draw using triangles
    for(int i = 0; i < NUM_TRIANGLES_IN_CIRCLE; i++) {
        outlineBuf.insert(outlineBuf.end(), {
            radius * cos(i *  twicePi / NUM_TRIANGLES_IN_CIRCLE), radius * sin(i * twicePi / NUM_TRIANGLES_IN_CIRCLE),
            radius * cos((i+1) *  twicePi / NUM_TRIANGLES_IN_CIRCLE), radius * sin((i+1) * twicePi / NUM_TRIANGLES_IN_CIRCLE),
            (radius+OUTER_EXTRA_RADIUS) * cos((i+1) *  twicePi / NUM_TRIANGLES_IN_CIRCLE), (radius+OUTER_EXTRA_RADIUS) * sin((i+1) * twicePi / NUM_TRIANGLES_IN_CIRCLE),
            radius * cos(i *  twicePi / NUM_TRIANGLES_IN_CIRCLE), radius * sin(i * twicePi / NUM_TRIANGLES_IN_CIRCLE),
            (radius+OUTER_EXTRA_RADIUS) * cos(i *  twicePi / NUM_TRIANGLES_IN_CIRCLE), (radius+OUTER_EXTRA_RADIUS) * sin(i * twicePi / NUM_TRIANGLES_IN_CIRCLE),
            (radius+OUTER_EXTRA_RADIUS) * cos((i+1) *  twicePi / NUM_TRIANGLES_IN_CIRCLE), (radius+OUTER_EXTRA_RADIUS) * sin((i+1) * twicePi / NUM_TRIANGLES_IN_CIRCLE),
        });
        borderBuf.insert(borderBuf.end(), {
            DEFAULT_RADIUS_LOW * cos(i *  twicePi / NUM_TRIANGLES_IN_CIRCLE), DEFAULT_RADIUS_LOW * sin(i * twicePi / NUM_TRIANGLES_IN_CIRCLE),
            DEFAULT_RADIUS_LOW * cos((i+1) *  twicePi / NUM_TRIANGLES_IN_CIRCLE), DEFAULT_RADIUS_LOW * sin((i+1) * twicePi / NUM_TRIANGLES_IN_CIRCLE),
            (border_outer_radius) * cos((i+1) *  twicePi / NUM_TRIANGLES_IN_CIRCLE), (border_outer_radius) * sin((i+1) * twicePi / NUM_TRIANGLES_IN_CIRCLE),
            DEFAULT_RADIUS_LOW * cos(i *  twicePi / NUM_TRIANGLES_IN_CIRCLE), DEFAULT_RADIUS_LOW * sin(i * twicePi / NUM_TRIANGLES_IN_CIRCLE),
            (border_outer_radius) * cos(i *  twicePi / NUM_TRIANGLES_IN_CIRCLE), (border_outer_radius) * sin(i * twicePi / NUM_TRIANGLES_IN_CIRCLE),
            (border_outer_radius) * cos((i+1) *  twicePi / NUM_TRIANGLES_IN_CIRCLE), (border_outer_radius) * sin((i+1) * twicePi / NUM_TRIANGLES_IN_CIRCLE),
        });
        insideBuf.insert(insideBuf.end(), {circle_radius * cos(i *  twicePi / NUM_TRIANGLES_IN_CIRCLE), circle_radius * sin(i * twicePi / NUM_TRIANGLES_IN_CIRCLE)});
        // for (auto i: vertexBuf)
        //     std::cout << i << ' ';
        // std::cout << std::endl;
        // for (auto i: circleBuf)
        //     std::cout << i << ' ';
    }
    insideBuf.insert(insideBuf.end(), {circle_radius, 0});

    // Draw the inside using fan

    // Draw using lines
    // for(int i = 0; i < NUM_TRIANGLES_IN_CIRCLE; i++) {
    //     vertexBuf.insert(vertexBuf.end(), {
    //         radius * cos(i *  twicePi / NUM_TRIANGLES_IN_CIRCLE), radius * sin(i * twicePi / NUM_TRIANGLES_IN_CIRCLE),
    //     });
    // }

}

GLCircleElementInfo CircleElement::glinfo() {
    return {color.r, color.g, color.b, pos.x, pos.y, _radius};
}

void CircleElement::tick(long time) {
    if (entry_time <= time && time < exit_time) {
        _radius = (DEFAULT_RADIUS_HIGH - DEFAULT_RADIUS_LOW) * ((float)exit_time - time)/(exit_time - entry_time) + DEFAULT_RADIUS_LOW;
    }
}
