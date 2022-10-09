R"(
#version 150
in vec3 vCol1;
in vec2 vPos;
in vec2 vCenter;
in float vRadius;
uniform uint mode;
out vec4 color;
void main()
{
    // This logic makes the outline scale without getting much thinner
    float fac = vRadius;
    float dist_desired = length(vPos) - 1.0;
    float alpha = vRadius < 0.1 ? 1.0 : (1.0 - 4.0 * (vRadius - 0.1));
    float fadein = vRadius < 0.2 ? 1.0 : (1.0 - 10.0 * (vRadius - 0.2));

    // The color of the fill and outline can be different
    if (mode == 1u) {
        gl_Position = vec4(vPos * fac + ((1.0-fac) * dist_desired * vPos) + vCenter, 0.0, 1.0);
        color = vec4(vCol1, clamp(min(alpha, fadein), 0.0, 1.0));
    } else if (mode == 2u) {
        gl_Position = vec4(vPos + vCenter, 0.0, 1.0);
        color = vec4(vCol1 * 1.2, clamp(fadein, 0.0, 0.8));
    } else if (mode == 3u) {
        gl_Position = vec4(vPos + vCenter, 0.0, 1.0);
        color = vec4(1.0, 1.0, 1.0, clamp(fadein, 0.0, 0.8));
    } else if (mode == 4u) {
        gl_Position = vec4(vPos * fac + vCenter, 0.0, 1.0);
        color = vec4(vCol1, 1.0);
    }
}
)"
