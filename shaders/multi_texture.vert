#version 450


layout(binding = 0) uniform UniformBufferObject{
    vec2 viewRect;
}ubo;


layout(push_constant) uniform PushConstants {
    vec2 textPosition; 
    vec2 textScale;    
    vec4 textColor;    
    vec4 texCoords;   
    float drawTexture;
    uint texture_index;
} push;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

// mat4 temp[3][0] accesses the x position of the matrix

vec2 positions[6] = vec2[](
    vec2(-0.5f, -0.5f ),
    vec2( 0.5f,  0.5f ),
    vec2( 0.5f, -0.5f ),
    vec2(-0.5f, -0.5f ),
    vec2(-0.5f,  0.5f ),
    vec2( 0.5f, -0.5f )
);


void main(){
  // Scale the vertices by our desired size
    vec2 position = inPosition.xy * push.textScale.xy;

    // Adjust position based on screen coordinates
    // Note: we don't need to flip Y here since we do it later
    vec2 adjustedPosition = push.textPosition.xy;
    
    // Move to final position
    position += adjustedPosition;

    // // Convert to normalized device coordinates (-1 to 1)
    vec2 screenPos = position / ubo.viewRect.xy;  // Divide by screen dimensions
    screenPos = screenPos * 2.0 - 1.0;         // Convert 0-1 to -1 to 1
    
    gl_Position = vec4(screenPos, 0.0, 1.0);

    //scale and offset method
    // fragTexCoord.x = (inTexCoord.x * push.texCoords.z) + push.texCoords.x;
    // fragTexCoord.y = (inTexCoord.y * push.texCoords.w) + push.texCoords.y;

    //simple linear interpolation
    fragTexCoord = mix(push.texCoords.xy, push.texCoords.zw, inTexCoord);

}