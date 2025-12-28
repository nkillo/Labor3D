#version 450


layout(binding = 0) uniform UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
}ubo;

layout(push_constant) uniform PushConstants {
    vec4 camPos;    
    vec4 viewRect;
} push;

layout(location = 0) out vec4 outColor;


bool rayIntersectsAABB(vec3 rayOrigin, vec3 rayDirection, vec3 boxMin, vec3 boxMax)
{
    // Handle ray directions with components that are zero or near-zero
    vec3 invDir;
    invDir.x = abs(rayDirection.x) < 0.00001 ? sign(rayDirection.x) * 100000.0 : 1.0 / rayDirection.x;
    invDir.y = abs(rayDirection.y) < 0.00001 ? sign(rayDirection.y) * 100000.0 : 1.0 / rayDirection.y;
    invDir.z = abs(rayDirection.z) < 0.00001 ? sign(rayDirection.z) * 100000.0 : 1.0 / rayDirection.z;
    
    vec3 t0 = (boxMin - rayOrigin) * invDir;
    vec3 t1 = (boxMax - rayOrigin) * invDir;
    
    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);
    
    float tNear = max(max(tmin.x, tmin.y), tmin.z);
    float tFar = min(min(tmax.x, tmax.y), tmax.z);
    
    return (tNear <= tFar) && (tFar > 0.0);
}

void main() {
    // Camera position is already in world space
    vec3 cameraPos = push.camPos.xyz;
    
    // Ray origin is the camera position in world space
    vec3 rayOrigin = vec3(0, 0, 0);
    
    // Calculate normalized device coordinates (NDC)
    // In Vulkan, NDC Y ranges from -1 (bottom) to 1 (top)
    vec2 ndc = vec2(
        (gl_FragCoord.x / push.viewRect.x) * 2.0 - 1.0,
        (gl_FragCoord.y / push.viewRect.y) * 2.0 - 1.0
    );
    
    // Create clip space position
    vec4 clipSpacePos = vec4(ndc, 1.0, 1.0);
    
    // Transform from clip space to world space
    // We need to invert the projection and view matrices to go from clip to world
    mat4 invViewProj = inverse(ubo.proj * ubo.view);
    vec4 worldSpacePos = invViewProj * clipSpacePos;
    
    // Perform perspective division
    worldSpacePos /= worldSpacePos.w;
    
    // Calculate ray direction in world space
    vec3 rayDirWorldSpace = normalize(worldSpacePos.xyz);
    
    // Box position in world space (at origin in this case)
    vec3 boxCenterWorld = -cameraPos;

    // Box half-size (10x10x10 cube)
    vec3 boxSizeHalf = vec3(5.0, 5.0, 5.0);
    vec3 boxMinWorld = boxCenterWorld - boxSizeHalf;
    vec3 boxMaxWorld = boxCenterWorld + boxSizeHalf;
    
    // Perform ray-box intersection
    bool intersects = rayIntersectsAABB(rayOrigin, rayDirWorldSpace, boxMinWorld, boxMaxWorld);
    
    // Color based on intersection
    vec4 color = intersects ? vec4(0.0, 1.0, 0.0, 0.125) : vec4(1.0, 0.0, 0.0, 0.125);
    
    // Debug visualization: show ray direction
    // color = vec4(normalize(rayDirWorldSpace) * 0.5 + 0.5, 1.0);
    
    outColor = color;
    
    // Uncomment for debugging NDC coordinates
    // outColor = vec4(0.0, ndc.y , 0.0, 1.0);
    // outColor = vec4(rayDirWorldSpace, 1.0);

}