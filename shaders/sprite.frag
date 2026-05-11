#version 450

layout(set = 1, binding = 0) uniform sampler2D u_texture;

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec4 v_tint;

layout(location = 0) out vec4 out_color;

void main() {
    out_color = texture(u_texture, v_uv) * v_tint;
}
