#ifndef FPTVEC_H
#define FPTVEC_H

#include "fptc.h"

#define FPT_HALF 32768 //0.5f
#define FPT_QUARTER 16384 //0.25f
#define FPT_EIGHTH 8192 //0.125f
#define FPT_TENTH 6553 //0.1
#define FPT_HUNDREDTH 655 //0.01
#define FPT_THOUSANDTH 65 //0.001

struct fpt_vec3 {
    fpt x, y, z, pad;

    // Constructor (marked constexpr to keep it trivial)
    // constexpr fpt_vec3_create(fpt x_ = 0, fpt y_ = 0, fpt z_ = 0) : x(x_), y(y_), z(z_), pad(0) {}

    // Array operators
    fpt& operator[](int i) {
        fpt* arr = &x;  // Get pointer to first element
        return arr[i];  // Use pointer arithmetic instead of a switch
    }

    const fpt& operator[](int i) const {
        const fpt* arr = &x;
        return arr[i];
    }
};

// Unary negation operator
inline fpt_vec3 operator-(const fpt_vec3& v) {
    return {-v.x, -v.y, -v.z};
}

inline fpt_vec3& operator-=(fpt_vec3& lhs, const fpt& rhs) {
    lhs.x  = fpt_sub(lhs.x, rhs);
    lhs.y  = fpt_sub(lhs.y, rhs);
    lhs.z  = fpt_sub(lhs.z, rhs);
    return lhs;
}

inline fpt_vec3& operator-=(fpt_vec3& lhs, const ivec3& rhs) {
    lhs.x  = fpt_sub(lhs.x, rhs.x);
    lhs.y  = fpt_sub(lhs.y, rhs.y);
    lhs.z  = fpt_sub(lhs.z, rhs.z);
    return lhs;
}


inline bool operator==(const fpt_vec3& lhs, const fpt_vec3& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

inline bool operator!=(const fpt_vec3& lhs, const fpt_vec3& rhs) {
    return !(lhs == rhs);
}

// You might also want to add compound assignment operators for completeness:
inline fpt_vec3& operator-=(fpt_vec3& lhs, const fpt_vec3& rhs) {
    lhs.x  = fpt_sub(lhs.x, rhs.x);
    lhs.y  = fpt_sub(lhs.y, rhs.y);
    lhs.z  = fpt_sub(lhs.z, rhs.z);
    return lhs;
}

inline fpt_vec3 operator-(fpt_vec3& lhs, const fpt_vec3& rhs) {
    return {
        fpt_sub(lhs.x, rhs.x),
        fpt_sub(lhs.y, rhs.y),
        fpt_sub(lhs.z, rhs.z)
    };
}


inline fpt_vec3 operator-(fpt_vec3& lhs, const fpt& rhs) {
    return {
        fpt_sub(lhs.x, rhs),
        fpt_sub(lhs.y, rhs),
        fpt_sub(lhs.z, rhs)
    };
}

inline fpt_vec3& operator+=(fpt_vec3& lhs, const fpt& rhs) {
    lhs.x  = fpt_add(lhs.x, rhs);
    lhs.y  = fpt_add(lhs.y, rhs);
    lhs.z  = fpt_add(lhs.z, rhs);
    return lhs;
}

// You might also want to add compound assignment operators for completeness:
inline fpt_vec3& operator+=(fpt_vec3& lhs, const fpt_vec3& rhs) {
    lhs.x  = fpt_add(lhs.x, rhs.x);
    lhs.y  = fpt_add(lhs.y, rhs.y);
    lhs.z  = fpt_add(lhs.z, rhs.z);
    return lhs;
}

inline fpt_vec3 operator+(const fpt_vec3& lhs, const fpt_vec3& rhs) {
    return {
        fpt_add(lhs.x, rhs.x),
        fpt_add(lhs.y, rhs.y),
        fpt_add(lhs.z, rhs.z)
    };
}


inline fpt_vec3 operator+(const fpt_vec3& lhs, const fpt& rhs) {
    return {
        fpt_add(lhs.x, rhs),
        fpt_add(lhs.y, rhs),
        fpt_add(lhs.z, rhs)
    };
}



inline fpt_vec3& operator*=(fpt_vec3& lhs, const fpt& rhs) {
    lhs.x = fpt_mul(lhs.x, rhs);
    lhs.y = fpt_mul(lhs.y, rhs);
    lhs.z = fpt_mul(lhs.z, rhs);
    return lhs;
}

// Vector * scalar
inline fpt_vec3 operator*(const fpt_vec3& lhs, const fpt& rhs) {
    return {
        fpt_mul(lhs.x, rhs),
        fpt_mul(lhs.y, rhs),
        fpt_mul(lhs.z, rhs)
    };
}

// Element-wise vector multiplication
inline fpt_vec3 operator*(const fpt_vec3& lhs, const fpt_vec3& rhs) {
    return {
        fpt_mul(lhs.x, rhs.x),
        fpt_mul(lhs.y, rhs.y),
        fpt_mul(lhs.z, rhs.z)
    };
}


inline fpt_vec3 operator/(const fpt_vec3& lhs, const fpt_vec3& rhs) {
    return {
        fpt_div(lhs.x, rhs.x),
        fpt_div(lhs.y, rhs.y),
        fpt_div(lhs.z, rhs.z)
    };
}

inline fpt_vec3 operator/(const fpt& lhs, const fpt_vec3& rhs) {
    return {
        fpt_div(lhs, rhs.x),
        fpt_div(lhs, rhs.y),
        fpt_div(lhs, rhs.z)
    };
}

// scalar * Vector
inline fpt_vec3 operator*(const fpt& lhs, const fpt_vec3& rhs) {
    return {
        fpt_mul(lhs, rhs.x),
        fpt_mul(lhs, rhs.y),
        fpt_mul(lhs, rhs.z)
    };
}

// And the binary subtraction operator if you don't have it already
inline fpt_vec3 operator-(const fpt_vec3& lhs, const fpt_vec3& rhs) {
    return {
        fpt_sub(lhs.x, rhs.x),
        fpt_sub(lhs.y, rhs.y),
        fpt_sub(lhs.z, rhs.z)
    };
}

// And the binary subtraction operator if you don't have it already
inline fpt_vec3 operator-(const fpt_vec3& lhs, const fpt& rhs) {
    return {
        fpt_sub(lhs.x, rhs),
        fpt_sub(lhs.y, rhs),
        fpt_sub(lhs.z, rhs)
    };
}


struct fpt_vec4{
    fpt x;
    fpt y;
    fpt z;
    fpt w;

        fpt& operator[](int i) {
        switch(i) {
            case 0: return x;
            case 1: return y;
            case 2: return z;
            case 3: return w;
            default: return x; // or some other fallback
        }
    }

    // Const version of array subscript operator
    const fpt& operator[](int i) const {
        switch(i) {
            case 0: return x;
            case 1: return y;
            case 2: return z;
            case 3: return w;
            default: return x;
        }
    }
};
typedef struct fpt_vec4 fpt_quat;
// typedef struct fpt_vec3 fpt_ivec3;

// Vector addition
static inline fpt_vec3 fpt_vec3_add(fpt_vec3 a, fpt_vec3 b) {
    fpt_vec3 result = {
        fpt_add(a.x, b.x),
        fpt_add(a.y, b.y),
        fpt_add(a.z, b.z)
    };
    return result;
}

static inline fpt_vec3 fpt_vec3_add(fpt_vec3 a, fpt b) {
    fpt_vec3 result = {
        fpt_add(a.x, b),
        fpt_add(a.y, b),
        fpt_add(a.z, b)
    };
    return result;
}


// Vector scaling
static inline fpt_vec3 fpt_vec3_scale(fpt_vec3 v, fpt scale) {
    fpt_vec3 result = {
        fpt_mul(v.x, scale),
        fpt_mul(v.y, scale),
        fpt_mul(v.z, scale)
    };
    return result;
}

// Dot product
static inline fpt fpt_vec3_dot(fpt_vec3 a, fpt_vec3 b) {
    return fpt_add(
        fpt_add(
            fpt_mul(a.x, b.x),
            fpt_mul(a.y, b.y)
        ),
        fpt_mul(a.z, b.z)
    );
}

// Length squared (avoid sqrt when possible)
static inline fpt fpt_vec3_length_sq(fpt_vec3 v) {
    return fpt_vec3_dot(v, v);
}

// Vector length/magnitude
static inline fpt fpt_vec3_length(fpt_vec3 v) {
    return fpt_sqrt(fpt_vec3_length_sq(v));
}

// Normalize
static inline fpt_vec3 fpt_vec3_normalize(fpt_vec3 v) {
    fpt length = fpt_vec3_length(v);
    if (length == 0) return v;
    
    return fpt_vec3_scale(v, fpt_div(FPT_ONE, length));
}

// Cross product
static inline fpt_vec3 fpt_vec3_cross(fpt_vec3 a, fpt_vec3 b) {
    fpt_vec3 result = {
        fpt_sub(fpt_mul(a.y, b.z), fpt_mul(a.z, b.y)),
        fpt_sub(fpt_mul(a.z, b.x), fpt_mul(a.x, b.z)),
        fpt_sub(fpt_mul(a.x, b.y), fpt_mul(a.y, b.x))
    };
    return result;
}

inline fpt_vec3 operator%(fpt_vec3 lhs, fpt_vec3 rhs) {
    return fpt_vec3_cross(lhs, rhs);
}

inline fpt_vec3& operator%=(fpt_vec3& lhs, const fpt_vec3& rhs) {
    lhs = fpt_vec3_cross(lhs, rhs);
    return lhs;
}

// Some convenient constructors
static inline fpt_vec3 fpt_vec3_create(fpt x, fpt y, fpt z) {
    fpt_vec3 result = {x, y, z};
    return result;
}

static inline fpt_vec4 fpt_vec4_create(fpt x, fpt y, fpt z, fpt w){
    fpt_vec4 result = {x, y, z, w};
    return result;
}

static inline fpt_vec4 fpt_vec4_create(fpt_vec3 v, fpt w){
    fpt_vec4 result = {v.x, v.y, v.z, w};
    return result;
}


static inline fpt_vec3 fpt_vec3_create(fpt v) {
    fpt_vec3 result = {v, v, v};
    return result;
}

static inline fpt_quat fpt_quat_create(fpt x, fpt y, fpt z, fpt w) {
    fpt_quat result = {x, y, z, w};
    return result;
}

inline fpt fpt_sign(fpt value) {
    // Return +FPT_ONE for positive, -FPT_ONE for negative, 0 for zero
    if (value > 0) return FPT_ONE;
    if (value < 0) return -FPT_ONE;
    return 0;
}

inline fpt_vec3 fpt_vec3_sign(const fpt_vec3& v) {
    return {
        fpt_sign(v.x),
        fpt_sign(v.y),
        fpt_sign(v.z)
    };
}

static inline fpt_vec3 fpt_vec3_invert(fpt_vec3 a){
    return fpt_vec3_create(-a.x, -a.y, -a.z);
}


// Convert vec3 to quat with w=0
static inline fpt_quat fpt_vec3_to_quat(fpt_vec3 v) {
    return fpt_quat_create(v.x, v.y, v.z, 0);
}

// Get conjugate (negative xyz, same w)
static inline fpt_quat fpt_quat_conjugate(fpt_quat q) {
    return fpt_quat_create(-q.x, -q.y, -q.z, q.w);
}


// Identity quaternion
static inline fpt_quat fpt_quat_identity() {
    return fpt_quat_create(0, 0, 0, FPT_ONE);
}

// Quaternion multiplication - this combines rotations
static inline fpt_quat fpt_quat_mul(fpt_quat a, fpt_quat b) {
    fpt_quat result;
    
    // Real part (w)
    result.w = fpt_sub(fpt_sub(fpt_sub(
        fpt_mul(a.w, b.w),
        fpt_mul(a.x, b.x)),
        fpt_mul(a.y, b.y)),
        fpt_mul(a.z, b.z)
    );
    
    // Vector part (x,y,z)
    result.x = fpt_add(fpt_add(fpt_add(
        fpt_mul(a.w, b.x),
        fpt_mul(a.x, b.w)),
        fpt_mul(a.y, b.z)),
        fpt_mul(-a.z, b.y)  // Note the negative sign
    );
    
    result.y = fpt_add(fpt_add(fpt_add(
        fpt_mul(a.w, b.y),
        fpt_mul(a.y, b.w)),
        fpt_mul(a.z, b.x)),
        fpt_mul(-a.x, b.z)  // Note the negative sign
    );
    
    result.z = fpt_add(fpt_add(fpt_add(
        fpt_mul(a.w, b.z),
        fpt_mul(a.z, b.w)),
        fpt_mul(a.x, b.y)),
        fpt_mul(-a.y, b.x)  // Note the negative sign
    );
    
    return result;
}
// Create rotation from axis and angle
static inline fpt_quat fpt_quat_from_axis_angle(fpt_vec3 axis, fpt angle) {
    // Normalize axis first
    axis = fpt_vec3_normalize(axis);
    
    // Calculate sin(angle/2) and cos(angle/2)
    fpt half_angle = fpt_div(angle, i2fpt(2));
    fpt s = fpt_sin(half_angle);
    fpt c = fpt_cos(half_angle);
    
    fpt_quat result;
    result.x = fpt_mul(axis.x, s);
    result.y = fpt_mul(axis.y, s);
    result.z = fpt_mul(axis.z, s);
    result.w = c;
    
    return result;
}

// Normalize a quaternion (keeps rotation valid)
static inline fpt_quat fpt_quat_normalize(fpt_quat q) {
    // Calculate length (just like vector length)
    fpt length_sq = fpt_add(fpt_add(fpt_add(
        fpt_mul(q.x, q.x),
        fpt_mul(q.y, q.y)),
        fpt_mul(q.z, q.z)),
        fpt_mul(q.w, q.w)
    );
    
    fpt length = fpt_sqrt(length_sq);
    
    // Avoid division by zero
    if (length == 0) {
        return fpt_quat_identity();
    }
    
    // Scale each component
    fpt inv_length = fpt_div(FPT_ONE, length);
    fpt_quat result;
    result.x = fpt_mul(q.x, inv_length);
    result.y = fpt_mul(q.y, inv_length);
    result.z = fpt_mul(q.z, inv_length);
    result.w = fpt_mul(q.w, inv_length);
    
    return result;
}

// static inline fpt_vec3 glm_ivec3_to_fpt(glm::ivec3 v){
//     return fpt_vec3_create(
//         i2fpt(v.x),
//         i2fpt(v.y),
//         i2fpt(v.z)
//     );
// }

static inline vec3 fpt_to_flt_vec3(fpt_vec3 v) {
    vec3 result = {};
    result.x = fpt2fl(v.x);
    result.y = fpt2fl(v.y);
    result.z = fpt2fl(v.z);
    return result;
}



// static inline glm::vec3 fpt_to_flt_vec3(fpt_vec3 v) {
//     return glm::vec3_create(
//         fpt2fl(v.x),
//         fpt2fl(v.y),
//         fpt2fl(v.z)
//     );
// }

// static inline fpt_vec4 glm_to_fpt_vec4(glm::vec4 v) {
//     return fpt_vec4_create(
//         fl2fpt(v.x),
//         fl2fpt(v.y),
//         fl2fpt(v.z),
//         fl2fpt(v.w)
//     );
// }


// static inline glm::ivec3 fpt_to_glm_ivec3(fpt_vec3 v) {
//     return glm::ivec3_create(
//         fpt2i(v.x),
//         fpt2i(v.y),
//         fpt2i(v.z)
//     );
// }


// static inline fpt_vec3 flt_to_fpt_vec3(glm::vec3 v) {
//     return fpt_vec3_create(
//         fl2fpt(v.x),
//         fl2fpt(v.y),
//         fl2fpt(v.z)
//     );
// }


static inline fpt_vec3 ivec_to_fpt_vec3(ivec3 v) {
    fpt_vec3 result = {};
    result.x = fl2fpt(v.x);
    result.y = fl2fpt(v.y);
    result.z = fl2fpt(v.z);
    return result;
}

static inline quat fpt_to_flt_quat(fpt_quat q) {
    quat result = {};
    result.w = fpt2fl(q.w);
    result.x = fpt2fl(q.x);
    result.y = fpt2fl(q.y);
    result.z = fpt2fl(q.z);
    return result;
}

static inline fpt_vec3 flt_to_fpt_vec3(vec3 v) {
    fpt_vec3 result = {};
    result.x = fl2fpt(v.x);
    result.y = fl2fpt(v.y);
    result.z = fl2fpt(v.z);
    return result;
}

// static inline glm::vec4 fpt_to_glm_vec4(fpt_quat q) {
//     // Note: GLM constructor is (w, x, y, z) order
//     return glm::vec4(
//         fpt2fl(q.w),  // w first for glm
//         fpt2fl(q.x), 
//         fpt2fl(q.y),
//         fpt2fl(q.z)
//     );
// }

// static inline glm::vec4 fpt_to_glm_vec4(fpt_vec3 q, fpt c) {
//     // Note: GLM constructor is (w, x, y, z) order
//     return glm::vec4(
//         fpt2fl(c),  // w first for glm
//         fpt2fl(q.x), 
//         fpt2fl(q.y),
//         fpt2fl(q.z)
//     );
// }

// static inline fpt_quat glm_to_fpt_quat(glm::quat v){
//     return fpt_quat(
//         fl2fpt(v.x),
//         fl2fpt(v.y),
//         fl2fpt(v.z),
//         fl2fpt(v.w)
//     );
// }

static inline fpt fpt_clamp(fpt d, fpt min, fpt max) {
    const fpt t = d < min ? min : d;
    return t > max ? max : t;
}


static inline fpt fpt_degrees(fpt radians){
    // 180/π ≈ 57.2957795131
    // In fixed point (16.16), that's approximately 3754936 (0x394BB8)
    return fpt_mul(radians, 0x394BB8);  // radians * (180/π)
}

static inline fpt fpt_radians(fpt degrees) {
    // π/180 ≈ 0.0174532925199
    // In fixed point (16.16), that's approximately 1144 (0x478)
    return fpt_mul(degrees, 0x478);  // degrees * (π/180)
}

// Multiply vector by scalar
static inline fpt_vec3 fpt_vec3_mul_scalar(fpt_vec3 v, fpt scalar) {
    fpt_vec3 result = {
        fpt_mul(v.x, scalar),
        fpt_mul(v.y, scalar),
        fpt_mul(v.z, scalar)
    };
    return result;
}

static inline fpt_vec3 fpt_vec3_add_scaled(fpt_vec3& a, fpt_vec3 b, fpt scalar){
    // a.x += (b.x * scalar);
    // a.y += (b.y * scalar);
    // a.z += (b.z * scalar);
    a += (b*scalar);
    return a;
}

static inline fpt_vec3 fpt_quat_rotate_vec3(fpt_quat q, fpt_vec3 v) {
    // Get vector part of quaternion
    fpt_vec3 qvec = fpt_vec3_create(q.x, q.y, q.z);
    
    // First cross product
    fpt_vec3 uv = fpt_vec3_cross(qvec, v);
    
    // Second cross product
    fpt_vec3 uuv = fpt_vec3_cross(qvec, uv);
    
    // Scale uv by q.w
    uv = fpt_vec3_mul_scalar(uv, q.w);
    
    // Add uv and uuv
    fpt_vec3 sum = fpt_vec3_add(uv, uuv);
    
    // Multiply by 2
    sum = fpt_vec3_mul_scalar(sum, FPT_TWO);
    
    // Add to original vector
    return fpt_vec3_add(v, sum);
}

static inline fpt_quat fpt_angle_axis(fpt angle, fpt_vec3 axis) {
    // Normalize the axis
    axis = fpt_vec3_normalize(axis);

    // Compute sine and cosine of half the angle
    fpt halfAngle = fpt_div(angle, FPT_TWO);
    fpt s = fpt_sin(halfAngle);
    fpt c = fpt_cos(halfAngle);

    // Construct the quaternion
    fpt_quat q;
    q.w = c;
    q.x = fpt_mul(axis.x, s);
    q.y = fpt_mul(axis.y, s);
    q.z = fpt_mul(axis.z, s);

    return q;
}




static inline void fpt_quat_str(fpt_quat q, char* buffer, int offset) {
    // Each component gets its own null-terminated string
    fpt_str(q.x, buffer + (offset * 0), -1);
    buffer[(offset * 1) - 1] = '\0';  // Add null terminator
    
    fpt_str(q.y, buffer + (offset * 1), -1);
    buffer[(offset * 2) - 1] = '\0';
    
    fpt_str(q.z, buffer + (offset * 2), -1);
    buffer[(offset * 3) - 1] = '\0';
    
    fpt_str(q.w, buffer + (offset * 3), -1);
    buffer[(offset * 4) - 1] = '\0';
    
}

static inline void fpt_vec3_str(fpt_vec3 v, char* buffer, int offset){
    // Each component gets its own null-terminated string
    fpt_str(v.x, buffer + (offset * 0), -1);
    buffer[(offset * 1) - 1] = '\0';  // Add null terminator
    
    fpt_str(v.y, buffer + (offset * 1), -1);
    buffer[(offset * 2) - 1] = '\0';
    
    fpt_str(v.z, buffer + (offset * 2), -1);
    buffer[(offset * 3) - 1] = '\0';
}



/* or just do something simpler
        char wx[16], wy[16], wz[16], ww[16];
        fpt_str(fptQuatH.x, wx, -1);
        fpt_str(fptQuatH.y, wy, -1);
        fpt_str(fptQuatH.z, wz, -1);
        fpt_str(fptQuatH.w, ww, -1);
        printf("fptQuatH: %s %s %s %s\n", wx, wy, wz, ww);
        */

       // For a single fixed-point number
// inline fpt fpt_floor(fpt x) {
//     // In Q16.16, we want to clear out all fractional bits (lower 16 bits)
//     if (x >= 0) {
//         // For positive numbers, mask off the fractional part
//         return x & ~((1 << FPT_FBITS) - 1);  // Clear lower 16 bits
//     } else {
//         // For negative numbers, we need to round down
//         // If there are any fractional bits, subtract one whole unit
//         fpt fractional = x & ((1 << FPT_FBITS) - 1);  // Get fractional part
//         return fractional ? (x & ~((1 << FPT_FBITS) - 1)) - (1 << FPT_FBITS) : x;
//     }
// }

inline fpt fpt_floor(fpt x) {
    return  x & 0xFFFF0000;
}

inline fpt fpt_ceil(fpt x) {
    return x & 0xFFFF0000;
}

inline fpt fpt_mod(fpt a, fpt b) {
    // Ensure positive modulo result
    fpt result = a % b;
    if (result < 0) {
        result += b;
    }
    return result;
}


// For single fpt values
inline fpt fpt_min(fpt a, fpt b) {
    return (a < b) ? a : b;
}

inline fpt fpt_max(fpt a, fpt b) {
    return (a > b) ? a : b;
}

// For fpt_vec3 component-wise max
inline fpt fpt_vec3_max_component(fpt_vec3 v) {
    return fpt_max(fpt_max(fpt_abs(v.x), fpt_abs(v.y)), fpt_abs(v.z));
}

inline fpt fpt_vec3_min_component(fpt_vec3 v) {
    return fpt_min(fpt_min(fpt_abs(v.x), fpt_abs(v.y)), fpt_abs(v.z));
}


// If you need component-wise operations between two vectors:
inline fpt_vec3 fpt_vec3_min(fpt_vec3 a, fpt_vec3 b) {
    return fpt_vec3{
        fpt_min(a.x, b.x),
        fpt_min(a.y, b.y),
        fpt_min(a.z, b.z)
    };
}

inline fpt_vec3 fpt_vec3_max(fpt_vec3 a, fpt_vec3 b) {
    return fpt_vec3{
        fpt_max(a.x, b.x),
        fpt_max(a.y, b.y),
        fpt_max(a.z, b.z)
    };
}

// IVEC3

// inline fpt_ivec3 operator+(const fpt_ivec3& lhs, const fpt_ivec3& rhs) {
//     return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
// }

// inline bool operator==(const fpt_ivec3& lhs, const fpt_ivec3& rhs) {
//     return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
// }

// // If you need to convert back to glm::ivec3 for rendering/display
// inline glm::ivec3 fpt_ivec3_to_glm(const fpt_ivec3& v) {
//     return glm::ivec3_create(v.x, v.y, v.z);
// }





// Helper functions
// inline fpt_ivec3 fpt_ivec3_create(int32_t x, int32_t y, int32_t z) {
//     return {x, y, z};
// }





//////////////////////////////////////// UNUSED FIXED POINT MATRIX SHIT ////////////////////////////////////////////////////
struct fpt_mat4 {
    fpt m[4][4]; 

        // Return pointer to column for first []
    fpt* operator[](int col) {
        return m[col];
    }

    // Const version for when matrix is const
    const fpt* operator[](int col) const {
        return m[col];
    }

};

struct fpt_mat3 {
    fpt m[3][3]; 

        // Return pointer to column for first []
    fpt* operator[](int col) {
        return m[col];
    }

    // Const version for when matrix is const
    const fpt* operator[](int col) const {
        return m[col];
    }

};

//mat3 layout
// [col0][row0]  [col1][row0]  [col2][row0]
// [col0][row1]  [col1][row1]  [col2][row1]
// [col0][row2]  [col1][row2]  [col2][row2]

inline fpt_mat4 matrixMultiply(const fpt_mat4& a, const fpt_mat4& b) {
    fpt_mat4 result = {};
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            // Need careful handling of fixed point multiplication here
            // Might need intermediate scaling to prevent overflow
            for(int k = 0; k < 4; k++) {
                result.m[i][j] = fpt_add(result.m[i][j], fpt_mul(a.m[i][k], b.m[k][j]));
            }
        }
    }
    return result;
}

// Matrix-vector multiplication
inline fpt_vec4 matrixVectorMul(const fpt_mat4& m, const fpt_vec4& v) {
    fpt_vec4 result;
    for(int i = 0; i < 4; i++) {
        result[i] =     fpt_add(fpt_add(fpt_add(fpt_mul(m.m[i][0], v.x), 
                                                fpt_mul(m.m[i][1], v.y)), 
                                                fpt_mul(m.m[i][2], v.z)), 
                                                fpt_mul(m.m[i][3], v.w));
    }
    return result;
}

// Identity matrix creation
inline fpt_mat4 mat4_create_identity() {
    fpt_mat4 result = {};
    result.m[0][0] = FPT_ONE;  // Your fixed point 1.0
    result.m[1][1] = FPT_ONE;
    result.m[2][2] = FPT_ONE;
    result.m[3][3] = FPT_ONE;
    return result;
}

    // Identity matrix creation
    inline fpt_mat3 mat3_create_identity() {
        fpt_mat3 result = {};
        result.m[0][0] = FPT_ONE;  // Your fixed point 1.0
        result.m[1][1] = FPT_ONE;
        result.m[2][2] = FPT_ONE;
        return result;
    }


    //to switch between column and row major matrices
    // inline fpt_mat4 transpose(const fpt_mat4& m) {
    //     return fpt_mat4(
    //         m[0][0], m[1][0], m[2][0], m[3][0],
    //         m[0][1], m[1][1], m[2][1], m[3][1], 
    //         m[0][2], m[1][2], m[2][2], m[3][2],
    //         m[0][3], m[1][3], m[2][3], m[3][3]
    //     );
    // }

    
    inline void transpose(const fpt_mat4* in, fpt_mat4* out) {
        for(int i = 0; i < 4; i++)
            for(int j = 0; j < 4; j++)
                out->m[j][i] = in->m[i][j];
    }


//row major orderered matrix 4
// inline fpt_mat4 fpt_quat_to_mat4(fpt_quat q) {
//     fpt xx = fpt_mul(q.x , q.x);
//     fpt xy = fpt_mul(q.x , q.y);
//     fpt xz = fpt_mul(q.x , q.z);
//     fpt xw = fpt_mul(q.x , q.w);
//     fpt yy = fpt_mul(q.y , q.y);
//     fpt yz = fpt_mul(q.y , q.z);
//     fpt yw = fpt_mul(q.y , q.w);
//     fpt zz = fpt_mul(q.z , q.z);
//     fpt zw = fpt_mul(q.z , q.w);

//     return fpt_mat4(
//         fpt_sub(FPT_ONE ,   fpt_mul(FPT_TWO , fpt_add(yy , zz))),                       fpt_mul(FPT_TWO , fpt_sub(xy , zw)),                        fpt_mul(FPT_TWO , fpt_add(xz , yw)),    0,
//                             fpt_mul(FPT_TWO , fpt_add(xy , zw)),    fpt_sub(FPT_ONE ,   fpt_mul(FPT_TWO , fpt_add(xx , zz))),                       fpt_mul(FPT_TWO , fpt_sub(yz , xw)),    0,
//                             fpt_mul(FPT_TWO , fpt_sub(xz , yw)),                        fpt_mul(FPT_TWO , fpt_add(yz , xw)),    fpt_sub(FPT_ONE ,   fpt_mul(FPT_TWO , fpt_add(xx , yy))),   0,
//                             0,                                      0,                                                          0,                                                          1
//     );

    //column major
    inline fpt_mat4 fpt_quat_to_mat4(fpt_quat q) {
        fpt xx = fpt_mul(q.x, q.x);
        fpt xy = fpt_mul(q.x, q.y);
        fpt xz = fpt_mul(q.x, q.z);
        fpt xw = fpt_mul(q.x, q.w);
        fpt yy = fpt_mul(q.y, q.y);
        fpt yz = fpt_mul(q.y, q.z);
        fpt yw = fpt_mul(q.y, q.w);
        fpt zz = fpt_mul(q.z, q.z);
        fpt zw = fpt_mul(q.z, q.w);

        fpt_mat4 mat;
        // First column
        mat[0][0] = fpt_sub(FPT_ONE, fpt_mul(FPT_TWO, fpt_add(yy, zz)));
        mat[0][1] = fpt_mul(FPT_TWO, fpt_add(xy, zw));
        mat[0][2] = fpt_mul(FPT_TWO, fpt_sub(xz, yw));
        mat[0][3] = 0;

        // Second column
        mat[1][0] = fpt_mul(FPT_TWO, fpt_sub(xy, zw));
        mat[1][1] = fpt_sub(FPT_ONE, fpt_mul(FPT_TWO, fpt_add(xx, zz)));
        mat[1][2] = fpt_mul(FPT_TWO, fpt_add(yz, xw));
        mat[1][3] = 0;

        // Third column
        mat[2][0] = fpt_mul(FPT_TWO, fpt_add(xz, yw));
        mat[2][1] = fpt_mul(FPT_TWO, fpt_sub(yz, xw));
        mat[2][2] = fpt_sub(FPT_ONE, fpt_mul(FPT_TWO, fpt_add(xx, yy)));
        mat[2][3] = 0;

        // Fourth column
        mat[3][0] = 0;
        mat[3][1] = 0;
        mat[3][2] = 0;
        mat[3][3] = FPT_ONE;

        return mat;
    }

    inline fpt_mat4 fpt_translate(const fpt_vec3& pos) {
        fpt_mat4 mat = {};
        
        mat[0][0] = FPT_ONE; // Column 0, Row 0
        mat[1][0] = 0;       // Column 1, Row 0
        mat[2][0] = 0;       // Column 2, Row 0
        mat[3][0] = pos.x;   // Column 3, Row 0
        
        mat[0][1] = 0;       // Column 0, Row 1
        mat[1][1] = FPT_ONE; // Column 1, Row 1
        mat[2][1] = 0;       // Column 2, Row 1
        mat[3][1] = pos.y;   // Column 3, Row 1
        
        mat[0][2] = 0;       // Column 0, Row 2
        mat[1][2] = 0;       // Column 1, Row 2
        mat[2][2] = FPT_ONE; // Column 2, Row 2
        mat[3][2] = pos.z;   // Column 3, Row 2
        
        mat[0][3] = 0;       // Column 0, Row 3
        mat[1][3] = 0;       // Column 1, Row 3
        mat[2][3] = 0;       // Column 2, Row 3
        mat[3][3] = FPT_ONE; // Column 3, Row 3
        
        return mat;
    }
    // Matrix multiplication for column-major matrices
    inline fpt_mat4 fpt_mul_mat4(const fpt_mat4& a, const fpt_mat4& b) {
        fpt_mat4 result;
        for(int j = 0; j < 4; j++) {         // Column
            for(int i = 0; i < 4; i++) {     // Row
                result[j][i] =  fpt_mul(a[0][i], b[j][0]) + 
                                fpt_mul(a[1][i], b[j][1]) +
                                fpt_mul(a[2][i], b[j][2]) +
                                fpt_mul(a[3][i], b[j][3]);
            }
        }
        return result;
    }



    inline fpt_mat4 operator*(const fpt_mat4& lhs, const fpt_mat4& rhs) {
         return fpt_mul_mat4(lhs, rhs);
    }


    inline fpt_vec3 fpt_mul_mat3_vec3(const fpt_mat3& m, const fpt_vec3& v) {
        fpt_vec3 vec3 = {};

        vec3.x = fpt_add(fpt_add(fpt_mul(m[0][0], v.x) , fpt_mul(m[1][0], v.y)) , fpt_mul(m[2][0], v.z));
        vec3.y = fpt_add(fpt_add(fpt_mul(m[0][1], v.x) , fpt_mul(m[1][1], v.y)) , fpt_mul(m[2][1], v.z));
        vec3.z = fpt_add(fpt_add(fpt_mul(m[0][2], v.x) , fpt_mul(m[1][2], v.y)) , fpt_mul(m[2][2], v.z));

        return vec3;
    }


    inline fpt_vec3 operator*(const fpt_mat3& m, const fpt_vec3& v){
        return fpt_mul_mat3_vec3(m, v);
    }

    inline fpt_vec4 fpt_mul_mat4_vec4(const fpt_mat4& m, const fpt_vec4& v) {
        fpt_vec4 vec4 = {};
        vec4.x = fpt_mul(m[0][0], v.x) + fpt_mul(m[1][0], v.y) + fpt_mul(m[2][0], v.z) + fpt_mul(m[3][0], v.w);
        vec4.y = fpt_mul(m[0][1], v.x) + fpt_mul(m[1][1], v.y) + fpt_mul(m[2][1], v.z) + fpt_mul(m[3][1], v.w);
        vec4.z = fpt_mul(m[0][2], v.x) + fpt_mul(m[1][2], v.y) + fpt_mul(m[2][2], v.z) + fpt_mul(m[3][2], v.w);
        vec4.w = fpt_mul(m[0][3], v.x) + fpt_mul(m[1][3], v.y) + fpt_mul(m[2][3], v.z) + fpt_mul(m[3][3], v.w);
        return vec4;
    }

    inline fpt_vec4 operator*(const fpt_mat4& m, const fpt_vec4& v) {
        return fpt_mul_mat4_vec4(m, v);
    }


    static inline fpt_vec3 fpt_vec3_create(fpt_vec4 v) {
        fpt_vec3 result = {v.x, v.y, v.z};
        return result;
    }


    inline fpt_mat4 fpt_scale(const fpt_vec3& scale) {
        fpt_mat4 result = mat4_create_identity();
        result.m[0][0] = scale.x;
        result.m[1][1] = scale.y;
        result.m[2][2] = scale.z;
        return result;
    }

    // Convert quaternion to 3x3 rotation matrix (no translation)
    inline fpt_mat3 fpt_quat_to_mat3(fpt_quat q) {
        fpt xx = fpt_mul(q.x, q.x);
        fpt xy = fpt_mul(q.x, q.y);
        fpt xz = fpt_mul(q.x, q.z);
        fpt xw = fpt_mul(q.x, q.w);
        fpt yy = fpt_mul(q.y, q.y);
        fpt yz = fpt_mul(q.y, q.z);
        fpt yw = fpt_mul(q.y, q.w);
        fpt zz = fpt_mul(q.z, q.z);
        fpt zw = fpt_mul(q.z, q.w);
        
        fpt_mat3 mat = {};
        mat[0][0] =             fpt_sub(FPT_ONE, fpt_mul(FPT_TWO, fpt_add(yy, zz)));    // Column 0
        mat[1][0] =             fpt_mul(FPT_TWO, fpt_add(xy, zw));
        mat[2][0] =             fpt_mul(FPT_TWO, fpt_sub(xz, yw));


         mat[0][1] =    fpt_mul(FPT_TWO, fpt_sub(xy, zw));                       // Column 1
         mat[1][1] =    fpt_sub(FPT_ONE, fpt_mul(FPT_TWO, fpt_add(xx, zz)));
         mat[2][1] =    fpt_mul(FPT_TWO, fpt_add(yz, xw));

           mat[0][2] =  fpt_mul(FPT_TWO, fpt_add(xz, yw));                    // Column 2
           mat[1][2] =  fpt_mul(FPT_TWO, fpt_sub(yz, xw));
           mat[2][2] =  fpt_sub(FPT_ONE, fpt_mul(FPT_TWO, fpt_add(xx, yy)));
        return mat;
    }

    inline fpt_quat fpt_mat3_to_quat(fpt_mat3 m) {
        // Calculate squared terms
        fpt fourXSquaredMinus1 = fpt_sub(m.m[0][0], fpt_add(m.m[1][1], m.m[2][2]));
        fpt fourYSquaredMinus1 = fpt_sub(m.m[1][1], fpt_add(m.m[0][0], m.m[2][2]));
        fpt fourZSquaredMinus1 = fpt_sub(m.m[2][2], fpt_add(m.m[0][0], m.m[1][1]));
        fpt fourWSquaredMinus1 = fpt_add(fpt_add(m.m[0][0], m.m[1][1]), m.m[2][2]);

        // Find biggest component
        int biggestIndex = 0;
        fpt fourBiggestSquaredMinus1 = fourWSquaredMinus1;
        
        if(fourXSquaredMinus1 > fourBiggestSquaredMinus1) {
            fourBiggestSquaredMinus1 = fourXSquaredMinus1;
            biggestIndex = 1;
        }
        if(fourYSquaredMinus1 > fourBiggestSquaredMinus1) {
            fourBiggestSquaredMinus1 = fourYSquaredMinus1;
            biggestIndex = 2;
        }
        if(fourZSquaredMinus1 > fourBiggestSquaredMinus1) {
            fourBiggestSquaredMinus1 = fourZSquaredMinus1;
            biggestIndex = 3;
        }

        fpt biggestVal = fpt_mul(
            fpt_sqrt(fpt_add(fourBiggestSquaredMinus1, FPT_ONE)),
            fl2fpt(0.5f)
        );
        
        fpt mult = fpt_div(fl2fpt(0.25f), biggestVal);

        fpt_quat result;

        switch(biggestIndex) {
            case 0:
                result.w = biggestVal;
                result.x = fpt_mul(fpt_sub(m.m[1][2], m.m[2][1]), mult);
                result.y = fpt_mul(fpt_sub(m.m[2][0], m.m[0][2]), mult);
                result.z = fpt_mul(fpt_sub(m.m[0][1], m.m[1][0]), mult);
                break;
            case 1:
                result.w = fpt_mul(fpt_sub(m.m[1][2], m.m[2][1]), mult);
                result.x = biggestVal;
                result.y = fpt_mul(fpt_add(m.m[0][1], m.m[1][0]), mult);
                result.z = fpt_mul(fpt_add(m.m[2][0], m.m[0][2]), mult);
                break;
            case 2:
                result.w = fpt_mul(fpt_sub(m.m[2][0], m.m[0][2]), mult);
                result.x = fpt_mul(fpt_add(m.m[0][1], m.m[1][0]), mult);
                result.y = biggestVal;
                result.z = fpt_mul(fpt_add(m.m[1][2], m.m[2][1]), mult);
                break;
            case 3:
                result.w = fpt_mul(fpt_sub(m.m[0][1], m.m[1][0]), mult);
                result.x = fpt_mul(fpt_add(m.m[2][0], m.m[0][2]), mult);
                result.y = fpt_mul(fpt_add(m.m[1][2], m.m[2][1]), mult);
                result.z = biggestVal;
                break;
        }

        return result;
    }


    inline bool fpt_vec3_not_equal(const fpt_vec3& a, const fpt_vec3& b) {
        return a.x != b.x || a.y != b.y || a.z != b.z;
    }

    
    inline bool fpt_quat_not_equal(const fpt_quat& a, const fpt_quat& b) {
        return a.x != b.x || a.y != b.y || a.z != b.z || a.w != b.w;
    }

    // static inline fpt_mat4 glm_to_fpt_mat4(glm::mat4 m) {
    //     fpt_mat4 mat4;
    //     mat4[0][0] = fl2fpt(m[0][0]);
    //     mat4[1][0] = fl2fpt(m[1][0]);
    //     mat4[2][0] = fl2fpt(m[2][0]);
    //     mat4[3][0] = fl2fpt(m[3][0]);

    //     mat4[0][1] = fl2fpt(m[0][1]);
    //     mat4[1][1] = fl2fpt(m[1][1]);
    //     mat4[2][1] = fl2fpt(m[2][1]);
    //     mat4[3][1] = fl2fpt(m[3][1]);

    //     mat4[0][2] = fl2fpt(m[0][2]);
    //     mat4[1][2] = fl2fpt(m[1][2]);
    //     mat4[2][2] = fl2fpt(m[2][2]);
    //     mat4[3][2] = fl2fpt(m[3][2]);

    //     mat4[0][3] = fl2fpt(m[0][3]);
    //     mat4[1][3] = fl2fpt(m[1][3]);
    //     mat4[2][3] = fl2fpt(m[2][3]);
    //     mat4[3][3] = fl2fpt(m[3][3]);

    //     return mat4;
    // }

    // static inline glm::mat4 fpt_to_glm_mat4(fpt_mat4 m) {
    //     glm::mat4 mat4;
    //     mat4[0][0] = fpt2fl(m[0][0]);
    //     mat4[1][0] = fpt2fl(m[1][0]);
    //     mat4[2][0] = fpt2fl(m[2][0]);
    //     mat4[3][0] = fpt2fl(m[3][0]);

    //     mat4[0][1] = fpt2fl(m[0][1]);
    //     mat4[1][1] = fpt2fl(m[1][1]);
    //     mat4[2][1] = fpt2fl(m[2][1]);
    //     mat4[3][1] = fpt2fl(m[3][1]);

    //     mat4[0][2] = fpt2fl(m[0][2]);
    //     mat4[1][2] = fpt2fl(m[1][2]);
    //     mat4[2][2] = fpt2fl(m[2][2]);
    //     mat4[3][2] = fpt2fl(m[3][2]);

    //     mat4[0][3] = fpt2fl(m[0][3]);
    //     mat4[1][3] = fpt2fl(m[1][3]);
    //     mat4[2][3] = fpt2fl(m[2][3]);
    //     mat4[3][3] = fpt2fl(m[3][3]);

    //     return mat4;
    // }

    // static inline fpt_mat3 glm_to_fpt_mat3(glm::mat3 m) {
    //     fpt_mat3 mat3;
    //     // Column 0
    //     mat3.m[0][0] = fl2fpt(m[0][0]);
    //     mat3.m[0][1] = fl2fpt(m[0][1]);
    //     mat3.m[0][2] = fl2fpt(m[0][2]);
        
    //     // Column 1
    //     mat3.m[1][0] = fl2fpt(m[1][0]);
    //     mat3.m[1][1] = fl2fpt(m[1][1]);
    //     mat3.m[1][2] = fl2fpt(m[1][2]);
        
    //     // Column 2
    //     mat3.m[2][0] = fl2fpt(m[2][0]);
    //     mat3.m[2][1] = fl2fpt(m[2][1]);
    //     mat3.m[2][2] = fl2fpt(m[2][2]);
        
    //     return mat3;
    // }

    // static inline glm::mat3 fpt_to_glm_mat3(fpt_mat3 m) {
    //     glm::mat3 mat3;
    //     mat3[0][0] = fpt2fl(m[0][0]);
    //     mat3[1][0] = fpt2fl(m[1][0]);
    //     mat3[2][0] = fpt2fl(m[2][0]);

    //     mat3[0][1] = fpt2fl(m[0][1]);
    //     mat3[1][1] = fpt2fl(m[1][1]);
    //     mat3[2][1] = fpt2fl(m[2][1]);

    //     mat3[0][2] = fpt2fl(m[0][2]);
    //     mat3[1][2] = fpt2fl(m[1][2]);
    //     mat3[2][2] = fpt2fl(m[2][2]);

    //     return mat3;
    // }



	inline fpt_mat4 lookAtLH(fpt_vec3 const& eye, fpt_vec3 const& center, fpt_vec3 const& up)
	{
		fpt_vec3 const f = (fpt_vec3_normalize(center - eye));
		fpt_vec3 const s = (fpt_vec3_normalize(fpt_vec3_cross(up, f)));
		fpt_vec3 const u = (fpt_vec3_cross(f, s));

		fpt_mat4 Result = mat4_create_identity();
		Result[0][0] = s.x;
		Result[1][0] = s.y;
		Result[2][0] = s.z;
		Result[0][1] = u.x;
		Result[1][1] = u.y;
		Result[2][1] = u.z;
		Result[0][2] = f.x;
		Result[1][2] = f.y;
		Result[2][2] = f.z;
		Result[3][0] = -fpt_vec3_dot(s, eye);
		Result[3][1] = -fpt_vec3_dot(u, eye);
		Result[3][2] = -fpt_vec3_dot(f, eye);
		return Result;
	}

static inline fpt fpt_inversesqrt(fpt x) {
    return fpt_div(FPT_ONE, fpt_sqrt(x));
}

// And for vectors:
static inline fpt_vec3 fpt_vec3_inversesqrt(fpt_vec3 v) {
    fpt_vec3 result;
    result.x = fpt_inversesqrt(v.x);
    result.y = fpt_inversesqrt(v.y);
    result.z = fpt_inversesqrt(v.z);
    return result;
}


static inline fpt_quat fpt_quat_look_at(fpt_vec3 from, fpt_vec3 to, fpt_vec3 up) {
    // Get forward direction
    fpt_vec3 forward = fpt_vec3_normalize(to - from);
    
    // Get right vector
    fpt_vec3 right = fpt_vec3_normalize(fpt_vec3_cross(up, forward));
    
    // Get corrected up vector
    fpt_vec3 upDir = fpt_vec3_cross(forward, right);

    // Now we have our rotation matrix formed by [right, upDir, forward]
    // Convert this rotation matrix to quaternion
    fpt_quat result;
    
    fpt trace = fpt_add(fpt_add(right.x, upDir.y), forward.z);
    
    if(trace > 0) {
        fpt s = fpt_sqrt(fpt_add(trace, FPT_ONE));
        result.w = fpt_div(s, FPT_TWO);
        s = fpt_div(FPT_ONE, fpt_mul(FPT_TWO, s));
        result.x = fpt_mul(fpt_sub(upDir.z, forward.y), s);
        result.y = fpt_mul(fpt_sub(forward.x, right.z), s);
        result.z = fpt_mul(fpt_sub(right.y, upDir.x), s);
    } else {
        // Just picking largest diagonal element for stability
        result.w = fpt_div(fpt_sqrt(fpt_add(fpt_add(fpt_add(right.x, upDir.y), forward.z), FPT_ONE)) , FPT_TWO);
        result.x = fpt_div(fpt_sqrt(fpt_add(fpt_add(right.x, -upDir.y), -forward.z)) , FPT_TWO);
        result.y = fpt_div(fpt_sqrt(fpt_add(fpt_add(-right.x, upDir.y), -forward.z)) , FPT_TWO);
        result.z = fpt_div(fpt_sqrt(fpt_add(fpt_add(-right.x, -upDir.y), forward.z)) , FPT_TWO);
    }
    
    return fpt_quat_normalize(result);
}

// // Normalize with minimum length check
// 	static inline glm::quat fpt_quat_LookAtLH(fpt_vec3 const& direction, fpt_vec3 const& up)
// 	{
// 		fpt_mat3 Result;

// 		// Result[2] = direction;
//         Result[0][2] = direction.x;
// 		Result[1][2] = direction.y;
// 		Result[2][2] = direction.z;
		
//         fpt_vec3 const& Right = fpt_vec3_cross(up, direction);
        
//         fpt_vec3 temp = fpt_vec3_mul_scalar(Right, 
//             fpt_inversesqrt(
//                 fpt_max(
//                     1, //the tiniest fixed point value
//                     fpt_vec3_dot(Right, Right)
//                 )
//             )
//         );

//         Result[0][0] = temp.x;
// 		Result[1][0] = temp.y;
// 		Result[2][0] = temp.z;

//         fpt_vec3 cross  = fpt_vec3_cross(direction, temp); 
		
//         Result[0][1] = cross.x;
//         Result[1][1] = cross.y;
//         Result[2][1] = cross.z;

// 		// return fpt_mat3_to_quat(Result);
// 		return glm::quat_cast(fpt_to_glm_mat3(Result));

// 	}

static inline fpt_quat fpt_quat_LookAtRH(fpt_vec3 const& direction, fpt_vec3 const& up)
{
    fpt_mat3 fptResult;
    
    // Column 2 (-direction)
    fptResult.m[2][0] = -direction.x;  // First entry in third column
    fptResult.m[2][1] = -direction.y;  // Second entry in third column
    fptResult.m[2][2] = -direction.z;  // Third entry in third column

    // Calculate right vector
    fpt_vec3 fptRight = fpt_vec3_cross(up, fpt_vec3_create(-direction.x, -direction.y, -direction.z));
    fptRight = fpt_vec3_mul_scalar(fptRight, 
        fpt_inversesqrt(
            fpt_max(
                fl2fpt(0.0001f),
                fpt_vec3_dot(fptRight, fptRight)
            )
        )
    );

    // Column 0 (right vector)
    fptResult.m[0][0] = fptRight.x;
    fptResult.m[0][1] = fptRight.y;
    fptResult.m[0][2] = fptRight.z;

    // Column 1 (up vector = forward × right)
    fpt_vec3 result1 = fpt_vec3_cross(
        fpt_vec3_create(-direction.x, -direction.y, -direction.z),
        fptRight
    );
    fptResult.m[1][0] = result1.x;
    fptResult.m[1][1] = result1.y;
    fptResult.m[1][2] = result1.z;

    return fpt_mat3_to_quat(fptResult);
}

static inline fpt fpt_quat_dot(fpt_quat a, fpt_quat b) {
    return fpt_add(
        fpt_add(
            fpt_add(
                fpt_mul(a.x, b.x),
                fpt_mul(a.y, b.y)
            ),
            fpt_mul(a.z, b.z)
        ),
        fpt_mul(a.w, b.w)
    );
}

static inline fpt_quat fpt_quat_inverse(const fpt_quat& q) {
    // Compute the conjugate
    fpt_quat conj = fpt_quat_conjugate(q);
    
    // Compute the dot product (magnitude squared)
    fpt dot = fpt_quat_dot(q, q);
    
    // Divide the conjugate by the dot product
    fpt_quat inv;
    inv.w = fpt_div(conj.w, dot);
    inv.x = fpt_div(conj.x, dot);
    inv.y = fpt_div(conj.y, dot);
    inv.z = fpt_div(conj.z, dot);
    
    return inv;
}

//DOES NOT HANDLE SCALING
static inline fpt_mat4 fpt_inverse_transform(const fpt_mat4& m) {
    fpt_mat4 result;
    
    // Extract the 3x3 rotation matrix and transpose it
    // Note: Column-major order
    result.m[0][0] = m.m[0][0];  result.m[0][1] = m.m[1][0];  result.m[0][2] = m.m[2][0];
    result.m[1][0] = m.m[0][1];  result.m[1][1] = m.m[1][1];  result.m[1][2] = m.m[2][1];
    result.m[2][0] = m.m[0][2];  result.m[2][1] = m.m[1][2];  result.m[2][2] = m.m[2][2];
    
    // Calculate -R^T * T
    result.m[3][0] = -fpt_add(fpt_add(
        fpt_mul(result.m[0][0], m.m[3][0]),
        fpt_mul(result.m[1][0], m.m[3][1])),
        fpt_mul(result.m[2][0], m.m[3][2]));
        
    result.m[3][1] = -fpt_add(fpt_add(
        fpt_mul(result.m[0][1], m.m[3][0]),
        fpt_mul(result.m[1][1], m.m[3][1])),
        fpt_mul(result.m[2][1], m.m[3][2]));
        
    result.m[3][2] = -fpt_add(fpt_add(
        fpt_mul(result.m[0][2], m.m[3][0]),
        fpt_mul(result.m[1][2], m.m[3][1])),
        fpt_mul(result.m[2][2], m.m[3][2]));
    
    // Set bottom row to [0 0 0 1]
    result.m[0][3] = 0;
    result.m[1][3] = 0;
    result.m[2][3] = 0;
    result.m[3][3] = FPT_ONE;  // Assuming FPT_ONE is your fixed-point representation of 1.0
    
    return result;
}

//does this handle scaling correctly? compare with glm::inverse
/*
static inline fpt_mat4 fpt_inverse_transform(const fpt_mat4& m) {
    fpt_mat4 result;

    // Extract the 3x3 upper-left submatrix (rotation and scaling)
    fpt_mat3 upper_left = {
        {m.m[0][0], m.m[0][1], m.m[0][2]},
        {m.m[1][0], m.m[1][1], m.m[1][2]},
        {m.m[2][0], m.m[2][1], m.m[2][2]}
    };

    // Compute the inverse of the upper-left 3x3 matrix (handles scaling and rotation)
    fpt_mat3 inv_upper_left = fpt_inverse_3x3(upper_left);

    // Copy the inverted 3x3 matrix into the result
    result.m[0][0] = inv_upper_left.m[0][0]; result.m[0][1] = inv_upper_left.m[0][1]; result.m[0][2] = inv_upper_left.m[0][2];
    result.m[1][0] = inv_upper_left.m[1][0]; result.m[1][1] = inv_upper_left.m[1][1]; result.m[1][2] = inv_upper_left.m[1][2];
    result.m[2][0] = inv_upper_left.m[2][0]; result.m[2][1] = inv_upper_left.m[2][1]; result.m[2][2] = inv_upper_left.m[2][2];

    // Calculate -R^T * T (where R^T is the inverted upper-left matrix)
    result.m[3][0] = -fpt_add(fpt_add(
        fpt_mul(inv_upper_left.m[0][0], m.m[3][0]),
        fpt_mul(inv_upper_left.m[1][0], m.m[3][1])),
        fpt_mul(inv_upper_left.m[2][0], m.m[3][2]));
        
    result.m[3][1] = -fpt_add(fpt_add(
        fpt_mul(inv_upper_left.m[0][1], m.m[3][0]),
        fpt_mul(inv_upper_left.m[1][1], m.m[3][1])),
        fpt_mul(inv_upper_left.m[2][1], m.m[3][2]));
        
    result.m[3][2] = -fpt_add(fpt_add(
        fpt_mul(inv_upper_left.m[0][2], m.m[3][0]),
        fpt_mul(inv_upper_left.m[1][2], m.m[3][1])),
        fpt_mul(inv_upper_left.m[2][2], m.m[3][2]));

    // Set bottom row to [0 0 0 1]
    result.m[0][3] = 0;
    result.m[1][3] = 0;
    result.m[2][3] = 0;
    result.m[3][3] = FPT_ONE;  // Assuming FPT_ONE is your fixed-point representation of 1.0

    return result;
}
//*/

struct fpt_flatmat3{  //3x3 matrix
    fpt m[9];
};

struct fpt_flatmat34{ //3x4 matrix //the missing row will always be 0 0 0 1, we can just magically program in the 1 in our operations
    fpt m[12];
};

static inline fpt_flatmat34 fpt_flatmat34_create(){
        fpt_flatmat34 mat;
        mat.m[0]  = FPT_ONE;
        mat.m[1]  = 0;
        mat.m[2]  = 0;
        mat.m[3]  = 0;
        mat.m[4]  = 0;
        mat.m[5]  = FPT_ONE;
        mat.m[6]  = 0;
        mat.m[7]  = 0;
        mat.m[8]  = 0;
        mat.m[9]  = 0;
        mat.m[10] = FPT_ONE;
        mat.m[11] = 0;
        return mat;
}

static inline fpt_flatmat3 fpt_flatmat3_create(){
        fpt_flatmat3 mat;
        mat.m[0]  = 0;
        mat.m[1]  = 0;
        mat.m[2]  = 0;
        mat.m[3]  = 0;
        mat.m[4]  = 0;
        mat.m[5]  = 0;
        mat.m[6]  = 0;
        mat.m[7]  = 0;
        mat.m[8]  = 0;
        mat.m[9]  = 0;
        return mat;
}


static inline fpt_vec3 fpt_vec3_flatmat3_transform(const fpt_vec3& vec, const fpt_flatmat3& mat){
    fpt_vec3 result;

    result.x = fpt_add(fpt_add(fpt_mul(vec.x , mat.m[0])  , fpt_mul(vec.y , mat.m[1])) , fpt_mul(vec.z , mat.m[2]));
    result.y = fpt_add(fpt_add(fpt_mul(vec.x , mat.m[3])  , fpt_mul(vec.y , mat.m[4])) , fpt_mul(vec.z , mat.m[5]));
    result.z = fpt_add(fpt_add(fpt_mul(vec.x , mat.m[6])  , fpt_mul(vec.y , mat.m[7])) , fpt_mul(vec.z , mat.m[8]));

    return result;
}


static inline fpt_vec3 fpt_vec3_flatmat34_transform(const fpt_vec3& vec, const fpt_flatmat34& mat){
    fpt_vec3 result;

    result.x = fpt_add(fpt_add(fpt_add(fpt_mul(vec.x , mat.m[0])  , fpt_mul(vec.y , mat.m[1])) , fpt_mul(vec.z , mat.m[2]) ), mat.m[3]);
    result.y = fpt_add(fpt_add(fpt_add(fpt_mul(vec.x , mat.m[4])  , fpt_mul(vec.y , mat.m[5])) , fpt_mul(vec.z , mat.m[6]) ), mat.m[7]);
    result.z = fpt_add(fpt_add(fpt_add(fpt_mul(vec.x , mat.m[8])  , fpt_mul(vec.y , mat.m[9])) , fpt_mul(vec.z , mat.m[10])), mat.m[11]);

    return result;
}

static inline fpt_flatmat3 fpt_flatmat3_mult(const fpt_flatmat3& a, const fpt_flatmat3& b) {
    return fpt_flatmat3{
        fpt_add(fpt_add(fpt_mul(a.m[0], b.m[0]) ,fpt_mul(a.m[1], b.m[3])), fpt_mul(a.m[2], b.m[6])), 
        fpt_add(fpt_add(fpt_mul(a.m[0], b.m[1]) ,fpt_mul(a.m[1], b.m[4])), fpt_mul(a.m[2], b.m[7])), 
        fpt_add(fpt_add(fpt_mul(a.m[0], b.m[2]) ,fpt_mul(a.m[1], b.m[5])), fpt_mul(a.m[2], b.m[8])), 

        fpt_add(fpt_add(fpt_mul(a.m[3], b.m[0]) ,fpt_mul(a.m[4], b.m[3])), fpt_mul(a.m[5], b.m[6])), 
        fpt_add(fpt_add(fpt_mul(a.m[3], b.m[1]) ,fpt_mul(a.m[4], b.m[4])), fpt_mul(a.m[5], b.m[7])), 
        fpt_add(fpt_add(fpt_mul(a.m[3], b.m[2]) ,fpt_mul(a.m[4], b.m[5])), fpt_mul(a.m[5], b.m[8])), 

        fpt_add(fpt_add(fpt_mul(a.m[6], b.m[0]) ,fpt_mul(a.m[7], b.m[3])), fpt_mul(a.m[8], b.m[6])), 
        fpt_add(fpt_add(fpt_mul(a.m[6], b.m[1]) ,fpt_mul(a.m[7], b.m[4])), fpt_mul(a.m[8], b.m[7])), 
        fpt_add(fpt_add(fpt_mul(a.m[6], b.m[2]) ,fpt_mul(a.m[7], b.m[5])), fpt_mul(a.m[8], b.m[8])),
    };

}

static inline fpt_flatmat34 fpt_flatmat34_mult(const fpt_flatmat3& a, const fpt_flatmat3& b){
    fpt_flatmat34 result;

    result.m[0]  = fpt_add(fpt_add(fpt_mul(a.m[0], b.m[0]) ,fpt_mul(a.m[4], b.m[1])), fpt_mul(a.m[8] , b.m[2]  ));
    result.m[4]  = fpt_add(fpt_add(fpt_mul(a.m[0], b.m[4]) ,fpt_mul(a.m[4], b.m[5])), fpt_mul(a.m[8] , b.m[6]  ));
    result.m[8]  = fpt_add(fpt_add(fpt_mul(a.m[0], b.m[8]) ,fpt_mul(a.m[4], b.m[9])), fpt_mul(a.m[8] , b.m[10] ));

    result.m[1]  = fpt_add(fpt_add(fpt_mul(a.m[1], b.m[0]) ,fpt_mul(a.m[5], b.m[1])), fpt_mul(a.m[9] , b.m[2]  ));
    result.m[5]  = fpt_add(fpt_add(fpt_mul(a.m[1], b.m[4]) ,fpt_mul(a.m[5], b.m[5])), fpt_mul(a.m[9] , b.m[6]  ));
    result.m[9]  = fpt_add(fpt_add(fpt_mul(a.m[1], b.m[8]) ,fpt_mul(a.m[5], b.m[9])), fpt_mul(a.m[9] , b.m[10] ));
    
    result.m[2]  = fpt_add(fpt_add(fpt_mul(a.m[2], b.m[0]) ,fpt_mul(a.m[6], b.m[1])), fpt_mul(a.m[10], b.m[2]  ));
    result.m[6]  = fpt_add(fpt_add(fpt_mul(a.m[2], b.m[4]) ,fpt_mul(a.m[6], b.m[5])), fpt_mul(a.m[10], b.m[6]  ));
    result.m[10] = fpt_add(fpt_add(fpt_mul(a.m[2], b.m[8]) ,fpt_mul(a.m[6], b.m[9])), fpt_mul(a.m[10], b.m[10] ));
    
    result.m[3]  = fpt_add(fpt_add(fpt_add(fpt_mul(a.m[3], b.m[0]) ,fpt_mul(a.m[7], b.m[1])), fpt_mul(a.m[11], b.m[2]  )), b.m[3]);
    result.m[7]  = fpt_add(fpt_add(fpt_add(fpt_mul(a.m[3], b.m[4]) ,fpt_mul(a.m[7], b.m[5])), fpt_mul(a.m[11], b.m[6]  )), b.m[7]);
    result.m[11] = fpt_add(fpt_add(fpt_add(fpt_mul(a.m[3], b.m[8]) ,fpt_mul(a.m[7], b.m[9])), fpt_mul(a.m[11], b.m[10] )), b.m[11]);

    return result;
}

//sets matrix a to the inverse of matrix b
static inline void fpt_flatmat3_set_inverse(fpt_flatmat3& a, const fpt_flatmat3& b){
    fpt t1 = fpt_mul(b.m[0], b.m[4]);
    fpt t2 = fpt_mul(b.m[0], b.m[5]);
    fpt t3 = fpt_mul(b.m[1], b.m[3]);
    fpt t4 = fpt_mul(b.m[2], b.m[3]);
    fpt t5 = fpt_mul(b.m[1], b.m[6]);
    fpt t6 = fpt_mul(b.m[2], b.m[6]);

    //calculate the determinant
    fpt det = fpt_sub(fpt_add(fpt_add(fpt_sub(fpt_sub(fpt_mul(t1, b.m[8]) , fpt_mul(t2, b.m[7])) , fpt_mul(t3, b.m[8])) , fpt_mul(t4, b.m[7])) , fpt_mul(t5, b.m[5])) , fpt_mul(t6, b.m[4]));

    //make sure the determinant is non zero
    if(det == 0)return;
    fpt invd = fpt_div(FPT_ONE, det);

    a.m[0] =  fpt_mul(fpt_sub(fpt_mul(b.m[4], b.m[8]), fpt_mul(b.m[5], b.m[7])) , invd);
    a.m[1] = -fpt_mul(fpt_sub(fpt_mul(b.m[1], b.m[8]), fpt_mul(b.m[2], b.m[7])) , invd);
    a.m[2] =  fpt_mul(fpt_sub(fpt_mul(b.m[1], b.m[5]), fpt_mul(b.m[2], b.m[4])) , invd);
    a.m[3] = -fpt_mul(fpt_sub(fpt_mul(b.m[3], b.m[8]), fpt_mul(b.m[5], b.m[6])) , invd);
    a.m[4] =  fpt_mul(fpt_sub(fpt_mul(b.m[0], b.m[8]), t6) , invd);
    a.m[5] = -fpt_mul(fpt_sub(t2,t4), invd);
    a.m[6] =  fpt_mul(fpt_sub(fpt_mul(b.m[3], b.m[7]), fpt_mul(b.m[4], b.m[6])) , invd);
    a.m[7] = -fpt_mul(fpt_sub(fpt_mul(b.m[0], b.m[7]), t5) , invd);
    a.m[8] =  fpt_mul(fpt_sub(t1, t3) , invd);


}

static inline fpt fpt_flatmat34_determinant(fpt_flatmat34 m){
    
    return fpt_add(fpt_sub(fpt_sub(fpt_add(fpt_add(
    fpt_mul(fpt_mul(m.m[8],m.m[5]), m.m[2]) , 
    fpt_mul(fpt_mul(m.m[4],m.m[9]), m.m[2])), 
    fpt_mul(fpt_mul(m.m[8],m.m[1]), m.m[6])) ,
    fpt_mul(fpt_mul(m.m[0],m.m[9]), m.m[6])) , 
    fpt_mul(fpt_mul(m.m[4],m.m[1]), m.m[10])) ,
    fpt_mul(fpt_mul(m.m[0],m.m[5]), m.m[10]));
}

//sets matrix a to the inverse of matrix b
static inline void set_inverse(fpt_flatmat34& a, const fpt_flatmat34& b){

    fpt det = fpt_flatmat34_determinant(a);
    if(det == 0)return;
    det = fpt_div(FPT_ONE, det);

    a.m[0] = (fpt_mul(fpt_add(fpt_mul(-b.m[9],b.m[6])    ,   fpt_mul(b.m[5], b.m[10])), det));
    a.m[4] = (fpt_mul(fpt_sub(fpt_mul( b.m[8],b.m[6])    ,   fpt_mul(b.m[4], b.m[10])), det));
    a.m[8] = (fpt_mul(fpt_add(fpt_mul(-b.m[8],b.m[5])    ,   fpt_mul(fpt_mul(b.m[4],b.m[9]),b.m[15])),det));


    a.m[1] = (fpt_mul(fpt_sub(fpt_mul( b.m[9],b.m[2])    ,   fpt_mul(b.m[1], b.m[10])), det));
    a.m[5] = (fpt_mul(fpt_add(fpt_mul(-b.m[8],b.m[2])    ,   fpt_mul(b.m[0], b.m[10])), det));
    a.m[9] = (fpt_mul(fpt_sub(fpt_mul( b.m[8],b.m[1])    ,   fpt_mul(fpt_mul(b.m[0],b.m[9]),b.m[15])),det));


    a.m[2] = (fpt_mul(fpt_add(fpt_mul(-b.m[5],b.m[2])    ,   fpt_mul(fpt_mul(b.m[1],b.m[6]),b.m[15])), det));
    a.m[6] = (fpt_mul(fpt_sub(fpt_mul(+b.m[4],b.m[2])    ,   fpt_mul(fpt_mul(b.m[0],b.m[6]),b.m[15])), det));
    a.m[10]= (fpt_mul(fpt_add(fpt_mul(-b.m[4],b.m[1])    ,   fpt_mul(fpt_mul(b.m[0],b.m[5]),b.m[15])), det));


    a.m[3] =  fpt_mul(
              fpt_sub( fpt_mul(fpt_mul(b.m[9],b.m[6]) , b.m[3] )
    ,         fpt_sub( fpt_mul(fpt_mul(b.m[5],b.m[10]), b.m[3] )
    ,         fpt_add( fpt_mul(fpt_mul(b.m[9],b.m[2]) , b.m[7] )
    ,         fpt_add( fpt_mul(fpt_mul(b.m[1],b.m[10]), b.m[7] )
    ,         fpt_sub( fpt_mul(fpt_mul(b.m[5],b.m[2]) , b.m[11])  
    ,                  fpt_mul(fpt_mul(b.m[1],b.m[6]) , b.m[11]) )))))

    ,det);

    a.m[7] = fpt_mul(
              fpt_add( fpt_mul(fpt_mul(b.m[8],b.m[6]) , b.m[3] )
    ,         fpt_add( fpt_mul(fpt_mul(b.m[4],b.m[10]), b.m[3] )
    ,         fpt_sub( fpt_mul(fpt_mul(b.m[8],b.m[2]) , b.m[7] )
    ,         fpt_sub( fpt_mul(fpt_mul(b.m[0],b.m[10]), b.m[7] )
    ,         fpt_add( fpt_mul(fpt_mul(b.m[4],b.m[2]) , b.m[11])  
    ,                  fpt_mul(fpt_mul(b.m[0],b.m[6]) , b.m[11]) )))))

    ,det);

    a.m[11] = fpt_mul(
              fpt_sub( fpt_mul(fpt_mul(b.m[8],b.m[5]), b.m[3] )
    ,         fpt_sub( fpt_mul(fpt_mul(b.m[4],b.m[9]), b.m[3] )
    ,         fpt_add( fpt_mul(fpt_mul(b.m[8],b.m[1]), b.m[7] )
    ,         fpt_add( fpt_mul(fpt_mul(b.m[0],b.m[9]), b.m[7] )
    ,         fpt_sub( fpt_mul(fpt_mul(b.m[4],b.m[1]), b.m[11])  
    ,                  fpt_mul(fpt_mul(b.m[0],b.m[5]), b.m[11]) )))))

    ,det);

}

static inline void set_transpose(fpt_flatmat3& a, const fpt_flatmat3& b){
    a.m[0] = b.m[0];
    a.m[1] = b.m[3];
    a.m[2] = b.m[6];
    a.m[3] = b.m[1];
    a.m[4] = b.m[4];
    a.m[5] = b.m[7];
    a.m[6] = b.m[2];
    a.m[7] = b.m[5];
    a.m[8] = b.m[8];
}

static inline void set_orientation(fpt_flatmat3& a, const fpt_quat& q){
    a.m[0] = fpt_sub(FPT_ONE ,  fpt_add(fpt_mul(fpt_mul(q.y,q.y),FPT_TWO) , fpt_mul(fpt_mul(q.z,q.z),FPT_TWO)) );
    a.m[1] =                    fpt_add(fpt_mul(fpt_mul(q.x,q.y),FPT_TWO) , fpt_mul(fpt_mul(q.z,q.w),FPT_TWO)) ;
    a.m[2] =                    fpt_sub(fpt_mul(fpt_mul(q.x,q.z),FPT_TWO) , fpt_mul(fpt_mul(q.y,q.w),FPT_TWO)) ;
    a.m[3] =                    fpt_sub(fpt_mul(fpt_mul(q.x,q.y),FPT_TWO) , fpt_mul(fpt_mul(q.z,q.w),FPT_TWO)) ;
    a.m[4] = fpt_sub(FPT_ONE ,  fpt_add(fpt_mul(fpt_mul(q.x,q.x),FPT_TWO) , fpt_mul(fpt_mul(q.z,q.z),FPT_TWO)) );
    a.m[5] =                    fpt_add(fpt_mul(fpt_mul(q.y,q.z),FPT_TWO) , fpt_mul(fpt_mul(q.x,q.w),FPT_TWO)) ;
    a.m[6] =                    fpt_add(fpt_mul(fpt_mul(q.x,q.z),FPT_TWO) , fpt_mul(fpt_mul(q.y,q.w),FPT_TWO)) ;
    a.m[7] =                    fpt_sub(fpt_mul(fpt_mul(q.y,q.z),FPT_TWO) , fpt_mul(fpt_mul(q.x,q.w),FPT_TWO)) ;
    a.m[8] = fpt_sub(FPT_ONE ,  fpt_add(fpt_mul(fpt_mul(q.x,q.x),FPT_TWO) , fpt_mul(fpt_mul(q.y,q.y),FPT_TWO)) );

}


/**
* Sets this matrix to be the rotation matrix corresponding to
* the given quaternion.
*/
static inline void set_orientation_and_pos(fpt_flatmat34& a, const fpt_quat &q, const fpt_vec3 &pos)
{
    a.m[0]  = fpt_sub(FPT_ONE ,fpt_add(fpt_mul(fpt_mul(q.y,q.y), FPT_TWO) , fpt_mul(fpt_mul(q.z,q.z),FPT_TWO)));
    a.m[1]  =                  fpt_add(fpt_mul(fpt_mul(q.x,q.y), FPT_TWO) , fpt_mul(fpt_mul(q.z,q.w),FPT_TWO));
    a.m[2]  =                  fpt_sub(fpt_mul(fpt_mul(q.x,q.z), FPT_TWO) , fpt_mul(fpt_mul(q.y,q.w),FPT_TWO));
    a.m[3]  = pos.x;
    a.m[4]  =                  fpt_sub(fpt_mul(fpt_mul(q.x,q.y), FPT_TWO) , fpt_mul(fpt_mul(q.z,q.w), FPT_TWO));
    a.m[5]  = fpt_sub(FPT_ONE ,fpt_add(fpt_mul(fpt_mul(q.x,q.x), FPT_TWO) , fpt_mul(fpt_mul(q.z,q.z), FPT_TWO)));
    a.m[6]  =                  fpt_add(fpt_mul(fpt_mul(q.y,q.z), FPT_TWO) , fpt_mul(fpt_mul(q.x,q.w), FPT_TWO));
    a.m[7]  = pos.y;
    a.m[8]  =                  fpt_add(fpt_mul(fpt_mul(q.x,q.z), FPT_TWO) , fpt_mul(fpt_mul(q.y,q.w), FPT_TWO));
    a.m[9]  =                  fpt_sub(fpt_mul(fpt_mul(q.y,q.z), FPT_TWO) , fpt_mul(fpt_mul(q.x,q.w), FPT_TWO));
    a.m[10] = fpt_sub(FPT_ONE ,fpt_add(fpt_mul(fpt_mul(q.x,q.x), FPT_TWO) , fpt_mul(fpt_mul(q.y,q.y), FPT_TWO)));
    a.m[11] = pos.z;
}


static inline fpt_vec3 local_to_world(const fpt_vec3& local, const fpt_flatmat34& transform){
    return fpt_vec3_flatmat34_transform(local, transform);
}




static inline fpt_vec3 transform_inverse(const fpt_flatmat34& mat, const fpt_vec3& vec){
    fpt_vec3 tmp = vec;
    tmp.x = fpt_sub(tmp.x,mat.m[3] );
    tmp.y = fpt_sub(tmp.y,mat.m[7] );
    tmp.z = fpt_sub(tmp.z,mat.m[11]);

    return fpt_vec3{
        fpt_add(fpt_add(fpt_mul(tmp.x , mat.m[0]) , fpt_mul(tmp.y , mat.m[4])) , fpt_mul(tmp.z , mat.m[8] )),
        fpt_add(fpt_add(fpt_mul(tmp.x , mat.m[1]) , fpt_mul(tmp.y , mat.m[5])) , fpt_mul(tmp.z , mat.m[9] )),
        fpt_add(fpt_add(fpt_mul(tmp.x , mat.m[2]) , fpt_mul(tmp.y , mat.m[6])) , fpt_mul(tmp.z , mat.m[10])),
    };
}

static inline fpt_vec3 world_to_local( const fpt_flatmat34& transform, const fpt_vec3& world){
    // fpt_flatmat34 inverse_transform = fpt_flatmat34_create();
    // set_inverse(inverse_transform, transform);
    // return fpt_vec3_flatmat34_transform(world, inverse_transform);

    //can be simplified by calling transform_inverse
    return transform_inverse(transform, world);

}

static inline fpt_vec3 transform_direction( const fpt_flatmat34& m, const fpt_vec3& vec){
    return fpt_vec3{
        fpt_add(fpt_add(fpt_mul(vec.x ,m.m[0]) , fpt_mul(vec.y, m.m[1])) , fpt_mul(vec.z , m.m[2] )),
        fpt_add(fpt_add(fpt_mul(vec.x ,m.m[4]) , fpt_mul(vec.y, m.m[5])) , fpt_mul(vec.z , m.m[6] )),
        fpt_add(fpt_add(fpt_mul(vec.x ,m.m[8]) , fpt_mul(vec.y, m.m[9])) , fpt_mul(vec.z , m.m[10])),
    };

}

static inline fpt_vec3 transform_inverse_direction( const fpt_flatmat34& m, const fpt_vec3& vec){
    return fpt_vec3{
        fpt_add(fpt_add(fpt_mul(vec.x ,m.m[0]) , fpt_mul(vec.y, m.m[4])) , fpt_mul(vec.z , m.m[8] )),
        fpt_add(fpt_add(fpt_mul(vec.x ,m.m[1]) , fpt_mul(vec.y, m.m[5])) , fpt_mul(vec.z , m.m[9] )),
        fpt_add(fpt_add(fpt_mul(vec.x ,m.m[2]) , fpt_mul(vec.y, m.m[6])) , fpt_mul(vec.z , m.m[10])),
    };
}

static inline fpt_vec3 fpt_flatmat3_transform_transpose(const fpt_flatmat3& m, const fpt_vec3& v){
    return fpt_vec3{
        fpt_add(fpt_add(fpt_mul(v.x ,m.m[0]) , fpt_mul(v.y, m.m[3])) , fpt_mul(v.z , m.m[6])),
        fpt_add(fpt_add(fpt_mul(v.x ,m.m[1]) , fpt_mul(v.y, m.m[4])) , fpt_mul(v.z , m.m[7])),
        fpt_add(fpt_add(fpt_mul(v.x ,m.m[2]) , fpt_mul(v.y, m.m[5])) , fpt_mul(v.z , m.m[8])),
    };
}

static inline fpt_vec3 local_to_world_dirn(const fpt_vec3& local, const fpt_flatmat34& transform){
    return transform_direction(transform, local);
}

static inline fpt_vec3 world_to_local_dirn(const fpt_vec3& world, const fpt_flatmat34& transform){
    return transform_inverse_direction(transform, world);
}


static inline fpt_quat fpt_quat_rotate_by_vec3(const fpt_quat& q, const fpt_vec3& v){
    fpt_quat quat = {0};

    quat.x = v.x;
    quat.y = v.y;
    quat.z = v.z;
    quat.w = 0;

    return quat;
}

static inline void fpt_quat_add_scaled_vector(fpt_quat& q, const fpt_vec3& v, const fpt& scale){
    fpt_quat temp_quat = {0};
    temp_quat.x = fpt_mul(v.x, scale);
    temp_quat.y = fpt_mul(v.y, scale);
    temp_quat.z = fpt_mul(v.z, scale);
    temp_quat.w = 0;

    // printf("given rotation vector: %3.3f %3.3f %3.3f\n", fpt2fl(v.x),fpt2fl(v.y),fpt2fl(v.z));
    // printf("calculated quat      : %3.3f %3.3f %3.3f %3.3f\n", fpt2fl(temp_quat.x),fpt2fl(temp_quat.y),fpt2fl(temp_quat.z), fpt2fl(temp_quat.w));

    temp_quat = fpt_quat_mul(temp_quat, q);

    // printf("post mult quat       : %3.3f %3.3f %3.3f %3.3f\n", fpt2fl(temp_quat.x),fpt2fl(temp_quat.y),fpt2fl(temp_quat.z), fpt2fl(temp_quat.w));

    q.w = fpt_add(q.w, fpt_mul(temp_quat.w, FPT_HALF));
    q.x = fpt_add(q.x, fpt_mul(temp_quat.x, FPT_HALF));
    q.y = fpt_add(q.y, fpt_mul(temp_quat.y, FPT_HALF));
    q.z = fpt_add(q.z, fpt_mul(temp_quat.z, FPT_HALF));
} 


inline bool operator==(const fpt_quat& lhs, const fpt_quat& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

inline bool operator!=(const fpt_quat& lhs, const fpt_quat& rhs) {
    return !(lhs == rhs);
}


static inline void fpt_flatmat3_set_inertia_tensor_coefficients(fpt_flatmat3& mat, fpt ix, fpt iy, fpt iz, fpt ixy = 0, fpt ixz = 0, fpt iyz = 0){
    mat.m[0] = ix;
    mat.m[1] = mat.m[3] = -ixy;
    mat.m[2] = mat.m[6] = -ixz;
    mat.m[4] = iy;
    mat.m[5] = mat.m[7] = -iyz;
    mat.m[8] = iz;
}

//19661 = 0.3 * 2^16
//sets value of matrix as a rectangular block aligned with body's coordinates
static inline void fpt_flatmat3_set_block_inertia_tensor(fpt_flatmat3& mat, const fpt_vec3& half_sizes, fpt mass){
    fpt_vec3 squares = half_sizes * half_sizes;
    fpt_flatmat3_set_inertia_tensor_coefficients(mat, 
        fpt_mul(19661, fpt_mul(mass , fpt_add(squares.y , squares.z))),
        fpt_mul(19661, fpt_mul(mass , fpt_add(squares.x , squares.z))),
        fpt_mul(19661, fpt_mul(mass , fpt_add(squares.x , squares.y))));
}


// static inline void debug_set_inertia_tensor_coefficients(glm::mat3& mat, float ix, float iy, float iz, float ixy = 0.0f, float ixz = 0.0f, float iyz = 0.0f){
//     mat[0][0] = ix;
//     mat[1][0] = mat[0][1] = -ixy;
//     mat[2][0] = mat[0][2] = -ixz;
//     mat[1][1] = iy;
//     mat[2][1] = mat[1][2] = -iyz;
//     mat[2][2] = iz;
// }
// static inline void debug_setBlockInertiaTensor(glm::mat3& mat, const glm::vec3 &halfSizes, float mass)
// {
//     glm::vec3 squares = halfSizes * halfSizes;
//     debug_set_inertia_tensor_coefficients(mat,
//         0.3f*mass*(squares.y + squares.z),
//         0.3f*mass*(squares.x + squares.z),
//         0.3f*mass*(squares.x + squares.y));
// }


static inline void fpt_flatmat3_set_diagonal(fpt_flatmat3& mat, fpt a, fpt b, fpt c){
        fpt_flatmat3_set_inertia_tensor_coefficients(mat, a, b, c);
}

//sets left to right
static inline void fpt_set_flatmat3(fpt_flatmat3& lhs, const fpt_flatmat3& rhs) {
    lhs.m[0] = rhs.m[0];
    lhs.m[1] = rhs.m[1];
    lhs.m[2] = rhs.m[2];
    lhs.m[3] = rhs.m[3];
    lhs.m[4] = rhs.m[4];
    lhs.m[5] = rhs.m[5];
    lhs.m[6] = rhs.m[6];
    lhs.m[7] = rhs.m[7];
    lhs.m[8] = rhs.m[8];
}


static inline void fpt_set_flatmat34(fpt_flatmat34& lhs, const fpt_flatmat34& rhs) {
    lhs.m[0]  = rhs.m[0];
    lhs.m[1]  = rhs.m[1];
    lhs.m[2]  = rhs.m[2];
    lhs.m[3]  = rhs.m[3];
    lhs.m[4]  = rhs.m[4];
    lhs.m[5]  = rhs.m[5];
    lhs.m[6]  = rhs.m[6];
    lhs.m[7]  = rhs.m[7];
    lhs.m[8]  = rhs.m[8];
    lhs.m[9]  = rhs.m[9];
    lhs.m[10] = rhs.m[10];
    lhs.m[11] = rhs.m[11];
}

/// @brief represents one axis/column of the matrix. row 3 corresponds to the position of the transform matrix
/// @param m fpt_flatmat34 (3x4 fixed point matrix)
/// @param i the row to return
/// @return fpt_vec3 
static inline fpt_vec3 fpt_flatmat34_get_axis(const fpt_flatmat34& m, int i){
    // /*TAG*/printf("get axis: %3.3f %3.3f %3.3f\n", fpt2fl(m.m[i]), fpt2fl(m.m[i+4]), fpt2fl(m.m[i+8]));
    return fpt_vec3_create(m.m[i], m.m[i+4], m.m[i+8]);
}

static inline void print_fpt_flatmat34(const fpt_flatmat34& m){
    printf("%10.5f %10.5f %10.5f %10.5f\n", fpt2fl(m.m[0]), fpt2fl(m.m[1]), fpt2fl(m.m[2] ), fpt2fl(m.m[3] ));
    printf("%10.5f %10.5f %10.5f %10.5f\n", fpt2fl(m.m[4]), fpt2fl(m.m[5]), fpt2fl(m.m[6] ), fpt2fl(m.m[7] ));
    printf("%10.5f %10.5f %10.5f %10.5f\n", fpt2fl(m.m[8]), fpt2fl(m.m[9]), fpt2fl(m.m[10]), fpt2fl(m.m[11]));
}

static inline void print_fpt_flatmat3(const char* string, const fpt_flatmat3& m){
    printf("%s\n", string);
    printf("%10.5f %10.5f %10.5f\n", fpt2fl(m.m[0]), fpt2fl(m.m[1]), fpt2fl(m.m[2] ));
    printf("%10.5f %10.5f %10.5f\n", fpt2fl(m.m[3]), fpt2fl(m.m[4]), fpt2fl(m.m[5] ));
    printf("%10.5f %10.5f %10.5f\n", fpt2fl(m.m[6]), fpt2fl(m.m[7]), fpt2fl(m.m[8]));
}


static inline void print_fpt_quat(const char* string, const fpt_quat& q){
    printf("%s %10.5f %10.5f %10.5f %10.5f\n", string, fpt2fl(q.x), fpt2fl(q.y), fpt2fl(q.z), fpt2fl(q.w));
}

static inline void print_fpt_vec3(const char* string, const fpt_vec3& v){
    printf("%s %10.5f %10.5f %10.5f\n", string, fpt2fl(v.x),fpt2fl(v.y),fpt2fl(v.z));
}

static inline void print_fpt(const char* string, const fpt a){
    printf("%s %10.5f\n", string, fpt2fl(a));
}

// //too lazy to go back to the other way after copying code
// static inline void print_glm_vec3(const char* string, const glm::vec3& v){
//     printf("%s %10.5f %10.5f %10.5f\n", string, (v.x),(v.y),(v.z));
// }

static inline void print_float(const char* string, const float a){
    printf("%s %10.5f\n", string, (a));
}

static inline void fpt_flatmat3_set_components(fpt_flatmat3& m, fpt_vec3& a, fpt_vec3& b, fpt_vec3& c){
    m.m[0] = a.x; 
    m.m[1] = b.x;
    m.m[2] = c.x;

    m.m[3] = a.y;
    m.m[4] = b.y;
    m.m[5] = c.y;

    m.m[6] = a.z;
    m.m[7] = b.z;
    m.m[8] = c.z;
}


static inline fpt_flatmat34 fpt_flatmat34_with_flatmat3_vec3(fpt_flatmat3& m, fpt_vec3& pos){
    fpt_flatmat34 mat;
    mat.m[0]  = m.m[0];
    mat.m[1]  = m.m[1];
    mat.m[2]  = m.m[2];
    mat.m[3]  = pos.x;
    mat.m[4]  = m.m[3];
    mat.m[5]  = m.m[4];
    mat.m[6]  = m.m[5];
    mat.m[7]  = pos.y;
    mat.m[8]  = m.m[6];
    mat.m[9]  = m.m[7];
    mat.m[10] = m.m[8];
    mat.m[11] = pos.z;
    return mat;
}


// static inline glm::mat4 fpt_to_glm_flatmat34(const fpt_flatmat34& m) {
//     glm::mat4 mat;
//     // First column
//     mat[0][0] = fpt2fl(m.m[0]);
//     mat[1][0] = fpt2fl(m.m[4]);
//     mat[2][0] = fpt2fl(m.m[8]);
//     mat[3][0] = 0;

//     // Second column
//     mat[0][1] = fpt2fl(m.m[1]);
//     mat[1][1] = fpt2fl(m.m[5]);
//     mat[2][1] = fpt2fl(m.m[9]);
//     mat[3][1] = 0;

//     // Third column
//     mat[0][2] = fpt2fl(m.m[2]);
//     mat[1][2] = fpt2fl(m.m[6]);
//     mat[2][2] = fpt2fl(m.m[10]);
//     mat[3][2] = 0;

//     // Fourth column (translation)
//     mat[0][3] = fpt2fl(m.m[3]);
//     mat[1][3] = fpt2fl(m.m[7]);
//     mat[2][3] = fpt2fl(m.m[11]);
//     mat[3][3] = 1;

//     return mat;
// }


static inline bool fpt_check(fpt fpt, float fl, const char* string = ""){
    float discrepancy = 0.05f;
    bool pass = true;
    float diff = fabs(fpt2fl(fpt) - fl);
    if(diff > discrepancy){
        pass = false;
    }
    if(!pass){
        printf("WRONG VALUE IN FPT %s!\n", string);
        printf("fpt  : %6.2f\n",fpt2fl(fpt));
        printf("fl   : %6.2f\n",fl);
        printf("diff : %6.2f\n",diff);
    }
    return pass;
}

// static inline bool fpt_vec3_check(fpt_vec3& fpt, glm::vec3 fl, const char* string = ""){
//     float discrepancy = 0.05f;
//     bool pass = true;
// glm::vec3 diff = glm::vec3(0.0f);
//     diff.x = glm::abs(fpt2fl(fpt.x) - fl.x);
//     diff.y = glm::abs(fpt2fl(fpt.y) - fl.y);
//     diff.z = glm::abs(fpt2fl(fpt.z) - fl.z);
//     if(diff.x > discrepancy){
//         pass = false;
//     }
//     if(diff.y > discrepancy){
//         pass = false;
//     }

//     if(diff.z > discrepancy){
//         pass = false;
//     }
//     if(!pass){
//         printf("WRONG VALUE IN VECTOR %s!\n", string);
//         printf("fpt  : %6.2f %6.2f %6.2f\n",fpt2fl(fpt.x), fpt2fl(fpt.y), fpt2fl(fpt.z));
//         printf("fl   : %6.2f %6.2f %6.2f\n",fl.x, fl.y, fl.z);
//         printf("diff : %6.2f %6.2f %6.2f\n",diff.x, diff.y, diff.z);
//     }
//     return pass;

// }



// static inline bool fpt_flatmat3_check(fpt_flatmat3& fpt, glm::mat3 fl, const char* string = ""){
//     float discrepancy = 0.05f;
//     // float discrepancy = 0.01f;
//     bool pass = true;

//     glm::mat3 diff;

//     diff[0][0] = glm::abs(fl[0][0] - fpt2fl(fpt.m[0]));
//     diff[0][1] = glm::abs(fl[0][1] - fpt2fl(fpt.m[3]));
//     diff[0][2] = glm::abs(fl[0][2] - fpt2fl(fpt.m[6]));

//     // Second column
//     diff[1][0] = glm::abs(fl[1][0] - fpt2fl(fpt.m[1]));
//     diff[1][1] = glm::abs(fl[1][1] - fpt2fl(fpt.m[4]));
//     diff[1][2] = glm::abs(fl[1][2] - fpt2fl(fpt.m[7]));

//     // Third column
//     diff[2][0] = glm::abs(fl[2][0] - fpt2fl(fpt.m[2]));
//     diff[2][1] = glm::abs(fl[2][1] - fpt2fl(fpt.m[5]));
//     diff[2][2] = glm::abs(fl[2][2] - fpt2fl(fpt.m[8]));

//     if( diff[0][0] > discrepancy ||
//         diff[1][0] > discrepancy ||
//         diff[2][0] > discrepancy ||
//         diff[0][1] > discrepancy ||
//         diff[1][1] > discrepancy ||
//         diff[2][1] > discrepancy ||
//         diff[0][2] > discrepancy ||
//         diff[1][2] > discrepancy ||
//         diff[2][2] > discrepancy ){
//         pass = false;
//     }

//     if(!pass){
//         printf("WRONG VALUE IN MAT3! %s\n", string);
//         printf("fpt  : %6.2f %6.2f %6.2f\n",fpt2fl(fpt.m[0]), fpt2fl(fpt.m[1]), fpt2fl(fpt.m[2]));
//         printf("fpt  : %6.2f %6.2f %6.2f\n",fpt2fl(fpt.m[3]), fpt2fl(fpt.m[4]), fpt2fl(fpt.m[5]));
//         printf("fpt  : %6.2f %6.2f %6.2f\n",fpt2fl(fpt.m[6]), fpt2fl(fpt.m[7]), fpt2fl(fpt.m[8]));

//         printf("fl   : %6.2f %6.2f %6.2f\n",fl[0][0], fl[1][0], fl[2][0]);
//         printf("fl   : %6.2f %6.2f %6.2f\n",fl[0][1], fl[1][1], fl[2][1]);
//         printf("fl   : %6.2f %6.2f %6.2f\n",fl[0][2], fl[1][2], fl[2][2]);
        
//         printf("diff : %6.2f %6.2f %6.2f\n",diff[0][0], diff[1][0], diff[2][0]);
//         printf("diff : %6.2f %6.2f %6.2f\n",diff[0][1], diff[1][1], diff[2][1]);
//         printf("diff : %6.2f %6.2f %6.2f\n",diff[0][2], diff[1][2], diff[2][2]);
//     }
//     return pass;

// }


// static inline bool fpt_flatmat34_check(const fpt_flatmat34& fpt, glm::mat4 fl, const char* string = ""){
//     float discrepancy = 0.05f;
//     // float discrepancy = 0.01f;
//     bool pass = true;

//     glm::mat4 diff;

//     diff[0][0] = glm::abs(fl[0][0] - fpt2fl(fpt.m[0]));
//     diff[0][1] = glm::abs(fl[0][1] - fpt2fl(fpt.m[4]));
//     diff[0][2] = glm::abs(fl[0][2] - fpt2fl(fpt.m[8]));

//     // Second column
//     diff[1][0] = glm::abs(fl[1][0] - fpt2fl(fpt.m[1]));
//     diff[1][1] = glm::abs(fl[1][1] - fpt2fl(fpt.m[5]));
//     diff[1][2] = glm::abs(fl[1][2] - fpt2fl(fpt.m[9]));

//     // Third column
//     diff[2][0] = glm::abs(fl[2][0] - fpt2fl(fpt.m[2]));
//     diff[2][1] = glm::abs(fl[2][1] - fpt2fl(fpt.m[6]));
//     diff[2][2] = glm::abs(fl[2][2] - fpt2fl(fpt.m[10]));

//     //fourth column
//     diff[3][0] = glm::abs(fl[3][0] - fpt2fl(fpt.m[3]));
//     diff[3][1] = glm::abs(fl[3][1] - fpt2fl(fpt.m[7]));
//     diff[3][2] = glm::abs(fl[3][2] - fpt2fl(fpt.m[11]));

//     if( diff[0][0] > discrepancy ||
//         diff[1][0] > discrepancy ||
//         diff[2][0] > discrepancy ||
//         diff[0][1] > discrepancy ||
//         diff[1][1] > discrepancy ||
//         diff[2][1] > discrepancy ||
//         diff[0][2] > discrepancy ||
//         diff[1][2] > discrepancy ||
//         diff[1][2] > discrepancy ||
//         diff[1][3] > discrepancy ||
//         diff[1][3] > discrepancy ||
//         diff[2][3] > discrepancy ){
//         pass = false;
//     }

//     if(!pass){
//         printf("WRONG VALUE IN MAT4 %s!\n", string);
//         printf("fpt  : %6.2f %6.2f %6.2f %6.2f\n",fpt2fl(fpt.m[0]), fpt2fl(fpt.m[1]), fpt2fl(fpt.m[2]), fpt2fl(fpt.m[3]));
//         printf("fpt  : %6.2f %6.2f %6.2f %6.2f\n",fpt2fl(fpt.m[4]), fpt2fl(fpt.m[5]), fpt2fl(fpt.m[6]), fpt2fl(fpt.m[7]));
//         printf("fpt  : %6.2f %6.2f %6.2f %6.2f\n",fpt2fl(fpt.m[8]), fpt2fl(fpt.m[9]), fpt2fl(fpt.m[10]),fpt2fl(fpt.m[11]));

//         printf("fl   : %6.2f %6.2f %6.2f %6.2f\n",fl[0][0], fl[1][0], fl[2][0], fl[3][0]);
//         printf("fl   : %6.2f %6.2f %6.2f %6.2f\n",fl[0][1], fl[1][1], fl[2][1], fl[3][1]);
//         printf("fl   : %6.2f %6.2f %6.2f %6.2f\n",fl[0][2], fl[1][2], fl[2][2], fl[3][2]);
        
//         printf("diff : %6.2f %6.2f %6.2f %6.2f\n",diff[0][0], diff[1][0], diff[2][0], diff[3][0]);
//         printf("diff : %6.2f %6.2f %6.2f %6.2f\n",diff[0][1], diff[1][1], diff[2][1], diff[3][1]);
//         printf("diff : %6.2f %6.2f %6.2f %6.2f\n",diff[0][2], diff[1][2], diff[2][2], diff[3][2]);
//     }
//     return pass;

    
// }

// static inline glm::mat3 fpt_to_glm_mat3(fpt_flatmat3& m){
//     glm::mat3 mat;
//     mat[0][0] = fpt2fl(m.m[0]);
//     mat[0][1] = fpt2fl(m.m[3]);
//     mat[0][2] = fpt2fl(m.m[6]);

//     // Second column
//     mat[1][0] = fpt2fl(m.m[1]);
//     mat[1][1] = fpt2fl(m.m[4]);
//     mat[1][2] = fpt2fl(m.m[7]);

//     // Third column
//     mat[2][0] = fpt2fl(m.m[2]);
//     mat[2][1] = fpt2fl(m.m[5]);
//     mat[2][2] = fpt2fl(m.m[8]);
//     return mat;
// }




// static inline glm::mat4 fpt_to_glm_mat4(fpt_flatmat34& m){
//     glm::mat4 mat;
//     mat[0][0] = fpt2fl(m.m[0]);
//     mat[0][1] = fpt2fl(m.m[4]);
//     mat[0][2] = fpt2fl(m.m[8]);
//     mat[1][0] = fpt2fl(m.m[1]);
//     mat[1][1] = fpt2fl(m.m[5]);
//     mat[1][2] = fpt2fl(m.m[9]);
//     mat[2][0] = fpt2fl(m.m[2]);
//     mat[2][1] = fpt2fl(m.m[6]);
//     mat[2][2] = fpt2fl(m.m[10]);
//     mat[3][0] = fpt2fl(m.m[3]);
//     mat[3][1] = fpt2fl(m.m[7]);
//     mat[3][2] = fpt2fl(m.m[11]);
//     return mat;
// }

static inline void fpt_flatmat3_set_skew_symmetric(fpt_flatmat3& m, const fpt_vec3& v){
    m.m[0] =  0;
    m.m[1] = -v.z;
    m.m[2] =  v.y;
    m.m[3] =  v.z;
    m.m[4] =  0;
    m.m[5] = -v.x;
    m.m[6] = -v.y;
    m.m[7] =  v.x;
    m.m[8] =  0;
}

static inline fpt_flatmat3 fpt_flatmat3_mul_flatmat3(fpt_flatmat3& a, const fpt_flatmat3& b){
    fpt_flatmat3 mat;
    mat.m[0] = fpt_add(fpt_add(fpt_mul(a.m[0],b.m[0]) , fpt_mul(a.m[1],b.m[3])) , fpt_mul(a.m[2],b.m[6]));
    mat.m[1] = fpt_add(fpt_add(fpt_mul(a.m[0],b.m[1]) , fpt_mul(a.m[1],b.m[4])) , fpt_mul(a.m[2],b.m[7]));
    mat.m[2] = fpt_add(fpt_add(fpt_mul(a.m[0],b.m[2]) , fpt_mul(a.m[1],b.m[5])) , fpt_mul(a.m[2],b.m[8]));
    mat.m[3] = fpt_add(fpt_add(fpt_mul(a.m[3],b.m[0]) , fpt_mul(a.m[4],b.m[3])) , fpt_mul(a.m[5],b.m[6]));
    mat.m[4] = fpt_add(fpt_add(fpt_mul(a.m[3],b.m[1]) , fpt_mul(a.m[4],b.m[4])) , fpt_mul(a.m[5],b.m[7]));
    mat.m[5] = fpt_add(fpt_add(fpt_mul(a.m[3],b.m[2]) , fpt_mul(a.m[4],b.m[5])) , fpt_mul(a.m[5],b.m[8]));
    mat.m[6] = fpt_add(fpt_add(fpt_mul(a.m[6],b.m[0]) , fpt_mul(a.m[7],b.m[3])) , fpt_mul(a.m[8],b.m[6]));
    mat.m[7] = fpt_add(fpt_add(fpt_mul(a.m[6],b.m[1]) , fpt_mul(a.m[7],b.m[4])) , fpt_mul(a.m[8],b.m[7]));
    mat.m[8] = fpt_add(fpt_add(fpt_mul(a.m[6],b.m[2]) , fpt_mul(a.m[7],b.m[5])) , fpt_mul(a.m[8],b.m[8]));
    return mat;
}


static inline void fpt_flatmat3_mul_scalar(fpt_flatmat3& a, const fpt& b){
    a.m[0] = fpt_mul(a.m[0], b);
    a.m[1] = fpt_mul(a.m[1], b);
    a.m[2] = fpt_mul(a.m[2], b);
    a.m[3] = fpt_mul(a.m[3], b);
    a.m[4] = fpt_mul(a.m[4], b);
    a.m[5] = fpt_mul(a.m[5], b);
    a.m[6] = fpt_mul(a.m[6], b);
    a.m[7] = fpt_mul(a.m[7], b);
    a.m[8] = fpt_mul(a.m[8], b);
}

static inline void fpt_flatmat3_add_flatmat3(fpt_flatmat3& a, const fpt_flatmat3& b){
    a.m[0] = fpt_add(a.m[0], b.m[0]);     
    a.m[1] = fpt_add(a.m[1], b.m[1]);     
    a.m[2] = fpt_add(a.m[2], b.m[2]);     
    a.m[3] = fpt_add(a.m[3], b.m[3]);     
    a.m[4] = fpt_add(a.m[4], b.m[4]);     
    a.m[5] = fpt_add(a.m[5], b.m[5]);     
    a.m[6] = fpt_add(a.m[6], b.m[6]);     
    a.m[7] = fpt_add(a.m[7], b.m[7]);     
    a.m[8] = fpt_add(a.m[8], b.m[8]);     
}

static inline fpt_flatmat3 fpt_flatmat3_transpose(const fpt_flatmat3& a){
    fpt_flatmat3 m;
    m.m[0] = a.m[0];
    m.m[1] = a.m[3];
    m.m[2] = a.m[6];
    m.m[3] = a.m[1];
    m.m[4] = a.m[4];
    m.m[5] = a.m[7];
    m.m[6] = a.m[2];
    m.m[7] = a.m[5];
    m.m[8] = a.m[8];
    return m;
}


#include "constants.h"

static inline fpt adjustToChunkRange(fpt value) {
    value = fpt_mod(value + FPT_HALF_CHUNK_SIZE, FPT_CHUNK_SIZE) - FPT_HALF_CHUNK_SIZE;
    return value;
}

static inline float debug_adjustToChunkRange(float value) {
    // printf("before value: %f\n", value);
    value = fmod(value + HALF_CHUNK_SIZE, CHUNK_SIZE);
    if (value < 0) value += CHUNK_SIZE; // Ensure value is in [0, CHUNK_SIZE)
    value -= HALF_CHUNK_SIZE; // Shift back to [-HALF_CHUNK_SIZE, HALF_CHUNK_SIZE)
    // printf("after value: %f\n", value);
    return value;
}



// More general rotation without acos
static inline fpt_quat rotation_between_vectors(fpt_vec3 from, fpt_vec3 to) {
    fpt_quat result;
    
    // Normalize inputs
    from = fpt_vec3_normalize(from);
    to = fpt_vec3_normalize(to);
    
    // Calculate half vector
    fpt_vec3 half = {
        fpt_add(from.x, to.x),
        fpt_add(from.y, to.y),
        fpt_add(from.z, to.z)
    };
    // Normalize half
    half = fpt_vec3_normalize(half);
    
    // Cross product
    result.x = fpt_sub(fpt_mul(from.y, half.z), fpt_mul(from.z, half.y));
    result.y = fpt_sub(fpt_mul(from.z, half.x), fpt_mul(from.x, half.z));
    result.z = fpt_sub(fpt_mul(from.x, half.y), fpt_mul(from.y, half.x));
    
    // W component is dot product of from and half
    result.w = fpt_add(fpt_mul(from.x, half.x), fpt_add(fpt_mul(from.y, half.y), fpt_mul(from.z, half.z)));
    
    return result;
}


//normalized linear interpolation
static inline fpt_quat fpt_nlerp(fpt_quat q1, fpt_quat q2, fpt t) {
    fpt_quat result;
    
    // Step 1: Calculate dot product to determine if we need to flip
    fpt dot = fpt_add(fpt_add(fpt_mul(q1.x, q2.x), fpt_mul(q1.y, q2.y)), 
                      fpt_add(fpt_mul(q1.z, q2.z), fpt_mul(q1.w, q2.w)));
    
    // Step 2: Ensure shortest path by flipping the second quaternion if needed
    if (dot < 0) {
        q2.x = -q2.x;
        q2.y = -q2.y;
        q2.z = -q2.z;
        q2.w = -q2.w;
    }
    
    // Step 3: Simple linear interpolation between components
    fpt one_minus_t = fpt_sub(FPT_ONE, t);
    result.x = fpt_add(fpt_mul(one_minus_t, q1.x), fpt_mul(t, q2.x));
    result.y = fpt_add(fpt_mul(one_minus_t, q1.y), fpt_mul(t, q2.y));
    result.z = fpt_add(fpt_mul(one_minus_t, q1.z), fpt_mul(t, q2.z));
    result.w = fpt_add(fpt_mul(one_minus_t, q1.w), fpt_mul(t, q2.w));
    
    // Step 4: Normalize the result to ensure it's a valid quaternion
    fpt inv_len = fpt_sqrt(fpt_add  (fpt_add(fpt_mul(result.x, result.x), fpt_mul(result.y, result.y)),
                                     fpt_add(fpt_mul(result.z, result.z), fpt_mul(result.w, result.w))));
    result.x = fpt_mul(result.x, inv_len);
    result.y = fpt_mul(result.y, inv_len);
    result.z = fpt_mul(result.z, inv_len);
    result.w = fpt_mul(result.w, inv_len);
    
    return result;
}



static inline fpt_quat fpt_nlerp_y_axis(fpt_quat q1, fpt_quat q2, fpt t) {
    fpt_quat result = {0, 0, 0, FPT_ONE}; // Start with identity
    
    // print_fpt_quat("current rotation: ", q1);
    // print_fpt_quat("dest    rotation: ", q2);
    // print_fpt(     "timestep        : ", t);

    // Step 1: Calculate dot product (simplified for Y-axis rotation)
    fpt dot = fpt_add(fpt_mul(q1.y, q2.y), fpt_mul(q1.w, q2.w));
    // print_fpt(     "dot             : ", dot);
    
    // Step 2: Ensure shortest path
    if (dot < 0) {
        q2.y = -q2.y;
        q2.w = -q2.w;
    }
    
    // Step 3: Linear interpolation (only Y and W components)
    fpt one_minus_t = fpt_sub(FPT_ONE, t);
    // print_fpt(     "one_minus_t     : ", one_minus_t);

    result.y = fpt_add(fpt_mul(one_minus_t, q1.y), fpt_mul(t, q2.y));
    result.w = fpt_add(fpt_mul(one_minus_t, q1.w), fpt_mul(t, q2.w));

    // print_fpt(     "result.y        : ", result.y);
    // print_fpt(     "result.w        : ", result.w);
    
    // Step 4: Normalize (simplified for Y-axis rotation)
    fpt inv_len = fpt_sqrt(fpt_add(fpt_mul(result.y, result.y), fpt_mul(result.w, result.w)));
    // print_fpt(     "inv_len         : ", inv_len);
    
    result.y = fpt_mul(result.y, inv_len);
    result.w = fpt_mul(result.w, inv_len);
    
    // print_fpt(     "result.y        : ", result.y);
    // print_fpt(     "result.w        : ", result.w);

    result = fpt_quat_normalize(result);

    return result;
}


static inline fpt_quat rotate_in_direction_of_movement_y(fpt_vec3& direction, fpt_quat& rotation){
    fpt_quat result = fpt_quat_create(0,0,0,FPT_ONE);
    //for floating point/render only rotation
    if(fpt_abs(direction.x) > FPT_THOUSANDTH || fpt_abs(direction.z) > FPT_THOUSANDTH){
    //     glm::vec3 direction = glm::vec3(fpt2fl(transComp.desired_movement.x), 0,fpt2fl(transComp.desired_movement.z)); 

    //     // Since model points in +Z axis by default, use that as reference
    //     glm::vec3 modelForward = glm::vec3(0.0f, 0.0f, 1.0f);

    //     // Find rotation between model's forward direction and desired direction
    //     float angle = glm::acos(glm::dot(modelForward, direction));
    //     glm::vec3 rotationAxis = glm::cross(modelForward, direction);
    //     if (glm::length(rotationAxis) > 0.0001f) {  // Check if vectors aren't parallel
    //         rotationAxis = glm::normalize(rotationAxis);
    //         entity_rotation = glm::angleAxis(angle, rotationAxis);
    //     }

        //faster fixed point method?
        fpt x = direction.x;
        fpt z = direction.z;

        fpt len = fpt_sqrt(fpt_add(fpt_mul(x,x),fpt_mul(z,z)));
        
        if(len == 0)return rotation;

        x = fpt_div(x, len);
        z = fpt_div(z, len);
        // Shortcut for quaternion that rotates from (0,0,1) to (x,0,z)
        // This avoids all trigonometric functions

        // The quaternion will have form (x*sin(θ/2), y*sin(θ/2), z*sin(θ/2), cos(θ/2))
        // For Y-axis rotation: (0, sin(θ/2), 0, cos(θ/2))

        // Calculate the half-angle directly from the dot product
        fpt dot = z;  // dot((0,0,1), (x,0,z)) = z

        // cos(θ/2) can be calculated as sqrt((1 + dot)/2)
        fpt w = fpt_sqrt(fpt_div(fpt_add(FPT_ONE, dot), FPT_TWO));

        // sin(θ/2) can be calculated as sqrt((1 - dot)/2)
        fpt sin_half = fpt_sqrt(fpt_div(fpt_sub(FPT_ONE, dot), FPT_TWO));

        // The sign of y component depends on which way we're rotating
        fpt y = (x < 0) ? -sin_half : sin_half;

        result.w = w;
        result.y = y;
        return result;
    }
    return rotation;
}
                      


// // Correct quaternion vector rotation
// static inline fpt_vec3 fpt_quat_rotate_vec3(const fpt_quat& q, const fpt_vec3& v) {
//     // Normalized quaternion is crucial for accurate rotation
//     fpt_quat normalized_q = fpt_quat_normalize(q);
    
//     // Quaternion rotation formula: v' = q * v * q^-1
//     // Where v is converted to a pure quaternion
//     fpt_quat v_quat = fpt_vec3_to_quat(v);
    
//     // Multiplication order is critical
//     fpt_quat rotated_v_quat = fpt_quat_mul(
//         fpt_quat_mul(normalized_q, v_quat), 
//         fpt_quat_conjugate(normalized_q)
//     );
    
//     return fpt_vec3_create(rotated_v_quat.x, rotated_v_quat.y, rotated_v_quat.z);
// }

// // Extracting forward vector
// static inline fpt_vec3 fpt_get_forward_from_quat(const fpt_quat& rotation) {
//     // Default forward vector (+Z in this case)
//     fpt_vec3 forward = fpt_vec3_create(0, 0, FPT_ONE);
    
//     // Rotate the forward vector
//     return fpt_quat_rotate_vec3(rotation, forward);
// }


static inline fpt_vec3 fpt_get_forward_matrix_method(const fpt_quat& q) {
    // Convert quaternion to rotation matrix elements
    fpt w = q.w;
    fpt x = q.x;
    fpt y = q.y;
    fpt z = q.z;
    
    // Third column of rotation matrix represents forward direction
    fpt_vec3 forward = fpt_vec3_create(
        fpt_mul(FPT_TWO , fpt_add(fpt_mul(x , z) , fpt_mul(w , y))),               // x component
        fpt_mul(FPT_TWO , fpt_sub(fpt_mul(y , z) , fpt_mul(w , x))),               // y component
        fpt_sub(FPT_ONE , fpt_mul(FPT_TWO , fpt_add(fpt_mul(x , x) , fpt_mul(y , y))))      // z component (scaled)
    );
    
    return forward;
}


#endif FPTVEC_H