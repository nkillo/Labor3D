#version 450

layout(push_constant) uniform PushConstants {
    vec2 textPosition; 
    vec2 textScale;    
    vec4 textColor;    
    vec4 texCoords; 
    uvec4 misc; 
    vec4 misc2;
} push;

layout(binding = 1) uniform sampler texSampler;
layout(binding = 2) uniform texture2D textures[2];

// layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;


void main() {
    
    // float alpha = texture(texSampler, fragTexCoord).r;  // Font texture is grayscale
    // if(alpha == 0){
        // alpha = 1;
    // }
    // outColor = texture(texSampler, fragTexCoord);

    // outColor = vec4(push.textColor.rgb, push.textColor.a * alpha);
    
    //need to push the font index to use once we have more setup
    outColor =      vec4(texture(sampler2D(textures[push.misc.z], texSampler), fragTexCoord) * push.textColor);

    // outColor = vec4(texture(texSampler, fragTexCoord) * push.textColor);
    // outColor = vec4(1,1,1,1);

    // outColor = vec4(fragColor * texture(texSampler, fragTexCoord).rgb, 1.0);
}