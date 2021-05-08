#version 450

// attributes
layout(location = 0) in vec3 vertices;

// uniforms
layout(set=0, binding = 0) uniform UniformCamera {
    mat4 proj;
    mat4 view;
} u_camera;

layout(set=1, binding = 1) uniform UniformModel {
    mat4 model;
} u_model;

void main() {
    gl_Position = u_camera.proj * u_camera.view * u_model.model * vec4(vertices, 1.0);
}