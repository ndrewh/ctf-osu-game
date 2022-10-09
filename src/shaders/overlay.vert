R"(
#version 150
in vec2 vPos;
in vec2 vTexCoord;
uniform uint mode;
out vec4 color;
out vec2 texCoord;
void main()
{
    gl_Position = vec4(vPos, 0.0, 1.0);
    texCoord = vTexCoord;

    if (mode == 2u) {
        color = vec4(0.21875, 0.4375, 0.203125, 1.0);
    } else if (mode == 3u) {
        color = vec4(1.0, 0.0, 0.0, 1.0);
    } else {
        color = vec4(1.0, 1.0, 1.0, 1.0);
    }
}
)"
