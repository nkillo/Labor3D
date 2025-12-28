#version 450

layout(binding = 0) uniform UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    mat4 invViewProj;
}ubo;


layout(push_constant) uniform PushConstants {
    mat4 model;    
    vec4 misc;
    vec4 mouse; //x y are pos, z w are delta
    vec2 viewRect;
} push;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in uint inFaceID;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) flat out uint fragFaceID;



void main(){
    vec4 pos = ubo.proj * ubo.view * push.model * vec4(inPosition, 1.0);
    
    // Set z = w to ensure maximum depth (appears at far plane)
    gl_Position = vec4(pos.x, pos.y, pos.w, pos.w);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    fragFaceID = inFaceID;
}