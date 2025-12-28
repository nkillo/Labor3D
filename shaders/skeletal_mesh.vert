#version 450


layout(binding = 0) uniform UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 viewProj;
}ubo;


layout(push_constant) uniform PushData{
    mat4 model;
    vec4 color;
}push;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 texCoord;

// mat4 temp[3][0] accesses the x position of the matrix

void main(){

    gl_Position = ubo.viewProj * push.model * vec4(inPosition, 1.0);

    fragColor = vec4(push.color * inColor);
    texCoord = inTexCoord;
}