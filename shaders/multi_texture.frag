#version 450



layout(push_constant) uniform PushConstants {
    vec2 textPosition; 
    vec2 textScale;    
    vec4 textColor;    
    vec4 texCoords;   
    float drawTexture;
    uint texture_index;
} push;


layout(binding = 1) uniform sampler texSampler;
layout(binding = 2) uniform texture2D textures[4];

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

// mat4 temp[3][0] accesses the x position of the matrix




void main(){

    outColor =      texture(sampler2D(textures[push.texture_index], texSampler), fragTexCoord);

}