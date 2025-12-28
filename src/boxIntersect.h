#pragma once


struct Triangle{
    vec3 v0;
    vec3 v1;
    vec3 v2;
};


#define COLLISION_DEBUGS 0

#if COLLISION_DEBUGS
    #define COLLISION_DEBUG_PRINTF(fmt, ...)            printf(fmt, ##__VA_ARGS__)
    #define COLLISION_DEBUG_PRINT_FPT_VEC3(string, v)   print_fpt_vec3(string, v)
#else
    #define COLLISION_DEBUG_PRINTF(fmt, ...)            ((void)0)
    #define COLLISION_DEBUG_PRINT_FPT_VEC3(string, v)   ((void)0)
#endif

// Check if 2 aabbs intersect
inline bool aabbIntersectTest(const vec3& min, const vec3& max, const vec3& otherMin,const vec3& otherMax) {

    // Perform the intersection test between the two AABBs
    bool separated = 
        otherMax.x < min.x || otherMin.x > max.x ||
        otherMax.y < min.y || otherMin.y > max.y ||
        otherMax.z < min.z || otherMin.z > max.z;

    return !separated;
}

inline bool aabbIntersectTest(const fpt_vec3& min, const fpt_vec3& max, const fpt_vec3& otherMin, const fpt_vec3& otherMax) {

    // Perform the intersection test between the two AABBs
    bool separated = 
        otherMax.x < min.x || otherMin.x > max.x ||
        otherMax.y < min.y || otherMin.y > max.y ||
        otherMax.z < min.z || otherMin.z > max.z;

    return !separated;
}

inline bool aabbIntersectTest_penetration(
    const fpt_vec3& min, const fpt_vec3& max, 
    const fpt_vec3& otherMin, const fpt_vec3& otherMax, 
    fpt_vec3& penetrationVector) 
{
    if (otherMax.x < min.x || otherMin.x > max.x ||
        otherMax.y < min.y || otherMin.y > max.y ||
        otherMax.z < min.z || otherMin.z > max.z) 
    {
        return false; // No intersection
    }

    // Compute penetration depth on each axis
    fpt penetrationX = fpt_min(max.x - otherMin.x, otherMax.x - min.x);
    fpt penetrationY = fpt_min(max.y - otherMin.y, otherMax.y - min.y);
    fpt penetrationZ = fpt_min(max.z - otherMin.z, otherMax.z - min.z);

    // Determine axis of least penetration
    if (penetrationX < penetrationY && penetrationX < penetrationZ) {
        penetrationVector = { fpt_mul(penetrationX , (min.x < otherMin.x ? -FPT_ONE : FPT_ONE)), 0, 0 };
    } 
    else if (penetrationY < penetrationZ) {
        penetrationVector = { 0, fpt_mul(penetrationY , (min.y < otherMin.y ? -FPT_ONE : FPT_ONE)), 0 };
    } 
    else {
        penetrationVector = { 0, 0, fpt_mul(penetrationZ , (min.z < otherMin.z ? -FPT_ONE : FPT_ONE)) };
    }

    return true;
}

inline fpt swept_aabb(const fpt_vec3& min1, const fpt_vec3& max1, const fpt_vec3& min2, const fpt_vec3& max2, const fpt_vec3& mov1, fpt_vec3& normal){
    //col_dist is between 0 and 1, indicating where the collision occurs in the movement of box 1
    //mov1 is the movement of box 1, normal is the resolution normal
    // print_fpt_vec3("entity min START: ", min1);
    // print_fpt_vec3("entity max START: ", max1);
    // print_fpt_vec3("entity min MOVED: ", min1 + mov1);
    // print_fpt_vec3("entity max MOVED: ", max1 + mov1);
    // print_fpt_vec3("movement  : ", mov1);

    // print_fpt_vec3("box min   : ", min2);
    // print_fpt_vec3("box max   : ", max2);
    // if(mov1.y < 0 && mov1.y > fl2fpt(-0.004)){
    //     printf("DEBUG CASE!\n");
    // }
    // if(fpt_abs(mov1.x) < FPT_THOUSANDTH && mov1.x != 0)print_fpt_vec3("MOV1 X IS LESS THAN FPT_THOUSANDTH! (65 or 0.001):", mov1);
    // if(fpt_abs(mov1.y) < FPT_THOUSANDTH && mov1.y != 0)print_fpt_vec3("MOV1 Y IS LESS THAN FPT_THOUSANDTH! (65 or 0.001):", mov1);
    // if(fpt_abs(mov1.z) < FPT_THOUSANDTH && mov1.z != 0){print_fpt_vec3("MOV1 Z IS LESS THAN FPT_THOUSANDTH! (65 or 0.001):", mov1);}
    fpt xInvEntry, yInvEntry, zInvEntry; //specifies how far away the closest edges of the objects are from eachother
    fpt xInvExit,  yInvExit,  zInvExit;    //distance to the far side of the object
    //these values are the inverse time until it hits the other object on the axis
    if(mov1.x > 0){
        xInvEntry = fpt_sub(min2.x, max1.x);
        xInvExit  = fpt_sub(max2.x, min1.x);
    }else{
        xInvEntry = fpt_sub(max2.x, min1.x);
        xInvExit  = fpt_sub(min2.x, max1.x);
    }
    if(mov1.y > 0){
        yInvEntry = fpt_sub(min2.y, max1.y);
        yInvExit  = fpt_sub(max2.y, min1.y);
    }else{
        yInvEntry = fpt_sub(max2.y, min1.y);
        yInvExit  = fpt_sub(min2.y, max1.y);
    }
    if(mov1.z > 0){
        zInvEntry = fpt_sub(min2.z, max1.z);
        zInvExit  = fpt_sub(max2.z, min1.z);
    }else{
        zInvEntry = fpt_sub(max2.z, min1.z);
        zInvExit  = fpt_sub(min2.z, max1.z);
    }

    // printf("INV ENTRY TIMES  %10.5f %10.5f %10.5f\n", fpt2fl(xInvEntry),fpt2fl(yInvEntry),fpt2fl(zInvEntry));

    fpt xEntry, yEntry, zEntry;
    fpt xExit,  yExit,  zExit;
    // if(fpt_abs(mov1.x) < FPT_THOUSANDTH){//prevent tiny movements/divide by 0/large numbers
    if(mov1.x == 0){
        xEntry = -65536000;//1000 in fixed point
        xExit  =  65536000;
    }else{
        xEntry = fpt_div(xInvEntry, mov1.x);
        xExit  = fpt_div(xInvExit , mov1.x);
    }
    // if(fpt_abs(mov1.y) < FPT_THOUSANDTH){
    if(mov1.y == 0){
        yEntry = -65536000;
        yExit  =  65536000;
    }else{
        yEntry = fpt_div(yInvEntry, mov1.y);
        yExit  = fpt_div(yInvExit , mov1.y);
    }
    // if(fpt_abs(mov1.z) < FPT_THOUSANDTH){
    if(mov1.z == 0){
        zEntry = -65536000;
        zExit  =  65536000;
    }else{
        zEntry = fpt_div(zInvEntry, mov1.z);
        zExit  = fpt_div(zInvExit , mov1.z);
    }

    COLLISION_DEBUG_PRINTF("ENTRY TIMES  %10.5f %10.5f %10.5f\n", fpt2fl(xEntry),fpt2fl(yEntry),fpt2fl(zEntry));
    fpt entryTime = fpt_max(xEntry, fpt_max(yEntry, zEntry));
    fpt exitTime  = fpt_min(xExit , fpt_min(yExit , zExit));

    //if there was no collision, if all the times are less than 0, or if any times are greater than 1, there was no collision
    if((entryTime > exitTime) || (xEntry < 0 && yEntry < 0 && zEntry < 0) || (xEntry > FPT_ONE || yEntry > FPT_ONE || zEntry > FPT_ONE)){
        normal.x = 0;
        normal.y = 0;
        normal.z = 0;
        return FPT_ONE;
    }else{//there was a collision
        fpt_vec3 norm = fpt_vec3_create(0);

        if(xEntry > yEntry && xEntry > zEntry){
            if(xInvEntry < 0){
                norm.x = FPT_ONE;      // Positive normal
            } else if(xInvEntry > 0) {
                norm.x = -FPT_ONE;     // Negative normal
            } else {
                // Exactly touching case - use movement direction
                norm.x = (mov1.x > 0) ? -FPT_ONE : FPT_ONE;
            }
        }else if(yEntry > xEntry && yEntry > zEntry){
            if(yInvEntry < 0){
                norm.y = FPT_ONE;      // Positive normal
            } else if(yInvEntry > 0) {
                norm.y = -FPT_ONE;     // Negative normal
            } else {
                // Exactly touching case - use movement direction
                norm.y = (mov1.y > 0) ? -FPT_ONE : FPT_ONE;
            }
        }else{//z axis collision occurred first
            if(zInvEntry < 0){
                norm.z = FPT_ONE;      // Positive normal
            } else if(zInvEntry > 0) {
                norm.z = -FPT_ONE;     // Negative normal
            } else {
                // Exactly touching case - use movement direction
                norm.z = (mov1.z > 0) ? -FPT_ONE : FPT_ONE;
            }
        }
        // printf("SWEPT AABB RETURNING NORMAL %f %f %f, ENTRYTIME: %10.5f\n", fpt2fl(norm.x),fpt2fl(norm.y),fpt2fl(norm.z),fpt2fl(entryTime));
        normal = norm;
        return entryTime;
    }


    return FPT_ONE;//default, invalid code path, no collision assumed
}

inline bool aabbOutsideOrIntersectWallTest(const vec3& aabbMin, const vec3& aabbMax, 
                                                  const vec3& obbAABBMin, const vec3& obbAABBMax) {

    // Check if the OBB's AABB is outside the larger AABB or intersecting its walls
    bool outsideOrIntersect = 
        obbAABBMax.x > aabbMax.x || obbAABBMin.x < aabbMin.x || // Intersects or is outside the X walls
        obbAABBMax.y > aabbMax.y || obbAABBMin.y < aabbMin.y || // Intersects or is outside the Y walls
        obbAABBMax.z > aabbMax.z || obbAABBMin.z < aabbMin.z;   // Intersects or is outside the Z walls

    return outsideOrIntersect;
}

inline bool aabbOutsideOrIntersectWallTest(fpt_vec3& aabbMin,    fpt_vec3& aabbMax, 
                                                  fpt_vec3& obbAABBMin, fpt_vec3& obbAABBMax) {

    // Check if the OBB's AABB is outside the larger AABB or intersecting its walls
    bool outsideOrIntersect = 
        obbAABBMax.x > aabbMax.x || obbAABBMin.x < aabbMin.x || // Intersects or is outside the X walls
        obbAABBMax.y > aabbMax.y || obbAABBMin.y < aabbMin.y || // Intersects or is outside the Y walls
        obbAABBMax.z > aabbMax.z || obbAABBMin.z < aabbMin.z;   // Intersects or is outside the Z walls

    return outsideOrIntersect;
}


// Returns the squared distance between a point p and an AABB b
inline float SqDistPointAABB(const vec3& p, const vec3& aabbMin, const vec3& aabbMax )
{
    float sqDist = 0.0f;
    if( p.x < aabbMin.x ) sqDist += (aabbMin.x - p.x) * (aabbMin.x - p.x);
    if( p.x > aabbMax.x ) sqDist += (p.x - aabbMax.x) * (p.x - aabbMax.x);
    
    if( p.y < aabbMin.y ) sqDist += (aabbMin.y - p.y) * (aabbMin.y - p.y);
    if( p.y > aabbMax.y ) sqDist += (p.y - aabbMax.y) * (p.y - aabbMax.y);

    if( p.z < aabbMin.z ) sqDist += (aabbMin.z - p.z) * (aabbMin.z - p.z);
    if( p.z > aabbMax.z ) sqDist += (p.z - aabbMax.z) * (p.z - aabbMax.z);
    return sqDist;
}

// Returns the squared distance between a point p and an AABB b
inline fpt SqDistPointAABB(fpt_vec3& p, fpt_vec3& aabbMin, fpt_vec3& aabbMax )
{
    fpt sqDist = 0;

    if( p.x < aabbMin.x ) sqDist = fpt_add(sqDist,fpt_mul(fpt_sub(aabbMin.x, p.x) , fpt_sub(aabbMin.x , p.x)));
    if( p.x > aabbMax.x ) sqDist = fpt_add(sqDist,fpt_mul(fpt_sub(p.x ,aabbMax.x) , fpt_sub(p.x , aabbMax.x)));

    if( p.y < aabbMin.y ) sqDist = fpt_add(sqDist,fpt_mul(fpt_sub(aabbMin.y, p.y) , fpt_sub(aabbMin.y , p.y)));
    if( p.y > aabbMax.y ) sqDist = fpt_add(sqDist,fpt_mul(fpt_sub(p.y ,aabbMax.y) , fpt_sub(p.y , aabbMax.y)));

    if( p.z < aabbMin.z ) sqDist = fpt_add(sqDist,fpt_mul(fpt_sub(aabbMin.z, p.z) , fpt_sub(aabbMin.z , p.z)));
    if( p.z > aabbMax.z ) sqDist = fpt_add(sqDist,fpt_mul(fpt_sub(p.z ,aabbMax.z) , fpt_sub(p.z , aabbMax.z)));

    return sqDist;
}

inline bool sphereIntersectsAABB(const vec3& sphereCenter, const float sphereRadius, const vec3& aabbMin, const vec3& aabbMax){
    // Compute squared distance between sphere center and AABB
    // the sqrt(dist) is fine to use as well, but this is faster.
    float sqDist = SqDistPointAABB( sphereCenter, aabbMin, aabbMax );

    // Sphere and AABB intersect if the (squared) distance between them is
    // less than the (squared) sphere radius.
    return sqDist <= sphereRadius * sphereRadius;
}


inline bool sphereIntersectsAABB(fpt_vec3& sphereCenter, fpt sphereRadius, fpt_vec3& aabbMin, fpt_vec3& aabbMax){
    // Compute squared distance between sphere center and AABB
    // the sqrt(dist) is fine to use as well, but this is faster.
    fpt sqDist = SqDistPointAABB( sphereCenter, aabbMin, aabbMax );

    // Sphere and AABB intersect if the (squared) distance between them is
    // less than the (squared) sphere radius.
    return sqDist <= fpt_mul(sphereRadius, sphereRadius);
}




inline bool triangleIntersectsAABB(const Triangle& triangle, const vec3& aabbMin, const vec3& aabbMax) {
    
    //face normals for AABBx
    vec3 u0 = vec3_create(1.0f, 0.0f, 0.0f);
    vec3 u1 = vec3_create(0.0f, 1.0f, 0.0f);
    vec3 u2 = vec3_create(0.0f, 0.0f, 1.0f);
    vec3 u_vectors[3] = {u0, u1, u2};
    
    //printf("triangleIntersectsAABBTest\n");
    //printf("aabbMin: %f, %f, %f\n", aabbMin.x, aabbMin.y, aabbMin.z);
    //printf("aabbMax: %f, %f, %f\n", aabbMax.x, aabbMax.y, aabbMax.z);
    //printf("v0 triangle %f, %f, %f...\n", triangle.v0.x, triangle.v0.y, triangle.v0.z);
    //printf("v1 triangle %f, %f, %f...\n", triangle.v1.x, triangle.v1.y, triangle.v1.z);
    //printf("v2 triangle %f, %f, %f...\n", triangle.v2.x, triangle.v2.y, triangle.v2.z);
    //all shapes are already assumed to be relative to origin
    vec3 aabbCenter = (aabbMin + aabbMax) * 0.5f;
    vec3 aabbHalfExtents = (aabbMax - aabbMin) * 0.5f;

    vec3 v0 = triangle.v0 - aabbCenter;
    vec3 v1 = triangle.v1 - aabbCenter;
    vec3 v2 = triangle.v2 - aabbCenter;
    

    aabbCenter -= aabbCenter;
    
    //printf("relativized coords:\n");
    //printf("v0 triangle %f, %f, %f...\n", v0.x, v0.y, v0.z);
    //printf("v1 triangle %f, %f, %f...\n", v1.x, v1.y, v1.z);
    //printf("v2 triangle %f, %f, %f...\n", v2.x, v2.y, v2.z);
    //printf("aabbCenter: %f, %f, %f\n", aabbCenter.x, aabbCenter.y, aabbCenter.z);

    //calculate triangle edges
    vec3 e0 = v1 - v0;
    vec3 e1 = v2 - v1;
    vec3 e2 = v0 - v2;

    // Define arrays for u-vectors and e-vectors
    vec3 e_vectors[3] = {e0, e1, e2};

    for(int i = 0; i < 3; i++){
        for(int j = 0; j < 3; j++){
            vec3 axis = vec3_cross(u_vectors[i], e_vectors[j]);
            if(axis == vec3_create(0.0f)){
                //if the axis is 0, it is not useful as they are parallel and will not intersect, skip it
                continue;
            }
            //printf("u_vectors[%d]: %f, %f, %f\n", i, u_vectors[i].x, u_vectors[i].y, u_vectors[i].z);
            //printf("e_vectors[%d]: %f, %f, %f\n", j, e_vectors[j].x, e_vectors[j].y, e_vectors[j].z);
            //printf("axis: %f, %f, %f\n", axis.x, axis.y, axis.z);
            // Project all 3 vertices of the triangle onto the Seperating axis
            float p0 = vec3_dot(v0, axis);
            float p1 = vec3_dot(v1, axis);
            float p2 = vec3_dot(v2, axis);
            //printf("p0: %f, p1: %f, p2: %f\n", p0, p1, p2);
            //r is the radius of the aabb when projected onto the seperating axis
            float r = aabbHalfExtents.x * fabs(axis.x) +
                        aabbHalfExtents.y * fabs(axis.y) +
                        aabbHalfExtents.z * fabs(axis.z);
            //printf("r = (%f * %f) + (%f * %f) + (%f * %f)\n", aabbHalfExtents.x,abs(axis.x), aabbHalfExtents.y,abs(axis.y), aabbHalfExtents.z, abs(axis.z));
            //printf("r: %f\n", r);
            // Now do the actual test, basically see if either of
            // the most extreme of the triangle points intersects r
            // You might need to write Min & Max functions that take 3 arguments
            if (max(-max(max(p0, p1), p2), min(min(p0, p1), p2)) > r) {
                //printf("max p: %f\n", max(max(p0, p1), p2));
                //printf("min p: %f\n", min(min(p0, p1), p2));
                // This means BOTH of the points of the projected triangle
                // are outside the projected half-length of the AABB
                // Therefore the axis is seperating and we can exit
                //printf("seperating axis found\n");
                return false;
            }
        }
        // SAT Tests for AABB Face Normals
        float p0 = vec3_dot(v0, u_vectors[i]);
        float p1 = vec3_dot(v1, u_vectors[i]);
        float p2 = vec3_dot(v2, u_vectors[i]);
        float minp = min(min(p0, p1), p2);
        float maxp = max(max(p0, p1), p2);

        if (minp > aabbHalfExtents.e[i] || maxp < -aabbHalfExtents.e[i]) {
            //printf("AABB FACE NORMAL seperating axis found\n");
            return false; // Separating axis found
        }
    }

    // SAT Test for Triangle Face Normal
    vec3 triangleNormal = vec3_cross(e0, e1);
    float p0 = vec3_dot(v0, triangleNormal);
    float p1 = vec3_dot(v1, triangleNormal);
    float p2 = vec3_dot(v2, triangleNormal);
    float minp = min(min(p0, p1), p2);
    float maxp = max(max(p0, p1), p2);
    float r = aabbHalfExtents.x     * fabs(vec3_dot(u0, triangleNormal)) +
                aabbHalfExtents.y   * fabs(vec3_dot(u1, triangleNormal)) +
                aabbHalfExtents.z   * fabs(vec3_dot(u2, triangleNormal));

    if (minp > r || maxp < -r) {
        //printf("TRIANGLE FACE NORMAL seperating axis found\n");
        return false; // Separating axis found
    }

    // Passed testing for all 13 seperating axis that exist!

    //printf("NO seperating axis found\n");
    //printf("CUBE INTERSECTS TRIANGLE\n");
    //printf("triangle coordinates: %f, %f, %f\n", triangle.v0.x, triangle.v0.y, triangle.v0.z);
    //printf("triangle coordinates: %f, %f, %f\n", triangle.v1.x, triangle.v1.y, triangle.v1.z);
    //printf("triangle coordinates: %f, %f, %f\n", triangle.v2.x, triangle.v2.y, triangle.v2.z);
    //printf("aabbMin: %f, %f, %f\n", aabbMin.x, aabbMin.y, aabbMin.z);
    //printf("aabbMax: %f, %f, %f\n", aabbMax.x, aabbMax.y, aabbMax.z);
    return true;  // No separating axis found, intersection occurs
}
