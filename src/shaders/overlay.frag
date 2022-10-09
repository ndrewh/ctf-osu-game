R"(
#version 150
out vec4 fragColor;
in vec4 color;
in vec2 texCoord;
uniform sampler2D tex;
void main()
{
    fragColor = vec4(color.xyz, texture(tex, texCoord).w);
}
)"
