#version 450


layout(binding = 0) uniform UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
}ubo;



layout(push_constant) uniform PushData{
    mat4 model;
    vec4 color;
    float scale;
}push;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

// mat4 temp[3][0] accesses the x position of the matrix

void main(){
    vec3 pos = inPosition;
    pos.y *= -1;
    gl_Position = ubo.proj * ubo.view * push.model * vec4(pos * push.scale, 1.0);

    fragColor = vec4(push.color);
    fragTexCoord = inTexCoord;
}