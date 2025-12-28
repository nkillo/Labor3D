#pragma once
#include "entity.h"

inline bool fpt_mul_causes_over_under_flow(fpt A, fpt B){
    
    if ((A) < FPT_ZERO && B < FPT_ZERO) {
        if ((fptd)(A) < ((fptd)FPT_MAX << FPT_FBITS) / (fptd)B){
            return true;
        } 
    } else if (B > FPT_ZERO) {
        if ((fptd)(A) > ((fptd)FPT_MAX << FPT_FBITS) / (fptd)B){
            return true;
        }
        
        if ((fptd)(A) < ((fptd)FPT_MIN << FPT_FBITS) / (fptd)B){
            return true;
        }
        
    } else if (B < FPT_ZERO) {
        if ((fptd)(A) > ((fptd)FPT_MIN << FPT_FBITS) / (fptd)B){
            return true;
        }
    }
    return false;
}

inline bool HitBoundingBox(const fpt_vec3& minB, const fpt_vec3& maxB, 
                    const fpt_vec3& origin, const fpt_vec3& dir, 
                    fpt_vec3& intersectionPoint,
                    const fpt_vec3& worldRayOrigin = fpt_vec3_create(0.0f, 0.0f, 0.0f),
                    const fpt_vec3& worldRayDirection = fpt_vec3_create(0.0f, 0.0f, 0.0f),
                    bool obbTest = false) {
    //if(obbTest)//printf("dir: %f, %f, %f\n", dir.x, dir.y, dir.z);

    // Quick check for nearly-parallel rays
    if (fpt_abs(dir.x) < 80 || //66 is something like .001 << 16 
        fpt_abs(dir.y) < 80 || 
        fpt_abs(dir.z) < 80) {
        return false;
    }
    fpt_vec3 invDir = FPT_ONE /  dir;

    // Early exit if inverse direction would be too large
    if (fpt_abs(fpt_div(FPT_ONE , invDir.x)) > 50075262 || //1000 << 16
        fpt_abs(fpt_div(FPT_ONE , invDir.y)) > 50075262 ||
        fpt_abs(fpt_div(FPT_ONE , invDir.z)) > 50075262) {
        return false;
    }



    //TODO:
    //LIMITS THE RANGE OF THE RAY CAST, this is just an overflow check, if we want a farther ray cast we will need to develop a workaround
    if(fpt_mul_causes_over_under_flow(minB.x - origin.x, invDir.x))return false;
    if(fpt_mul_causes_over_under_flow(minB.y - origin.y, invDir.y))return false;
    if(fpt_mul_causes_over_under_flow(minB.z - origin.z, invDir.z))return false;

    if(fpt_mul_causes_over_under_flow(maxB.x - origin.x, invDir.x))return false;
    if(fpt_mul_causes_over_under_flow(maxB.y - origin.y, invDir.y))return false;
    if(fpt_mul_causes_over_under_flow(maxB.z - origin.z, invDir.z))return false;




    fpt_vec3 tMin = (minB - origin) * invDir;
    fpt_vec3 tMax = (maxB - origin) * invDir;


    fpt t0 = fpt_max(fpt_max(fpt_min(tMin.x, tMax.x), fpt_min(tMin.y, tMax.y)), fpt_min(tMin.z, tMax.z));
    fpt t1 = fpt_min(fpt_min(fpt_max(tMin.x, tMax.x), fpt_max(tMin.y, tMax.y)), fpt_max(tMin.z, tMax.z));



    if (t1 < 0 || t0 > t1) {
        ///*TAG*/spdlog::info("fpt no intersection");
        return false; // No intersection
    }

    if (t0 > 0) {  
        intersectionPoint = origin + (dir * t0); // Calculate intersection point
    } else {  
        // If t0 is behind, check if t1 is in front  
        if (t1 > 0) {  
            intersectionPoint = origin + (dir * t1); // Calculate intersection point
        } else {  
            // Both t0 and t1 are behind, no valid intersection  
        }
    }
    //if(obbTest)//printf("does intersect\n");
    //if(obbTest)//printf("t0: %f, t1: %f\n", t0, t1);

    vec3 fptIntersectPoint = fpt_to_flt_vec3(intersectionPoint);
  

    return true;
}

//DOESN'T HANDLE SCALED MATRICES
inline bool RayIntersectsOBB(const fpt_vec3& rayOrigin, const fpt_vec3& rayDirection, const ObbComp& obb, fpt_vec3 pos, fpt_vec3& intersectionPoint) {
    // Create transformation matrix for the OBB

    //in case the localCenter is not the position, we need to rotate around the localCenter
    //applying the actual obb scale would throw off the intersection test, as the case of the spike,
    //a z scale of 12 would intersect far outside of the actual z bounds

    fpt_mat4 fptmodelMatrix = (fpt_translate(pos) * 
                            fpt_quat_to_mat4(obb.rotation)) *
                            fpt_translate(-obb.localCenter);
  
    fpt_mat4 inverseModelMatrix = fpt_inverse_transform(fptmodelMatrix);
    



    fpt_vec4 localRayOrigin =    fpt_mul_mat4_vec4(inverseModelMatrix , fpt_vec4_create(rayOrigin, FPT_ONE));
    fpt_vec4 localRayDirection = fpt_mul_mat4_vec4(inverseModelMatrix , fpt_vec4_create(rayDirection, 0));

    // Perform AABB intersection test in local space
    fpt_vec3 localMin = obb.localCenter - obb.extents;
    fpt_vec3 localMax = obb.localCenter + obb.extents;

    

    if (HitBoundingBox(localMin, localMax, fpt_vec3_create(localRayOrigin.x,localRayOrigin.y,localRayOrigin.z), fpt_vec3_normalize(fpt_vec3_create(localRayDirection.x,localRayDirection.y,localRayDirection.z)), intersectionPoint, rayOrigin, rayDirection,  true)) {
 
        return true;
    }

    return false;
}
