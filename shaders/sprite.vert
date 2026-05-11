#version 450

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_uv;

layout(set = 0, binding = 0) uniform Camera {
    mat4 view_proj;
} u_camera;

layout(push_constant) uniform Push {
    mat4 model;
    vec4 tint;
} pc;

layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec4 v_tint;

void main() {
    gl_Position = u_camera.view_proj * pc.model * vec4(in_position, 0.0, 1.0);
    v_uv   = in_uv;
    v_tint = pc.tint;
}
