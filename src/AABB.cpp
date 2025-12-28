#include "AABB.h"

AABB subdivideAABB(const AABB& aabb, const int divisions, const int index){
    fpt_vec3 dimensions = aabb.max - aabb.min;
    fpt_vec3 largest = largestAxis(dimensions);
    fpt step = 0;
    AABB newAABB = aabb;

    if (largest.x == 1) {
        step = fpt_div(dimensions.x , divisions);
        newAABB.min.x = fpt_add(newAABB.min.x, fpt_mul(step , index));
        newAABB.max.x = fpt_add(newAABB.min.x, step);
    } else if (largest.y == 1) {
        step = fpt_div(dimensions.y , divisions);
        newAABB.min.y = fpt_add(newAABB.min.y, fpt_mul(step , index));
        newAABB.max.y = fpt_add(newAABB.min.y, step);
    } else {
        step = fpt_div(dimensions.z , divisions);
        newAABB.min.z = fpt_add(newAABB.min.z, fpt_mul(step , index));
        newAABB.max.z = fpt_add(newAABB.min.z, step);
    }

    return newAABB;
}
//

fpt_vec3 largestAxis(fpt_vec3& dimensions){
    if (dimensions.x >= dimensions.y && dimensions.x >= dimensions.z) {
        return fpt_vec3_create(FPT_ONE, 0, 0);
    } else if (dimensions.y >= dimensions.x && dimensions.y >= dimensions.z) {
        return fpt_vec3_create(0, FPT_ONE, 0);
    } else {
        return fpt_vec3_create(0, 0, FPT_ONE);
    }
}
