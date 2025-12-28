#version 450

//some types like dvec3 use 2 SLOTS
// layout(location = 0) in dvec3 inPosition;
// layout(location = 2) in vec3 inColor; //<- LIKE SO

//example for storage images
// layout (binding = 0, rgba8) uniform readonly image2D inputImage;
// layout (binding = 1, rgba8) uniform writeonly image2D outputImage;

//compute shader reading/writing example
// vec3 pixel = imageLoad(inputImage, ivec2(gl_GlobalInvocationID.xy)).rgb;
// imageStore(outputImage, ivec2(gl_GlobalInvocationID.xy), pixel);

layout(binding = 0) uniform UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
}ubo;

struct Particle{
    vec2 position;
    vec2 velocity;
    vec4 color;
};

layout(std140, binding = 2)readonly buffer ParticleSSBOIn {
    Particle particlesIn[];
};

layout(std140, binding = 3)buffer ParticleSSBOOut {
    Particle particlesOut[];
};


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;


// vec2 positions[3] = vec2[](
//     vec2( 0.0, -0.5),
//     vec2( 0.5,  0.5),
//     vec2(-0.5,  0.5)
// );


// vec3 colors[3] = vec3[](
    // vec3(1.0, 0.0, 0.0),
    // vec3(0.0, 1.0, 0.0),
    // vec3(0.0, 0.0, 1.0)
// );
// 

void main(){
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}