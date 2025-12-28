#version 450


layout(binding = 0) uniform UniformBufferObject{
    vec4 viewRect;
    vec4 misc;
    vec4 mouse;
}ubo;

layout(push_constant) uniform PushConstants {
    vec2 position; 
    vec2 scale;    
    vec4 color;    
    vec4 texCoords;   
    uvec4 misc; //x = drawTexture, y = type of shape to draw
    vec4 misc2;

} push;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

// mat4 temp[3][0] accesses the x position of the matrix

vec2 positions[6] = vec2[](
    vec2(-0.5f,  0.5f ),
    vec2( 0.5f,  0.5f ),
    vec2( 0.5f, -0.5f ),
    vec2(-0.5f, -0.5f ),
    vec2(-0.5f,  0.5f ),
    vec2( 0.5f, -0.5f )
);


void main(){
  // Scale the vertices by our desired size
    vec2 position = inPosition.xy * push.scale.xy;

 
    vec2 adjustedPosition = push.position.xy;
    
    position += adjustedPosition;

    vec2 screenPos = position / ubo.viewRect.xy;  // Divide by screen dimensions
    screenPos = screenPos * 2.0 - 1.0;         // Convert 0-1 to -1 to 1

    gl_Position = vec4(screenPos, 0.0, 1.0);
    // gl_Position = vec4(screenPos, push.misc2.z, 1.0);
    // gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);



    //simple linear interpolation
    fragTexCoord = mix(push.texCoords.xy, push.texCoords.zw, inTexCoord);

}