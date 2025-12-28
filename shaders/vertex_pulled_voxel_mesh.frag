#version 450

// $input v_position, v_color0, v_ao, v_normal0, v_texcoord0
// #include <bgfx_shader.sh>

// SAMPLER2D(s_texColor, 1);
// SAMPLER2D(s_texColor1, 2);
// SAMPLER2D(s_texColor2, 3);

// uniform vec4 u_renderSettings;


layout(push_constant) uniform PushConstants {
    vec4 vertexPullerSettings;    
    mat4 model;
    uint ssboIndex;
} push;

layout(binding = 2) uniform sampler2D texSampler;


layout(location = 0) in vec4 fragColor;
layout(location = 1) in float fragAO;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;


void main()
{

    // vec2 tiledTexCoord = fract(v_texcoord0);
    
    // vec4 texColor = texture2D(s_texColor, tiledTexCoord);

    // vec3 absNormal = abs(v_normal0);
    
    // if (absNormal.y > 0.5) {  // Top/Bottom faces (Y axis)
    //     texColor = texture2D(s_texColor, tiledTexCoord);  // e.g., grass/dirt
    // }
    // else if (absNormal.x > 0.5) {  // Left/Right faces (X axis)
    //     texColor = texture2D(s_texColor1, tiledTexCoord);  // e.g., side texture
    // }
    // else {  // Front/Back faces (Z axis)
    //     texColor = texture2D(s_texColor2, tiledTexCoord);  // e.g., another side texture
    // }


    // gl_FragColor = texColor;  // Optional: blend with vertex color
// 

    // float4 color = float4(1.0f, 1.0f, 1.0f, 1.0f);
    vec4 color = fragColor * fragAO;
    outColor = vec4(color.x, color.y, color.z, 1.0);


    // gl_FragColor = vec4(v_normal0, 1.0f) * v_ao;    //normal and ao
    // gl_FragColor = vec4(v_normal0, 1.0f);           //normal
    // gl_FragColor = v_color0;                        //white
    // gl_FragColor = v_color0 * v_ao;                  //white and ao

}