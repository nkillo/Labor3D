#pragma once
#include "fptvec.h"

struct AABB{
    fpt_vec3 min;
    fpt_vec3 max;
};

struct FPT_AABB{
    fpt_vec3 min;
    fpt_vec3 max;
};

AABB subdivideAABB(const AABB& aabb, const int divisions, const int index);
fpt_vec3 largestAxis(fpt_vec3& dimensions);
