#include "frustum.h"


        
    // Function to extract the AABB from the view-projection matrix
void extractFrustumAABB(mat4& invProjMatrix, mat4& invViewMatrix, 
                       /* const vec3& worldCameraPos, */ vec3& min, vec3& max) {

    // Corners of the NDC cube
    vec4 ndcCorners[8] = {
        { -1.0f, -1.0f,  0.0f, 1.0f },
        {  1.0f, -1.0f,  0.0f, 1.0f },
        {  1.0f,  1.0f,  0.0f, 1.0f },
        { -1.0f,  1.0f,  0.0f, 1.0f },
        { -1.0f, -1.0f,  1.0f, 1.0f },
        {  1.0f, -1.0f,  1.0f, 1.0f },
        {  1.0f,  1.0f,  1.0f, 1.0f },
        { -1.0f,  1.0f,  1.0f, 1.0f }
    };

    min = vec3_create( FLT_MAX);
    max = vec3_create(-FLT_MAX);

    // printf("HANDLE FRUSTUM EXTRACTION HERE!\n");
    for (int i = 0; i < 8; i++) {
        // NDC to view space
        vec4 viewCorner = mat4_vec4_product(invProjMatrix, ndcCorners[i]);
        if (viewCorner.w > 1e-6f) {  // Avoid division by zero
            viewCorner.x /= viewCorner.w;
            viewCorner.y /= viewCorner.w;
            viewCorner.z /= viewCorner.w;
            viewCorner.w = 1.0f;
        }
        // Rotate from view space to world space orientation
        vec4 worldOrientedCorner = mat4_vec4_product(invViewMatrix, viewCorner);
        
        // Add world camera position to get final world position
        //leave out the camera to get the local position
        vec3 worldCorner = vec3_create(worldOrientedCorner);// + worldCameraPos;


        min = vec3_min(min, worldCorner);
        max = vec3_max(max, worldCorner);
    }
   
}



