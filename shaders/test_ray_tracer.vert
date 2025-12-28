#version 450

layout(binding = 0) uniform UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
}ubo;

layout(push_constant) uniform PushConstants {
    vec4 camPos;    
    vec4 viewRect;
} push;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;


void main(){
    gl_Position = vec4(inPosition.x * 2,inPosition.y * 2, inPosition.z, 1.0);
}