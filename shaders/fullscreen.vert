#version 450

layout(location = 0) out vec2 out_uv;

void main() {
    // Fullscreen triangle from the vertex index; uv covers [0,1] on screen.
    out_uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(out_uv * 2.0 - 1.0, 0.0, 1.0);
}
