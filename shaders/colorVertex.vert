#version 450

struct PerEntity{
    mat4 model;
    vec4 color;
};

layout(binding = 0) uniform UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 viewProj;
}ubo;

layout(std140, binding = 1) readonly buffer PerEntitySSBO{
    PerEntity entity[ ];
};

layout(push_constant) uniform PushData{
    uint entityIndex;
}push;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

// mat4 temp[3][0] accesses the x position of the matrix

void main(){

    gl_Position = ubo.viewProj * entity[push.entityIndex].model * vec4(inPosition, 1.0);

    fragColor = vec4(entity[push.entityIndex].color * inColor);
   
}