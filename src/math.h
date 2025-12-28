#ifndef MATH_H
#define MATH_H




#define handmade_max(a,b)            (((a) > (b)) ? (a) : (b))
#define handmade_min(a,b)            (((a) < (b)) ? (a) : (b))

static inline float fclamp(float d, float min, float max) {
    const float t = d < min ? min : d;
    return t > max ? max : t;
}

static inline int iclamp(int d, int min, int max) {
    const int t = d < min ? min : d;
    return t > max ? max : t;
}

static inline uint32_t uclamp(uint32_t d, uint32_t min, uint32_t max) {
    const uint32_t t = d < min ? min : d;
    return t > max ? max : t;
}






// Clamp to [-PI, PI]
static float wrap_pi(float x) {
    const float pi = 3.14159265f;
    const float two_pi = 6.28318530f;
    while (x > pi) x -= two_pi;
    while (x < -pi) x += two_pi;
    return x;
}


static inline float fast_acos(float x) { //max error of 5 degrees
   return ((-0.798325*x*x-0.686357)*x+1.570796);
}

static inline float fast_sinf(float x) {
    x = wrap_pi(x);
    const float B = 4.0f / 3.14159265f;
    const float C = -4.0f / (3.14159265f * 3.14159265f);
    float y = B * x + C * x * fabsf(x);
    // 0.225 correction for better accuracy
    const float P = 0.225f;
    y = P * (y * fabsf(y) - y) + y;
    return y;
}


static inline float fast_cosf(float x) {
    return fast_sinf(x + 1.57079632f); // pi/2
}



static inline float fast_tanf(float x) {
    float s = fast_sinf(x);
    float c = fast_cosf(x);
    if (fabsf(c) < 1e-4f) return 1e10f; // infinity guard
    return s / c;
}


static inline float fast_sqrtf(float number) {
    if (number <= 0.0f) return 0.0f;

    float x = number;
    float xhalf = 0.5f * x;
    int i = *(int*)&x;               
    i = 0x5f3759df - (i >> 1);       
    x = *(float*)&i;
    x = x * (1.5f - xhalf * x * x);  // 1st iteration
    x = x * (1.5f - xhalf * x * x);  // 2nd iteration
    return number * x;
}


struct vec2{
    union{
        struct{
            float x, y;
        };
        float e[2];
    };
};


struct uvec2{
    union{
        struct{
            uint32_t x, y;
        };
        uint32_t e[2];
    };
};

struct ivec2{
    union{
        struct{
            int32_t x, y;
        };
        int32_t e[2];
    };
};

struct vec3{
    union{
        struct{
            float x, y, z, pad0;
        };
        float e[4];
    };

};

struct ivec3{
    union{
        struct{
            int32_t x, y, z, pad0;
        };
        int32_t e[4];
    };
};

struct uvec3{
    union{
        struct{
            uint32_t x, y, z, pad0;
        };
        uint32_t e[4];
    };
};


struct vec4{
    union{
        struct{
            float x, y, z, w;
        };
        float e[4];
    };

};

struct uvec4{
    union{
        struct{
            uint32_t x, y, z, w;
        };
        uint32_t e[4];
    };

};


struct mat3{
    float m[9];
};


struct mat34{
    float m[12];
};

// 1   0   0   10  x
// 0   1   0  -20  y
// 0   0   1   30  z

// 0   0   0   1

// SSE

// _mm_128

// 128 bits
// 128 / 32 = 4


struct mat4{
    float m[16];
};


//SSE INTEL CODE
#include <xmmintrin.h>

//4x FLOAT
struct f32_4x{
    union{
        __m128 sse;
        float e[4];
    };
};

inline f32_4x operator+(f32_4x a, f32_4x b){
    f32_4x result = {_mm_add_ps(a.sse, b.sse)};
    return result;
}
inline f32_4x operator-(f32_4x a, f32_4x b){
    f32_4x result = {_mm_sub_ps(a.sse, b.sse)};
    return result;
}
inline f32_4x operator*(f32_4x a, f32_4x b){
    f32_4x result = {_mm_mul_ps(a.sse, b.sse)};
    return result;
}
inline f32_4x lerp4x(f32_4x t, f32_4x a, f32_4x b){
    f32_4x result = a + t * (b - a);
    return result;
}
inline f32_4x noiseFade4x(f32_4x t){
    __m128 t2    = _mm_mul_ps(t.sse,t.sse);
    __m128 t3    = _mm_mul_ps(t2,t.sse);
    __m128 inner = _mm_add_ps(_mm_mul_ps(t.sse, _mm_sub_ps(_mm_mul_ps(t.sse, _mm_set1_ps(6.0f)), _mm_set1_ps(15.0f))), _mm_set1_ps(10.0f));
    f32_4x result = {(_mm_mul_ps(t3, inner))};
    return result;

}

inline __m128 operator+(__m128 a, __m128 b){
    return _mm_add_ps(a, b);
}
inline __m128 operator-(__m128 a, __m128 b){
    return _mm_sub_ps(a, b);
}
inline __m128 operator*(__m128 a, __m128 b){
    return _mm_mul_ps(a, b);
}

inline __m128 noiseFade4x(__m128 t){
    __m128 t2    = _mm_mul_ps(t,t);
    __m128 t3    = _mm_mul_ps(t2,t);
    __m128 inner = _mm_add_ps(_mm_mul_ps(t, _mm_sub_ps(_mm_mul_ps(t, _mm_set1_ps(6.0f)), _mm_set1_ps(15.0f))), _mm_set1_ps(10.0f));
    return _mm_mul_ps(t3, inner);

}

inline __m128 lerp4x(__m128 t, __m128 a, __m128 b){
    return (a + t * (b - a));
}

#include <emmintrin.h>
#include <immintrin.h> //AVX2

//4x UINT 32
struct u32_4x{
    union{
        __m128i sse;
        uint32_t e[4];
    };
};

inline u32_4x operator+(u32_4x a, u32_4x b){
    u32_4x result = {_mm_add_epi32(a.sse, b.sse)};
    return result;
}
inline u32_4x operator-(u32_4x a, u32_4x b){
    u32_4x result = {_mm_sub_epi32(a.sse, b.sse)};
    return result;
}
inline u32_4x operator*(u32_4x a, u32_4x b){
    u32_4x result = {_mm_mul_epi32(a.sse, b.sse)};
    // u32_4x result = {_mm_mullo_epi32(a.sse, b.sse)}; //AVX2
    return result;
}
inline u32_4x lerp4x(u32_4x t, u32_4x a, u32_4x b){
    u32_4x result = a + t * (b - a);
    return result;
}
inline f32_4x select4x(f32_4x mask, f32_4x a, f32_4x b){
    f32_4x result = {_mm_blendv_ps(a.sse, b.sse, mask.sse)};
    return result;
}



//END SSE INTEL CODE

uint32_t hash3d(uint32_t x, uint32_t y, uint32_t z) {
    uint32_t h = x * 374761393u + y * 668265263u + z * 2246822519u; // Large primes
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

static inline void rng_seed(uint32_t* rng_state, uint32_t seed){
    *rng_state = seed ? seed : 0x5AA55005; //soos fallback seed
}

static inline uint32_t rng_next_u32(uint32_t* rng_state){
    uint32_t x = *rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *rng_state = x;
    return x;
}

static inline float rng_next_f32(uint32_t* rng_state){
    //convert to float in [0, 1)
    return (rng_next_u32(rng_state) >> 8) * (1.0f / 16777216.0f); //2^24
}

static inline uint32_t log2_floor(uint32_t x) {
    uint32_t r = 0;
    while (x >>= 1) ++r;
    return r;
}

// Unary negation operator
inline vec3 operator-(const vec3& v) {
    return {-v.x, -v.y, -v.z};
}

inline ivec3 operator-(const ivec3& v) {
    return {-v.x, -v.y, -v.z};
}


inline vec3& operator-=(vec3& lhs, const float& rhs) {
    lhs.x  = (lhs.x - rhs);
    lhs.y  = (lhs.y - rhs);
    lhs.z  = (lhs.z - rhs);
    return lhs;
}


inline bool operator==(const vec3& lhs, const vec3& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

inline bool operator==(const uvec3& lhs, const uvec3& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

inline bool operator==(const ivec3& lhs, const ivec3& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

inline bool operator!=(const vec3& lhs, const vec3& rhs) {
    return !(lhs == rhs);
}

// You might also want to add compound assignment operators for completeness:
inline vec3& operator-=(vec3& lhs, const vec3& rhs) {
    lhs.x  = (lhs.x - rhs.x);
    lhs.y  = (lhs.y - rhs.y);
    lhs.z  = (lhs.z - rhs.z);
    return lhs;
}

inline vec3 operator-(vec3& lhs, const vec3& rhs) {
    return {
        (lhs.x -  rhs.x),
        (lhs.y -  rhs.y),
        (lhs.z -  rhs.z)
    };
}


inline vec3 operator-(vec3& lhs, const float& rhs) {
    return {
        (lhs.x -  rhs),
        (lhs.y -  rhs),
        (lhs.z -  rhs)
    };
}


inline vec3 operator-(float lhs, vec3& rhs) {
    return {
        (lhs -  rhs.x),
        (lhs -  rhs.y),
        (lhs -  rhs.z)
    };
}

inline vec3& operator+=(vec3& lhs, const float& rhs) {
    lhs.x  = (lhs.x + rhs);
    lhs.y  = (lhs.y + rhs);
    lhs.z  = (lhs.z + rhs);
    return lhs;
}



inline ivec3& operator+=(ivec3& lhs, const ivec3&  rhs) {
    lhs.x  = (lhs.x + rhs.x);
    lhs.y  = (lhs.y + rhs.y);
    lhs.z  = (lhs.z + rhs.z);
    return lhs;
}

inline ivec3& operator-=(ivec3& lhs, const ivec3&  rhs) {
    lhs.x  = (lhs.x - rhs.x);
    lhs.y  = (lhs.y - rhs.y);
    lhs.z  = (lhs.z - rhs.z);
    return lhs;
}


// You might also want to add compound assignment operators for completeness:
inline vec3& operator+=(vec3& lhs, const vec3& rhs) {
    lhs.x  = (lhs.x + rhs.x);
    lhs.y  = (lhs.y + rhs.y);
    lhs.z  = (lhs.z + rhs.z);
    return lhs;
}

inline vec3 operator+(const vec3& lhs, const vec3& rhs) {
    return {
        (lhs.x + rhs.x),
        (lhs.y + rhs.y),
        (lhs.z + rhs.z)
    };
}


inline vec3 operator+(const vec3& lhs, const float& rhs) {
    return {
        (lhs.x +  rhs),
        (lhs.y +  rhs),
        (lhs.z +  rhs)
    };
}


inline vec4 operator+(const vec4& lhs, const vec4& rhs) {
    return {
        (lhs.x +  rhs.x),
        (lhs.y +  rhs.y),
        (lhs.z +  rhs.z),
        (lhs.w +  rhs.w)
    };
}



inline vec4 operator-(const vec4& lhs, const vec4& rhs) {
    return {
        (lhs.x -  rhs.x),
        (lhs.y -  rhs.y),
        (lhs.z -  rhs.z),
        (lhs.w -  rhs.w)
    };
}

inline vec2 operator*(const vec2& lhs, const float rhs) {
    return {
        (lhs.x *  rhs),
        (lhs.y *  rhs),
    };
}

inline vec4 operator*(const vec4& lhs, const float rhs) {
    return {
        (lhs.x *  rhs),
        (lhs.y *  rhs),
        (lhs.z *  rhs),
        (lhs.w *  rhs)
    };
}

inline vec4 operator*(const vec4& lhs, const vec4& rhs) {
    return {
        (lhs.x *  rhs.x),
        (lhs.y *  rhs.y),
        (lhs.z *  rhs.z),
        (lhs.w *  rhs.w)
    };
}


inline vec4 operator+(const vec4& lhs, const float rhs) {
    return {
        (lhs.x +  rhs),
        (lhs.y +  rhs),
        (lhs.z +  rhs),
        (lhs.w +  rhs)
    };
}

inline vec4 operator*(const float lhs, const vec4& rhs) {
    return {
        (lhs *  rhs.x),
        (lhs *  rhs.y),
        (lhs *  rhs.z),
        (lhs *  rhs.w)
    };
}

inline vec4 operator-(const float lhs, const vec4& rhs) {
    return {
        (lhs -  rhs.x),
        (lhs -  rhs.y),
        (lhs -  rhs.z),
        (lhs -  rhs.w)
    };
}

inline vec4 operator-(const vec4& v) {
    return {
        (-v.x),
        (-v.y),
        (-v.z),
        (-v.w)
    };
}

inline vec4 operator+(const float lhs, const vec4& rhs) {
    return {
        (lhs +  rhs.x),
        (lhs +  rhs.y),
        (lhs +  rhs.z),
        (lhs +  rhs.w)
    };
}





inline vec3& operator*=(vec3& lhs, const float& rhs) {
    lhs.x = (lhs.x *  rhs);
    lhs.y = (lhs.y *  rhs);
    lhs.z = (lhs.z *  rhs);
    return lhs;
}

// Vector * scalar
inline vec3 operator*(const vec3& lhs, const float& rhs) {
    return {
        (lhs.x *  rhs),
        (lhs.y *  rhs),
        (lhs.z *  rhs)
    };
}

// Element-wise vector multiplication
inline vec3 operator*(const vec3& lhs, const vec3& rhs) {
    return {
        (lhs.x * rhs.x),
        (lhs.y * rhs.y),
        (lhs.z * rhs.z)
    };
}


inline vec3 operator/(const vec3& lhs, const vec3& rhs) {
    return {
        (lhs.x / rhs.x),
        (lhs.y / rhs.y),
        (lhs.z / rhs.z)
    };
}

inline vec3 operator/(const float& lhs, const vec3& rhs) {
    return {
        (lhs / rhs.x),
        (lhs / rhs.y),
        (lhs / rhs.z)
    };
}

// scalar * Vector
inline vec3 operator*(const float& lhs, const vec3& rhs) {
    return {
        (lhs * rhs.x),
        (lhs * rhs.y),
        (lhs * rhs.z)
    };
}

// And the binary subtraction operator if you don't have it already
inline vec3 operator-(const vec3& lhs, const vec3& rhs) {
    return {
        (lhs.x - rhs.x),
        (lhs.y - rhs.y),
        (lhs.z - rhs.z)
    };
}

// And the binary subtraction operator if you don't have it already
inline vec3 operator-(const vec3& lhs, const float& rhs) {
    return {
        (lhs.x -  rhs),
        (lhs.y -  rhs),
        (lhs.z -  rhs)
    };
}


typedef struct vec4 quat;
// typedef struct vec3 float_ivec3;

// Vector addition
static inline vec3 vec3_add(vec3 a, vec3 b) {
    vec3 result = {
        (a.x + b.x),
        (a.y + b.y),
        (a.z + b.z)
    };
    return result;
}


// Vector scaling
static inline vec3 vec3_scale(vec3 v, float scale) {
    vec3 result = {
        (v.x * scale),
        (v.y * scale),
        (v.z * scale)
    };
    return result;
}

// Dot product
static inline float vec3_dot(vec3 a, vec3 b) {
    return ((a.x * b.x) + (a.y * b.y) + (a.z * b.z));
}
static inline float vec4_dot(vec4 a, vec4 b) {
    return ((a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w));
}


// Length squared (avoid sqrt when possible)
static inline float vec3_length_sq(vec3 v) {
    return vec3_dot(v, v);
}

// Vector length/magnitude
static inline float vec3_length(vec3 v) {
    return fast_sqrtf(vec3_length_sq(v));
}

// Normalize
static inline vec3 vec3_normalize(vec3 v) {
    float length = vec3_length(v);
    if (length == 0) return v;
    
    return vec3_scale(v, (1.0f /  length));
}

static inline float vec2_dot(vec2 a, vec2 b){
    return ((a.x * b.x) + (a.y * b.y)); 
}

static inline float vec2_length_sq(vec2 v){
    return vec2_dot(v, v);
}

static inline float vec2_length(vec2 v){
    return fast_sqrtf(vec2_length_sq(v));
}

static inline vec2 vec2_scale(vec2 v, float scale){
    vec2 result = {};
    result.x = v.x * scale;
    result.y = v.y * scale;
    return result;
}

static inline vec2 vec2_normalize(vec2 v){
    float length = vec2_length(v);
    if(length == 0)return v;
    return vec2_scale(v, (1.0f / length));
}

// Cross product
static inline vec3 vec3_cross(vec3 a, vec3 b) {
    vec3 result = {
        ((a.y * b.z) - (a.z * b.y)),
        ((a.z * b.x) - (a.x * b.z)),
        ((a.x * b.y) - (a.y * b.x))
    };
    return result;
}

inline vec3 operator%(vec3 lhs, vec3 rhs) {
    return vec3_cross(lhs, rhs);
}

inline vec3& operator%=(vec3& lhs, const vec3& rhs) {
    lhs = vec3_cross(lhs, rhs);
    return lhs;
}

// Some convenient constructors
static inline vec3 vec3_create(float x, float y, float z) {
    vec3 result = {x, y, z};
    return result;
}

static inline vec3 vec3_create(vec4& v) {
    vec3 result = {v.x, v.y, v.z};
    return result;
}

static inline ivec3 ivec3_create(int32_t x, int32_t y, int32_t z) {
    ivec3 result = {x, y, z};
    return result;
}
static inline ivec3 ivec3_create(int32_t a) {
    ivec3 result = {a, a, a};
    return result;
}

static inline ivec3 ivec3_create(vec3 v) {
    ivec3 result = {
        (int32_t)v.x, 
        (int32_t)v.y, 
        (int32_t)v.z};
    return result;
}

static inline vec3 ivec3_add_scaled(ivec3& a, ivec3& b, float scalar){
    vec3 result = {};
    result.x = ((a.x + b.x) * scalar);
    result.y = ((a.y + b.y) * scalar);
    result.z = ((a.z + b.z) * scalar);
    return result;
}

static inline vec3 ivec3_float_product(ivec3& v, float f){
    vec3 result = {};
    result.x = (v.x * f);
    result.y = (v.y * f);
    result.z = (v.z * f);
    return result;
}

static inline uvec3 uvec3_create(uint32_t x, uint32_t y, uint32_t z) {
    uvec3 result = {x, y, z};
    return result;
}
static inline uvec3 uvec3_create(uint32_t a) {
    uvec3 result = {a, a, a};
    return result;
}


//IVEC3 OVERLOADS

inline ivec3 operator-(ivec3& lhs, const ivec3& rhs) {
    return {
        (lhs.x - rhs.x),
        (lhs.y - rhs.y),
        (lhs.z - rhs.z)
    };
}

inline ivec3 operator+(ivec3& lhs, const ivec3& rhs) {
    return {
        (lhs.x + rhs.x),
        (lhs.y + rhs.y),
        (lhs.z + rhs.z)
    };
}

inline ivec3 operator*(ivec3& lhs, const ivec3& rhs) {
    return {
        (lhs.x * rhs.x),
        (lhs.y * rhs.y),
        (lhs.z * rhs.z)
    };
}

inline ivec3 operator/(ivec3& lhs, const int32_t& rhs) {
    return {
        (lhs.x / rhs),
        (lhs.y / rhs),
        (lhs.z / rhs)
    };
}

inline vec3 operator*(ivec3& lhs, const float& rhs) {
    return {
        (lhs.x * rhs),
        (lhs.y * rhs),
        (lhs.z * rhs)
    };
}

inline ivec3 operator*(ivec3& lhs, const int32_t& rhs) {
    return {
        (lhs.x * rhs),
        (lhs.y * rhs),
        (lhs.z * rhs)
    };
}




inline bool operator!=(const ivec3& lhs, const ivec3& rhs) {
    return !(lhs == rhs);
}


//IVEC3 OVERLOADS END






static inline float degrees(float radians){
    // 180/π ≈ 57.2957795131f
    // In fixed point (16.16), that's approximately 3754936 (0x394BB8)
    return (radians * 57.2957795131f);  // radians * (180/π)
}

static inline float radians(float degrees) {
    // π/180 ≈ 0.0174532925199
    // In fixed point (16.16), that's approximately 1144 (0x478)
    return (degrees * 0.0174532925199f);  // degrees * (π/180)
}

// Multiply vector by scalar
static inline vec3 vec3_mul_scalar(vec3 v, float scalar) {
    vec3 result = {
        (v.x * scalar),
        (v.y * scalar),
        (v.z * scalar)
    };
    return result;
}

static inline vec3 vec3_add_scaled(vec3& a, vec3 b, float scalar){
    // a.x += (b.x * scalar);
    // a.y += (b.y * scalar);
    // a.z += (b.z * scalar);
    a += (b*scalar);
    return a;
}

static inline vec3 quat_rotate_vec3(quat q, vec3 v) {
    // Get vector part of quaternion
    vec3 qvec = vec3_create(q.x, q.y, q.z);
    
    // First cross product
    vec3 uv = vec3_cross(qvec, v);
    
    // Second cross product
    vec3 uuv = vec3_cross(qvec, uv);
    
    // Scale uv by q.w
    uv = vec3_mul_scalar(uv, q.w);
    
    // Add uv and uuv
    vec3 sum = vec3_add(uv, uuv);
    
    // Multiply by 2
    sum = vec3_mul_scalar(sum, 2.0f);
    
    // Add to original vector
    return vec3_add(v, sum);
}

static inline quat float_angle_axis(float angle, vec3 axis) {
    // Normalize the axis
    axis = vec3_normalize(axis);

    // Compute sine and cosine of half the angle
    float halfAngle = (angle /  2.0f);
    float s = fast_sinf(halfAngle);
    float c = fast_cosf(halfAngle);

    // Construct the quaternion
    quat q;
    q.w = c;
    q.x = (axis.x * s);
    q.y = (axis.y * s);
    q.z = (axis.z * s);

    return q;
}






// // For single float values
// inline float min(float a, float b) {
//     return (a < b) ? a : b;
// }

// inline float max(float a, float b) {
//     return (a > b) ? a : b;
// }





	// inline mat4 lookAtLH(vec3 const& eye, vec3 const& center, vec3 const& up)
	// {
	// 	vec3 const f = (vec3_normalize(center - eye));
	// 	vec3 const s = (vec3_normalize(vec3_cross(up, f)));
	// 	vec3 const u = (vec3_cross(f, s));

	// 	float_mat4 Result = mat4_create_identity();
	// 	Result[0][0] = s.x;
	// 	Result[1][0] = s.y;
	// 	Result[2][0] = s.z;
	// 	Result[0][1] = u.x;
	// 	Result[1][1] = u.y;
	// 	Result[2][1] = u.z;
	// 	Result[0][2] = f.x;
	// 	Result[1][2] = f.y;
	// 	Result[2][2] = f.z;
	// 	Result[3][0] = -vec3_dot(s, eye);
	// 	Result[3][1] = -vec3_dot(u, eye);
	// 	Result[3][2] = -vec3_dot(f, eye);
	// 	return Result;
	// }

static inline float inversesqrt(float x) {
    return (1.0f / fast_sqrtf(x));
}

// And for vectors:
static inline vec3 vec3_inversesqrt(vec3 v) {
    vec3 result;
    result.x = inversesqrt(v.x);
    result.y = inversesqrt(v.y);
    result.z = inversesqrt(v.z);
    return result;
}




// static inline quat quat_LookAtRH(vec3 const& direction, vec3 const& up)
// {
//     mat3 floatResult;
    
//     // Column 2 (-direction)
//     floatResult.m[2] = -direction.x;  // First entry in third column
//     floatResult.m[5] = -direction.y;  // Second entry in third column
//     floatResult.m[8] = -direction.z;  // Third entry in third column

//     // Calculate right vector
//     vec3 floatRight = vec3_cross(up, vec3_create(-direction.x, -direction.y, -direction.z));
//     floatRight = vec3_mul_scalar(floatRight, 
//         float_inversesqrt(
//             float_max(
//                 fl2float(0.0001f),
//                 vec3_dot(floatRight, floatRight)
//             )
//         )
//     );

//     // Column 0 (right vector)
//     floatResult.m[0][0] = floatRight.x;
//     floatResult.m[0][1] = floatRight.y;
//     floatResult.m[0][2] = floatRight.z;

//     // Column 1 (up vector = forward × right)
//     vec3 result1 = vec3_cross(
//         vec3_create(-direction.x, -direction.y, -direction.z),
//         floatRight
//     );
//     floatResult.m[1][0] = result1.x;
//     floatResult.m[1][1] = result1.y;
//     floatResult.m[1][2] = result1.z;

//     return float_mat3_to_quat(floatResult);
// }

static inline float quat_dot(quat a, quat b) {
    return (
        (
            (
                (a.x * b.x)+
                (a.y * b.y)
            )+
            (a.z* b.z)
        )+
        (a.w* b.w)
    );
}

//DOES NOT HANDLE SCALING
// static inline float_mat4 float_inverse_transform(const float_mat4& m) {
//     float_mat4 result;
    
//     // Extract the 3x3 rotation matrix and transpose it
//     // Note: Column-major order
//     result.m[0][0] = m.m[0][0];  result.m[0][1] = m.m[1][0];  result.m[0][2] = m.m[2][0];
//     result.m[1][0] = m.m[0][1];  result.m[1][1] = m.m[1][1];  result.m[1][2] = m.m[2][1];
//     result.m[2][0] = m.m[0][2];  result.m[2][1] = m.m[1][2];  result.m[2][2] = m.m[2][2];
    
//     // Calculate -R^T * T
//     result.m[3][0] = -float_add(float_add(
//         float_mul(result.m[0][0], m.m[3][0]),
//         float_mul(result.m[1][0], m.m[3][1])),
//         float_mul(result.m[2][0], m.m[3][2]));
        
//     result.m[3][1] = -float_add(float_add(
//         float_mul(result.m[0][1], m.m[3][0]),
//         float_mul(result.m[1][1], m.m[3][1])),
//         float_mul(result.m[2][1], m.m[3][2]));
        
//     result.m[3][2] = -float_add(float_add(
//         float_mul(result.m[0][2], m.m[3][0]),
//         float_mul(result.m[1][2], m.m[3][1])),
//         float_mul(result.m[2][2], m.m[3][2]));
    
//     // Set bottom row to [0 0 0 1]
//     result.m[0][3] = 0;
//     result.m[1][3] = 0;
//     result.m[2][3] = 0;
//     result.m[3][3] = 1.0f;  // Assuming 1.0f is your fixed-point representation of 1.0
    
//     return result;
// }


static inline mat34 mat34_create(){
        mat34 mat;
        mat.m[0]  = 1.0f;
        mat.m[1]  = 0;
        mat.m[2]  = 0;
        mat.m[3]  = 0;
        mat.m[4]  = 0;
        mat.m[5]  = 1.0f;
        mat.m[6]  = 0;
        mat.m[7]  = 0;
        mat.m[8]  = 0;
        mat.m[9]  = 0;
        mat.m[10] = 1.0f;
        mat.m[11] = 0;
        return mat;
}

static inline mat3 mat3_create(){
        mat3 mat;
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


static inline vec3 vec3_mat3_transform(const vec3& vec, const mat3& mat){
    vec3 result;

    result.x = (((vec.x * mat.m[0])  + (vec.y * mat.m[1])) + (vec.z * mat.m[2]));
    result.y = (((vec.x * mat.m[3])  + (vec.y * mat.m[4])) + (vec.z * mat.m[5]));
    result.z = (((vec.x * mat.m[6])  + (vec.y * mat.m[7])) + (vec.z * mat.m[8]));

    return result;
}


static inline vec3 mat34_transform(const vec3& vec, const mat34& mat){
    vec3 result;

    result.x = ((((vec.x * mat.m[0])  + (vec.y * mat.m[1])) + (vec.z * mat.m[2]) )+ mat.m[3]);
    result.y = ((((vec.x * mat.m[4])  + (vec.y * mat.m[5])) + (vec.z * mat.m[6]) )+ mat.m[7]);
    result.z = ((((vec.x * mat.m[8])  + (vec.y * mat.m[9])) + (vec.z * mat.m[10]))+ mat.m[11]);

    return result;
}

static inline mat3 mat3_mult(const mat3& a, const mat3& b) {
    return mat3{
        (((a.m[0]* b.m[0]) + (a.m[1] * b.m[3]))+ (a.m[2]* b.m[6])), 
        (((a.m[0]* b.m[1]) + (a.m[1] * b.m[4]))+ (a.m[2]* b.m[7])), 
        (((a.m[0]* b.m[2]) + (a.m[1] * b.m[5]))+ (a.m[2]* b.m[8])), 
        (((a.m[3]* b.m[0]) + (a.m[4] * b.m[3]))+ (a.m[5]* b.m[6])), 
        (((a.m[3]* b.m[1]) + (a.m[4] * b.m[4]))+ (a.m[5]* b.m[7])), 
        (((a.m[3]* b.m[2]) + (a.m[4] * b.m[5]))+ (a.m[5]* b.m[8])), 
        (((a.m[6]* b.m[0]) + (a.m[7] * b.m[3]))+ (a.m[8]* b.m[6])), 
        (((a.m[6]* b.m[1]) + (a.m[7] * b.m[4]))+ (a.m[8]* b.m[7])), 
        (((a.m[6]* b.m[2]) + (a.m[7] * b.m[5]))+ (a.m[8]* b.m[8])),
    };

}

static inline mat34 mat34_mult(const mat3& a, const mat3& b){
    mat34 result;

    result.m[0]  = (((a.m[0]* b.m[0]) + (a.m[4]* b.m[1])) + (a.m[8]  * b.m[2]  ));
    result.m[4]  = (((a.m[0]* b.m[4]) + (a.m[4]* b.m[5])) + (a.m[8]  * b.m[6]  ));
    result.m[8]  = (((a.m[0]* b.m[8]) + (a.m[4]* b.m[9])) + (a.m[8]  * b.m[10] ));
    result.m[1]  = (((a.m[1]* b.m[0]) + (a.m[5]* b.m[1])) + (a.m[9]  * b.m[2]  ));
    result.m[5]  = (((a.m[1]* b.m[4]) + (a.m[5]* b.m[5])) + (a.m[9]  * b.m[6]  ));
    result.m[9]  = (((a.m[1]* b.m[8]) + (a.m[5]* b.m[9])) + (a.m[9]  * b.m[10] ));
    result.m[2]  = (((a.m[2]* b.m[0]) + (a.m[6]* b.m[1])) + (a.m[10] * b.m[2]  ));
    result.m[6]  = (((a.m[2]* b.m[4]) + (a.m[6]* b.m[5])) + (a.m[10] * b.m[6]  ));
    result.m[10] = (((a.m[2]* b.m[8]) + (a.m[6]* b.m[9])) + (a.m[10] * b.m[10] ));
    
    result.m[3]  = ((((a.m[3] * b.m[0]) + (a.m[7] * b.m[1])) + (a.m[11] * b.m[2]  )) + b.m[3]);
    result.m[7]  = ((((a.m[3] * b.m[4]) + (a.m[7] * b.m[5])) + (a.m[11] * b.m[6]  )) + b.m[7]);
    result.m[11] = ((((a.m[3] * b.m[8]) + (a.m[7] * b.m[9])) + (a.m[11] * b.m[10] )) + b.m[11]);

    return result;
}

//sets matrix a to the inverse of matrix b
static inline void mat3_set_inverse(mat3& a, const mat3& b){
    float t1 = (b.m[0] * b.m[4]);
    float t2 = (b.m[0] * b.m[5]);
    float t3 = (b.m[1] * b.m[3]);
    float t4 = (b.m[2] * b.m[3]);
    float t5 = (b.m[1] * b.m[6]);
    float t6 = (b.m[2] * b.m[6]);

    //calculate the determinant
    float det = ((((((t1 * b.m[8]) - (t2 * b.m[7])) - (t3 * b.m[8])) + (t4 * b.m[7])) + (t5 * b.m[5])) - (t6 * b.m[4]));

    //make sure the determinant is non zero
    if(det == 0)return;
    float invd = (1.0f / det);

    a.m[0] =  (((b.m[4] * b.m[8]) - (b.m[5] * b.m[7])) * invd);
    a.m[1] = -(((b.m[1] * b.m[8]) - (b.m[2] * b.m[7])) * invd);
    a.m[2] =  (((b.m[1] * b.m[5]) - (b.m[2] * b.m[4])) * invd);
    a.m[3] = -(((b.m[3] * b.m[8]) - (b.m[5] * b.m[6])) * invd);
    a.m[4] =  (((b.m[0] * b.m[8]) - t6) * invd);
    a.m[5] = -((t2 - t4) * invd);
    a.m[6] =  (((b.m[3] * b.m[7]) - (b.m[4] * b.m[6])) * invd);
    a.m[7] = -(((b.m[0] * b.m[7]) - t5) * invd);
    a.m[8] =  ((t1 - t3) * invd);


}

static inline float mat34_determinant(mat34 m){
    
    return (((((((m.m[8]*m.m[5])* m.m[2]) + ((m.m[4]*m.m[9])* m.m[2])) + ((m.m[8]*m.m[1])* m.m[6])) -((m.m[0]*m.m[9])* m.m[6])) - ((m.m[4]*m.m[1])* m.m[10])) +((m.m[0]*m.m[5])* m.m[10]));
}

//sets matrix a to the inverse of matrix b
static inline void set_inverse(mat34& a, const mat34& b){

    float det = mat34_determinant(a);
    if(det == 0)return;
    det = (1.0f / det);

    a.m[0] = ((((-b.m[9]*b.m[6])    +   (b.m[5]* b.m[10]))               * det));
    a.m[4] = (((( b.m[8]*b.m[6])    -   (b.m[4]* b.m[10]))               * det));
    a.m[8] = ((((-b.m[8]*b.m[5])    +   ((b.m[4]*b.m[9])*b.m[15]))*det));


    a.m[1] = (((( b.m[9]*b.m[2])    -   (b.m[1]* b.m[10])) * det));
    a.m[5] = ((((-b.m[8]*b.m[2])    +   (b.m[0]* b.m[10])) * det));
    a.m[9] = (((( b.m[8]*b.m[1])    -   ((b.m[0]*b.m[9])*b.m[15])) *det));


    a.m[2] = ((((-b.m[5]*b.m[2])    +   ((b.m[1]*b.m[6])*b.m[15])) * det));
    a.m[6] = ((((+b.m[4]*b.m[2])    -   ((b.m[0]*b.m[6])*b.m[15])) * det));
    a.m[10]= ((((-b.m[4]*b.m[1])    +   ((b.m[0]*b.m[5])*b.m[15])) * det));


    a.m[3] =  (( ((b.m[9]*b.m[6]) * b.m[3] )-
    ( ((b.m[5]*b.m[10])* b.m[3] )-
    ( ((b.m[9]*b.m[2]) * b.m[7] )+
    ( ((b.m[1]*b.m[10])* b.m[7] )+
    ( ((b.m[5]*b.m[2]) * b.m[11])-  
    ((b.m[1]*b.m[6]) * b.m[11]) )))))*det);

    a.m[7] = (( ((b.m[8]*b.m[6]) * b.m[3] )+
    ( ((b.m[4]*b.m[10])* b.m[3] )+
    ( ((b.m[8]*b.m[2]) * b.m[7] )-
    ( ((b.m[0]*b.m[10])* b.m[7] )-
    ( ((b.m[4]*b.m[2]) * b.m[11])+  
    ((b.m[0]*b.m[6]) * b.m[11]) )))))*det);

    a.m[11] = (( ((b.m[8]*b.m[5])* b.m[3] )-
    ( ((b.m[4]*b.m[9])* b.m[3] )-
    ( ((b.m[8]*b.m[1])* b.m[7] )+
    ( ((b.m[0]*b.m[9])* b.m[7] )+
    ( ((b.m[4]*b.m[1])* b.m[11])  -((b.m[0]*b.m[5])* b.m[11]) )))))*det);

}



static inline void set_transpose(mat3& a, const mat3& b){
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

static inline void set_orientation(mat3& a, const quat& q){
    a.m[0] =        (1.0f -  (((q.y*q.y)*2.0f) + ((q.z*q.z)*2.0f)) );
    a.m[1] =                 (((q.x*q.y)*2.0f) + ((q.z*q.w)*2.0f)) ;
    a.m[2] =                 (((q.x*q.z)*2.0f) - ((q.y*q.w)*2.0f)) ;
    a.m[3] =                 (((q.x*q.y)*2.0f) - ((q.z*q.w)*2.0f)) ;
    a.m[4] =        (1.0f -  (((q.x*q.x)*2.0f) + ((q.z*q.z)*2.0f)) );
    a.m[5] =                 (((q.y*q.z)*2.0f) + ((q.x*q.w)*2.0f)) ;
    a.m[6] =                 (((q.x*q.z)*2.0f) + ((q.y*q.w)*2.0f)) ;
    a.m[7] =                 (((q.y*q.z)*2.0f) - ((q.x*q.w)*2.0f)) ;
    a.m[8] =        (1.0f -  (((q.x*q.x)*2.0f) + ((q.y*q.y)*2.0f)) );

}


/**
* Sets this matrix to be the rotation matrix corresponding to
* the given quaternion.
*/
static inline void set_orientation_and_pos(mat34& a, const quat &q, const vec3 &pos)
{
    a.m[0]  = (1.0f -(((q.y*q.y)* 2.0f) + ((q.z*q.z)*2.0f)));
    a.m[1]  =        (((q.x*q.y)* 2.0f) + ((q.z*q.w)*2.0f));
    a.m[2]  =        (((q.x*q.z)* 2.0f) - ((q.y*q.w)*2.0f));
    a.m[3]  = pos.x;
    a.m[4]  =        (((q.x*q.y)* 2.0f) - ((q.z*q.w)* 2.0f));
    a.m[5]  = (1.0f -(((q.x*q.x)* 2.0f) + ((q.z*q.z)* 2.0f)));
    a.m[6]  =        (((q.y*q.z)* 2.0f) + ((q.x*q.w)* 2.0f));
    a.m[7]  = pos.y;
    a.m[8]  =        (((q.x*q.z)* 2.0f) + ((q.y*q.w)* 2.0f));
    a.m[9]  =        (((q.y*q.z)* 2.0f) - ((q.x*q.w)* 2.0f));
    a.m[10] = (1.0f -(((q.x*q.x)* 2.0f) + ((q.y*q.y)* 2.0f)));
    a.m[11] = pos.z;
}




static inline vec3 transform_inverse(const mat34& mat, const vec3& vec){
    vec3 tmp = vec;
    tmp.x = (tmp.x - mat.m[3] );
    tmp.y = (tmp.y - mat.m[7] );
    tmp.z = (tmp.z - mat.m[11]);

    return vec3{
        (((tmp.x * mat.m[0]) +(tmp.y * mat.m[4])) + (tmp.z * mat.m[8] )),
        (((tmp.x * mat.m[1]) +(tmp.y * mat.m[5])) + (tmp.z * mat.m[9] )),
        (((tmp.x * mat.m[2]) +(tmp.y * mat.m[6])) + (tmp.z * mat.m[10])),
    };
}


static inline vec3 local_to_world(const vec3& local, const mat34& transform){
    return transform_inverse(transform, local);
}



static inline vec3 world_to_local( const mat34& transform, const vec3& world){
    // mat34 inverse_transform = mat34_create();
    // set_inverse(inverse_transform, transform);
    // return vec3_mat34_transform(world, inverse_transform);

    //can be simplified by calling transform_inverse
    return transform_inverse(transform, world);

}

static inline vec3 transform_direction( const mat34& m, const vec3& vec){
    return vec3{
        (((vec.x * m.m[0]) + (vec.y * m.m[1])) + (vec.z * m.m[2] )),
        (((vec.x * m.m[4]) + (vec.y * m.m[5])) + (vec.z * m.m[6] )),
        (((vec.x * m.m[8]) + (vec.y * m.m[9])) + (vec.z * m.m[10])),
    };

}

static inline vec3 transform_inverse_direction( const mat34& m, const vec3& vec){
    return vec3{
        (((vec.x * m.m[0]) + (vec.y * m.m[4])) + (vec.z * m.m[8] )),
        (((vec.x * m.m[1]) + (vec.y * m.m[5])) + (vec.z * m.m[9] )),
        (((vec.x * m.m[2]) + (vec.y * m.m[6])) + (vec.z * m.m[10])),
    };
}

static inline vec3 mat3_transform_transpose(const mat3& m, const vec3& v){
    return vec3{
        (((v.x * m.m[0]) + (v.y * m.m[3])) + (v.z * m.m[6])),
        (((v.x * m.m[1]) + (v.y * m.m[4])) + (v.z * m.m[7])),
        (((v.x * m.m[2]) + (v.y * m.m[5])) + (v.z * m.m[8])),
    };
}

static inline vec3 local_to_world_dirn(const vec3& local, const mat34& transform){
    return transform_direction(transform, local);
}

static inline vec3 world_to_local_dirn(const vec3& world, const mat34& transform){
    return transform_inverse_direction(transform, world);
}


static inline quat quat_rotate_by_vec3(const quat& q, const vec3& v){
    quat quat = {0};

    quat.x = v.x;
    quat.y = v.y;
    quat.z = v.z;
    quat.w = 0;

    return quat;
}

inline bool operator==(const quat& lhs, const quat& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

inline bool operator!=(const quat& lhs, const quat& rhs) {
    return !(lhs == rhs);
}


static inline void mat3_set_inertia_tensor_coefficients(mat3& mat, float ix, float iy, float iz, float ixy = 0, float ixz = 0, float iyz = 0){
    mat.m[0] = ix;
    mat.m[1] = mat.m[3] = -ixy;
    mat.m[2] = mat.m[6] = -ixz;
    mat.m[4] = iy;
    mat.m[5] = mat.m[7] = -iyz;
    mat.m[8] = iz;
}

//19661 = 0.3 * 2^16
//sets value of matrix as a rectangular block aligned with body's coordinates
static inline void mat3_set_block_inertia_tensor(mat3& mat, const vec3& half_sizes, float mass){
    vec3 squares = half_sizes * half_sizes;
    mat3_set_inertia_tensor_coefficients(mat, 
        (0.3f * (mass * (squares.y + squares.z))),
        (0.3f * (mass * (squares.x + squares.z))),
        (0.3f * (mass * (squares.x + squares.y))));
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


static inline void mat3_set_diagonal(mat3& mat, float a, float b, float c){
        mat3_set_inertia_tensor_coefficients(mat, a, b, c);
}

//sets left to right
static inline void set_mat3(mat3& lhs, const mat3& rhs) {
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


static inline void set_mat34(mat34& lhs, const mat34& rhs) {
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

// /// @brief represents one axis/column of the matrix. row 3 corresponds to the position of the transform matrix
// /// @param m mat34 (3x4 fixed point matrix)
// /// @param i the row to return
// /// @return vec3 
// static inline vec3 mat34_get_axis(const mat34& m, int i){
//     // /*TAG*/printf("get axis: %3.3f %3.3f %3.3f\n", float2fl(m.m[i]), float2fl(m.m[i+4]), float2fl(m.m[i+8]));
//     return vec3_create(m.m[i], m.m[i+4], m.m[i+8]);
// }

// static inline void print_mat34(const mat34& m){
//     printf("%10.5f %10.5f %10.5f %10.5f\n", float2fl(m.m[0]), float2fl(m.m[1]), float2fl(m.m[2] ), float2fl(m.m[3] ));
//     printf("%10.5f %10.5f %10.5f %10.5f\n", float2fl(m.m[4]), float2fl(m.m[5]), float2fl(m.m[6] ), float2fl(m.m[7] ));
//     printf("%10.5f %10.5f %10.5f %10.5f\n", float2fl(m.m[8]), float2fl(m.m[9]), float2fl(m.m[10]), float2fl(m.m[11]));
// }

// static inline void print_mat3(const char* string, const mat3& m){
//     printf("%s\n", string);
//     printf("%10.5f %10.5f %10.5f\n", float2fl(m.m[0]), float2fl(m.m[1]), float2fl(m.m[2] ));
//     printf("%10.5f %10.5f %10.5f\n", float2fl(m.m[3]), float2fl(m.m[4]), float2fl(m.m[5] ));
//     printf("%10.5f %10.5f %10.5f\n", float2fl(m.m[6]), float2fl(m.m[7]), float2fl(m.m[8]));
// }


// static inline void print_quat(const char* string, const quat& q){
//     printf("%s %10.5f %10.5f %10.5f %10.5f\n", string, float2fl(q.x), float2fl(q.y), float2fl(q.z), float2fl(q.w));
// }

// static inline void print_vec3(const char* string, const vec3& v){
//     printf("%s %10.5f %10.5f %10.5f\n", string, float2fl(v.x),float2fl(v.y),float2fl(v.z));
// }

// static inline void print_float(const char* string, const float a){
//     printf("%s %10.5f\n", string, float2fl(a));
// }

// // //too lazy to go back to the other way after copying code
// // static inline void print_glm_vec3(const char* string, const glm::vec3& v){
// //     printf("%s %10.5f %10.5f %10.5f\n", string, (v.x),(v.y),(v.z));
// // }

// static inline void print_float(const char* string, const float a){
//     printf("%s %10.5f\n", string, (a));
// }

static inline void mat3_set_components(mat3& m, vec3& a, vec3& b, vec3& c){
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


static inline mat34 mat34_with_mat3_vec3(mat3& m, vec3& pos){
    mat34 mat;
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


// static inline glm::mat4 float_to_glm_mat34(const mat34& m) {
//     glm::mat4 mat;
//     // First column
//     mat[0][0] = float2fl(m.m[0]);
//     mat[1][0] = float2fl(m.m[4]);
//     mat[2][0] = float2fl(m.m[8]);
//     mat[3][0] = 0;

//     // Second column
//     mat[0][1] = float2fl(m.m[1]);
//     mat[1][1] = float2fl(m.m[5]);
//     mat[2][1] = float2fl(m.m[9]);
//     mat[3][1] = 0;

//     // Third column
//     mat[0][2] = float2fl(m.m[2]);
//     mat[1][2] = float2fl(m.m[6]);
//     mat[2][2] = float2fl(m.m[10]);
//     mat[3][2] = 0;

//     // Fourth column (translation)
//     mat[0][3] = float2fl(m.m[3]);
//     mat[1][3] = float2fl(m.m[7]);
//     mat[2][3] = float2fl(m.m[11]);
//     mat[3][3] = 1;

//     return mat;
// }


// static inline bool float_check(float float, float fl, const char* string = ""){
//     float discrepancy = 0.05f;
//     bool pass = true;
//     float diff = ffabs(float2fl(float) - fl);
//     if(diff > discrepancy){
//         pass = false;
//     }
//     if(!pass){
//         printf("WRONG VALUE IN float %s!\n", string);
//         printf("float  : %6.2f\n",float2fl(float));
//         printf("fl   : %6.2f\n",fl);
//         printf("diff : %6.2f\n",diff);
//     }
//     return pass;
// }

// static inline bool vec3_check(vec3& float, glm::vec3 fl, const char* string = ""){
//     float discrepancy = 0.05f;
//     bool pass = true;
// glm::vec3 diff = glm::vec3(0.0f);
//     diff.x = glm::fabs(float2fl(float.x) - fl.x);
//     diff.y = glm::fabs(float2fl(float.y) - fl.y);
//     diff.z = glm::fabs(float2fl(float.z) - fl.z);
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
//         printf("float  : %6.2f %6.2f %6.2f\n",float2fl(float.x), float2fl(float.y), float2fl(float.z));
//         printf("fl   : %6.2f %6.2f %6.2f\n",fl.x, fl.y, fl.z);
//         printf("diff : %6.2f %6.2f %6.2f\n",diff.x, diff.y, diff.z);
//     }
//     return pass;

// }



// static inline bool mat3_check(mat3& float, glm::mat3 fl, const char* string = ""){
//     float discrepancy = 0.05f;
//     // float discrepancy = 0.01f;
//     bool pass = true;

//     glm::mat3 diff;

//     diff[0][0] = glm::fabs(fl[0][0] - float2fl(float.m[0]));
//     diff[0][1] = glm::fabs(fl[0][1] - float2fl(float.m[3]));
//     diff[0][2] = glm::fabs(fl[0][2] - float2fl(float.m[6]));

//     // Second column
//     diff[1][0] = glm::fabs(fl[1][0] - float2fl(float.m[1]));
//     diff[1][1] = glm::fabs(fl[1][1] - float2fl(float.m[4]));
//     diff[1][2] = glm::fabs(fl[1][2] - float2fl(float.m[7]));

//     // Third column
//     diff[2][0] = glm::fabs(fl[2][0] - float2fl(float.m[2]));
//     diff[2][1] = glm::fabs(fl[2][1] - float2fl(float.m[5]));
//     diff[2][2] = glm::fabs(fl[2][2] - float2fl(float.m[8]));

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
//         printf("float  : %6.2f %6.2f %6.2f\n",float2fl(float.m[0]), float2fl(float.m[1]), float2fl(float.m[2]));
//         printf("float  : %6.2f %6.2f %6.2f\n",float2fl(float.m[3]), float2fl(float.m[4]), float2fl(float.m[5]));
//         printf("float  : %6.2f %6.2f %6.2f\n",float2fl(float.m[6]), float2fl(float.m[7]), float2fl(float.m[8]));

//         printf("fl   : %6.2f %6.2f %6.2f\n",fl[0][0], fl[1][0], fl[2][0]);
//         printf("fl   : %6.2f %6.2f %6.2f\n",fl[0][1], fl[1][1], fl[2][1]);
//         printf("fl   : %6.2f %6.2f %6.2f\n",fl[0][2], fl[1][2], fl[2][2]);
        
//         printf("diff : %6.2f %6.2f %6.2f\n",diff[0][0], diff[1][0], diff[2][0]);
//         printf("diff : %6.2f %6.2f %6.2f\n",diff[0][1], diff[1][1], diff[2][1]);
//         printf("diff : %6.2f %6.2f %6.2f\n",diff[0][2], diff[1][2], diff[2][2]);
//     }
//     return pass;

// }


// static inline bool mat34_check(const mat34& float, glm::mat4 fl, const char* string = ""){
//     float discrepancy = 0.05f;
//     // float discrepancy = 0.01f;
//     bool pass = true;

//     glm::mat4 diff;

//     diff[0][0] = glm::fabs(fl[0][0] - float2fl(float.m[0]));
//     diff[0][1] = glm::fabs(fl[0][1] - float2fl(float.m[4]));
//     diff[0][2] = glm::fabs(fl[0][2] - float2fl(float.m[8]));

//     // Second column
//     diff[1][0] = glm::fabs(fl[1][0] - float2fl(float.m[1]));
//     diff[1][1] = glm::fabs(fl[1][1] - float2fl(float.m[5]));
//     diff[1][2] = glm::fabs(fl[1][2] - float2fl(float.m[9]));

//     // Third column
//     diff[2][0] = glm::fabs(fl[2][0] - float2fl(float.m[2]));
//     diff[2][1] = glm::fabs(fl[2][1] - float2fl(float.m[6]));
//     diff[2][2] = glm::fabs(fl[2][2] - float2fl(float.m[10]));

//     //fourth column
//     diff[3][0] = glm::fabs(fl[3][0] - float2fl(float.m[3]));
//     diff[3][1] = glm::fabs(fl[3][1] - float2fl(float.m[7]));
//     diff[3][2] = glm::fabs(fl[3][2] - float2fl(float.m[11]));

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
//         printf("float  : %6.2f %6.2f %6.2f %6.2f\n",float2fl(float.m[0]), float2fl(float.m[1]), float2fl(float.m[2]), float2fl(float.m[3]));
//         printf("float  : %6.2f %6.2f %6.2f %6.2f\n",float2fl(float.m[4]), float2fl(float.m[5]), float2fl(float.m[6]), float2fl(float.m[7]));
//         printf("float  : %6.2f %6.2f %6.2f %6.2f\n",float2fl(float.m[8]), float2fl(float.m[9]), float2fl(float.m[10]),float2fl(float.m[11]));

//         printf("fl   : %6.2f %6.2f %6.2f %6.2f\n",fl[0][0], fl[1][0], fl[2][0], fl[3][0]);
//         printf("fl   : %6.2f %6.2f %6.2f %6.2f\n",fl[0][1], fl[1][1], fl[2][1], fl[3][1]);
//         printf("fl   : %6.2f %6.2f %6.2f %6.2f\n",fl[0][2], fl[1][2], fl[2][2], fl[3][2]);
        
//         printf("diff : %6.2f %6.2f %6.2f %6.2f\n",diff[0][0], diff[1][0], diff[2][0], diff[3][0]);
//         printf("diff : %6.2f %6.2f %6.2f %6.2f\n",diff[0][1], diff[1][1], diff[2][1], diff[3][1]);
//         printf("diff : %6.2f %6.2f %6.2f %6.2f\n",diff[0][2], diff[1][2], diff[2][2], diff[3][2]);
//     }
//     return pass;

    
// }

// static inline glm::mat3 float_to_glm_mat3(mat3& m){
//     glm::mat3 mat;
//     mat[0][0] = float2fl(m.m[0]);
//     mat[0][1] = float2fl(m.m[3]);
//     mat[0][2] = float2fl(m.m[6]);

//     // Second column
//     mat[1][0] = float2fl(m.m[1]);
//     mat[1][1] = float2fl(m.m[4]);
//     mat[1][2] = float2fl(m.m[7]);

//     // Third column
//     mat[2][0] = float2fl(m.m[2]);
//     mat[2][1] = float2fl(m.m[5]);
//     mat[2][2] = float2fl(m.m[8]);
//     return mat;
// }




// static inline glm::mat4 float_to_glm_mat4(mat34& m){
//     glm::mat4 mat;
//     mat[0][0] = float2fl(m.m[0]);
//     mat[0][1] = float2fl(m.m[4]);
//     mat[0][2] = float2fl(m.m[8]);
//     mat[1][0] = float2fl(m.m[1]);
//     mat[1][1] = float2fl(m.m[5]);
//     mat[1][2] = float2fl(m.m[9]);
//     mat[2][0] = float2fl(m.m[2]);
//     mat[2][1] = float2fl(m.m[6]);
//     mat[2][2] = float2fl(m.m[10]);
//     mat[3][0] = float2fl(m.m[3]);
//     mat[3][1] = float2fl(m.m[7]);
//     mat[3][2] = float2fl(m.m[11]);
//     return mat;
// }

static inline void mat3_set_skew_symmetric(mat3& m, const vec3& v){
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

static inline mat3 mat3_mul_mat3(mat3& a, const mat3& b){
    mat3 mat;
    mat.m[0] = (((a.m[0] * b.m[0]) + (a.m[1] * b.m[3])) + (a.m[2] * b.m[6]));
    mat.m[1] = (((a.m[0] * b.m[1]) + (a.m[1] * b.m[4])) + (a.m[2] * b.m[7]));
    mat.m[2] = (((a.m[0] * b.m[2]) + (a.m[1] * b.m[5])) + (a.m[2] * b.m[8]));
    mat.m[3] = (((a.m[3] * b.m[0]) + (a.m[4] * b.m[3])) + (a.m[5] * b.m[6]));
    mat.m[4] = (((a.m[3] * b.m[1]) + (a.m[4] * b.m[4])) + (a.m[5] * b.m[7]));
    mat.m[5] = (((a.m[3] * b.m[2]) + (a.m[4] * b.m[5])) + (a.m[5] * b.m[8]));
    mat.m[6] = (((a.m[6] * b.m[0]) + (a.m[7] * b.m[3])) + (a.m[8] * b.m[6]));
    mat.m[7] = (((a.m[6] * b.m[1]) + (a.m[7] * b.m[4])) + (a.m[8] * b.m[7]));
    mat.m[8] = (((a.m[6] * b.m[2]) + (a.m[7] * b.m[5])) + (a.m[8] * b.m[8]));
    return mat;
}


static inline void mat3_mul_scalar(mat3& a, const float& b){
    a.m[0] = (a.m[0] * b);
    a.m[1] = (a.m[1] * b);
    a.m[2] = (a.m[2] * b);
    a.m[3] = (a.m[3] * b);
    a.m[4] = (a.m[4] * b);
    a.m[5] = (a.m[5] * b);
    a.m[6] = (a.m[6] * b);
    a.m[7] = (a.m[7] * b);
    a.m[8] = (a.m[8] * b);
}

static inline void mat3_add_mat3(mat3& a, const mat3& b){
    a.m[0] = (a.m[0] + b.m[0]);     
    a.m[1] = (a.m[1] + b.m[1]);     
    a.m[2] = (a.m[2] + b.m[2]);     
    a.m[3] = (a.m[3] + b.m[3]);     
    a.m[4] = (a.m[4] + b.m[4]);     
    a.m[5] = (a.m[5] + b.m[5]);     
    a.m[6] = (a.m[6] + b.m[6]);     
    a.m[7] = (a.m[7] + b.m[7]);     
    a.m[8] = (a.m[8] + b.m[8]);     
}

static inline mat3 mat3_transpose(const mat3& a){
    mat3 m;
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




// More general rotation without acos
static inline quat rotation_between_vectors(vec3 from, vec3 to) {
    quat result;
    
    // Normalize inputs
    from = vec3_normalize(from);
    to = vec3_normalize(to);
    
    // Calculate half vector
    vec3 half = {
        (from.x + to.x),
        (from.y + to.y),
        (from.z + to.z)
    };
    // Normalize half
    half = vec3_normalize(half);
    
    // Cross product
    result.x = ((from.y * half.z) - (from.z * half.y));
    result.y = ((from.z * half.x) - (from.x * half.z));
    result.z = ((from.x * half.y) - (from.y * half.x));
    
    // W component is dot product of from and half
    result.w = ((from.x * half.x) + ((from.y * half.y) + (from.z * half.z)));
    
    return result;
}


static inline vec3 get_forward_matrix(const quat& q) {
    // Convert quaternion to rotation matrix elements
    float w = q.w;
    float x = q.x;
    float y = q.y;
    float z = q.z;
    
    // Third column of rotation matrix represents forward direction
    vec3 forward = vec3_create(
        (2.0f * ((x * z) + (w * y))),               // x component
        (2.0f * ((y * z) - (w * x))),               // y component
        (1.0f - (2.0f * ((x * x) + (y * y))))      // z component (scaled)
    );
    
    return forward;
}






static inline quat quat_create(float x, float y, float z, float w) {
    quat result = {x, y, z, w};
    return result;
}




// Identity quaternion
static inline quat quat_identity() {
    return quat_create(0, 0, 0, 1.0f);
}

// Normalize a quaternion (keeps rotation valid)
static inline quat quat_normalize(quat q) {
    // Calculate length (just like vector length)
    float length_sq = (
        (q.x * q.x) + 
        (q.y * q.y) + 
        (q.z * q.z) + 
        (q.w * q.w)
    );
    
    float length = fast_sqrtf(length_sq);
    
    // Avoid division by zero
    if (length == 0) {
        return quat_identity();
    }
    
    // Scale each component
    float inv_length = (1.0f / length);
    quat result;
    result.x = (q.x *  inv_length);
    result.y = (q.y *  inv_length);
    result.z = (q.z *  inv_length);
    result.w = (q.w *  inv_length);
    
    return result;
}



static inline quat quat_look_at(vec3 from, vec3 to, vec3 up) {
    // Get forward direction
    vec3 forward = vec3_normalize(to - from);
    
    // Get right vector
    vec3 right = vec3_normalize(vec3_cross(up, forward));
    
    // Get corrected up vector
    vec3 upDir = vec3_cross(forward, right);

    // Now we have our rotation matrix formed by [right, upDir, forward]
    // Convert this rotation matrix to quaternion
    quat result;
    
    float trace = ((right.x + upDir.y) + forward.z);
    
    if(trace > 0) {
        float s = fast_sqrtf((trace + 1.0f));
        result.w = (s / 2.0f);
        s = (1.0f / (2.0f * s));
        result.x = ((upDir.z  - forward.y)* s);
        result.y = ((forward.x- right.z)  * s);
        result.z = ((right.y  - upDir.x)  * s);
    } else {
        // Just picking largest diagonal element for stability
        result.w = (fast_sqrtf((((right.x + upDir.y) + forward.z)+ 1.0f))       / 2.0f);
        result.x = (fast_sqrtf(((right.x + -upDir.y)             + -forward.z)) / 2.0f);
        result.y = (fast_sqrtf(((-right.x + upDir.y)             + -forward.z)) / 2.0f);
        result.z = (fast_sqrtf(((-right.x + -upDir.y)            + forward.z))  / 2.0f);
    }
    
    return quat_normalize(result);
}

inline float sign(float value) {
    // Return +1.0f for positive, -1.0f for negative, 0 for zero
    if (value > 0) return 1.0f;
    if (value < 0) return -1.0f;
    return 0;
}


static inline quat float_nlerp_y_axis(quat q1, quat q2, float t) {
    quat result = {0, 0, 0, 1.0f}; // Start with identity
    
    // print_quat("current rotation: ", q1);
    // print_quat("dest    rotation: ", q2);
    // print_float(     "timestep        : ", t);

    // Step 1: Calculate dot product (simplified for Y-axis rotation)
    float dot = ((q1.y * q2.y) + (q1.w * q2.w));
    // print_float(     "dot             : ", dot);
    
    // Step 2: Ensure shortest path
    if (dot < 0) {
        q2.y = -q2.y;
        q2.w = -q2.w;
    }
    
    // Step 3: Linear interpolation (only Y and W components)
    float one_minus_t = (1.0f - t);
    // print_float(     "one_minus_t     : ", one_minus_t);

    result.y = ((one_minus_t * q1.y) + (t * q2.y));
    result.w = ((one_minus_t * q1.w) + (t * q2.w));

    // print_float(     "result.y        : ", result.y);
    // print_float(     "result.w        : ", result.w);
    
    // Step 4: Normalize (simplified for Y-axis rotation)
    float inv_len = fast_sqrtf(((result.y* result.y) + (result.w * result.w)));
    // print_float(     "inv_len         : ", inv_len);
    
    result.y = (result.y * inv_len);
    result.w = (result.w * inv_len);
    
    // print_float(     "result.y        : ", result.y);
    // print_float(     "result.w        : ", result.w);

    result = quat_normalize(result);

    return result;
}


static inline quat rotate_in_direction_of_movement_y(vec3& direction, quat& rotation){
    quat result = quat_create(0,0,0,1.0f);
    //for floating point/render only rotation
    if(fabs(direction.x) > 0.001 || fabs(direction.z) > 0.001){
    //     glm::vec3 direction = glm::vec3(float2fl(transComp.desired_movement.x), 0,float2fl(transComp.desired_movement.z)); 

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
        float x = direction.x;
        float z = direction.z;

        float len = fast_sqrtf(((x*x) + (z*z)));
        
        if(len == 0)return rotation;

        x = (x / len);
        z = (z / len);
        // Shortcut for quaternion that rotates from (0,0,1) to (x,0,z)
        // This avoids all trigonometric functions

        // The quaternion will have form (x*sin(θ/2), y*sin(θ/2), z*sin(θ/2), cos(θ/2))
        // For Y-axis rotation: (0, sin(θ/2), 0, cos(θ/2))

        // Calculate the half-angle directly from the dot product
        float dot = z;  // dot((0,0,1), (x,0,z)) = z

        // cos(θ/2) can be calculated as sqrt((1 + dot)/2)
        float w = fast_sqrtf(((1.0f +  dot) / 2.0f));

        // sin(θ/2) can be calculated as sqrt((1 - dot)/2)
        float sin_half = fast_sqrtf(((1.0f -  dot) / 2.0f));

        // The sign of y component depends on which way we're rotating
        float y = (x < 0) ? -sin_half : sin_half;

        result.w = w;
        result.y = y;
        return result;
    }
    return rotation;
}
                      



static inline vec4 vec4_create(float a){
    vec4 result = {a, a, a, a};
    return result;
}

static inline vec4 vec4_create(float x, float y, float z, float w){
    vec4 result = {x, y, z, w};
    return result;
}

static inline vec4 vec4_create(vec3 v, float w){
    vec4 result = {v.x, v.y, v.z, w};
    return result;
}

static inline vec2 vec2_create(float v) {
    vec2 result = {v, v};
    return result;
}
static inline vec2 vec2_create(float x, float y) {
    vec2 result = {x, y};
    return result;
}


static inline vec3 vec3_create(float v) {
    vec3 result = {v, v, v};
    return result;
}

static inline vec3 vec3_create(ivec3 v) {
    vec3 result = {};
    result.x = (float)v.x; 
    result.y = (float)v.y; 
    result.z = (float)v.z; 
    return result;
}





inline vec3 vec3_sign(const vec3& v) {
    vec3 result = {};
    result.x = v.x > 0 ? 1 : -1;
    result.y = v.y > 0 ? 1 : -1;
    result.z = v.z > 0 ? 1 : -1;
    return result;
}

static inline vec3 vec3_invert(vec3 a){
    return vec3_create(-a.x, -a.y, -a.z);
}


// Convert vec3 to quat with w=0
static inline quat vec3_to_quat(vec3 v) {
    return quat_create(v.x, v.y, v.z, 0);
}

// Get conjugate (negative xyz, same w)
static inline quat quat_conjugate(quat q) {
    return quat_create(-q.x, -q.y, -q.z, q.w);
}


// Quaternion multiplication - this combines rotations
static inline quat quat_mul(quat a, quat b) {
    quat result = {};
        
    // Scalar part (w)
    result.w = (a.w * b.w) - (a.x * b.x) - (a.y * b.y) - (a.z * b.z);
    
    // Vector part (x, y, z)
    result.x = (a.w * b.x) + (a.x * b.w) + (a.y * b.z) - (a.z * b.y);
    result.y = (a.w * b.y) + (a.y * b.w) + (a.z * b.x) - (a.x * b.z);
    result.z = (a.w * b.z) + (a.z * b.w) + (a.x * b.y) - (a.y * b.x);
    
    return result;
}
// Create rotation from axis and angle
static inline quat quat_from_axis_angle(vec3 axis, float angle) {
    // Normalize axis first
    axis = vec3_normalize(axis);
    
    // Calculate sin(angle/2) and cos(angle/2)
    float half_angle = (angle / 2.0f);
    float s = fast_sinf(half_angle);
    float c = fast_cosf(half_angle);
    
    quat result;
    result.x = (axis.x * s);
    result.y = (axis.y * s);
    result.z = (axis.z * s);
    result.w = c;
    
    return result;
}




// For vec3 component-wise max
inline float vec3_max_comp(vec3 v) {
    return handmade_max(handmade_max((v.x), (v.y)), (v.z));
}

inline float vec3_min_comp(vec3 v) {
    return handmade_min(handmade_min((v.x), (v.y)), (v.z));
}


// If you need component-wise operations between two vectors:
inline vec3 vec3_min(vec3 a, vec3 b) {
    return vec3{
        handmade_min(a.x, b.x),
        handmade_min(a.y, b.y),
        handmade_min(a.z, b.z)
    };
}

inline vec3 vec3_max(vec3 a, vec3 b) {
    return vec3{
        handmade_max(a.x, b.x),
        handmade_max(a.y, b.y),
        handmade_max(a.z, b.z)
    };
}

inline vec4 vec4_min(vec4 a, vec4 b) {
    return vec4{
        handmade_min(a.x, b.x),
        handmade_min(a.y, b.y),
        handmade_min(a.z, b.z),
        handmade_min(a.w, b.w)

    };
}

inline vec4 vec4_max(vec4 a, vec4 b) {
    return vec4{
        handmade_max(a.x, b.x),
        handmade_max(a.y, b.y),
        handmade_max(a.z, b.z),
        handmade_max(a.w, b.w)
    };
}


static inline mat4 mat4_identity(){
        mat4 mat = {};
        mat.m[0]  = 1.0f;
        mat.m[1]  = 0.0f;
        mat.m[2]  = 0.0f;
        mat.m[3]  = 0.0f;

        mat.m[4]  = 0.0f;
        mat.m[5]  = 1.0f;
        mat.m[6]  = 0.0f;
        mat.m[7]  = 0.0f;

        mat.m[8]  = 0.0f;
        mat.m[9]  = 0.0f;
        mat.m[10] = 1.0f;
        mat.m[11] = 0.0f;

        mat.m[12] = 0.0f;
        mat.m[13] = 0.0f;
        mat.m[14] = 0.0f;
        mat.m[15] = 1.0f;
        return mat;
    }


    static inline   void print_mat4(mat4& mat){
        printf("%10.5f %10.5f %10.5f %10.5f\n", mat.m[0],  mat.m[1],  mat.m[2],  mat.m[3]);
        printf("%10.5f %10.5f %10.5f %10.5f\n", mat.m[4],  mat.m[5],  mat.m[6],  mat.m[7]);
        printf("%10.5f %10.5f %10.5f %10.5f\n", mat.m[8],  mat.m[9],  mat.m[10], mat.m[11]);
        printf("%10.5f %10.5f %10.5f %10.5f\n", mat.m[12], mat.m[13], mat.m[14], mat.m[15]);
    }


    

    static inline void mat4_rotate(mat4* dest,
    const mat4* src,
    float angle,    // in radians
    vec3 rotation_axis)
{
    // 1) Normalize rotation axis
    rotation_axis = vec3_normalize(rotation_axis);

    // 2) Compute trig terms
    float c  = fast_cosf(angle);
    float s  = fast_sinf(angle);
    float t = 1.0f - c;   

    // 3) Build the 4×4 rotation matrix R (column‑major)
    float R[16];
    R[0] = rotation_axis.x*rotation_axis.x*t + c                ;     R[4] = rotation_axis.x*rotation_axis.y*t - rotation_axis.z*s;   R[8]  = rotation_axis.x*rotation_axis.z*t + rotation_axis.y*s;   R[12] = 0.0f;
    R[1] = rotation_axis.y*rotation_axis.x*t + rotation_axis.z*s;     R[5] = rotation_axis.y*rotation_axis.y*t + c;                   R[9]  = rotation_axis.y*rotation_axis.z*t - rotation_axis.x*s;   R[13] = 0.0f;
    R[2] = rotation_axis.z*rotation_axis.x*t - rotation_axis.y*s;     R[6] = rotation_axis.z*rotation_axis.y*t + rotation_axis.x*s;   R[10] = rotation_axis.z*rotation_axis.z*t + c;                   R[14] = 0.0f;
    R[3] = 0.0f;                R[7] = 0.0f;              R[11] = 0.0f;              R[15] = 1.0f;


    // 4) Multiply src * R → dest (column‑major multiply)
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                // src element at (row, k) is src->m[k*4 + row]
                // R element at (k, col) is R[col*4 + k]
                sum += src->m[k*4 + row] * R[col*4 + k];
            }
            dest->m[col*4 + row] = sum;
        }
    }
}


static inline void mat4_lookAt(mat4* dest,
    vec3 eye,
    vec3 center,
    vec3 up)
{
    if((fabs(eye.x - center.x) < 0.001f) &&
       (fabs(eye.y - center.y) < 0.001f) &&
       (fabs(eye.z - center.z) < 0.001f)){
        *dest = mat4_identity();
        return;
    }

    // Calculate z axis = normalized(eye - center)
    float z0 = eye.x - center.x;
    float z1 = eye.y - center.y;
    float z2 = eye.z - center.z;

    float z_len = fast_sqrtf(z0*z0 + z1*z1 + z2*z2);

    // Normalize z axis
    z0 /= z_len;
    z1 /= z_len;
    z2 /= z_len;

    // Calculate x axis = normalized(cross(up, z))
    float x0 = up.y * z2 - up.z * z1;
    float x1 = up.z * z0 - up.x * z2;
    float x2 = up.x * z1 - up.y * z0;

    float x_len = fast_sqrtf(x0*x0 + x1*x1 + x2*x2);

    // Handle degenerate case where up and z are parallel
    if(x_len < 0.000001f) {
        // Choose arbitrary x axis perpendicular to z
        vec3 arbitrary = (fabs(z0) < 0.9f) ? 
            (vec3_create(1.0f, 0.0f, 0.0f)) : (vec3_create(0.0f, 1.0f, 0.0f));
            
        // Recalculate x = cross(arbitrary, z)
        x0 = arbitrary.y * z2 - arbitrary.z * z1;
        x1 = arbitrary.z * z0 - arbitrary.x * z2;
        x2 = arbitrary.x * z1 - arbitrary.y * z0;
        
        x_len = fast_sqrtf(x0*x0 + x1*x1 + x2*x2);
    }

    // Normalize x axis
    x0 /= x_len;
    x1 /= x_len;
    x2 /= x_len;

    // Calculate y axis = cross(z, x)
    float y0 = z1 * x2 - z2 * x1;
    float y1 = z2 * x0 - z0 * x2;
    float y2 = z0 * x1 - z1 * x0;

    // No need to check length or normalize y - it's guaranteed to be unit length
    // since x and z are orthogonal unit vectors

    // Build rotation matrix (transposed) - first three columns are the x, y, z basis vectors
    dest->m[0] = x0;
    dest->m[1] = y0;
    dest->m[2] = z0;
    dest->m[3] = 0;

    dest->m[4] = x1;
    dest->m[5] = y1;
    dest->m[6] = z1;
    dest->m[7] = 0;

    dest->m[8] = x2;
    dest->m[9] = y2;
    dest->m[10] = z2;
    dest->m[11] = 0;

    // Translation component - dot products of eye with negative basis vectors
    dest->m[12] = -(x0 * eye.x + x1 * eye.y + x2 * eye.z);
    dest->m[13] = -(y0 * eye.x + y1 * eye.y + y2 * eye.z);
    dest->m[14] = -(z0 * eye.x + z1 * eye.y + z2 * eye.z);
    dest->m[15] = 1;
    
}


static inline void mat4_perspective(mat4* dest, 
    float fovy, 
    float aspect, 
    float zNear, 
    float zFar) {
    // Calculate the focal length
    float f = 1.0f / fast_tanf(fovy * 0.5f);
    
    // Clear matrix first
    memset(dest->m, 0, sizeof(float) * 16);
    
    // Set perspective transformation values
    dest->m[0]  = f / aspect;
    dest->m[5]  = -f; // Vulkan NDC Y-flip
    
    // Vulkan uses depth range [0, 1] instead of [-1, 1]
    dest->m[10] = zFar / (zNear - zFar);
    dest->m[11] = -1.0f;
    dest->m[14] = -(zFar * zNear) / (zFar - zNear);
    
    // Other elements remain 0
}


// Billboard matrix function
// Makes an object always face the camera
static inline void mat4_billboard(mat4* dest, 
    vec3 objectPosition, 
    vec3 cameraPosition,
    vec3 upVector)
{
    // 1. Start with identity matrix
    *dest = mat4_identity();

    // 2. Calculate the direction from object to camera (this will be our forward vector)
    vec3 forward = {
    cameraPosition.x - objectPosition.x,
    cameraPosition.y - objectPosition.y,
    cameraPosition.z - objectPosition.z
    };
    forward = vec3_normalize(forward);

    // 3. Calculate right vector (cross product of up and forward)
    vec3 right = vec3_cross(upVector, forward);
    right = vec3_normalize(right);

    // 4. Recalculate corrected up vector (cross product of forward and right)
    vec3 up = vec3_cross(forward, right);

    // 5. Build rotation matrix (column-major)
    // First column (right vector)
    dest->m[0] = right.x;
    dest->m[1] = right.y;
    dest->m[2] = right.z;

    // Second column (up vector)
    dest->m[4] = up.x;
    dest->m[5] = up.y;
    dest->m[6] = up.z;

    // Third column (forward vector)
    dest->m[8] = forward.x;
    dest->m[9] = forward.y;
    dest->m[10] = forward.z;

    // 6. Set translation to object position
    dest->m[12] = objectPosition.x;
    dest->m[13] = objectPosition.y;
    dest->m[14] = objectPosition.z;
}

static inline void mat4_billboard_camera_relative(mat4* dest, 
    vec3 objectPosition, 
    vec3 cameraPosition)
{
// For camera-relative rendering, we just need a rotation matrix
// that aligns the object with the view direction

// 1. Start with identity matrix
*dest = mat4_identity();

// 2. Calculate object position relative to camera
vec3 relative_pos = {
objectPosition.x - cameraPosition.x,
objectPosition.y - cameraPosition.y,
objectPosition.z - cameraPosition.z
};

// 3. In camera-relative rendering, the camera is at the origin (0,0,0)
// and the forward direction is along -z
// We want the billboard to face the origin

// Calculate forward vector (from object to camera origin)
vec3 forward = vec3_normalize(relative_pos);
forward.x = -forward.x;  // Invert because we want to face the camera
forward.y = -forward.y;
forward.z = -forward.z;

// 4. Choose an up vector (typically world up)
vec3 upVector = {0.0f, 1.0f, 0.0f};

// 5. Calculate right vector (cross product of up and forward)
vec3 right = vec3_cross(upVector, forward);
right = vec3_normalize(right);

// 6. Recalculate corrected up vector (cross product of forward and right)
vec3 up = vec3_cross(forward, right);

// 7. Build rotation matrix (column-major)
// First column (right vector)
dest->m[0] = right.x;
dest->m[1] = right.y;
dest->m[2] = right.z;

// Second column (up vector)
dest->m[4] = up.x;
dest->m[5] = up.y;
dest->m[6] = up.z;

// Third column (forward vector)
dest->m[8] = forward.x;
dest->m[9] = forward.y;
dest->m[10] = forward.z;

// 8. Set translation to object position relative to camera
dest->m[12] = relative_pos.x;
dest->m[13] = relative_pos.y;
dest->m[14] = relative_pos.z;
}



static inline vec4 mat4_row0(mat4& m){
    vec4 result = {};
    result.x = m.m[0];
    result.y = m.m[1];
    result.z = m.m[2];
    result.w = m.m[3];
    return result;
}
static inline vec4 mat4_row1(mat4& m){
    vec4 result = {};
    result.x = m.m[4];
    result.y = m.m[5];
    result.z = m.m[6];
    result.w = m.m[7];
    return result;
}
static inline vec4 mat4_row2(mat4& m){
    vec4 result = {};
    result.x = m.m[8];
    result.y = m.m[9];
    result.z = m.m[10];
    result.w = m.m[11];
    return result;
}
static inline vec4 mat4_row3(mat4& m){
    vec4 result = {};
    result.x = m.m[12];
    result.y = m.m[13];
    result.z = m.m[14];
    result.w = m.m[15];
    return result;
}
static inline vec4 mat4_col0(mat4& m){
    vec4 result = {};
    result.x = m.m[0];
    result.y = m.m[4];
    result.z = m.m[8];
    result.w = m.m[12];
    return result;
}
static inline vec4 mat4_col1(mat4& m){
    vec4 result = {};
    result.x = m.m[1];
    result.y = m.m[5];
    result.z = m.m[9];
    result.w = m.m[13];
    return result;
}
static inline vec4 mat4_col2(mat4& m){
    vec4 result = {};
    result.x = m.m[2];
    result.y = m.m[6];
    result.z = m.m[10];
    result.w = m.m[14];
    return result;
}
static inline vec4 mat4_col3(mat4& m){
    vec4 result = {};
    result.x = m.m[3];
    result.y = m.m[7];
    result.z = m.m[11];
    result.w = m.m[15];
    return result;
}


static inline quat quat_inverse(const quat& q) {
    // Compute the conjugate
    quat conj = quat_conjugate(q);
    
    // Compute the dot product (magnitude squared)
    float dot = quat_dot(q, q);
    
    // Divide the conjugate by the dot product
    quat inv;
    inv.w = (conj.w / dot);
    inv.x = (conj.x / dot);
    inv.y = (conj.y / dot);
    inv.z = (conj.z / dot);
    
    return inv;
}



static inline void quat_add_scaled_vector(quat& q, const vec3& v, const float& scale){
    quat temp_quat = {0};
    temp_quat.x = (v.x * scale);
    temp_quat.y = (v.y * scale);
    temp_quat.z = (v.z * scale);
    temp_quat.w = 0;

    // printf("given rotation vector: %3.3f %3.3f %3.3f\n", float2fl(v.x),float2fl(v.y),float2fl(v.z));
    // printf("calculated quat      : %3.3f %3.3f %3.3f %3.3f\n", float2fl(temp_quat.x),float2fl(temp_quat.y),float2fl(temp_quat.z), float2fl(temp_quat.w));

    temp_quat = quat_mul(temp_quat, q);

    // printf("post mult quat       : %3.3f %3.3f %3.3f %3.3f\n", float2fl(temp_quat.x),float2fl(temp_quat.y),float2fl(temp_quat.z), float2fl(temp_quat.w));

    q.w = (q.w + (temp_quat.w * 0.5f));
    q.x = (q.x + (temp_quat.x * 0.5f));
    q.y = (q.y + (temp_quat.y * 0.5f));
    q.z = (q.z + (temp_quat.z * 0.5f));
} 



static inline vec4 mat4_vec4_product(mat4& m, vec4& v){
    vec4 result = {};
    result.x = m.m[0] * v.x + m.m[4] * v.y + m.m[8]  * v.z + m.m[12] * v.w;
    result.y = m.m[1] * v.x + m.m[5] * v.y + m.m[9]  * v.z + m.m[13] * v.w;
    result.z = m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z + m.m[14] * v.w;
    result.w = m.m[3] * v.x + m.m[7] * v.y + m.m[11] * v.z + m.m[15] * v.w;
    return result;
}


static inline vec4 vec4_multiply(vec4 a, vec4 b) {
    vec4 result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
    result.w = a.w * b.w;
    return result;
}

static inline bool stable_mat4_inverse(const mat4* m, mat4* result) {
    // Direct approach with added numerical safeguards
    float coef00 = m->m[10] * m->m[15] - m->m[14] * m->m[11];
    float coef02 = m->m[6] * m->m[15] - m->m[14] * m->m[7];
    float coef03 = m->m[6] * m->m[11] - m->m[10] * m->m[7];
    
    float coef04 = m->m[9] * m->m[15] - m->m[13] * m->m[11];
    float coef06 = m->m[5] * m->m[15] - m->m[13] * m->m[7];
    float coef07 = m->m[5] * m->m[11] - m->m[9] * m->m[7];
    
    float coef08 = m->m[9] * m->m[14] - m->m[13] * m->m[10];
    float coef10 = m->m[5] * m->m[14] - m->m[13] * m->m[6];
    float coef11 = m->m[5] * m->m[10] - m->m[9] * m->m[6];
    
    float coef12 = m->m[8] * m->m[15] - m->m[12] * m->m[11];
    float coef14 = m->m[4] * m->m[15] - m->m[12] * m->m[7];
    float coef15 = m->m[4] * m->m[11] - m->m[8] * m->m[7];
    
    float coef16 = m->m[8] * m->m[14] - m->m[12] * m->m[10];
    float coef18 = m->m[4] * m->m[14] - m->m[12] * m->m[6];
    float coef19 = m->m[4] * m->m[10] - m->m[8] * m->m[6];
    
    float coef20 = m->m[8] * m->m[13] - m->m[12] * m->m[9];
    float coef22 = m->m[4] * m->m[13] - m->m[12] * m->m[5];
    float coef23 = m->m[4] * m->m[9] - m->m[8] * m->m[5];
    
    vec4 fac0 = vec4_create(coef00, coef00, coef02, coef03);
    vec4 fac1 = vec4_create(coef04, coef04, coef06, coef07);
    vec4 fac2 = vec4_create(coef08, coef08, coef10, coef11);
    vec4 fac3 = vec4_create(coef12, coef12, coef14, coef15);
    vec4 fac4 = vec4_create(coef16, coef16, coef18, coef19);
    vec4 fac5 = vec4_create(coef20, coef20, coef22, coef23);
    
    vec4 vec0 = vec4_create(m->m[4], m->m[0], m->m[0], m->m[0]);
    vec4 vec1 = vec4_create(m->m[5], m->m[1], m->m[1], m->m[1]);
    vec4 vec2 = vec4_create(m->m[6], m->m[2], m->m[2], m->m[2]);
    vec4 vec3 = vec4_create(m->m[7], m->m[3], m->m[3], m->m[3]);
    
    vec4 inv0 = vec4_create(
        vec1.x * fac0.x - vec2.x * fac1.x + vec3.x * fac2.x,
        vec1.y * fac0.y - vec2.y * fac1.y + vec3.y * fac2.y,
        vec1.z * fac0.z - vec2.z * fac1.z + vec3.z * fac2.z,
        vec1.w * fac0.w - vec2.w * fac1.w + vec3.w * fac2.w
    );
    
    vec4 inv1 = vec4_create(
        vec0.x * fac0.x - vec2.x * fac3.x + vec3.x * fac4.x,
        vec0.y * fac0.y - vec2.y * fac3.y + vec3.y * fac4.y,
        vec0.z * fac0.z - vec2.z * fac3.z + vec3.z * fac4.z,
        vec0.w * fac0.w - vec2.w * fac3.w + vec3.w * fac4.w
    );
    
    vec4 inv2 = vec4_create(
        vec0.x * fac1.x - vec1.x * fac3.x + vec3.x * fac5.x,
        vec0.y * fac1.y - vec1.y * fac3.y + vec3.y * fac5.y,
        vec0.z * fac1.z - vec1.z * fac3.z + vec3.z * fac5.z,
        vec0.w * fac1.w - vec1.w * fac3.w + vec3.w * fac5.w
    );
    
    vec4 inv3 = vec4_create(
        vec0.x * fac2.x - vec1.x * fac4.x + vec2.x * fac5.x,
        vec0.y * fac2.y - vec1.y * fac4.y + vec2.y * fac5.y,
        vec0.z * fac2.z - vec1.z * fac4.z + vec2.z * fac5.z,
        vec0.w * fac2.w - vec1.w * fac4.w + vec2.w * fac5.w
    );
    
    // Apply sign corrections
    vec4 signA = vec4_create(1.0f, -1.0f, 1.0f, -1.0f);
    vec4 signB = vec4_create(-1.0f, 1.0f, -1.0f, 1.0f);
    
    // Element-wise multiplication
    inv0 = vec4_multiply(inv0, signA);
    inv1 = vec4_multiply(inv1, signB);
    inv2 = vec4_multiply(inv2, signA);
    inv3 = vec4_multiply(inv3, signB);
    
    
    // Compute determinant using the first row/column
    float det = 
        m->m[0] * inv0.x +
        m->m[1] * inv1.x +
        m->m[2] * inv2.x +
        m->m[3] * inv3.x;
    
    // Check for numerical instability
    const float EPSILON = 1e-8f;
    if (fabs(det) < EPSILON) {
        // Matrix is nearly singular - return identity or indicate failure
        *result = mat4_identity();
        return false;
    }
    
    // Calculate inverse by dividing by determinant
    float oneOverDet = 1.0f / det;
    
    // Scale and store result
    result->m[0]  = inv0.x * oneOverDet;
    result->m[1]  = inv0.y * oneOverDet;
    result->m[2]  = inv0.z * oneOverDet;
    result->m[3]  = inv0.w * oneOverDet;
    result->m[4]  = inv1.x * oneOverDet;
    result->m[5]  = inv1.y * oneOverDet;
    result->m[6]  = inv1.z * oneOverDet;
    result->m[7]  = inv1.w * oneOverDet;
    result->m[8]  = inv2.x * oneOverDet;
    result->m[9]  = inv2.y * oneOverDet;
    result->m[10] = inv2.z * oneOverDet;
    result->m[11] = inv2.w * oneOverDet;
    result->m[12] = inv3.x * oneOverDet;
    result->m[13] = inv3.y * oneOverDet;
    result->m[14] = inv3.z * oneOverDet;
    result->m[15] = inv3.w * oneOverDet;
    
    return true;
}

static inline bool test_mat4_inverse(mat4* result, const mat4* m) {
    // Start with an augmented matrix [M|I]
    float aug[4][8];
    int i, j, k;
    float max, temp, factor;
    
    // Initialize the augmented matrix
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            aug[i][j] = m->m[i * 4 + j];
            aug[i][j + 4] = (i == j) ? 1.0f : 0.0f;
        }
    }
    
    // Perform Gauss-Jordan elimination with partial pivoting
    for (i = 0; i < 4; i++) {
        // Find the pivot row
        max = fabsf(aug[i][i]);
        int pivot_row = i;
        
        for (j = i + 1; j < 4; j++) {
            if (fabsf(aug[j][i]) > max) {
                max = fabsf(aug[j][i]);
                pivot_row = j;
            }
        }
        
        // Check for singularity (very small pivot indicates a non-invertible matrix)
        if (max < 1e-10f) {
            // Matrix is singular or nearly singular
            memset(result->m, 0, sizeof(float) * 16);
            return false;
        }
        
        // Swap rows if needed
        if (pivot_row != i) {
            for (j = 0; j < 8; j++) {
                temp = aug[i][j];
                aug[i][j] = aug[pivot_row][j];
                aug[pivot_row][j] = temp;
            }
        }
        
        // Scale the pivot row
        float pivot = aug[i][i];
        for (j = 0; j < 8; j++) {
            aug[i][j] /= pivot;
        }
        
        // Eliminate other rows
        for (j = 0; j < 4; j++) {
            if (j != i) {
                factor = aug[j][i];
                for (k = 0; k < 8; k++) {
                    aug[j][k] -= factor * aug[i][k];
                }
            }
        }
    }
    
    // Extract the inverse matrix
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            result->m[i * 4 + j] = aug[i][j + 4];
        }
    }
    
    return true;
}


static inline bool mat4_inverse(const float* m, float* inv)
{
    // Calculate coefficients for the cofactor matrix
    float coef00 = m[10] * m[15] - m[14] * m[11];
    float coef02 = m[6] * m[15] - m[14] * m[7];
    float coef03 = m[6] * m[11] - m[10] * m[7];

    float coef04 = m[9] * m[15] - m[13] * m[11];
    float coef06 = m[5] * m[15] - m[13] * m[7];
    float coef07 = m[5] * m[11] - m[9] * m[7];

    float coef08 = m[9] * m[14] - m[13] * m[10];
    float coef10 = m[5] * m[14] - m[13] * m[6];
    float coef11 = m[5] * m[10] - m[9] * m[6];

    float coef12 = m[8] * m[15] - m[12] * m[11];
    float coef14 = m[4] * m[15] - m[12] * m[7];
    float coef15 = m[4] * m[11] - m[8] * m[7];

    float coef16 = m[8] * m[14] - m[12] * m[10];
    float coef18 = m[4] * m[14] - m[12] * m[6];
    float coef19 = m[4] * m[10] - m[8] * m[6];

    float coef20 = m[8] * m[13] - m[12] * m[9];
    float coef22 = m[4] * m[13] - m[12] * m[5];
    float coef23 = m[4] * m[9] - m[8] * m[5];

    // First row of cofactor matrix
    float fac0[4] = { coef00, coef00, coef02, coef03 };
    float fac1[4] = { coef04, coef04, coef06, coef07 };
    float fac2[4] = { coef08, coef08, coef10, coef11 };
    float fac3[4] = { coef12, coef12, coef14, coef15 };
    float fac4[4] = { coef16, coef16, coef18, coef19 };
    float fac5[4] = { coef20, coef20, coef22, coef23 };

    // Matrix elements for adjugate calculations
    float vec0[4] = { m[4], m[0], m[0], m[0] };
    float vec1[4] = { m[5], m[1], m[1], m[1] };
    float vec2[4] = { m[6], m[2], m[2], m[2] };
    float vec3[4] = { m[7], m[3], m[3], m[3] };

    // Calculate rows of the adjugate matrix
    float inv0[4];
    float inv1[4];
    float inv2[4];
    float inv3[4];

    for (int i = 0; i < 4; i++) {
        inv0[i] = vec1[i] * fac0[i] - vec2[i] * fac1[i] + vec3[i] * fac2[i];
        inv1[i] = vec0[i] * fac0[i] - vec2[i] * fac3[i] + vec3[i] * fac4[i];
        inv2[i] = vec0[i] * fac1[i] - vec1[i] * fac3[i] + vec3[i] * fac5[i];
        inv3[i] = vec0[i] * fac2[i] - vec1[i] * fac4[i] + vec2[i] * fac5[i];
    }

    // Apply sign corrections
    float signA[4] = { 1.0f, -1.0f, 1.0f, -1.0f };
    float signB[4] = { -1.0f, 1.0f, -1.0f, 1.0f };

    // Multiply by signs
    for (int i = 0; i < 4; i++) {
        inv0[i] *= signA[i];
        inv1[i] *= signB[i];
        inv2[i] *= signA[i];
        inv3[i] *= signB[i];
    }

    // Calculate the determinant
    float row0[4] = { inv0[0], inv1[0], inv2[0], inv3[0] };
    float dot0[4];
    
    // Dot product of first row of original matrix with first column of adjugate
    for (int i = 0; i < 4; i++) {
        dot0[i] = m[i * 4] * row0[i];
    }
    
    float det = dot0[0] + dot0[1] + dot0[2] + dot0[3];

    // Check if matrix is invertible
    if (fabs(det) < 1e-8f) {
        return false;  // Matrix is not invertible
    }

    float oneOverDet = 1.0f / det;

    // Scale the adjugate matrix to get the inverse
    // Store the result in column-major order
    inv[0] = inv0[0] * oneOverDet;
    inv[1] = inv0[1] * oneOverDet;
    inv[2] = inv0[2] * oneOverDet;
    inv[3] = inv0[3] * oneOverDet;
    
    inv[4] = inv1[0] * oneOverDet;
    inv[5] = inv1[1] * oneOverDet;
    inv[6] = inv1[2] * oneOverDet;
    inv[7] = inv1[3] * oneOverDet;
    
    inv[8] = inv2[0] * oneOverDet;
    inv[9] = inv2[1] * oneOverDet;
    inv[10] = inv2[2] * oneOverDet;
    inv[11] = inv2[3] * oneOverDet;
    
    inv[12] = inv3[0] * oneOverDet;
    inv[13] = inv3[1] * oneOverDet;
    inv[14] = inv3[2] * oneOverDet;
    inv[15] = inv3[3] * oneOverDet;

    return true;
}



static inline vec3 CastMouseRay(float windoWidth, float windowHeight, mat4& invProj, mat4& invView, int x, int y){
    float rayX = (2.0f * x) / windoWidth - 1.0f;
    float rayY = 1.0f - (2.0f * y) / windowHeight;
    float rayZ = 1.0f;
    vec4 rayClip = vec4_create(rayX, rayY, -1.0f, 1.0f);


    vec4 rayEye = mat4_vec4_product(invProj, rayClip);
    //need to invert the rayEye.y for vulkan coordinates
    rayEye = vec4_create(rayEye.x, -rayEye.y, -1.0f, 0.0f); //unproject xy part, manually set zw to forwards and not a point


    vec4 temp = mat4_vec4_product(invView, rayEye);

    vec3 rayWorld = vec3_create(temp.x, temp.y, temp.z);
    // printf("rayWorld: %f %f %f\n", rayWorld.x, rayWorld.y, rayWorld.z);
    return vec3_normalize(rayWorld);
}

static inline mat4 mat4_translate(vec3 v){
    mat4 result = mat4_identity();
    result.m[12] = v.x;
    result.m[13] = v.y;
    result.m[14] = v.z;
    return result;
}

static inline mat4 mat4_mul(mat4& a, mat4& b){
    mat4 result = {};
    for(int col = 0; col < 4; ++col){
        for(int row = 0; row < 4; ++row){
            result.m[col * 4 + row] = 
                a.m[0 * 4 + row] * b.m[col * 4 + 0] +
                a.m[1 * 4 + row] * b.m[col * 4 + 1] +
                a.m[2 * 4 + row] * b.m[col * 4 + 2] +
                a.m[3 * 4 + row] * b.m[col * 4 + 3];
        }
    }
    return result;
}
static inline mat4 quat_to_mat4(quat q) {
    float xx = q.x * q.x;
    float yy = q.y * q.y;
    float zz = q.z * q.z;
    float xz = q.x * q.z;
    float xy = q.x * q.y;
    float yz = q.y * q.z;
    float wx = q.w * q.x;
    float wy = q.w * q.y;
    float wz = q.w * q.z;

    mat4 m = mat4_identity();

    m.m[0] = 1.0f - 2.0f * (yy + zz);
    m.m[1] = 2.0f * (xy + wz);
    m.m[2] = 2.0f * (xz - wy);
    m.m[3] = 0.0f;

    m.m[4] = 2.0f * (xy - wz);
    m.m[5] = 1.0f - 2.0f * (xx + zz);
    m.m[6] = 2.0f * (yz + wx);
    m.m[7] = 0.0f;

    m.m[8]  = 2.0f * (xz + wy);
    m.m[9]  = 2.0f * (yz - wx);
    m.m[10] = 1.0f - 2.0f * (xx + yy);
    m.m[11] = 0.0f;

    m.m[12] = 0.0f;
    m.m[13] = 0.0f;
    m.m[14] = 0.0f;
    m.m[15] = 1.0f;

    return m;
}

static inline vec3 vec3_negate(vec3 v){
    vec3 result = {};
    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;
    return result;
}

static inline void scale_mat4(mat4&mat, vec3& s){
    mat.m[0]  = s.x;
    mat.m[5]  = s.y;
    mat.m[10] = s.z;
}
static inline void scale_mat4(mat4& mat, float s){
    mat.m[0]  = s;
    mat.m[5]  = s;
    mat.m[10] = s;
}

static inline mat4 mat4_scale(vec3 s) {
    mat4 m = {0};

    m.m[0]  = s.x;
    m.m[5]  = s.y;
    m.m[10] = s.z;
    m.m[15] = 1.0f;

    return m;
}

inline float lerp(float x, float a, float b){
    return a + x * (b - a);
}

// Linear interpolation for quaternions (LERP)
// Note: This doesn't maintain constant angular velocity
static inline quat lerp(quat& a, quat& b, float t) {
    // Clamp t between 0 and 1
    t = handmade_max(0.0f, handmade_min(1.0f, t));
    
    quat result = {};
    result.x = a.x * (1.0f - t) + b.x * t;
    result.y = a.y * (1.0f - t) + b.y * t;
    result.z = a.z * (1.0f - t) + b.z * t;
    result.w = a.w * (1.0f - t) + b.w * t;
    result = quat_normalize(result);
    return result;
}



static inline quat nlerp_weighted(quat* qs, float* weights, int count){
    quat result = {};
    //sum all weighted quaternions
    for(int i = 0; i < count; i++){
        quat q = qs[i];
        //flip sing if necessary for shortest path (relative to first quarternion)
        if(i > 0){
            float dot = q.x * qs[0].x + q.y * qs[0].y + q.z * qs[0].z + q.w * qs[0].w;
            if(dot < 0){
                q.x = -q.x; q.y = -q.y; q.z = -q.z; q.w = -q.w;
            } 
        }
        result.x += q.x * weights[i];
        result.y += q.y * weights[i];
        result.z += q.z * weights[i];
        result.w += q.w * weights[i];
    }
    result = quat_normalize(result);
    return result;
}

//normalized linear interpolation
static inline quat nlerp(quat q1, quat q2, float t) {
    quat result;
    
    // Step 1: Calculate dot product to determine if we need to flip
    float dot = (((q1.x * q2.x) + (q1.y * q2.y)) + ((q1.z * q2.z) + (q1.w * q2.w)));
    
    // Step 2: Ensure shortest path by flipping the second quaternion if needed
    if (dot < 0) {
        q2.x = -q2.x;
        q2.y = -q2.y;
        q2.z = -q2.z;
        q2.w = -q2.w;
    }
    
    // Step 3: Simple linear interpolation between components
    result = lerp(q1, q2, t);
    
    // Step 4: Normalize the result to ensure it's a valid quaternion
    float inv_len = 1.0f / fast_sqrtf((((result.x * result.x) + (result.y * result.y)) + ((result.z * result.z) + (result.w * result.w))));
    result.x = (result.x * inv_len);
    result.y = (result.y * inv_len);
    result.z = (result.z * inv_len);
    result.w = (result.w * inv_len);
    
    return result;
}


// Spherical Linear Interpolation (SLERP) for quaternions
static inline quat slerp(quat& a, quat& b, float t) {
    // Clamp t between 0 and 1
    t = handmade_max(0.0f, handmade_min(1.0f, t));
    
    // Get cosine of angle between quaternions
    float cosTheta = quat_dot(a, b);
    quat bCopy = b;
    
    // If cosTheta is negative, we need to negate one quaternion
    // This ensures we take the shorter path around the sphere
    if (cosTheta < 0.0f) {
        bCopy.x = -b.x;
        bCopy.y = -b.y;
        bCopy.z = -b.z;
        bCopy.w = -b.w;
        cosTheta = -cosTheta;
    }
    
    // If quaternions are very close, use linear interpolation to avoid division by zero
    if (cosTheta > 0.9995f) {
        return lerp(a, bCopy, t);
    }
    
    // Use spherical interpolation
    float theta = acosf(cosTheta);
    float sinTheta = fast_sinf(theta);
    
    float factorA = fast_sinf((1.0f - t) * theta) / sinTheta;
    float factorB = fast_sinf(t * theta) / sinTheta;
    
    quat result = {};
    result.x = a.x * factorA + bCopy.x * factorB;
    result.y = a.y * factorA + bCopy.y * factorB;
    result.z = a.z * factorA + bCopy.z * factorB;
    result.w = a.w * factorA + bCopy.w * factorB;
    return result;
}



static inline vec3 mix(vec3& start, vec3& end, float t){
    vec3 result = {};
    result.x = (start.x * (1.0f - t) + (end.x * t));
    result.y = (start.y * (1.0f - t) + (end.y * t));
    result.z = (start.z * (1.0f - t) + (end.z * t));
    return result;
}

static inline vec3 vec3_weighted_sum(vec3* vecs, float* weights, int count){
    vec3 result = {};
    for(int i = 0; i < count; i++){
        result.x += vecs[i].x * weights[i];
        result.y += vecs[i].y * weights[i];
        result.z += vecs[i].z * weights[i];
    }
    return result;
}



// Convert a 3D world position to 2D screen coordinates
static inline vec2 WorldToScreen(vec3 worldPos, 
    mat4& viewMatrix, 
    mat4& projMatrix, 
    float windowWidth, 
    float windowHeight) 
{
vec4 worldPos4  = vec4_create(worldPos.x, worldPos.y, worldPos.z, 1.0f);
// Step 1: Transform world position to view space (camera space)
vec4 viewPos = mat4_vec4_product(viewMatrix, worldPos4);

// Step 2: Transform from view space to clip space
vec4 clipPos = mat4_vec4_product(projMatrix, viewPos);

// Step 3: Perspective division to get normalized device coordinates (NDC)
vec3 ndcPos = {
clipPos.x / clipPos.w,
clipPos.y / clipPos.w,
clipPos.z / clipPos.w
};

// Step 4: Convert NDC to window coordinates
vec2 screenPos = {
windowWidth * (ndcPos.x + 1.0f) / 2.0f,
windowHeight * (1.0f - ((ndcPos.y + 1.0f) / 2.0f)) // Flip Y for screen coordinates
};

return screenPos;
}

// For camera-relative rendering, you need to adjust the world position first
static inline vec2 WorldToScreen_CameraRelative(vec3 worldPos, 
                  vec3 cameraPos,
                  mat4& viewMatrix, 
                  mat4& projMatrix, 
                  float windowWidth, 
                  float windowHeight) 
{
// Adjust world position for camera-relative rendering
vec3 relativePos = {
worldPos.x - cameraPos.x,
worldPos.y - cameraPos.y,
worldPos.z - cameraPos.z
};

// Now use the standard projection with the relative position
// Since your view matrix is already set up for camera at origin
vec4 relPos = vec4_create(relativePos.x, relativePos.y, relativePos.z, 1.0f);
vec4 viewPos = mat4_vec4_product(viewMatrix, relPos);

// Continue with projection as before
vec4 clipPos = mat4_vec4_product(projMatrix, viewPos);

// Perspective division
vec3 ndcPos = {
clipPos.x / clipPos.w,
clipPos.y / clipPos.w,
clipPos.z / clipPos.w
};

// NDC to window coordinates
vec2 screenPos = {
windowWidth * (ndcPos.x + 1.0f) / 2.0f,
windowHeight * (1.0f - ((ndcPos.y + 1.0f) / 2.0f)) // Flip Y for screen coordinates
};

// You can also return the NDC z value (between -1 and 1) to test if object is in front of camera
// If ndcPos.z is > 1.0 or < -1.0, the point is outside the view frustum

return screenPos;
}



static inline float HalfLifeStep(float halfLife, float current, float target, float dt){
    float k = 0.69314718056f / (halfLife + 1e-6f);//ln(2)/t_half
    float a = 1.0f - expf(-k * dt);
    return ((current) + (target - current) * a); 
}


#endif //MATH_H