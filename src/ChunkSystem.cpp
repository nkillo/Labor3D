
// #include "labour/camera/WorldTransform.h"
// #include "labour/core/dispatcher.h"
// #include "labour/collision/ray.h"
// #include "labour/collision/boxIntersect.h"
#include "chunkManager.h"
#include "ChunkSystem.h"
// #include "labour/collision/entity_bvh/bvh.h"
// #include "labour/ecs/systems/PhysicsSystem.h"
// #include "labour/camera/frustum.h"
// #include "labour/util/performance_timer.h"
// #include "labour/util/meshManager.h"
// #include "imguiDocking/imgui.h"
// #include "labour/ecs/entity/entity.h"

// #include "labour/platform_game_context/shaderManager.h"
// #include "labour/collision/boxIntersect.h"
#include "binaryMesher.h"
#include "voxelHelpers.h"
#include "frustum.h"
// #include "labour/util/cgltfTest.h"

//0 for multithreading
#define CHUNK_SINGLE_THREADING 0
#define ENABLE_VOXEL_WORK 1
#define ENABLE_MESH_WORK 1
#define TERRAIN_FEATURES 1
#define DISABLE_MESHING 0
const char* GetChunkStage(ChunkStage stage){
    switch(stage){

        case chunkStage_null:                   {return "null";}break;
        case chunkStage_unloaded:               {return "unloaded";}break;
        case chunkStage_touched:                {return "touched";}break;//some mechanism has initiated the creation/voxelization/meshing process
        case chunkStage_firstPass:              {return "firstPass";}break;//multithreaded initial voxel gen
        case chunkStage_firstPassQueued:        {return "firstPassQueued";}break;
        case chunkStage_firstPassWorking:       {return "firstPassWorking";}break;
        case chunkStage_firstPassComplete:      {return "firstPassComplete";}break;
        case chunkStage_secondPass:             {return "secondPass";}break;//single threaded terrain features
        case chunkStage_secondPassQueued:       {return "secondPassQueued";}break;
        case chunkStage_secondPassWorking:      {return "secondPassWorking";}break;
        case chunkStage_secondPassComplete:     {return "secondPassComplete";}break;
        case chunkStage_meshing:                {return "meshing";}break;//multithreaded meshing
        case chunkStage_meshed:                 {return "meshed";}break;//multithreaded meshing
        case chunkStage_upload:                 {return "upload";}break;//upload the mesh to the GPU
        case chunkStage_uploaded:               {return "uploaded";}break;//upload the mesh to the GPU
        case chunkStage_flaggedForDestruction:  {return "flaggedForDestruction";}break;
        default:                                {return "NONE??";}break;
    }
}



inline vec3 vec3_step(const vec3& edge, const vec3& x) {
    return {
        x.x < edge.x ? 0.0f : 1.0f,
        x.y < edge.y ? 0.0f : 1.0f,
        x.z < edge.z ? 0.0f : 1.0f
    };
}

inline vec4 vec4_abs(vec4& v){
    return vec4_create(fabs(v.x), fabs(v.y),fabs(v.z),fabs(v.w));
}

inline vec4 vec4_step(const vec4& edge, const vec4& x) {
    return {
        x.x < edge.x ? 0.0f : 1.0f,
        x.y < edge.y ? 0.0f : 1.0f,
        x.z < edge.z ? 0.0f : 1.0f,
        x.w < edge.w ? 0.0f : 1.0f
    };
}

vec3 vec3_floor(vec3& v){
    return {floorf(v.x), floorf(v.y), floorf(v.z)};
}
ivec3 ivec3_floor(vec3& v){
    return {(int32_t)floorf(v.x), (int32_t)floorf(v.y), (int32_t)floorf(v.z)};
}

float fract(float f){
    return f - floorf(f);
}

vec3 vec3_fract(vec3 v){
    return (v - vec3_floor(v));
}


vec4 vec4_floor(vec4& v){
    return {floorf(v.x), floorf(v.y), floorf(v.z), floorf(v.w)};
}

vec3 mod289(vec3 x){
    vec3 floored = {floorf(x.x * (1.0 / 289.0)) * 289, floorf(x.y * (1.0 / 289.0)) * 289, floorf(x.z * (1.0 / 289.0)) * 289};
    return x - floored;
}
vec4 mod289(vec4 x){
    vec4 floored = {floorf(x.x * (1.0 / 289.0)) * 289, floorf(x.y * (1.0 / 289.0)) * 289, floorf(x.z * (1.0 / 289.0)) * 289, floorf(x.w * (1.0 / 289.0)) * 289};
    return x - floored;
}
vec4 permute(vec4 x){
    return mod289(((x*34.0) + 1.0) * x);
}
vec4 taylorInvSqrt(vec4 r){
    return 1.79284291400159 - 0.85373472095314 * r; 
}
//BEGIN NOISE GENERATION
//3d simplex noise
float snoise(vec3 v){
    //1/6, 1/3
    vec2 C = {0.16666666666, 0.33333333333};
    vec4 D = {0.0, 0.5, 1.0, 2.0};
    vec3 Cxxx = {C.x, C.x, C.x};
    vec3 Cyyy = {C.y, C.y, C.y};
    vec3 Dyyy = {D.y, D.y, D.y};

    //first corner
    vec3 i = vec3_floor(v + vec3_dot(v, Cyyy));
    vec3 x0 = v - i + vec3_dot(i, Cxxx);

    //other corners
    vec3 g = vec3_step({x0.y, x0.z, x0.x}, {x0.x, x0.y, x0.z});
    vec3 l = 1.0 - g;
    vec3 i1 = vec3_min({g.x, g.y, g.z}, {l.z, l.x, l.y});
    vec3 i2 = vec3_max({g.x, g.y, g.z}, {l.z, l.x, l.y});

    vec3 x1 = x0 - i1 + Cxxx;
    vec3 x2 = x0 - i2 + Cyyy;
    vec3 x3 = x0 - Dyyy;

    //permutations
    i = mod289(i);
    vec4 p = permute(permute(permute(
        i.z + vec4_create(0.0, i1.z, i2.z, 1.0) + i.y + vec4_create(0.0, i1.y, i2.y, 1.0) + i.x + vec4_create(0.0, i1.x, i2.x, 1.0)))
    );

    //grasdients: 7x7 points over a square, mapped onto an octahedron
     float n_ = 0.142857142857; // 1.0/7.0
    vec3 ns = n_ * vec3_create(D.w, D.y, D.z) - vec3_create(D.x, D.z, D.x);
    
    vec4 j = p - 49.0 * vec4_floor(p * ns.z * ns.z);
    
    vec4 x_ = vec4_floor(j * ns.z);
    vec4 y_ = vec4_floor(j - 7.0 * x_);
    
    vec4 x = x_ *ns.x + vec4_create(ns.y);
    vec4 y = y_ *ns.x + vec4_create(ns.y);
    vec4 h = 1.0 - vec4_abs(x) - vec4_abs(y);
    
    vec4 b0 = vec4_create(x.x, x.y, y.x, y.y);
    vec4 b1 = vec4_create(x.z, x.w, y.z, y.w);
    
    vec4 s0 = vec4_floor(b0)*2.0 + 1.0;
    vec4 s1 = vec4_floor(b1)*2.0 + 1.0;
    vec4 sh = -vec4_step(h, vec4_create(0.0));
    
    vec4 a0 = vec4_create(b0.x, b0.z, b0.y, b0.w) + vec4_create(s0.x, b0.z, b0.y, b0.w) * vec4_create(sh.x, b0.x, b0.y, b0.y);
    vec4 a1 = vec4_create(b1.x, b1.z, b1.y, b1.w) + vec4_create(s1.x, b1.z, b1.y, b1.w) * vec4_create(sh.z, b1.z, b1.w, b1.w);
    
    vec3 p0 = vec3_create(a0.x, a0.y, h.x);
    vec3 p1 = vec3_create(a0.z, a0.w, h.y);
    vec3 p2 = vec3_create(a1.x, a1.y, h.z);
    vec3 p3 = vec3_create(a1.z, a1.w, h.w);
    
    // Normalise gradients
    vec4 norm = taylorInvSqrt(vec4_create(vec3_dot(p0,p0), vec3_dot(p1,p1), vec3_dot(p2,p2), vec3_dot(p3,p3)));
    p0 *= norm.x;
    p1 *= norm.y;
    p2 *= norm.z;
    p3 *= norm.w;
    
    // Mix final noise value
    vec4 m = vec4_max(0.6 - vec4_create(vec3_dot(x0,x0), vec3_dot(x1,x1), vec3_dot(x2,x2), vec3_dot(x3,x3)), vec4_create(0.0));
    m = m * m;
    return 42.0 * vec4_dot(m*m, vec4_create(vec3_dot(p0,x0), vec3_dot(p1,x1), vec3_dot(p2,x2), vec3_dot(p3,x3)));
}


inline float gradXOR(u32 hash, float x, float y, float z){
    int h = hash & 15;
    int uSel = h < 8;
    int vSel = h < 4;
    int xSel = (h == 12 | h == 14);

    float u = uSel ? x : y;
    float xz = xSel ? x : z;
    float v = vSel ? y : xz;

    // bool uFlip = (h&1) == 1;
    // bool vFlip = (h&2) == 2;

    // float r0 = uFlip ? u*-1 : u;
    // float r1 = vFlip ? v*-1 : v;
    // float result = r0 + r1;
    u32 h1 = hash << 31;
    u32 h2 = (hash & 2) << 30;
    u32 ubits = *(u32*)&u;
    u32 vbits = *(u32*)&v;
    u32 resultUbits = ubits ^ h1;
    u32 resultVbits = vbits ^ h2;
    float resultU = *(float*)&resultUbits;
    float resultV = *(float*)&resultVbits;

    

    float result= resultU + resultV;
    return result;
}

inline __m128 gradLookup4x(chunk_data* chunkData, __m128i hash, __m128 x, __m128 y, __m128 z)
{
    u64 start = __rdtsc();

    // Pick gradient index from hash (0..11)
    __m128i idx = _mm_and_si128(hash, _mm_set1_epi32(11)); // 12 directions

    // Gather X,Y,Z from table

    __m128 gx = _mm_i32gather_ps(chunkData->gradientX, idx, 4);
    __m128 gy = _mm_i32gather_ps(chunkData->gradientY, idx, 4);
    __m128 gz = _mm_i32gather_ps(chunkData->gradientZ, idx, 4);

    // Dot with delta vector
    __m128 dot = _mm_add_ps(_mm_add_ps(_mm_mul_ps(gx, x), _mm_mul_ps(gy, y)),_mm_mul_ps(gz, z));

    chunkData->grad4xHits++;
    chunkData->grad4xTotal += (__rdtsc() - start);
    return dot;
}

inline float gradLookup(chunk_data* chunkData, u32 hash, float x, float y, float z)
{
    // Pick gradient index from hash (0..11)
    // __m128i idx = _mm_and_si128(hash, _mm_set1_epi32(11)); // 12 directions
    u32 idx = hash & 11;
    // Gather X,Y,Z from table

    // __m128 gx = _mm_i32gather_ps(chunkData->gradientX, idx, 4);
    // __m128 gy = _mm_i32gather_ps(chunkData->gradientY, idx, 4);
    // __m128 gz = _mm_i32gather_ps(chunkData->gradientZ, idx, 4);
    float gx = chunkData->gradientX[idx];
    float gy = chunkData->gradientY[idx];
    float gz = chunkData->gradientZ[idx];

    // Dot with delta vector
    // __m128 dot = _mm_add_ps(_mm_add_ps(_mm_mul_ps(gx, x), _mm_mul_ps(gy, y)),_mm_mul_ps(gz, z));
    float dot = gx*x + gy*y + gz*z;
    return dot;
}


inline __m128 actualGrad4x(chunk_data* chunkData, __m128i hash, __m128 x, __m128 y, __m128 z){
    u64 start = __rdtsc();
    __m128i _15  = _mm_set1_epi32(15);
    __m128i _14  = _mm_set1_epi32(14);
    __m128i _12  = _mm_set1_epi32(12);
    __m128i _8   = _mm_set1_epi32(8);
    __m128i _4   = _mm_set1_epi32(4);
    __m128i _2   = _mm_set1_epi32(2);
    __m128i _1   = _mm_set1_epi32(1);
    __m128  _n1  = _mm_set1_ps(-1);


    __m128i h = _mm_and_si128(hash,_15);                                                //hash & _15
    __m128i uSel = _mm_cmplt_epi32(h,_8);                                               //h < _8
    __m128i vSel = _mm_cmplt_epi32(h,_4);                                               //h < _8
    __m128i xSel = _mm_or_si128(_mm_cmpeq_epi32(h,_12), _mm_cmpeq_epi32(h, _14));       // h == _12 | h == _8



    __m128 u  = _mm_blendv_ps(y,x,_mm_castsi128_ps(uSel));
    __m128 xz = _mm_blendv_ps(z,x,_mm_castsi128_ps(xSel));
    __m128 v  = _mm_blendv_ps(xz,y,_mm_castsi128_ps(vSel));


    //flip masks
    #if 0
    __m128i uFlip = _mm_cmpeq_epi32(_mm_and_si128(h, _1), _1);
    __m128i vFlip = _mm_cmpeq_epi32(_mm_and_si128(h, _2), _2);

    __m128 r0 = _mm_or_ps(_mm_and_ps(_mm_castsi128_ps(uFlip), _mm_mul_ps(u, _n1)),
                          _mm_andnot_ps(_mm_castsi128_ps(uFlip), u));

    __m128 r1 = _mm_or_ps(_mm_and_ps(_mm_castsi128_ps(vFlip), _mm_mul_ps(v, _n1)),
                          _mm_andnot_ps(_mm_castsi128_ps(vFlip), v));
    __m128 result = _mm_add_ps(r0, r1);

    #else
        __m128i h1 = _mm_slli_epi32(hash, 31);
        __m128i h2 = _mm_slli_epi32(_mm_and_si128(hash,_2) , 30);

        __m128 result = _mm_add_ps(_mm_xor_ps(u, _mm_castsi128_ps(h1)),_mm_xor_ps(v, _mm_castsi128_ps(h2)));
    #endif
    
    chunkData->grad4xHits++;
    chunkData->grad4xTotal += (__rdtsc() - start);
    
    return result;
}

inline float grad(u32 hash, float x, float y, float z){
    int h = hash & 15;                                        //take the hashed value and take the first 4 bits of it (15 === 0b1111)
    #if 1            
    float u = h < 8 /*0b1000*/ ? x : y;                //if the most significant bit (MSB) of the hash is 0 then set u = x, otherwise y

    float v;

    if(h < 4 /*0b0100*/){                               //if the first and second significant bits are 0 set v = y
        v = y;
    }else if(h == 12 /*0b1100*/ || h == 14 /*0b1110*/){ //if the first and second significant bits are 1, set v = x
        v = x;
    }else{
        v = z;                                          //if the first and second significant bits are not equal (0/1, 1/0) set v = z
    }

    return ((h&1) == 0 ? u : -u)+((h&2) == 0 ? v : -v); //use the last 2 bits to decide if u and v are positive or negative
    #else
    switch(h)
    {
        case 0x0: return  x + y;
        case 0x1: return -x + y;
        case 0x2: return  x - y;
        case 0x3: return -x - y;
        case 0x4: return  x + z;
        case 0x5: return -x + z;
        case 0x6: return  x - z;
        case 0x7: return -x - z;
        case 0x8: return  y + z;
        case 0x9: return -y + z;
        case 0xA: return  y - z;
        case 0xB: return -y - z;
        case 0xC: return  y + x;
        case 0xD: return -y + z;
        case 0xE: return  y - x;
        case 0xF: return -y - z;
        default: return 0; // never happens
    }
    #endif
}

inline float noiseLerp(float x, float a, float b){
    return a + x * (b - a);
}

//fade function by ken perlin, eases coordinate values so that they will 'ease' towards integral values. this ends up smoothing the final output
inline float noiseFade(float t){
    return t * t * t * (t * (t * 6 - 15) + 10); //6t^5 - 15t^4 + 10t^3;
}

//simple unoptimized perlin noise
inline float perlinNoise(chunk_data* chunkData, vec3 vec){
    u64 start = __rdtsc();
    float fx = floorf(vec.x);
    float fy = floorf(vec.y);
    float fz = floorf(vec.z);
    
    u32 xi = (int)fx & 255;                  //calculate the 'unit cube' that the point asked will be located in                    
    u32 yi = (int)fy & 255;                  //the left bound is ( |_x_| , |_y_| , |_z_| ) and the right bound is that plus 1
    u32 zi = (int)fz & 255;                  //next we calculate the location (from 0.0 to 1.0) in that cube 
    
    f32 xf = vec.x - fx;               //we also fade the location to smooth the result
    f32 yf = vec.y - fy;               
    f32 zf = vec.z - fz;

    f32 u = noiseFade(xf);
    f32 v = noiseFade(yf);
    f32 w = noiseFade(zf);

    u32 a  = (u32)chunkData->perlinPermTable[xi  ] + yi; 
    u32 aa = (u32)chunkData->perlinPermTable[a   ] + zi; 
    u32 ab = (u32)chunkData->perlinPermTable[a+1 ] + zi; 
    u32 b  = (u32)chunkData->perlinPermTable[xi+1] + yi; 
    u32 ba = (u32)chunkData->perlinPermTable[b   ] + zi; 
    u32 bb = (u32)chunkData->perlinPermTable[b+1 ] + zi; 

    //add blended results from 8 corners of cube
    #if 1
        #if 1 //for the slightly faster lookup/gather function
            f32 c0 = grad(chunkData->perlinPermTable[aa  ], xf  , yf  , zf  );
            f32 c1 = grad(chunkData->perlinPermTable[ba  ], xf-1, yf  , zf  );
            f32 c2 = grad(chunkData->perlinPermTable[ab  ], xf  , yf-1, zf  );
            f32 c3 = grad(chunkData->perlinPermTable[bb  ], xf-1, yf-1, zf  );
            f32 c4 = grad(chunkData->perlinPermTable[aa+1], xf  , yf  , zf-1);
            f32 c5 = grad(chunkData->perlinPermTable[ba+1], xf-1, yf  , zf-1);
            f32 c6 = grad(chunkData->perlinPermTable[ab+1], xf  , yf-1, zf-1);
            f32 c7 = grad(chunkData->perlinPermTable[bb+1], xf-1, yf-1, zf-1);
        #else
            f32 c0 = gradLookup(chunkData, chunkData->perlinPermTable[aa  ], xf  , yf  , zf  );
            f32 c1 = gradLookup(chunkData, chunkData->perlinPermTable[ba  ], xf-1, yf  , zf  );
            f32 c2 = gradLookup(chunkData, chunkData->perlinPermTable[ab  ], xf  , yf-1, zf  );
            f32 c3 = gradLookup(chunkData, chunkData->perlinPermTable[bb  ], xf-1, yf-1, zf  );
            f32 c4 = gradLookup(chunkData, chunkData->perlinPermTable[aa+1], xf  , yf  , zf-1);
            f32 c5 = gradLookup(chunkData, chunkData->perlinPermTable[ba+1], xf-1, yf  , zf-1);
            f32 c6 = gradLookup(chunkData, chunkData->perlinPermTable[ab+1], xf  , yf-1, zf-1);
            f32 c7 = gradLookup(chunkData, chunkData->perlinPermTable[bb+1], xf-1, yf-1, zf-1);
        #endif
    #else //slower by atleast 50 cycles per function
        f32 c0 = gradXOR(chunkData->perlinPermTable[aa  ], xf  , yf  , zf  );
        f32 c1 = gradXOR(chunkData->perlinPermTable[ba  ], xf-1, yf  , zf  );
        f32 c2 = gradXOR(chunkData->perlinPermTable[ab  ], xf  , yf-1, zf  );
        f32 c3 = gradXOR(chunkData->perlinPermTable[bb  ], xf-1, yf-1, zf  );
        f32 c4 = gradXOR(chunkData->perlinPermTable[aa+1], xf  , yf  , zf-1);
        f32 c5 = gradXOR(chunkData->perlinPermTable[ba+1], xf-1, yf  , zf-1);
        f32 c6 = gradXOR(chunkData->perlinPermTable[ab+1], xf  , yf-1, zf-1);
        f32 c7 = gradXOR(chunkData->perlinPermTable[bb+1], xf-1, yf-1, zf-1);
    #endif

    f32 res = noiseLerp(w, noiseLerp(v, noiseLerp(u, c0, c1), noiseLerp(u, c2, c3)), noiseLerp(v, noiseLerp(u, c4, c5), noiseLerp(u, c6, c7)));
    res = (res + 1.0f) * 0.5f;

    
    chunkData->perlinNoise3dCyclesTotal += (__rdtsc() - start);
    chunkData->perlinNoise3dHits++;

    return res;

}



//simple unoptimized perlin noise
inline float perlinNoiseFactored(chunk_data* chunkData, float x, float y, float z){
    u64 start = __rdtsc();
    // if(vec.x < 0) vec.x -= 1.0f;
    // if(vec.y < 0) vec.y -= 1.0f;
    // if(vec.z < 0) vec.z -= 1.0f;
    
    u32 xi = (u32)x & 255;                  //calculate the 'unit cube' that the point asked will be located in                    
    u32 yi = (u32)y & 255;                  //the left bound is ( |_x_| , |_y_| , |_z_| ) and the right bound is that plus 1
    u32 zi = (u32)z & 255;                  //next we calculate the location (from 0.0 to 1.0) in that cube 
    
    f32 xf = x - (int)x;               //we also fade the location to smooth the result
    f32 yf = y - (int)y;               
    f32 zf = z - (int)z;

    f32 u = noiseFade(xf);
    f32 v = noiseFade(yf);
    f32 w = noiseFade(zf);

    u32 a  = (u32)chunkData->perlinPermTable[xi  ] + yi; 
    u32 aa = (u32)chunkData->perlinPermTable[a   ] + zi; 
    u32 ab = (u32)chunkData->perlinPermTable[a+1 ] + zi; 
    u32 b  = (u32)chunkData->perlinPermTable[xi+1] + yi; 
    u32 ba = (u32)chunkData->perlinPermTable[b   ] + zi; 
    u32 bb = (u32)chunkData->perlinPermTable[b+1 ] + zi; 

    u32 h0 = chunkData->perlinPermTable[aa  ];
    u32 h1 = chunkData->perlinPermTable[ba  ];
    u32 h2 = chunkData->perlinPermTable[ab  ];
    u32 h3 = chunkData->perlinPermTable[bb  ];
    u32 h4 = chunkData->perlinPermTable[aa+1];
    u32 h5 = chunkData->perlinPermTable[ba+1];
    u32 h6 = chunkData->perlinPermTable[ab+1];
    u32 h7 = chunkData->perlinPermTable[bb+1];



    
    // //add blended results from 8 corners of cube
    __m128i h00 = _mm_setr_epi32(h0, h2, h4, h6);
    __m128i h01 = _mm_setr_epi32(h1, h3, h5, h7);

    __m128 xf0 = _mm_set1_ps(xf);
    __m128 xf1 = _mm_set1_ps(xf-1);
    __m128 yf0 = _mm_setr_ps(yf, yf-1, yf, yf-1);
    __m128 zf0 = _mm_setr_ps(zf, zf, zf-1, zf-1);

    f32_4x l4x = {actualGrad4x(chunkData, h00, xf0, yf0, zf0)};
    f32_4x r4x = {actualGrad4x(chunkData, h01, xf1, yf0, zf0)};



    f32_4x u4x = {u,u,u,u};
    // l4x = {c0,c1,c2,c3};
    // r4x = {c4,c5,c6,c7};


    f32_4x r = lerp4x(u4x, l4x, r4x);



    f32 l4 = noiseLerp(v, r.e[0], r.e[1]);
    f32 l5 = noiseLerp(v, r.e[2], r.e[3]);


    f32 res = noiseLerp(w, l4, l5);
    
    
    
    res = (res + 1.0f) * 0.5f;

    
    chunkData->perlinNoise3dCyclesTotal += (__rdtsc() - start);
    chunkData->perlinNoise3dHits++;

    return res;

}


static inline uint32_t procHash(int x, int y, int z)
{
    uint32_t h = 2166136261u; // FNV offset basis
    h = (h ^ x) * 16777619u;
    h = (h ^ y) * 16777619u;
    h = (h ^ z) * 16777619u;
    return h;
}

__m128i hashPrimes4x(__m128i seed, __m128i x, __m128i y, __m128i z){
    __m128i hash = seed;
    __m128i hex = _mm_set1_epi32(0x27d4eb2d);
    hash = _mm_xor_si128(hash, x);
    hash = _mm_xor_si128(hash, y);
    hash = _mm_xor_si128(hash, z);
    hash = _mm_mullo_epi32(hash, hex);
    hash = _mm_xor_si128(hash, _mm_srli_epi32(hash, 15));

    return hash;
}

inline void
perlinNoiseFactored4xBetter(chunk_data *chunkData, float* x, float y, float z, float* output)
{
    u64 start = __rdtsc();

    #if 1
    // ---- 1) Adjust negatives to mimic floor for negative input
    // Without this, trunc() goes toward zero instead of down
    if(x[0] < 0) x[0] -= 1.f;  
    if(x[1] < 0) x[1] -= 1.f;  
    if(x[2] < 0) x[2] -= 1.f;  
    if(x[3] < 0) x[3] -= 1.f;  
    if(y    < 0) y    -= 1.f;  
    if(z    < 0) z    -= 1.f;  
    


    // ---- 2) Compute unit cube indices (floor & wrap)
    int xi[4] = {
        ((int)x[0]) & 255,
        ((int)x[1]) & 255,
        ((int)x[2]) & 255,
        ((int)x[3]) & 255
    };
    int yi = ((int)y) & 255;
    int zi = ((int)z) & 255;

    
    // ---- 3) Compute local position in cube [0,1]
    f32 xf[4] = { x[0] - (int)x[0], x[1] - (int)x[1], x[2] - (int)x[2], x[3] - (int)x[3] };
    f32 yf    =      y - (int)   y;
    f32 zf    =      z - (int)   z;
#else
    __m128 px = _mm_loadu_ps(x);
    __m128 py = _mm_set1_ps(y);
    __m128 pz = _mm_set1_ps(z);

    __m128 floorX = _mm_floor_ps(px);  // or trunc and fix
    __m128 floorY = _mm_floor_ps(py);  // these will be same across lanes
    __m128 floorZ = _mm_floor_ps(pz);

    // Subtract floors to get local position in unit cube
    __m128 xf0 = _mm_sub_ps(px, floorX);
    __m128 yf0 = _mm_sub_ps(py, floorY);
    __m128 zf0 = _mm_sub_ps(pz, floorZ);

    // Mask your indices SIMD style
    __m128i xi0 = _mm_and_si128(_mm_cvttps_epi32(floorX), _mm_set1_epi32(255));
    __m128i yi0 = _mm_and_si128(_mm_cvttps_epi32(floorY), _mm_set1_epi32(255));
    __m128i zi0 = _mm_and_si128(_mm_cvttps_epi32(floorZ), _mm_set1_epi32(255));

    __m128i xi1 = _mm_add_epi32(xi0, _mm_set1_epi32(1));
    __m128i yi1 = _mm_add_epi32(yi0, _mm_set1_epi32(1));
    __m128i zi1 = _mm_add_epi32(zi0, _mm_set1_epi32(1));
#endif
    // ---- 4) Compute fade curves (in SIMD later)
    u64 lookupStart = __rdtsc();
    chunkData->perlinNoise3dScalarTotal += (lookupStart - start);
#if 1
    // ---- 5) Do perm table lookup for each corner
    u32 h0[4], h1[4], h2[4], h3[4], h4[4], h5[4], h6[4], h7[4];

    for (int i = 0; i < 4; i++) {
    #if 1
    
        int a  = chunkData->perlinPermTable[ xi[i]   ] + yi;
        int aa = chunkData->perlinPermTable[ a       ] + zi;
        int ab = chunkData->perlinPermTable[ a + 1   ] + zi;
        int b  = chunkData->perlinPermTable[ xi[i]+1 ] + yi;
        int ba = chunkData->perlinPermTable[ b       ] + zi;
        int bb = chunkData->perlinPermTable[ b + 1   ] + zi;

        h0[i] = chunkData->perlinPermTable[ aa     ];
        h1[i] = chunkData->perlinPermTable[ ba     ];
        h2[i] = chunkData->perlinPermTable[ ab     ];
        h3[i] = chunkData->perlinPermTable[ bb     ];
        h4[i] = chunkData->perlinPermTable[ aa + 1 ];
        h5[i] = chunkData->perlinPermTable[ ba + 1 ];
        h6[i] = chunkData->perlinPermTable[ ab + 1 ];
        h7[i] = chunkData->perlinPermTable[ bb + 1 ];
    #else
        int xi0 = xi[i];
        int xi1 = xi[i]+1;

        h0[i] = procHash(xi0, yi, zi);
        h1[i] = procHash(xi1, yi, zi);
        h2[i] = procHash(xi0, yi+1, zi);
        h3[i] = procHash(xi1, yi+1, zi);
        h4[i] = procHash(xi0, yi, zi+1);
        h5[i] = procHash(xi1, yi, zi+1);
        h6[i] = procHash(xi0, yi+1, zi+1);
        h7[i] = procHash(xi1, yi+1, zi+1);
        #endif
    }

#endif
    // __m128i H0 = hashSIMD(xi0, yi0, zi0);
    // __m128i H1 = hashSIMD(xi1, yi0, zi0);
    // __m128i H2 = hashSIMD(xi0, yi1, zi0);
    // __m128i H3 = hashSIMD(xi1, yi1, zi0);

    // __m128i h = _mm_set1_epi32(2166136261u);
    // __m128i hu = _mm_set1_epi32(16777619u);

    // __m128i H0 =   _mm_xor_si128(h, xi0);
    //         H0 = _mm_mullo_epi32(H0, hu);
    //         H0 =   _mm_xor_si128(H0, yi0);
    //         H0 = _mm_mullo_epi32(H0, hu);
    //         H0 =   _mm_xor_si128(H0, zi0);
    //         H0 = _mm_mullo_epi32(H0, hu);

    // __m128i H1 =   _mm_xor_si128(h, xi1);
    //         H1 = _mm_mullo_epi32(H1, hu);
    //         H1 =   _mm_xor_si128(H1, yi0);
    //         H1 = _mm_mullo_epi32(H1, hu);
    //         H1 =   _mm_xor_si128(H1, zi0);
    //         H1 = _mm_mullo_epi32(H1, hu);

    // __m128i H2 =   _mm_xor_si128(h, xi0);
    //         H2 = _mm_mullo_epi32(H2, hu);
    //         H2 =   _mm_xor_si128(H2, yi1);
    //         H2 = _mm_mullo_epi32(H2, hu);
    //         H2 =   _mm_xor_si128(H2, zi0);
    //         H2 = _mm_mullo_epi32(H2, hu);

    // __m128i H3 =   _mm_xor_si128(h, xi1);
    //         H3 = _mm_mullo_epi32(H3, hu);
    //         H3 =   _mm_xor_si128(H3, yi1);
    //         H3 = _mm_mullo_epi32(H3, hu);
    //         H3 =   _mm_xor_si128(H3, zi0);
    //         H3 = _mm_mullo_epi32(H3, hu);



    // __m128i H4 =   _mm_xor_si128(h, xi0);
    //         H4 = _mm_mullo_epi32(H4, hu);
    //         H4 =   _mm_xor_si128(H4, yi0);
    //         H4 = _mm_mullo_epi32(H4, hu);
    //         H4 =   _mm_xor_si128(H4, zi1);
    //         H4 = _mm_mullo_epi32(H4, hu);

    // __m128i H5 =   _mm_xor_si128(h, xi1);
    //         H5 = _mm_mullo_epi32(H5, hu);
    //         H5 =   _mm_xor_si128(H5, yi0);
    //         H5 = _mm_mullo_epi32(H5, hu);
    //         H5 =   _mm_xor_si128(H5, zi1);
    //         H5 = _mm_mullo_epi32(H5, hu);

    // __m128i H6 =   _mm_xor_si128(h, xi0);
    //         H6 = _mm_mullo_epi32(H6, hu);
    //         H6 =   _mm_xor_si128(H6, yi1);
    //         H6 = _mm_mullo_epi32(H6, hu);
    //         H6 =   _mm_xor_si128(H6, zi1);
    //         H6 = _mm_mullo_epi32(H6, hu);

    // __m128i H7 =   _mm_xor_si128(h, xi1);
    //         H7 = _mm_mullo_epi32(H7, hu);
    //         H7 =   _mm_xor_si128(H7, yi1);
    //         H7 = _mm_mullo_epi32(H7, hu);
    //         H7 =   _mm_xor_si128(H7, zi1);
    //         H7 = _mm_mullo_epi32(H7, hu);

    // __m128i H4 = hashSIMD(xi0, yi0, zi1);
    // __m128i H5 = hashSIMD(xi1, yi0, zi1);
    // __m128i H6 = hashSIMD(xi0, yi1, zi1);
    // __m128i H7 = hashSIMD(xi1, yi1, zi1);



    u64 SIMDStart = __rdtsc();
    chunkData->perlinNoise3dScalarLookupTotal += (SIMDStart - lookupStart);

    // ---- 6) Pack to SIMD for gradient dot
    __m128 xf0 = _mm_setr_ps(xf[0], xf[1], xf[2], xf[3]);
    // __m128 xf0 = _mm_loadu_ps(xf); //no performance difference
    __m128 yf0 = _mm_set1_ps(yf);
    __m128 zf0 = _mm_set1_ps(zf);

    __m128 xf1 = _mm_sub_ps(xf0, _mm_set1_ps(1.0f));
    __m128 yf1 = _mm_sub_ps(yf0, _mm_set1_ps(1.0f));
    __m128 zf1 = _mm_sub_ps(zf0, _mm_set1_ps(1.0f));

    __m128 _15 = _mm_set1_ps(15.0f);
    __m128 _10 = _mm_set1_ps(10.0f);
    __m128 _6  = _mm_set1_ps(6.0f);

    // ---- 7) Compute fade curves packed
    // __m128 u = noiseFade4x(xf0);
    __m128 xt2    = _mm_mul_ps(xf0,xf0);
    __m128 xt3    = _mm_mul_ps(xt2,xf0);
    __m128 xinner = _mm_add_ps(_mm_mul_ps(xf0, _mm_sub_ps(_mm_mul_ps(xf0, _6), _15)), _10);
    __m128 u      = _mm_mul_ps(xt3, xinner);

    // __m128 v = noiseFade4x(yf0);
    __m128 yt2    = _mm_mul_ps(yf0,yf0);
    __m128 yt3    = _mm_mul_ps(yt2,yf0);
    __m128 yinner = _mm_add_ps(_mm_mul_ps(yf0, _mm_sub_ps(_mm_mul_ps(yf0, _6), _15)), _10);
    __m128 v      = _mm_mul_ps(yt3, yinner);
    
    // __m128 w = noiseFade4x(zf0);
    __m128 wt2    = _mm_mul_ps(zf0,zf0);
    __m128 wt3    = _mm_mul_ps(wt2,zf0);
    __m128 winner = _mm_add_ps(_mm_mul_ps(zf0, _mm_sub_ps(_mm_mul_ps(zf0, _6), _15)), _10);
    __m128 w      = _mm_mul_ps(wt3, winner);

    // ---- 8) Evaluate gradients packed: 8 corners
    #if 1
    __m128i H0 = _mm_setr_epi32(h0[0], h0[1], h0[2], h0[3]);
    __m128i H1 = _mm_setr_epi32(h1[0], h1[1], h1[2], h1[3]);
    __m128i H2 = _mm_setr_epi32(h2[0], h2[1], h2[2], h2[3]);
    __m128i H3 = _mm_setr_epi32(h3[0], h3[1], h3[2], h3[3]);
    __m128i H4 = _mm_setr_epi32(h4[0], h4[1], h4[2], h4[3]);
    __m128i H5 = _mm_setr_epi32(h5[0], h5[1], h5[2], h5[3]);
    __m128i H6 = _mm_setr_epi32(h6[0], h6[1], h6[2], h6[3]);
    __m128i H7 = _mm_setr_epi32(h7[0], h7[1], h7[2], h7[3]);
    #else
    __m128i H0 = hashPrimes4x(__m128i seed, __m128i x, __m128i y, __m128i z);
    #endif

    u64 SIMDLookupStart = __rdtsc();

    #if 0
        //25 cycles per function + fuction call overhead
        __m128 c0 = actualGrad4x(chunkData, H0, xf0, yf0, zf0);
        __m128 c1 = actualGrad4x(chunkData, H1, xf1, yf0, zf0);
        __m128 c2 = actualGrad4x(chunkData, H2, xf0, yf1, zf0);
        __m128 c3 = actualGrad4x(chunkData, H3, xf1, yf1, zf0);
        __m128 c4 = actualGrad4x(chunkData, H4, xf0, yf0, zf1);
        __m128 c5 = actualGrad4x(chunkData, H5, xf1, yf0, zf1);
        __m128 c6 = actualGrad4x(chunkData, H6, xf0, yf1, zf1);
        __m128 c7 = actualGrad4x(chunkData, H7, xf1, yf1, zf1);

    #else
        //20 cycles per function + function call overhead
        __m128 c0 = gradLookup4x(chunkData, H0, xf0, yf0, zf0);
        __m128 c1 = gradLookup4x(chunkData, H1, xf1, yf0, zf0);
        __m128 c2 = gradLookup4x(chunkData, H2, xf0, yf1, zf0);
        __m128 c3 = gradLookup4x(chunkData, H3, xf1, yf1, zf0);
        __m128 c4 = gradLookup4x(chunkData, H4, xf0, yf0, zf1);
        __m128 c5 = gradLookup4x(chunkData, H5, xf1, yf0, zf1);
        __m128 c6 = gradLookup4x(chunkData, H6, xf0, yf1, zf1);
        __m128 c7 = gradLookup4x(chunkData, H7, xf1, yf1, zf1);

    #endif
    //inlining the grad function calls are much faster
    // __m128i idx0 = _mm_and_si128(H0, _mm_set1_epi32(11)); // 12 directions
    // __m128   gx0 = _mm_i32gather_ps(chunkData->gradientX, idx0, 4);
    // __m128   gy0 = _mm_i32gather_ps(chunkData->gradientY, idx0, 4);
    // __m128   gz0 = _mm_i32gather_ps(chunkData->gradientZ, idx0, 4);

    // __m128i idx1 = _mm_and_si128(H1, _mm_set1_epi32(11));
    // __m128   gx1 = _mm_i32gather_ps(chunkData->gradientX, idx1, 4);
    // __m128   gy1 = _mm_i32gather_ps(chunkData->gradientY, idx1, 4);
    // __m128   gz1 = _mm_i32gather_ps(chunkData->gradientZ, idx1, 4);

    // __m128i idx2 = _mm_and_si128(H2, _mm_set1_epi32(11));
    // __m128   gx2 = _mm_i32gather_ps(chunkData->gradientX, idx2, 4);
    // __m128   gy2 = _mm_i32gather_ps(chunkData->gradientY, idx2, 4);
    // __m128   gz2 = _mm_i32gather_ps(chunkData->gradientZ, idx2, 4);

    // __m128i idx3 = _mm_and_si128(H3, _mm_set1_epi32(11));
    // __m128   gx3 = _mm_i32gather_ps(chunkData->gradientX, idx3, 4);
    // __m128   gy3 = _mm_i32gather_ps(chunkData->gradientY, idx3, 4);
    // __m128   gz3 = _mm_i32gather_ps(chunkData->gradientZ, idx3, 4);

    // __m128i idx4 = _mm_and_si128(H4, _mm_set1_epi32(11));
    // __m128   gx4 = _mm_i32gather_ps(chunkData->gradientX, idx4, 4);
    // __m128   gy4 = _mm_i32gather_ps(chunkData->gradientY, idx4, 4);
    // __m128   gz4 = _mm_i32gather_ps(chunkData->gradientZ, idx4, 4);

    // __m128i idx5 = _mm_and_si128(H5, _mm_set1_epi32(11));
    // __m128   gx5 = _mm_i32gather_ps(chunkData->gradientX, idx5, 4);
    // __m128   gy5 = _mm_i32gather_ps(chunkData->gradientY, idx5, 4);
    // __m128   gz5 = _mm_i32gather_ps(chunkData->gradientZ, idx5, 4);

    // __m128i idx6 = _mm_and_si128(H6, _mm_set1_epi32(11));
    // __m128   gx6 = _mm_i32gather_ps(chunkData->gradientX, idx6, 4);
    // __m128   gy6 = _mm_i32gather_ps(chunkData->gradientY, idx6, 4);
    // __m128   gz6 = _mm_i32gather_ps(chunkData->gradientZ, idx6, 4);

    // __m128i idx7 = _mm_and_si128(H7, _mm_set1_epi32(11));
    // __m128   gx7 = _mm_i32gather_ps(chunkData->gradientX, idx7, 4);
    // __m128   gy7 = _mm_i32gather_ps(chunkData->gradientY, idx7, 4);
    // __m128   gz7 = _mm_i32gather_ps(chunkData->gradientZ, idx7, 4);

    // __m128    c0 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(gx0, xf0), _mm_mul_ps(gy0, yf0)),_mm_mul_ps(gz0, zf0));
    // __m128    c1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(gx1, xf1), _mm_mul_ps(gy1, yf0)),_mm_mul_ps(gz1, zf0));
    // __m128    c2 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(gx2, xf0), _mm_mul_ps(gy2, yf1)),_mm_mul_ps(gz2, zf0));
    // __m128    c3 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(gx3, xf1), _mm_mul_ps(gy3, yf1)),_mm_mul_ps(gz3, zf0));
    // __m128    c4 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(gx4, xf0), _mm_mul_ps(gy4, yf0)),_mm_mul_ps(gz4, zf1));
    // __m128    c5 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(gx5, xf1), _mm_mul_ps(gy5, yf0)),_mm_mul_ps(gz5, zf1));
    // __m128    c6 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(gx6, xf0), _mm_mul_ps(gy6, yf1)),_mm_mul_ps(gz6, zf1));
    // __m128    c7 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(gx7, xf1), _mm_mul_ps(gy7, yf1)),_mm_mul_ps(gz7, zf1));
    
    chunkData->perlinNoise3dSIMDLookupTotal += (__rdtsc() - SIMDStart);


    // ---- 9) Interpolate: x
    __m128 x1 = _mm_add_ps(c0 , _mm_mul_ps(u , _mm_sub_ps(c1 , c0)));
    __m128 x2 = _mm_add_ps(c2 , _mm_mul_ps(u , _mm_sub_ps(c3 , c2)));
    __m128 x3 = _mm_add_ps(c4 , _mm_mul_ps(u , _mm_sub_ps(c5 , c4)));
    __m128 x4 = _mm_add_ps(c6 , _mm_mul_ps(u , _mm_sub_ps(c7 , c6)));

    // __m128 x1 = lerp4x(u, c0, c1);
    // __m128 x2 = lerp4x(u, c2, c3);
    // __m128 x3 = lerp4x(u, c4, c5);
    // __m128 x4 = lerp4x(u, c6, c7);

    // ---- 10) Interpolate: y
    // __m128 y1 = lerp4x(v, x1, x2);
    // __m128 y2 = lerp4x(v, x3, x4);
    __m128 y1 = _mm_add_ps(x1 , _mm_mul_ps(v , _mm_sub_ps(x2 , x1)));
    __m128 y2 = _mm_add_ps(x3 , _mm_mul_ps(v , _mm_sub_ps(x4 , x3)));

    // ---- 11) Interpolate: z
    // __m128 result = lerp4x(w, y1, y2);
    __m128 result = _mm_add_ps(y1 , _mm_mul_ps(w , _mm_sub_ps(y2 , y1)));

    // ---- 12) Normalize output to [0,1]
    result = _mm_mul_ps(_mm_add_ps(result, _mm_set1_ps(1.0f)), _mm_set1_ps(0.5f));


    // ---- 13) Return final packed vec4
    _mm_storeu_ps(output, result);

    chunkData->perlinNoise3dSIMDTotal += (__rdtsc() - SIMDStart);

    chunkData->perlinNoise3dCyclesTotal += (__rdtsc() - start);
    chunkData->perlinNoise3dHits++;

}


inline void
perlinNoise8x(chunk_data *chunkData, __m256 px, __m256 py, __m256 pz, float* output)
{
   
            u64 start = __rdtsc();


            __m256i primeX = _mm256_set1_epi32(501125321);
            __m256i primeY = _mm256_set1_epi32(1136930381u);
            __m256i primeZ = _mm256_set1_epi32(1720413743u);

            __m256  _15  = _mm256_set1_ps(15.0f);
            __m256i _11  = _mm256_set1_epi32(11);
            __m256  _10  = _mm256_set1_ps(10.0f);
            __m256  _6   = _mm256_set1_ps(6.0f);

            // __m256 py = _mm256_set1_ps(y);
            // __m256 pz = _mm256_set1_ps(z);

            __m256 floory = _mm256_floor_ps(py);    
            __m256 floorz = _mm256_floor_ps(pz);    

            __m256i yi = _mm256_cvttps_epi32(floory);
            __m256i zi = _mm256_cvttps_epi32(floorz);

            __m256 yf0 = _mm256_sub_ps(py, floory);
            __m256 zf0 = _mm256_sub_ps(pz, floorz);

            __m256 yf1 = _mm256_sub_ps(yf0, _mm256_set1_ps(1.0f));
            __m256 zf1 = _mm256_sub_ps(zf0, _mm256_set1_ps(1.0f));

            __m256i yp0 = _mm256_mullo_epi32(yi, primeY);
            __m256i zp0 = _mm256_mullo_epi32(zi, primeZ);

            __m256i yp1 = _mm256_add_epi32(yp0, primeY);
            __m256i zp1 = _mm256_add_epi32(zp0, primeZ);

            // __m128 v = noiseFade4x(yf0);
            __m256 yt2    = _mm256_mul_ps(yf0,yf0);
            __m256 yt3    = _mm256_mul_ps(yt2,yf0);
            __m256 yinner = _mm256_add_ps(_mm256_mul_ps(yf0, _mm256_sub_ps(_mm256_mul_ps(yf0, _6), _15)), _10);
            __m256 v      = _mm256_mul_ps(yt3, yinner);
            
            // __m128 w = noiseFade4x(zf0);
            __m256 wt2    = _mm256_mul_ps(zf0,zf0);
            __m256 wt3    = _mm256_mul_ps(wt2,zf0);
            __m256 winner = _mm256_add_ps(_mm256_mul_ps(zf0, _mm256_sub_ps(_mm256_mul_ps(zf0, _6), _15)), _10);
            __m256 w      = _mm256_mul_ps(wt3, winner);

            

                // __m256 px = _mm256_loadu_ps(x);
                __m256 floorx = _mm256_floor_ps(px);    
                __m256i xi = _mm256_cvttps_epi32(floorx);
                __m256 xf0 = _mm256_sub_ps(px, floorx);
                __m256 xf1 = _mm256_sub_ps(xf0, _mm256_set1_ps(1.0f));
                __m256i xp0 = _mm256_mullo_epi32(xi, primeX);
                __m256i xp1 = _mm256_add_epi32(xp0, primeX);

                // ---- 7) Compute fade curves packed
                // __m128 u = noiseFade4x(xf0);
                __m256 xt2    = _mm256_mul_ps(xf0,xf0);
                __m256 xt3    = _mm256_mul_ps(xt2,xf0);
                __m256 xinner = _mm256_add_ps(_mm256_mul_ps(xf0, _mm256_sub_ps(_mm256_mul_ps(xf0, _6), _15)), _10);
                __m256 u      = _mm256_mul_ps(xt3, xinner);



                // ---- 8) Evaluate gradients packed: 8 corners
                __m256i seed = _mm256_set1_epi32(1066037191);
                __m256i hex = _mm256_set1_epi32(0x27d4eb2d);
                __m256i hashx0   = _mm256_xor_si256(seed, xp0);
                __m256i hashx1   = _mm256_xor_si256(seed, xp1);
                __m256i hashx0y0 = _mm256_xor_si256(hashx0, yp0);
                __m256i hashx1y0 = _mm256_xor_si256(hashx1, yp0);
                __m256i hashx0y1 = _mm256_xor_si256(hashx0, yp1);
                __m256i hashx1y1 = _mm256_xor_si256(hashx1, yp1);

                __m256i hashx0y0z0 = _mm256_xor_si256(hashx0y0, zp0);
                __m256i hashx1y0z0 = _mm256_xor_si256(hashx1y0, zp0);
                __m256i hashx0y1z0 = _mm256_xor_si256(hashx0y1, zp0);
                __m256i hashx1y1z0 = _mm256_xor_si256(hashx1y1, zp0);
                __m256i hashx0y0z1 = _mm256_xor_si256(hashx0y0, zp1);
                __m256i hashx1y0z1 = _mm256_xor_si256(hashx1y0, zp1);
                __m256i hashx0y1z1 = _mm256_xor_si256(hashx0y1, zp1);
                __m256i hashx1y1z1 = _mm256_xor_si256(hashx1y1, zp1);

                __m256i hashx0y0z0hex = _mm256_mullo_epi32(hashx0y0z0, hex);
                __m256i hashx1y0z0hex = _mm256_mullo_epi32(hashx1y0z0, hex);
                __m256i hashx0y1z0hex = _mm256_mullo_epi32(hashx0y1z0, hex);
                __m256i hashx1y1z0hex = _mm256_mullo_epi32(hashx1y1z0, hex);
                __m256i hashx0y0z1hex = _mm256_mullo_epi32(hashx0y0z1, hex);
                __m256i hashx1y0z1hex = _mm256_mullo_epi32(hashx1y0z1, hex);
                __m256i hashx0y1z1hex = _mm256_mullo_epi32(hashx0y1z1, hex);
                __m256i hashx1y1z1hex = _mm256_mullo_epi32(hashx1y1z1, hex);

                __m256i H0 = _mm256_xor_si256(hashx0y0z0hex, _mm256_srli_epi32(hashx0y0z0hex, 15));
                __m256i H1 = _mm256_xor_si256(hashx1y0z0hex, _mm256_srli_epi32(hashx1y0z0hex, 15));
                __m256i H2 = _mm256_xor_si256(hashx0y1z0hex, _mm256_srli_epi32(hashx0y1z0hex, 15));
                __m256i H3 = _mm256_xor_si256(hashx1y1z0hex, _mm256_srli_epi32(hashx1y1z0hex, 15));
                __m256i H4 = _mm256_xor_si256(hashx0y0z1hex, _mm256_srli_epi32(hashx0y0z1hex, 15));
                __m256i H5 = _mm256_xor_si256(hashx1y0z1hex, _mm256_srli_epi32(hashx1y0z1hex, 15));
                __m256i H6 = _mm256_xor_si256(hashx0y1z1hex, _mm256_srli_epi32(hashx0y1z1hex, 15));
                __m256i H7 = _mm256_xor_si256(hashx1y1z1hex, _mm256_srli_epi32(hashx1y1z1hex, 15));

                //directly inlining saves us ~100 cycles total
                // __m128i H0 = hashPrimes4x(seed, xp0, yp0, zp0);
                // __m128i H1 = hashPrimes4x(seed, xp1, yp0, zp0);
                // __m128i H2 = hashPrimes4x(seed, xp0, yp1, zp0);
                // __m128i H3 = hashPrimes4x(seed, xp1, yp1, zp0);
                // __m128i H4 = hashPrimes4x(seed, xp0, yp0, zp1);
                // __m128i H5 = hashPrimes4x(seed, xp1, yp0, zp1);
                // __m128i H6 = hashPrimes4x(seed, xp0, yp1, zp1);
                // __m128i H7 = hashPrimes4x(seed, xp1, yp1, zp1);

                u64 SIMDLookupStart = __rdtsc();

                #if 0
                    //25 cycles per function + fuction call overhead
                    __m256 c0 = actualGrad4x(chunkData, H0, xf0, yf0, zf0);
                    __m256 c1 = actualGrad4x(chunkData, H1, xf1, yf0, zf0);
                    __m256 c2 = actualGrad4x(chunkData, H2, xf0, yf1, zf0);
                    __m256 c3 = actualGrad4x(chunkData, H3, xf1, yf1, zf0);
                    __m256 c4 = actualGrad4x(chunkData, H4, xf0, yf0, zf1);
                    __m256 c5 = actualGrad4x(chunkData, H5, xf1, yf0, zf1);
                    __m256 c6 = actualGrad4x(chunkData, H6, xf0, yf1, zf1);
                    __m256 c7 = actualGrad4x(chunkData, H7, xf1, yf1, zf1);

                #else
                    //20 cycles per function + function call overhead
                    // __m128 c0 = gradLookup4x(chunkData, H0, xf0, yf0, zf0);
                    // __m128 c1 = gradLookup4x(chunkData, H1, xf1, yf0, zf0);
                    // __m128 c2 = gradLookup4x(chunkData, H2, xf0, yf1, zf0);
                    // __m128 c3 = gradLookup4x(chunkData, H3, xf1, yf1, zf0);
                    // __m128 c4 = gradLookup4x(chunkData, H4, xf0, yf0, zf1);
                    // __m128 c5 = gradLookup4x(chunkData, H5, xf1, yf0, zf1);
                    // __m128 c6 = gradLookup4x(chunkData, H6, xf0, yf1, zf1);
                    // __m128 c7 = gradLookup4x(chunkData, H7, xf1, yf1, zf1);


                //inlining the grad function calls are much faster
                __m256i idx0 = _mm256_and_si256(H0, _11); // 12 directions
                __m256i idx1 = _mm256_and_si256(H1, _11);
                __m256i idx2 = _mm256_and_si256(H2, _11);
                __m256i idx3 = _mm256_and_si256(H3, _11);
                __m256i idx4 = _mm256_and_si256(H4, _11);
                __m256i idx5 = _mm256_and_si256(H5, _11);
                __m256i idx6 = _mm256_and_si256(H6, _11);
                __m256i idx7 = _mm256_and_si256(H7, _11);

                __m256   gx0 = _mm256_i32gather_ps(chunkData->gradientX, idx0, 4);
                __m256   gy0 = _mm256_i32gather_ps(chunkData->gradientY, idx0, 4);
                __m256   gz0 = _mm256_i32gather_ps(chunkData->gradientZ, idx0, 4);
                __m256   gx1 = _mm256_i32gather_ps(chunkData->gradientX, idx1, 4);
                __m256   gy1 = _mm256_i32gather_ps(chunkData->gradientY, idx1, 4);
                __m256   gz1 = _mm256_i32gather_ps(chunkData->gradientZ, idx1, 4);
                __m256   gx2 = _mm256_i32gather_ps(chunkData->gradientX, idx2, 4);
                __m256   gy2 = _mm256_i32gather_ps(chunkData->gradientY, idx2, 4);
                __m256   gz2 = _mm256_i32gather_ps(chunkData->gradientZ, idx2, 4);
                __m256   gx3 = _mm256_i32gather_ps(chunkData->gradientX, idx3, 4);
                __m256   gy3 = _mm256_i32gather_ps(chunkData->gradientY, idx3, 4);
                __m256   gz3 = _mm256_i32gather_ps(chunkData->gradientZ, idx3, 4);
                __m256   gx4 = _mm256_i32gather_ps(chunkData->gradientX, idx4, 4);
                __m256   gy4 = _mm256_i32gather_ps(chunkData->gradientY, idx4, 4);
                __m256   gz4 = _mm256_i32gather_ps(chunkData->gradientZ, idx4, 4);
                __m256   gx5 = _mm256_i32gather_ps(chunkData->gradientX, idx5, 4);
                __m256   gy5 = _mm256_i32gather_ps(chunkData->gradientY, idx5, 4);
                __m256   gz5 = _mm256_i32gather_ps(chunkData->gradientZ, idx5, 4);
                __m256   gx6 = _mm256_i32gather_ps(chunkData->gradientX, idx6, 4);
                __m256   gy6 = _mm256_i32gather_ps(chunkData->gradientY, idx6, 4);
                __m256   gz6 = _mm256_i32gather_ps(chunkData->gradientZ, idx6, 4);
                __m256   gx7 = _mm256_i32gather_ps(chunkData->gradientX, idx7, 4);
                __m256   gy7 = _mm256_i32gather_ps(chunkData->gradientY, idx7, 4);
                __m256   gz7 = _mm256_i32gather_ps(chunkData->gradientZ, idx7, 4);
                __m256    c0 = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(gx0, xf0), _mm256_mul_ps(gy0, yf0)),_mm256_mul_ps(gz0, zf0));
                __m256    c1 = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(gx1, xf1), _mm256_mul_ps(gy1, yf0)),_mm256_mul_ps(gz1, zf0));
                __m256    c2 = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(gx2, xf0), _mm256_mul_ps(gy2, yf1)),_mm256_mul_ps(gz2, zf0));
                __m256    c3 = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(gx3, xf1), _mm256_mul_ps(gy3, yf1)),_mm256_mul_ps(gz3, zf0));
                __m256    c4 = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(gx4, xf0), _mm256_mul_ps(gy4, yf0)),_mm256_mul_ps(gz4, zf1));
                __m256    c5 = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(gx5, xf1), _mm256_mul_ps(gy5, yf0)),_mm256_mul_ps(gz5, zf1));
                __m256    c6 = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(gx6, xf0), _mm256_mul_ps(gy6, yf1)),_mm256_mul_ps(gz6, zf1));
                __m256    c7 = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(gx7, xf1), _mm256_mul_ps(gy7, yf1)),_mm256_mul_ps(gz7, zf1));
                

                #endif



                // ---- 9) Interpolate: x
                __m256 x1 = _mm256_add_ps(c0 , _mm256_mul_ps(u , _mm256_sub_ps(c1 , c0)));
                __m256 x2 = _mm256_add_ps(c2 , _mm256_mul_ps(u , _mm256_sub_ps(c3 , c2)));
                __m256 x3 = _mm256_add_ps(c4 , _mm256_mul_ps(u , _mm256_sub_ps(c5 , c4)));
                __m256 x4 = _mm256_add_ps(c6 , _mm256_mul_ps(u , _mm256_sub_ps(c7 , c6)));

                // __m128 x1 = lerp4x(u, c0, c1);
                // __m128 x2 = lerp4x(u, c2, c3);
                // __m128 x3 = lerp4x(u, c4, c5);
                // __m128 x4 = lerp4x(u, c6, c7);

                // ---- 10) Interpolate: y
                // __m128 y1 = lerp4x(v, x1, x2);
                // __m128 y2 = lerp4x(v, x3, x4);
                __m256 y1 = _mm256_add_ps(x1 , _mm256_mul_ps(v , _mm256_sub_ps(x2 , x1)));
                __m256 y2 = _mm256_add_ps(x3 , _mm256_mul_ps(v , _mm256_sub_ps(x4 , x3)));

                // ---- 11) Interpolate: z
                // __m128 result = lerp4x(w, y1, y2);
                __m256 result = _mm256_add_ps(y1 , _mm256_mul_ps(w , _mm256_sub_ps(y2 , y1)));

                // ---- 12) Normalize output to [0,1]
                result = _mm256_mul_ps(_mm256_add_ps(result, _mm256_set1_ps(1.0f)), _mm256_set1_ps(0.5f));


                // ---- 13) Return final packed vec4
                _mm256_storeu_ps(output, result);




            chunkData->perlinNoise3dCyclesTotal += (__rdtsc() - start);
            chunkData->perlinNoise3dHits++;

}


//takes in total of 8 x coords, outputs 8 results
inline void
perlinNoiseFactoredFull4x2x(chunk_data *chunkData, float* x, float y, float z, float* output)
{


    u64 start = __rdtsc();


    __m128i primeX = _mm_set1_epi32(501125321);
    __m128i primeY = _mm_set1_epi32(1136930381u);
    __m128i primeZ = _mm_set1_epi32(1720413743u);

    __m128 _15 = _mm_set1_ps(15.0f);
    __m128i _11 = _mm_set1_epi32(11);
    __m128 _10 = _mm_set1_ps(10.0f);
    __m128 _6  = _mm_set1_ps(6.0f);

    __m128 py = _mm_set1_ps(y);
    __m128 pz = _mm_set1_ps(z);

    __m128 floory = _mm_floor_ps(py);    
    __m128 floorz = _mm_floor_ps(pz);    

    __m128i yi = _mm_cvttps_epi32(floory);
    __m128i zi = _mm_cvttps_epi32(floorz);

    __m128 yf0 = _mm_sub_ps(py, floory);
    __m128 zf0 = _mm_sub_ps(pz, floorz);

    __m128 yf1 = _mm_sub_ps(yf0, _mm_set1_ps(1.0f));
    __m128 zf1 = _mm_sub_ps(zf0, _mm_set1_ps(1.0f));

    __m128i yp0 = _mm_mullo_epi32(yi, primeY);
    __m128i zp0 = _mm_mullo_epi32(zi, primeZ);

    __m128i yp1 = _mm_add_epi32(yp0, primeY);
    __m128i zp1 = _mm_add_epi32(zp0, primeZ);

    // __m128 v = noiseFade4x(yf0);
    __m128 yt2    = _mm_mul_ps(yf0,yf0);
    __m128 yt3    = _mm_mul_ps(yt2,yf0);
    __m128 yinner = _mm_add_ps(_mm_mul_ps(yf0, _mm_sub_ps(_mm_mul_ps(yf0, _6), _15)), _10);
    __m128 v      = _mm_mul_ps(yt3, yinner);
    
    // __m128 w = noiseFade4x(zf0);
    __m128 wt2    = _mm_mul_ps(zf0,zf0);
    __m128 wt3    = _mm_mul_ps(wt2,zf0);
    __m128 winner = _mm_add_ps(_mm_mul_ps(zf0, _mm_sub_ps(_mm_mul_ps(zf0, _6), _15)), _10);
    __m128 w      = _mm_mul_ps(wt3, winner);

    
    for(int i = 0; i < 2; i++){

        __m128 px = _mm_loadu_ps(x + (i*4));

        __m128 floorx = _mm_floor_ps(px);    
        
        __m128i xi = _mm_cvttps_epi32(floorx);
        

        __m128 xf0 = _mm_sub_ps(px, floorx);
    

        __m128 xf1 = _mm_sub_ps(xf0, _mm_set1_ps(1.0f));


        __m128i xp0 = _mm_mullo_epi32(xi, primeX);


        __m128i xp1 = _mm_add_epi32(xp0, primeX);





        // ---- 7) Compute fade curves packed
        // __m128 u = noiseFade4x(xf0);
        __m128 xt2    = _mm_mul_ps(xf0,xf0);
        __m128 xt3    = _mm_mul_ps(xt2,xf0);
        __m128 xinner = _mm_add_ps(_mm_mul_ps(xf0, _mm_sub_ps(_mm_mul_ps(xf0, _6), _15)), _10);
        __m128 u      = _mm_mul_ps(xt3, xinner);



        // ---- 8) Evaluate gradients packed: 8 corners
        __m128i seed = _mm_set1_epi32(1066037191);
        __m128i hex = _mm_set1_epi32(0x27d4eb2d);
        __m128i hashx0 = _mm_xor_si128(seed, xp0);
        __m128i hashx1 = _mm_xor_si128(seed, xp1);
        __m128i hashx0y0 = _mm_xor_si128(hashx0, yp0);
        __m128i hashx1y0 = _mm_xor_si128(hashx1, yp0);
        __m128i hashx0y1 = _mm_xor_si128(hashx0, yp1);
        __m128i hashx1y1 = _mm_xor_si128(hashx1, yp1);

        __m128i hashx0y0z0 = _mm_xor_si128(hashx0y0, zp0);
        __m128i hashx1y0z0 = _mm_xor_si128(hashx1y0, zp0);
        __m128i hashx0y1z0 = _mm_xor_si128(hashx0y1, zp0);
        __m128i hashx1y1z0 = _mm_xor_si128(hashx1y1, zp0);
        __m128i hashx0y0z1 = _mm_xor_si128(hashx0y0, zp1);
        __m128i hashx1y0z1 = _mm_xor_si128(hashx1y0, zp1);
        __m128i hashx0y1z1 = _mm_xor_si128(hashx0y1, zp1);
        __m128i hashx1y1z1 = _mm_xor_si128(hashx1y1, zp1);

        __m128i hashx0y0z0hex = _mm_mullo_epi32(hashx0y0z0, hex);
        __m128i hashx1y0z0hex = _mm_mullo_epi32(hashx1y0z0, hex);
        __m128i hashx0y1z0hex = _mm_mullo_epi32(hashx0y1z0, hex);
        __m128i hashx1y1z0hex = _mm_mullo_epi32(hashx1y1z0, hex);
        __m128i hashx0y0z1hex = _mm_mullo_epi32(hashx0y0z1, hex);
        __m128i hashx1y0z1hex = _mm_mullo_epi32(hashx1y0z1, hex);
        __m128i hashx0y1z1hex = _mm_mullo_epi32(hashx0y1z1, hex);
        __m128i hashx1y1z1hex = _mm_mullo_epi32(hashx1y1z1, hex);

        __m128i H0 = _mm_xor_si128(hashx0y0z0hex, _mm_srli_epi32(hashx0y0z0hex, 15));
        __m128i H1 = _mm_xor_si128(hashx1y0z0hex, _mm_srli_epi32(hashx1y0z0hex, 15));
        __m128i H2 = _mm_xor_si128(hashx0y1z0hex, _mm_srli_epi32(hashx0y1z0hex, 15));
        __m128i H3 = _mm_xor_si128(hashx1y1z0hex, _mm_srli_epi32(hashx1y1z0hex, 15));
        __m128i H4 = _mm_xor_si128(hashx0y0z1hex, _mm_srli_epi32(hashx0y0z1hex, 15));
        __m128i H5 = _mm_xor_si128(hashx1y0z1hex, _mm_srli_epi32(hashx1y0z1hex, 15));
        __m128i H6 = _mm_xor_si128(hashx0y1z1hex, _mm_srli_epi32(hashx0y1z1hex, 15));
        __m128i H7 = _mm_xor_si128(hashx1y1z1hex, _mm_srli_epi32(hashx1y1z1hex, 15));

        //directly inlining saves us ~100 cycles total
        // __m128i H0 = hashPrimes4x(seed, xp0, yp0, zp0);
        // __m128i H1 = hashPrimes4x(seed, xp1, yp0, zp0);
        // __m128i H2 = hashPrimes4x(seed, xp0, yp1, zp0);
        // __m128i H3 = hashPrimes4x(seed, xp1, yp1, zp0);
        // __m128i H4 = hashPrimes4x(seed, xp0, yp0, zp1);
        // __m128i H5 = hashPrimes4x(seed, xp1, yp0, zp1);
        // __m128i H6 = hashPrimes4x(seed, xp0, yp1, zp1);
        // __m128i H7 = hashPrimes4x(seed, xp1, yp1, zp1);

        u64 SIMDLookupStart = __rdtsc();

        #if 0
            //25 cycles per function + fuction call overhead
            __m128 c0 = actualGrad4x(chunkData, H0, xf0, yf0, zf0);
            __m128 c1 = actualGrad4x(chunkData, H1, xf1, yf0, zf0);
            __m128 c2 = actualGrad4x(chunkData, H2, xf0, yf1, zf0);
            __m128 c3 = actualGrad4x(chunkData, H3, xf1, yf1, zf0);
            __m128 c4 = actualGrad4x(chunkData, H4, xf0, yf0, zf1);
            __m128 c5 = actualGrad4x(chunkData, H5, xf1, yf0, zf1);
            __m128 c6 = actualGrad4x(chunkData, H6, xf0, yf1, zf1);
            __m128 c7 = actualGrad4x(chunkData, H7, xf1, yf1, zf1);

        #else
            //20 cycles per function + function call overhead
            // __m128 c0 = gradLookup4x(chunkData, H0, xf0, yf0, zf0);
            // __m128 c1 = gradLookup4x(chunkData, H1, xf1, yf0, zf0);
            // __m128 c2 = gradLookup4x(chunkData, H2, xf0, yf1, zf0);
            // __m128 c3 = gradLookup4x(chunkData, H3, xf1, yf1, zf0);
            // __m128 c4 = gradLookup4x(chunkData, H4, xf0, yf0, zf1);
            // __m128 c5 = gradLookup4x(chunkData, H5, xf1, yf0, zf1);
            // __m128 c6 = gradLookup4x(chunkData, H6, xf0, yf1, zf1);
            // __m128 c7 = gradLookup4x(chunkData, H7, xf1, yf1, zf1);


        //inlining the grad function calls are much faster
        __m128i idx0 = _mm_and_si128(H0, _11); // 12 directions
        __m128i idx1 = _mm_and_si128(H1, _11);
        __m128i idx2 = _mm_and_si128(H2, _11);
        __m128i idx3 = _mm_and_si128(H3, _11);
        __m128i idx4 = _mm_and_si128(H4, _11);
        __m128i idx5 = _mm_and_si128(H5, _11);
        __m128i idx6 = _mm_and_si128(H6, _11);
        __m128i idx7 = _mm_and_si128(H7, _11);

        __m128   gx0 = _mm_i32gather_ps(chunkData->gradientX, idx0, 4);
        __m128   gy0 = _mm_i32gather_ps(chunkData->gradientY, idx0, 4);
        __m128   gz0 = _mm_i32gather_ps(chunkData->gradientZ, idx0, 4);

        __m128   gx1 = _mm_i32gather_ps(chunkData->gradientX, idx1, 4);
        __m128   gy1 = _mm_i32gather_ps(chunkData->gradientY, idx1, 4);
        __m128   gz1 = _mm_i32gather_ps(chunkData->gradientZ, idx1, 4);

        __m128   gx2 = _mm_i32gather_ps(chunkData->gradientX, idx2, 4);
        __m128   gy2 = _mm_i32gather_ps(chunkData->gradientY, idx2, 4);
        __m128   gz2 = _mm_i32gather_ps(chunkData->gradientZ, idx2, 4);

        __m128   gx3 = _mm_i32gather_ps(chunkData->gradientX, idx3, 4);
        __m128   gy3 = _mm_i32gather_ps(chunkData->gradientY, idx3, 4);
        __m128   gz3 = _mm_i32gather_ps(chunkData->gradientZ, idx3, 4);

        __m128   gx4 = _mm_i32gather_ps(chunkData->gradientX, idx4, 4);
        __m128   gy4 = _mm_i32gather_ps(chunkData->gradientY, idx4, 4);
        __m128   gz4 = _mm_i32gather_ps(chunkData->gradientZ, idx4, 4);

        __m128   gx5 = _mm_i32gather_ps(chunkData->gradientX, idx5, 4);
        __m128   gy5 = _mm_i32gather_ps(chunkData->gradientY, idx5, 4);
        __m128   gz5 = _mm_i32gather_ps(chunkData->gradientZ, idx5, 4);


        __m128   gx6 = _mm_i32gather_ps(chunkData->gradientX, idx6, 4);
        __m128   gy6 = _mm_i32gather_ps(chunkData->gradientY, idx6, 4);
        __m128   gz6 = _mm_i32gather_ps(chunkData->gradientZ, idx6, 4);

        __m128   gx7 = _mm_i32gather_ps(chunkData->gradientX, idx7, 4);
        __m128   gy7 = _mm_i32gather_ps(chunkData->gradientY, idx7, 4);
        __m128   gz7 = _mm_i32gather_ps(chunkData->gradientZ, idx7, 4);

        __m128    c0 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(gx0, xf0), _mm_mul_ps(gy0, yf0)),_mm_mul_ps(gz0, zf0));
        __m128    c1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(gx1, xf1), _mm_mul_ps(gy1, yf0)),_mm_mul_ps(gz1, zf0));
        __m128    c2 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(gx2, xf0), _mm_mul_ps(gy2, yf1)),_mm_mul_ps(gz2, zf0));
        __m128    c3 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(gx3, xf1), _mm_mul_ps(gy3, yf1)),_mm_mul_ps(gz3, zf0));
        __m128    c4 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(gx4, xf0), _mm_mul_ps(gy4, yf0)),_mm_mul_ps(gz4, zf1));
        __m128    c5 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(gx5, xf1), _mm_mul_ps(gy5, yf0)),_mm_mul_ps(gz5, zf1));
        __m128    c6 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(gx6, xf0), _mm_mul_ps(gy6, yf1)),_mm_mul_ps(gz6, zf1));
        __m128    c7 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(gx7, xf1), _mm_mul_ps(gy7, yf1)),_mm_mul_ps(gz7, zf1));
        

        #endif



        // ---- 9) Interpolate: x
        __m128 x1 = _mm_add_ps(c0 , _mm_mul_ps(u , _mm_sub_ps(c1 , c0)));
        __m128 x2 = _mm_add_ps(c2 , _mm_mul_ps(u , _mm_sub_ps(c3 , c2)));
        __m128 x3 = _mm_add_ps(c4 , _mm_mul_ps(u , _mm_sub_ps(c5 , c4)));
        __m128 x4 = _mm_add_ps(c6 , _mm_mul_ps(u , _mm_sub_ps(c7 , c6)));

        // __m128 x1 = lerp4x(u, c0, c1);
        // __m128 x2 = lerp4x(u, c2, c3);
        // __m128 x3 = lerp4x(u, c4, c5);
        // __m128 x4 = lerp4x(u, c6, c7);

        // ---- 10) Interpolate: y
        // __m128 y1 = lerp4x(v, x1, x2);
        // __m128 y2 = lerp4x(v, x3, x4);
        __m128 y1 = _mm_add_ps(x1 , _mm_mul_ps(v , _mm_sub_ps(x2 , x1)));
        __m128 y2 = _mm_add_ps(x3 , _mm_mul_ps(v , _mm_sub_ps(x4 , x3)));

        // ---- 11) Interpolate: z
        // __m128 result = lerp4x(w, y1, y2);
        __m128 result = _mm_add_ps(y1 , _mm_mul_ps(w , _mm_sub_ps(y2 , y1)));

        // ---- 12) Normalize output to [0,1]
        result = _mm_mul_ps(_mm_add_ps(result, _mm_set1_ps(1.0f)), _mm_set1_ps(0.5f));


        // ---- 13) Return final packed vec4
        _mm_storeu_ps(output + (i*4), result);


    }


    chunkData->perlinNoise3dCyclesTotal += (__rdtsc() - start);
    chunkData->perlinNoise3dHits++;

}




vec2 GetConstantVector(u32 v){
    u32 h = v & 3;
    if(h == 0){
        return { 1,  1};
    }else if(h == 1){
        return {-1,  1};
    }else if(h == 2){
        return {-1, -1};
    }else{
        return { 1, -1};
    }

}

//2d perlin noise
float perlin2d(chunk_data* chunkData, vec2 vec){
    u32 x = (u32)vec.x & 255; 
    u32 y = (u32)vec.y & 255; 

    float xf = vec.x - (u32)vec.x;
    float yf = vec.y - (u32)vec.y;

    vec2 topRight       = {xf-1.0f, yf-1.0f};
    vec2 topLeft        = {xf     , yf-1.0f};
    vec2 bottomRight    = {xf-1.0f, yf     };
    vec2 bottomLeft     = {xf     , yf     };

    //select a value from the permutation array for each of the 4 corners
    float valueTopRight     = chunkData->perlinPermTable[chunkData->perlinPermTable[x+1]+y+1]; 
    float valueTopLeft      = chunkData->perlinPermTable[chunkData->perlinPermTable[x  ]+y+1]; 
    float valueBottomRight  = chunkData->perlinPermTable[chunkData->perlinPermTable[x+1]+y  ]; 
    float valueBottomLeft   = chunkData->perlinPermTable[chunkData->perlinPermTable[x  ]+y  ];

    float dotTopRight       = vec2_dot(topRight     , GetConstantVector(valueTopRight    ));
    float dotTopLeft        = vec2_dot(topLeft      , GetConstantVector(valueTopLeft     ));
    float dotBottomRight    = vec2_dot(bottomRight  , GetConstantVector(valueBottomRight ));
    float dotBottomLeft     = vec2_dot(bottomLeft   , GetConstantVector(valueBottomLeft  ));

    float u = noiseFade(xf);
    float v = noiseFade(yf);

    return noiseLerp(u, noiseLerp(v, dotBottomLeft, dotTopLeft), noiseLerp(v, dotBottomRight, dotTopRight));

}

static float fnlFastFloor(float f) {return (f >= 0 ? (int)f : (int)f - 1);}

// static const int PRIME_X = 501125321;
// static const int PRIME_Y = 1136930381;
// static const int PRIME_Z = 1720413743;

#define PRIME_X 501125321
#define PRIME_Y 1136930381
#define PRIME_Z 1720413743

static inline int fnlHash3d(int xPrimed, int yPrimed, int zPrimed, int seed = 12345){
    int hash = seed ^ xPrimed ^ yPrimed ^ zPrimed;
    hash *= 0x27d4eb2d;
    // PRINT("hash: %d\n", hash);
    return hash;
}

static inline int fnlHash2d(int xPrimed, int yPrimed, int seed = 12345){
    int hash = seed ^ xPrimed ^ yPrimed;
    hash *= 0x27d4eb2d;
    return hash;
}

static inline float fnlGradCoord3d(chunk_data* chunkData, int xPrimed, int yPrimed, int zPrimed, float xd, float yd, float zd, int seed = 12345){
    int hash = fnlHash3d(xPrimed, yPrimed, zPrimed, seed);
    int hashShift = hash >> 15; 
    hash ^= hashShift;
    int test  = 63<<2;
    u32 test2 = 63<<2;
    hash &= 63 << 2;
    return xd * chunkData->gradients3d[hash] + yd * chunkData->gradients3d[hash | 1] + zd * chunkData->gradients3d[hash | 2];
}


static inline __m128 fnlGradCoord3d4x(chunk_data* chunkData, __m128i xPrimed, __m128i yPrimed, __m128i zPrimed, __m128 xd, __m128 yd, __m128 zd, int seed = 12345){

    __m128i seed4x = _mm_set1_epi32(12345);
    __m128i scale  = _mm_set1_epi32(0x27d4eb2d);
    __m128i hash4x = _mm_xor_si128(seed4x, _mm_xor_si128(xPrimed, _mm_xor_si128(yPrimed, zPrimed)));
    // hash4x = _mm_mul_epi32(hash4x, scale);
    // int a = ((int*)&hash4x)[0] * 0x27d4eb2d;
    // int b = ((int*)&hash4x)[1] * 0x27d4eb2d;
    // int c = ((int*)&hash4x)[2] * 0x27d4eb2d;
    // int d = ((int*)&hash4x)[3] * 0x27d4eb2d;

    hash4x = _mm_mullo_epi32(hash4x, scale);
    // __m128i hashShift = _mm_srli_epi32(hash4x, 15);
    __m128i hashShift = _mm_srai_epi32(hash4x, 15);
    // int shifta = ((int*)&hashShift)[0];
    // int shiftb = ((int*)&hashShift)[1];
    // int shiftc = ((int*)&hashShift)[2];
    // int shiftd = ((int*)&hashShift)[3];


    hash4x = _mm_xor_si128(hash4x, hashShift);


    __m128i mask = _mm_set1_epi32(63<<2);
    hash4x = _mm_and_si128(hash4x, mask);

    // return xd * chunkData->gradients3d[hash] + yd * chunkData->gradients3d[hash | 1] + zd * chunkData->gradients3d[hash | 2];

    // float out[4];

    // for (int i = 0; i < 4; ++i) {
    //     int h = ((int*)&hash4x)[i];  // extract scalar hash
    //     float gx = g[h];
    //     float gy = g[h | 1];
    //     float gz = g[h | 2];

    //     out[i] = ((float*)&xd)[i] * gx +
    //             ((float*)&yd)[i] * gy +
    //             ((float*)&zd)[i] * gz;
    // }
    // f32_4x result = {out[0],out[1],out[2],out[3]};
    // return xd * chunkData->gradients3d[hash] + yd * chunkData->gradients3d[hash | 1] + zd * chunkData->gradients3d[hash | 2];




    int h0 = _mm_extract_epi32(hash4x, 0);
    int h1 = _mm_extract_epi32(hash4x, 1); 
    int h2 = _mm_extract_epi32(hash4x, 2);
    int h3 = _mm_extract_epi32(hash4x, 3);

    __m128 gx = _mm_setr_ps(chunkData->gradients3d[h0],   chunkData->gradients3d[h1],   chunkData->gradients3d[h2],   chunkData->gradients3d[h3]);
    __m128 gy = _mm_setr_ps(chunkData->gradients3d[h0|1], chunkData->gradients3d[h1|1], chunkData->gradients3d[h2|1], chunkData->gradients3d[h3|1]);
    __m128 gz = _mm_setr_ps(chunkData->gradients3d[h0|2], chunkData->gradients3d[h1|2], chunkData->gradients3d[h2|2], chunkData->gradients3d[h3|2]);

    __m128 result = _mm_add_ps(_mm_mul_ps(xd, gx), 
                    _mm_add_ps(_mm_mul_ps(yd, gy), _mm_mul_ps(zd, gz)));


    return result;
}


static inline float fnlGradCoord2d(chunk_data* chunkData, int xPrimed, int yPrimed, float xd, float yd, int seed = 12345){
    int hash = fnlHash2d(xPrimed, yPrimed, seed);
    hash ^= hash >> 15;
    hash &= 63 << 1;
    return xd * chunkData->gradients2d[hash] + yd * chunkData->gradients2d[hash | 1];
}


float fnlPerlin3d(chunk_data* chunkData, vec3 vec){
    
    u64 start = __rdtsc();

    int x0 = (vec.x >= 0 ? (int)vec.x : (int)vec.x - 1);
    int y0 = (vec.y >= 0 ? (int)vec.y : (int)vec.y - 1);
    int z0 = (vec.z >= 0 ? (int)vec.z : (int)vec.z - 1);
    
    float xf0 = (float)(vec.x - x0);
    float yf0 = (float)(vec.y - y0);
    float zf0 = (float)(vec.z - z0);
    float xf1 = xf0 - 1;
    float yf1 = yf0 - 1;
    float zf1 = zf0 - 1;

    float xs = noiseFade(xf0);
    float ys = noiseFade(yf0);
    float zs = noiseFade(zf0);
    
    x0 *= PRIME_X;
    y0 *= PRIME_Y;
    z0 *= PRIME_Z;

    int x1 = x0 + PRIME_X;
    int y1 = y0 + PRIME_Y;
    int z1 = z0 + PRIME_Z;

    int seed = 12345;
    float xf00 = noiseLerp(xs, fnlGradCoord3d(chunkData, x0, y0, z0, xf0, yf0, zf0, seed), fnlGradCoord3d(chunkData, x1, y0, z0, xf1, yf0, zf0, seed));
    float xf10 = noiseLerp(xs, fnlGradCoord3d(chunkData, x0, y1, z0, xf0, yf1, zf0, seed), fnlGradCoord3d(chunkData, x1, y1, z0, xf1, yf1, zf0, seed));
    float xf01 = noiseLerp(xs, fnlGradCoord3d(chunkData, x0, y0, z1, xf0, yf0, zf1, seed), fnlGradCoord3d(chunkData, x1, y0, z1, xf1, yf0, zf1, seed));
    float xf11 = noiseLerp(xs, fnlGradCoord3d(chunkData, x0, y1, z1, xf0, yf1, zf1, seed), fnlGradCoord3d(chunkData, x1, y1, z1, xf1, yf1, zf1, seed));

    float yf00 = noiseLerp(ys, xf00, xf10);
    float yf01 = noiseLerp(ys, xf01, xf11);



    float result = noiseLerp(zs, yf00, yf01) * 0.964921414852142333984375f;

    chunkData->perlinNoise3dCyclesTotal += (__rdtsc() - start);
    chunkData->perlinNoise3dHits++;


    return result;
}


float fnlPerlin3dFactored(chunk_data* chunkData, vec3 vec){
    
    u64 start = __rdtsc();

    int x0 = (vec.x >= 0 ? (int)vec.x : (int)vec.x - 1);
    int y0 = (vec.y >= 0 ? (int)vec.y : (int)vec.y - 1);
    int z0 = (vec.z >= 0 ? (int)vec.z : (int)vec.z - 1);


    
    float xf0 = (float)(vec.x - x0);
    float yf0 = (float)(vec.y - y0);
    float zf0 = (float)(vec.z - z0);
    float xf1 = xf0 - 1;
    float yf1 = yf0 - 1;
    float zf1 = zf0 - 1;

    float ys = noiseFade(yf0);
    float zs = noiseFade(zf0);

    f32_4x xs4x = {_mm_set1_ps(noiseFade(xf0))};


    
    // f32_4x fadeInput = {xf0, yf0, zf0, 0};
    // f32_4x s4x = noiseFade4x(fadeInput);
    // f32_4x xs4x  = {_mm_set_ps1(s4x.e[0])};
    // float ys = s4x.e[1];
    // float zs = s4x.e[2];

    __m128i x1Primed = _mm_set1_epi32(((x0+1) * PRIME_X));
    __m128i x0Primed = _mm_set1_epi32((x0 * PRIME_X));
    __m128i yPrimed  = _mm_setr_epi32((y0 * PRIME_Y), ((y0+1) * PRIME_Y), (y0 * PRIME_Y), ((y0+1) * PRIME_Y));
    __m128i zPrimed  = _mm_setr_epi32((z0 * PRIME_Z), (z0 * PRIME_Z), ((z0+1) * PRIME_Z), ((z0+1) * PRIME_Z));
    
    __m128 xf04x = _mm_set_ps1(xf0);
    __m128 yf4x  = _mm_setr_ps(yf0, yf1, yf0, yf1);
    __m128 zf4x  = _mm_setr_ps(zf0, zf0, zf1, zf1);
    __m128 xf14x = _mm_set_ps1(xf1);


    f32_4x l4x = {fnlGradCoord3d4x(chunkData, x0Primed, yPrimed, zPrimed, xf04x, yf4x, zf4x)}; 
    f32_4x r4x = {fnlGradCoord3d4x(chunkData, x1Primed, yPrimed, zPrimed, xf14x, yf4x, zf4x)}; 

    // if(!(g0 == l4x.e[0]) || !(g2 == l4x.e[1]) || !(g4 == l4x.e[2]) || !(g6 == l4x.e[3])){
    //     int notequal = 0;
    // }
    // f32_4x l4x =  {g0, g2, g4, g6};
    // f32_4x r4x =  {g1, g3, g5, g7};
    
    f32_4x xf  = lerp4x(xs4x, l4x, r4x);


    // float xf00 = noiseLerp(xs, g0, g1);
    // float xf10 = noiseLerp(xs, g2, g3);
    // float xf01 = noiseLerp(xs, g4, g5);
    // float xf11 = noiseLerp(xs, g6, g7);

    float yf00 = noiseLerp(ys, xf.e[0], xf.e[1]);
    float yf01 = noiseLerp(ys, xf.e[2], xf.e[3]);



    float result = noiseLerp(zs, yf00, yf01) * 0.964921414852142333984375f;

    chunkData->perlinNoise3dCyclesTotal += (__rdtsc() - start);
    chunkData->perlinNoise3dHits++;


    return result;
}



void fnlPerlin3dScalar8x(float* result, chunk_data* chunkData, float* x, float y, float z){
    for(int i = 0; i < 8; i++){
        result[i] = (perlinNoiseFactored(chunkData, x[i], y, z)); 
    }
}



float fnlPerlin2d(chunk_data* chunkData, vec2 vec, int seed = 12345){
    int x0 = vec.x >= 0 ? (int)vec.x : ((int)vec.x - 1); 
    int y0 = vec.y >= 0 ? (int)vec.y : ((int)vec.y - 1);
    float xf0 = (float)(vec.x - x0); 
    float yf0 = (float)(vec.y - y0); 
    float xf1 = xf0 - 1;
    float yf1 = yf0 - 1;
    
    float xs = noiseFade(xf0);
    float ys = noiseFade(yf0);

    x0 *= PRIME_X;
    y0 *= PRIME_Y;
    
    int x1 = x0 + PRIME_X;
    int y1 = y0 + PRIME_Y;

    float xf00 = noiseLerp(xs, fnlGradCoord2d(chunkData, x0, y0, xf0, yf0, seed), fnlGradCoord2d(chunkData, x1, y0, xf1, yf0, seed));
    float xf01 = noiseLerp(xs, fnlGradCoord2d(chunkData, x0, y1, xf0, yf1, seed), fnlGradCoord2d(chunkData, x1, y1, xf1, yf1, seed));

    return noiseLerp(ys, xf00, xf01) * 1.4247691104677813f;
} 

float perlin2dTiled(chunk_data* chunkData, float x, float y, int period){

    int xi0 = ((int)floorf(x)) % period;
    int yi0 = ((int)floorf(y)) % period;
    if (xi0 < 0) xi0 += period;
    if (yi0 < 0) yi0 += period;

    int xi1 = (xi0 + 1) % period;
    int yi1 = (yi0 + 1) % period;

    float xf = x - floorf(x);
    float yf = y - floorf(y);

    float u = noiseFade(xf);
    float v = noiseFade(yf);

    int aa = chunkData->permutations2d[(chunkData->permutations2d[xi0] + yi0) % 512];
    int ab = chunkData->permutations2d[(chunkData->permutations2d[xi0] + yi1) % 512];
    int ba = chunkData->permutations2d[(chunkData->permutations2d[xi1] + yi0) % 512];
    int bb = chunkData->permutations2d[(chunkData->permutations2d[xi1] + yi1) % 512];

    vec2 g_aa = chunkData->directionGradients2d[aa];
    vec2 g_ab = chunkData->directionGradients2d[ab];
    vec2 g_ba = chunkData->directionGradients2d[ba];
    vec2 g_bb = chunkData->directionGradients2d[bb];

    vec2 d_aa = {xf    , yf    };
    vec2 d_ba = {xf - 1, yf    };
    vec2 d_ab = {xf    , yf - 1};
    vec2 d_bb = {xf - 1, yf - 1};

    float dot_aa = vec2_dot(g_aa, d_aa);
    float dot_ba = vec2_dot(g_ba, d_ba);
    float dot_ab = vec2_dot(g_ab, d_ab);
    float dot_bb = vec2_dot(g_bb, d_bb);

    float x1 = noiseLerp(u, dot_aa, dot_ba);
    float x2 = noiseLerp(u, dot_ab, dot_bb);

    float result = noiseLerp(v, x1, x2);
    // return (result+1.0f)*0.5f;
    return (result);
}


float perlin3dTiled(chunk_data* chunkData, float x, float y, float z, int periodX, int periodY, int periodZ) {
    // Wrap coordinates to periods
    int xi0 = ((int)floorf(x)) % periodX;
    int yi0 = ((int)floorf(y)) % periodY;
    int zi0 = ((int)floorf(z)) % periodZ;
    
    if (xi0 < 0) xi0 += periodX;
    if (yi0 < 0) yi0 += periodY;
    if (zi0 < 0) zi0 += periodZ;

    int xi1 = (xi0 + 1) % periodX;
    int yi1 = (yi0 + 1) % periodY;
    int zi1 = (zi0 + 1) % periodZ;

    // Fractional parts
    float xf = x - floorf(x);
    float yf = y - floorf(y);
    float zf = z - floorf(z);

    // Fade curves
    float u = noiseFade(xf);
    float v = noiseFade(yf);
    float w = noiseFade(zf);

    // Hash coordinates (assuming you have permutations3d and directionGradients3d arrays)
    int aaa = chunkData->permutations3d[(chunkData->permutations3d[(chunkData->permutations3d[xi0] + yi0) % 512] + zi0) % 512];
    int aba = chunkData->permutations3d[(chunkData->permutations3d[(chunkData->permutations3d[xi0] + yi1) % 512] + zi0) % 512];
    int aab = chunkData->permutations3d[(chunkData->permutations3d[(chunkData->permutations3d[xi0] + yi0) % 512] + zi1) % 512];
    int abb = chunkData->permutations3d[(chunkData->permutations3d[(chunkData->permutations3d[xi0] + yi1) % 512] + zi1) % 512];
    int baa = chunkData->permutations3d[(chunkData->permutations3d[(chunkData->permutations3d[xi1] + yi0) % 512] + zi0) % 512];
    int bba = chunkData->permutations3d[(chunkData->permutations3d[(chunkData->permutations3d[xi1] + yi1) % 512] + zi0) % 512];
    int bab = chunkData->permutations3d[(chunkData->permutations3d[(chunkData->permutations3d[xi1] + yi0) % 512] + zi1) % 512];
    int bbb = chunkData->permutations3d[(chunkData->permutations3d[(chunkData->permutations3d[xi1] + yi1) % 512] + zi1) % 512];

    // Get 3D gradient vectors
    vec3 g_aaa = chunkData->directionGradients3d[aaa];
    vec3 g_aba = chunkData->directionGradients3d[aba];
    vec3 g_aab = chunkData->directionGradients3d[aab];
    vec3 g_abb = chunkData->directionGradients3d[abb];
    vec3 g_baa = chunkData->directionGradients3d[baa];
    vec3 g_bba = chunkData->directionGradients3d[bba];
    vec3 g_bab = chunkData->directionGradients3d[bab];
    vec3 g_bbb = chunkData->directionGradients3d[bbb];

    // Distance vectors from each cube corner
    vec3 d_aaa = {xf    , yf    , zf    };
    vec3 d_baa = {xf - 1, yf    , zf    };
    vec3 d_aba = {xf    , yf - 1, zf    };
    vec3 d_bba = {xf - 1, yf - 1, zf    };
    vec3 d_aab = {xf    , yf    , zf - 1};
    vec3 d_bab = {xf - 1, yf    , zf - 1};
    vec3 d_abb = {xf    , yf - 1, zf - 1};
    vec3 d_bbb = {xf - 1, yf - 1, zf - 1};

    // Dot products
    float dot_aaa = vec3_dot(g_aaa, d_aaa);
    float dot_baa = vec3_dot(g_baa, d_baa);
    float dot_aba = vec3_dot(g_aba, d_aba);
    float dot_bba = vec3_dot(g_bba, d_bba);
    float dot_aab = vec3_dot(g_aab, d_aab);
    float dot_bab = vec3_dot(g_bab, d_bab);
    float dot_abb = vec3_dot(g_abb, d_abb);
    float dot_bbb = vec3_dot(g_bbb, d_bbb);

    // Trilinear interpolation
    float x1 = noiseLerp(u, dot_aaa, dot_baa);
    float x2 = noiseLerp(u, dot_aba, dot_bba);
    float x3 = noiseLerp(u, dot_aab, dot_bab);
    float x4 = noiseLerp(u, dot_abb, dot_bbb);

    float y1 = noiseLerp(v, x1, x2);
    float y2 = noiseLerp(v, x3, x4);

    float result = noiseLerp(w, y1, y2);
    return (result + 1.0f) * 0.5f;
}


float perlin4dTiled(chunk_data* chunkData, float x, float y, float z, float w,
                    int periodX, int periodY, int periodZ, int periodW) {

    int xi0 = ((int)floorf(x)) % periodX;
    int yi0 = ((int)floorf(y)) % periodY;
    int zi0 = ((int)floorf(z)) % periodZ;
    int wi0 = ((int)floorf(w)) % periodW;

    if (xi0 < 0) xi0 += periodX;
    if (yi0 < 0) yi0 += periodY;
    if (zi0 < 0) zi0 += periodZ;
    if (wi0 < 0) wi0 += periodW;

    int xi1 = (xi0 + 1) % periodX;
    int yi1 = (yi0 + 1) % periodY;
    int zi1 = (zi0 + 1) % periodZ;
    int wi1 = (wi0 + 1) % periodW;

    float xf = x - floorf(x);
    float yf = y - floorf(y);
    float zf = z - floorf(z);
    float wf = w - floorf(w);

    float u = noiseFade(xf);
    float v = noiseFade(yf);
    float t = noiseFade(zf);
    float s = noiseFade(wf);

    // 16 hashed lattice points
    #define HASH4(x, y, z, w) \
        chunkData->permutations4d[ \
            chunkData->permutations4d[ \
                chunkData->permutations4d[ \
                    chunkData->permutations4d[x] + y] + z] + w]

    int h0000 = HASH4(xi0, yi0, zi0, wi0);
    int h0001 = HASH4(xi0, yi0, zi0, wi1);
    int h0010 = HASH4(xi0, yi0, zi1, wi0);
    int h0011 = HASH4(xi0, yi0, zi1, wi1);
    int h0100 = HASH4(xi0, yi1, zi0, wi0);
    int h0101 = HASH4(xi0, yi1, zi0, wi1);
    int h0110 = HASH4(xi0, yi1, zi1, wi0);
    int h0111 = HASH4(xi0, yi1, zi1, wi1);
    int h1000 = HASH4(xi1, yi0, zi0, wi0);
    int h1001 = HASH4(xi1, yi0, zi0, wi1);
    int h1010 = HASH4(xi1, yi0, zi1, wi0);
    int h1011 = HASH4(xi1, yi0, zi1, wi1);
    int h1100 = HASH4(xi1, yi1, zi0, wi0);
    int h1101 = HASH4(xi1, yi1, zi0, wi1);
    int h1110 = HASH4(xi1, yi1, zi1, wi0);
    int h1111 = HASH4(xi1, yi1, zi1, wi1);

    // Dot products
    float dp[16];
    vec4 d[16] = {
        {xf,     yf,     zf,     wf    },
        {xf,     yf,     zf,     wf-1  },
        {xf,     yf,     zf-1,   wf    },
        {xf,     yf,     zf-1,   wf-1  },
        {xf,     yf-1,   zf,     wf    },
        {xf,     yf-1,   zf,     wf-1  },
        {xf,     yf-1,   zf-1,   wf    },
        {xf,     yf-1,   zf-1,   wf-1  },
        {xf-1,   yf,     zf,     wf    },
        {xf-1,   yf,     zf,     wf-1  },
        {xf-1,   yf,     zf-1,   wf    },
        {xf-1,   yf,     zf-1,   wf-1  },
        {xf-1,   yf-1,   zf,     wf    },
        {xf-1,   yf-1,   zf,     wf-1  },
        {xf-1,   yf-1,   zf-1,   wf    },
        {xf-1,   yf-1,   zf-1,   wf-1  },
    };

    int hashes[16] = {
        h0000, h0001, h0010, h0011,
        h0100, h0101, h0110, h0111,
        h1000, h1001, h1010, h1011,
        h1100, h1101, h1110, h1111,
    };

    for (int i = 0; i < 16; i++) {
        dp[i] = vec4_dot(chunkData->directionGradients4d[hashes[i]], d[i]);
    }

    // 4D LERP interpolation
    float x00 = noiseLerp(u, dp[0], dp[8]);
    float x01 = noiseLerp(u, dp[1], dp[9]);
    float x02 = noiseLerp(u, dp[2], dp[10]);
    float x03 = noiseLerp(u, dp[3], dp[11]);
    float x04 = noiseLerp(u, dp[4], dp[12]);
    float x05 = noiseLerp(u, dp[5], dp[13]);
    float x06 = noiseLerp(u, dp[6], dp[14]);
    float x07 = noiseLerp(u, dp[7], dp[15]);

    float y0 = noiseLerp(v, x00, x04);
    float y1 = noiseLerp(v, x01, x05);
    float y2 = noiseLerp(v, x02, x06);
    float y3 = noiseLerp(v, x03, x07);

    float z0 = noiseLerp(t, y0, y2);
    float z1 = noiseLerp(t, y1, y3);

    float result = noiseLerp(s, z0, z1);
    return (result + 1.0f) * 0.5f;
}



//VALUE NOISE
vec4 valueNoise3d(vec3 v){
    vec4 result = {};
    ivec3 p = ivec3_floor(v);
    vec3 w = vec3_fract(v);

    vec3 u = w*w*w*(w*(w*6.0f-15.0f)+10.0f);
    vec3 du = 30.0f*w*w*(w*(w-1.0f)+2.0f);

    float a = valueNoiseHash(p + ivec3_create(0,0,0));
    float b = valueNoiseHash(p + ivec3_create(1,0,0));
    float c = valueNoiseHash(p + ivec3_create(0,1,0));
    float d = valueNoiseHash(p + ivec3_create(1,1,0));

    float e = valueNoiseHash(p + ivec3_create(0,0,1));
    float f = valueNoiseHash(p + ivec3_create(1,0,1));
    float g = valueNoiseHash(p + ivec3_create(0,1,1));
    float h = valueNoiseHash(p + ivec3_create(1,1,1));

    float k0 = a;
    float k1 = b - a;
    float k2 = c - a;
    float k3 = e - a;

    float k4 =   a - b - c + d;
    float k5 =   a - c - e + g;
    float k6 =   a - b - e + f;
    float k7 = - a + b + c - d + e - f - g + h;

    return {-1.0f+2.0f*(k0 + k1 * u.x + k2 * u.y + k3 * u.z + k4 * u.x * u.y + k5 * u.y * u.z + k6 * u.z * u.x + k7 * u.x * u.y * u.z),
            2.0f*du.x*(k1 + k4*u.y + k6*u.z + k7*u.y*u.z),
            2.0f*du.y*(k2 + k5*u.z + k4*u.x + k7*u.z*u.x),
            2.0f*du.z*(k3 + k6*u.x + k5*u.y + k7*u.x*u.y)};


}



float getTiledNoiseValueHash(chunk_data* chunkData, int x, int y, int z){
    return chunkData->valueNoise3d[x & (VALUE_NOISE_TILE-1)][y & (VALUE_NOISE_TILE-1)][z & (VALUE_NOISE_TILE-1)];

}

vec4 valueNoise3dTiled(game_state* GameState, chunk_data* chunkData, vec3 v){
    u64 start = __rdtsc();

    ivec3 p = ivec3_floor(v);
    vec3 w = vec3_fract(v);

    vec3 u = w*w*w*(w*(w*6.0f-15.0f)+10.0f);
    vec3 du = 30.0f*w*w*(w*(w-1.0f)+2.0f);


    float a = getTiledNoiseValueHash(chunkData, p.x+0 , p.y+0 , p.z+0);
    float b = getTiledNoiseValueHash(chunkData, p.x+1 , p.y+0 , p.z+0);
    float c = getTiledNoiseValueHash(chunkData, p.x+0 , p.y+1 , p.z+0);
    float d = getTiledNoiseValueHash(chunkData, p.x+1 , p.y+1 , p.z+0);
    float e = getTiledNoiseValueHash(chunkData, p.x+0 , p.y+0 , p.z+1);
    float f = getTiledNoiseValueHash(chunkData, p.x+1 , p.y+0 , p.z+1);
    float g = getTiledNoiseValueHash(chunkData, p.x+0 , p.y+1 , p.z+1);
    float h = getTiledNoiseValueHash(chunkData, p.x+1 , p.y+1 , p.z+1);

    float k0 = a;
    float k1 = b - a;
    float k2 = c - a;
    float k3 = e - a;

    float k4 =   a - b - c + d;
    float k5 =   a - c - e + g;
    float k6 =   a - b - e + f;
    float k7 = - a + b + c - d + e - f - g + h;

    vec4 result = {-1.0f+2.0f*(k0 + k1 * u.x + k2 * u.y + k3 * u.z + k4 * u.x * u.y + k5 * u.y * u.z + k6 * u.z * u.x + k7 * u.x * u.y * u.z),
                    2.0f*du.x*(k1 + k4*u.y + k6*u.z + k7*u.y*u.z),
                    2.0f*du.y*(k2 + k5*u.z + k4*u.x + k7*u.z*u.x),
                    2.0f*du.z*(k3 + k6*u.x + k5*u.y + k7*u.x*u.y)};


    GameState->valNoise3dCyclesTotal += (__rdtsc() - start);
    GameState->valNoise3dHits++;

    return result;
}
//END VALUE NOISE



//END NOISE GENERATION


bool checkChunkDestructionRules(game_state* GameState, CameraComp& camera, ivec3& coords, uvec3& drawDistance, s32 centerX, s32 centerY, s32 centerZ){
    //add more rules as we build up the engine
    ivec3 camChunkCoords = camera.freeMode ? camera.temp_chunk_coords : camera.chunk_coords;
    coords -= camChunkCoords;
    if(abs(coords.x) > centerX || abs(coords.y) > centerY || abs(coords.z) > centerZ){
        return true;
    }
    return false;

}


void floodFillLightAcrossBrickmaps(chunk_data* chunkData, Brickmap64* bm, ivec3& brickmapCoords, ivec3& chunk_coords, int brickmapID, int chunkID){
    chunkData->lightBoundaryActiveQueueCount = 0;
    chunkData->lightBmCount = 0;
    
    chunkData->lightBrickmaps[chunkData->lightBoundaryActiveQueueCount];
    uint32_t& count = chunkData->lightBoundaryQueueCounts[0];
    for(int x = 1; x < 63; x++){
        for(int y = 1; y < 63; y++){
            for(int z = 1; z < 63; z++){
                ivec3 voxCoords = ivec3_create(x,y,z);
                uint32_t v = x + (y * 64) + (z * 64 * 64);
                // if(((bm->voxels[v] >> 10) & 0x3) == 1 ){//10th bit reserved for light
                if(((bm->voxels[v]) & 0xF) == 1 ){//10th bit reserved for light
                    bm->voxels[v] |= (15 << 12);
                    chunkData->lightBoundaryQueueValues[0][count] = 15;
                    chunkData->lightBoundaryQueuePositions[0][count] = voxCoords;
                    count++;
                }
            }
        }
    }

    chunkData->lightBrickmaps[0] = bm;
    chunkData->lightchunk_coords[0] = chunk_coords;
    chunkData->lightChunkIDs[0] = chunkID;
    chunkData->lightBMIDs[0] = brickmapID;
    chunkData->lightBmCount = 1;

    chunkData->lightBoundaryActiveQueueCount++;
    uint32_t currentBrickmap = 0;
    uint32_t totalBrickmaps = 1;
    while(currentBrickmap < chunkData->lightBmCount){
        uint32_t& lightQueueCount = chunkData->lightBoundaryQueueCounts[currentBrickmap];
        uint8_t* lightValues = chunkData->lightBoundaryQueueValues[currentBrickmap];
        ivec3* queueVoxCoords = chunkData->lightBoundaryQueuePositions[currentBrickmap];

        Brickmap64* currentBm = chunkData->lightBrickmaps[currentBrickmap];
        ivec3& currentchunk_coords = chunkData->lightchunk_coords[currentBrickmap];
        uint32_t& currentChunkID = chunkData->lightChunkIDs[currentBrickmap];
        uint32_t& currentBMID = chunkData->lightBMIDs[currentBrickmap];
        currentBrickmap++;
        bool edited = false;
        while(lightQueueCount > 0){

            //pop voxel, determine light value, compare with neighbors, propogate to non solid blocks
            lightQueueCount--;

            uint8_t lightVal = lightValues[lightQueueCount];
            ivec3 voxCoords = queueVoxCoords[lightQueueCount];
            //check 6 neighbords
            for(int i = 0; i < 6; i++){
                int light = lightVal - 1;
                int x = voxCoords.x;
                int y = voxCoords.y;
                int z = voxCoords.z;

                const int dx[] = {0, 0, 0, 0, 1, -1};
                const int dy[] = {0, 0, 1, -1, 0, 0};
                const int dz[] = {1, -1, 0, 0, 0, 0};

                x = voxCoords.x + dx[i];
                y = voxCoords.y + dy[i];
                z = voxCoords.z + dz[i];
                ivec3 adjVoxCoords = ivec3_create(x,y,z);
                if(z < 1 || z > 62 || y < 1 || y > 62 || x < 1 || x > 62){
                    if(z < 0 || z > 63 || y < 0 || y > 63 || x < 0 || x > 63)continue;

                    lightFloodFillBoundaries(chunkData, adjVoxCoords, currentchunk_coords, currentChunkID, light);

                    continue;
                }

            


                uint32_t v = x + (y * 64) + (z * 64 * 64);
                // uint8_t neighborLightValue = (currentBm->voxels[v] >> 12) & 0xF;
                uint8_t neighborLightValue = (0xF);
                if(neighborLightValue >= light)continue;//skip blocks with equal or greater light values
                neighborLightValue = light; //set neighbors light value, add it to the queue
                // currentBm->voxels[v] |= (neighborLightValue << 12);
                currentBm->voxels[v] |= (0xF);
                if(neighborLightValue <= 1)continue;//skip solid blocks for now, skip if light value is lowest
                // Or to check if it's air (lower 10 bits are zero)
                // if((currentBm->voxels[v] & 0x3FF))continue; //skip if voxel is solid
                if(currentBm->voxels[v])continue; //skip if voxel is solid

                lightValues[lightQueueCount] = neighborLightValue;
                queueVoxCoords[lightQueueCount] = ivec3_create(x,y,z);
                lightQueueCount++;
                edited = true;

            }


        }
        if(edited){
            bool found = false;
            for(int i = 0; i < chunkData->editedCount; i++){
                if(chunkData->editedBMIDs[i] == currentBMID && chunkData->editedChunkIDs[i] == currentChunkID){
                    found = true;
                    break;
                }
            }
            if(!found){
                chunkData->editedChunkIDs[chunkData->editedCount] = currentChunkID;
                chunkData->editedBMIDs[chunkData->editedCount] = currentBMID;
                chunkData->editedCount++;
            }
        }

    }


}

// void paddingTest(){
    
// }

inline bool isBMInArrays(uint32_t chunkID, uint32_t BMID, uint32_t* chunkArray, uint32_t* bmArray, uint32_t count){
    bool found = false;
    for(int i = 0; i < count; i++){
        if(chunkID == chunkArray[i] && BMID == bmArray[i]){
            found = true;
            break;
        }
    }
    return found;
}

void floodFillLight(chunk_data* chunkData, Brickmap64* bm){
        //flood fill lighting etc
        
    // for(int i = 0; i < pbmr3; i++){

    // }
    //ignore padding for now
    for(int x = 1; x < 63; x++){
        for(int y = 1; y < 63; y++){
            for(int z = 1; z < 63; z++){
                ivec3 voxCoords = ivec3_create(x,y,z);
                uint32_t v = x + (y * 64) + (z * 64 * 64);
                // if(((bm->voxels[v] >> 10) & 0x3) == 1 ){//10th bit reserved for light
                if(((bm->voxels[v]) & 0xF) == 1 ){//10th bit reserved for light
                    // bm->voxels[v] |= (15 << 12);
                    chunkData->lightQueueValues[chunkData->lightQueueCount] = 15;
                    chunkData->lightQueuePositions[chunkData->lightQueueCount] = voxCoords;
                    chunkData->lightQueueCount++;
                }
            }
        }
    }
    while(chunkData->lightQueueCount > 0){
        //pop voxel, determine light value, compare with neighbors, propogate to non solid blocks
        chunkData->lightQueueCount--;
        uint8_t lightVal = chunkData->lightQueueValues[chunkData->lightQueueCount];
        ivec3 voxCoords = chunkData->lightQueuePositions[chunkData->lightQueueCount];
        //check 6 neighbords
        for(int i = 0; i < 6; i++){

            int x = voxCoords.x;
            int y = voxCoords.y;
            int z = voxCoords.z;

            const int dx[] = {0, 0, 0, 0, 1, -1};
            const int dy[] = {0, 0, 1, -1, 0, 0};
            const int dz[] = {1, -1, 0, 0, 0, 0};

            x = voxCoords.x + dx[i];
            y = voxCoords.y + dy[i];
            z = voxCoords.z + dz[i];

            if(z < 1 || z > 62 || y < 1 || y > 62 || x < 1 || x > 62){
                //outside of brickmap range, continue
                continue;
                // lightFloodFillBoundaries(chunkData, voxCoords, ivec3& brickmapCoords, ivec3& chunk_coords, int brickmapID, int chunkID, uint8_t light,
                //                         int brickmapRes, uint16_t LODType);
            }
            uint32_t v = x + (y * 64) + (z * 64 * 64);
            // uint8_t neighborLightValue = (bm->voxels[v] >> 12) & 0xF;
            uint8_t neighborLightValue = 0xF;
            if(neighborLightValue >= lightVal)continue;//skip blocks with equal or greater light values
            neighborLightValue = lightVal - 1; //set neighbors light value, add it to the queue
            // bm->voxels[v] |= (neighborLightValue << 12);
            bm->voxels[v] |= (0xF);
            if(neighborLightValue <= 1)continue;//skip solid blocks for now, skip if light value is lowest
            // Or to check if it's air (lower 10 bits are zero)
            if((bm->voxels[v]))continue; //skip if voxel is solid

            chunkData->lightQueueValues[chunkData->lightQueueCount] = neighborLightValue;
            chunkData->lightQueuePositions[chunkData->lightQueueCount] = ivec3_create(x,y,z);
            chunkData->lightQueueCount++;

        }
    
    }
}

void findBrushBrickmaps(chunk_data* chunkData){

}


void recalculateBrushAABB(chunk_data* chunkData, fpt_vec3 brushPos, ivec3 brushOriginchunk_coords, fpt brushSize){
    // chunkData->fptBrushPos = chunkData->voxelRayCastResult.fptHitPosition;
    chunkData->fptBrushPos = brushPos;
    chunkData->brushCenterchunk_coords = brushOriginchunk_coords;

    chunkData->brushSize = fpt2fl(brushSize);

    chunkData->fptBrushBounds.min = brushPos - fpt_vec3_create(brushSize);
    chunkData->fptBrushBounds.max = brushPos + fpt_vec3_create(brushSize);
    // vec3 floatMin = fpt_to_flt_vec3(chunkData->fptBrushBounds.min);
    // vec3 floatMax = fpt_to_flt_vec3(chunkData->fptBrushBounds.max);
    // printf("fptBrushBounds Min: %f %f %f\n", floatMin.x,floatMin.y,floatMin.z);
    // printf("fptBrushBounds Max: %f %f %f\n", floatMax.x,floatMax.y,floatMax.z);

    chunkData->brushChunkCoordsCount = 0;

    // void intersectChunks(chunk_data* chunkData,const fpt_vec3& aabbMin       , const fpt_vec3& aabbMax      , const ivec3& startchunk_coords    , ivec3* chunkArray           , uint32_t arrayMaxSize, uint32_t& chunkCount) {
    intersectChunks(chunkData, chunkData->fptBrushBounds.min, chunkData->fptBrushBounds.max, chunkData->brushCenterchunk_coords, chunkData->brushChunkCoords, ArrayCount(chunkData->brushChunkCoords), chunkData->brushChunkCoordsCount);


    
    
    
    // printf("brush chunk coords count: %d\n", chunkData->brushChunkCoordsCount);

    chunkData->brushChunkIDCount = 0;
    
    for(int i = 0; i < chunkData->brushChunkCoordsCount; i++){
        /*TAG*/ivec3 currchunk_coords = chunkData->brushChunkCoords[i];
        // printf("brush chunk coords: %d %d %d\n", currchunk_coords.x,currchunk_coords.y,currchunk_coords.z);
        uint32_t chunkID = getchunkID(chunkData, chunkData->brushChunkCoords[i]);
        
        if(chunkID == NULL_CHUNK)continue;
        
    
        // fpt_vec3 chunkPos = chunkData->fptPositions[chunkID];
        ivec3 chunk_coords = chunkData->coords[chunkID];
        // COLLISION_DEBUG_PRINTF("chunkID: %d coords: %d %d %d\n", chunkID, chunk_coords.x, chunk_coords.y, chunk_coords.z);
        fpt_vec3 relativeChunkOffset = ivec_to_fpt_vec3(chunk_coords - brushOriginchunk_coords) * FPT_CHUNK_SIZE;

        // printf("chunk_coords:   %d %d %d\n", chunk_coords.x,chunk_coords.y,chunk_coords.z);

     
        // ivec3 bmCoords = ivec3_create(0);
        if(chunkData->brushChunkIDCount >= 64){
            //spdlog::error("BRUSH INTERSECTS TOO MANY BRICKMAPS, INCREASE ARRAY SIZE");
            continue;
        }

        bool found = false;
        for(int i = 0; i < chunkData->brushChunkIDCount; i++){
            if(chunkData->brushChunkIDs[i] == chunkID){
                found = true;
                break;
            }
        }
        if(found){
            continue;
        }

        
        // COLLISION_DEBUG_PRINTF("chunkID: %d, brickmapID %d\n", chunkID, brickmapID);
        fpt_vec3 adjustedMin = chunkData->fptBrushBounds.min - relativeChunkOffset;
        fpt_vec3 adjustedMax = chunkData->fptBrushBounds.max - relativeChunkOffset;
        ivec3 voxCoordsMin = fpt_calculatePaddedVoxelCoordsFloor(adjustedMin);
        ivec3 voxCoordsMax = fpt_calculatePaddedVoxelCoordsCeil(adjustedMax);
        // printf("Voxels Min:    %d %d %d\n", voxCoordsMin.x,voxCoordsMin.y,voxCoordsMin.z);
        // printf("Voxels Max:    %d %d %d\n", voxCoordsMax.x,voxCoordsMax.y,voxCoordsMax.z);

        chunkData->brushChunkIDs[chunkData->brushChunkIDCount] = chunkID;
        
        //store voxel bounds for later use
        chunkData->brushChunkVoxMin[chunkData->brushChunkIDCount] = voxCoordsMin;
        chunkData->brushChunkVoxMax[chunkData->brushChunkIDCount] = voxCoordsMax;

        chunkData->brushChunkIDCount++;

        checkPadding(chunkData, voxCoordsMin, voxCoordsMax, chunk_coords, chunkID);

    }
   

}

  int manhattanDistance(chunk_data* chunkData, fpt_vec3& chunkPos){
          // if(chunkData->manhattanOverride > 0 && chunkData->manhattanOverride <= 3){
          //   int type = chunkData->manhattanOverride;

          //   float dist = abs(chunkPos.x - cameraPos.x) + 
          //               abs(chunkPos.y - cameraPos.y) + 
          //               abs(chunkPos.z - cameraPos.z);
          
          //   // if (dist <= 128.0f){
          //   if (dist <= 196.0f){
          //     COLLISION_DEBUG_PRINTF("chunkPos: %f %f %f has LOD TYPE 1 DIST: %f\n", chunkPos.x,chunkPos.y,chunkPos.z, dist);
          //     return type;//LODLevel::High;
          //   }
          //   if (dist <= 256.0f) return 0;//LODLevel::Medium;
          //   if (dist <= 352.0f) return 0;//LODLevel::Low;
          //   return 0;//too far;


          // }
          // Manhattan distance is sum of absolute differences for each axis
            fpt dist = fpt_add(
                fpt_add(
                    fpt_abs(chunkPos.x),
                    fpt_abs(chunkPos.y)
                ),
                fpt_abs(chunkPos.z)
            );
        //   float dist = abs(chunkPos.x - cameraPos.x) + 
                    //    abs(chunkPos.y - cameraPos.y) + 
                    //    abs(chunkPos.z - cameraPos.z);
          
          // if (dist <= 128.0f){
          float floatDist = fpt2fl(dist);
          if (dist <= i2fpt(128)){//used to be 192
            // COLLISION_DEBUG_PRINTF("chunkPos: %f %f %f has LOD TYPE 1 DIST: %f\n", chunkPos.x,chunkPos.y,chunkPos.z, dist);
            // COLLISION_DEBUG_PRINTF("chunkPos: %f %f %f LOD TYPE 1 DIST: %f\n", floatChunkPos.x,floatChunkPos.y,floatChunkPos.z, floatDist);
            return 3; // used to be 1
          }
          return 0;//too far;
      }

    uint32_t assignBufferIndex(chunk_data* chunkData, uint32_t bmToBufferIndex){
        uint32_t bufferIndex = chunkData->bufferCount;
        chunkData->bmToBufferMap[bmToBufferIndex] = bufferIndex;
        chunkData->bufferToBmMap[bufferIndex] = bmToBufferIndex;
        chunkData->bufferCount++;
        // chunkData->highResHandles[bufferIndex] = bgfx::create etc
        return bufferIndex;
    }
    void removeBufferIndex(chunk_data* chunkData, uint32_t bmToBufferIndex){
        uint32_t lastIndex = chunkData->bufferCount - 1;
        // bgfx::destroy(highResHandles[removedIndex]) etc
        if(lastIndex != chunkData->bmToBufferMap[bmToBufferIndex]){
          uint32_t removedIndex = chunkData->bmToBufferMap[bmToBufferIndex];
        //   chunkData->highResHandles[removedIndex] = chunkData->highResHandles[lastIndex];

          uint32_t lastBrickmapIndex = chunkData->bufferToBmMap[lastIndex];
          chunkData->bmToBufferMap[lastBrickmapIndex] = removedIndex;

          chunkData->bufferToBmMap[removedIndex] = chunkData->bufferToBmMap[lastIndex];
        }
        chunkData->bmToBufferMap[bmToBufferIndex] = NULL_CHUNK;
        chunkData->bufferToBmMap[lastIndex] = NULL_CHUNK;
        chunkData->bufferCount--;
    }



/////////////////////////////////////////////////// NOISE POPULATION //////////////////////////////////////


bool is_voxel_in_padding_coords(uint32_t voxel_index, uint32_t brickmap_resolution) {
    // Calculate coordinates
    uint32_t x = voxel_index % brickmap_resolution;
    uint32_t y = (voxel_index / brickmap_resolution) % brickmap_resolution;
    uint32_t z = voxel_index / (brickmap_resolution * brickmap_resolution);

    // Check if the voxel is in the padding
    return x == 0 || x == brickmap_resolution - 1 ||
           y == 0 || y == brickmap_resolution - 1 ||
           z == 0 || z == brickmap_resolution - 1;
}
constexpr float invP2 = 1/4096;
constexpr float invP = 1/64;
//obsolete, moved into same function as voxel generation
#if 0
uint32_t binary_voxelize_brickmap_with_noise(chunk_data* chunkData, voxel_work* work, Brickmap64& brickmap, CoarseBrickmap64& coarse, bool excludePadding = false){
    // BEGIN_TIMED_BLOCK(VoxelizeBrickmap);
  
    // if(excludePadding){
  //   COLLISION_DEBUG_PRINTF("excluding padding\n");
  // }
    for(int coarseID = 0; coarseID < 512; coarseID++){
        Assert(chunkData->coarse_brickmaps[work->chunkID].active_count[coarseID] == 0);
    }
    
    float averageNoiseValue = 0.0f;
    uint32_t voxelsPopulated = 0;
    memset(work->noiseVoxels, 0, sizeof(uint8_t) * pbmr3);
    memset(brickmap.voxels, 0 , sizeof(uint8_t) * pbmr3);
    #define loop3d 0
    #if loop3d
    for (uint32_t i = 0; i < pbmr3; ++i) {
        float voxelNoiseValue = work->brickmap_noise[i];
        if (voxelNoiseValue > 0.1f) {
            //when we wanted to draw the skirt, but this conflicts with actually telling us about colliding voxels
            //   if(excludePadding && is_in_padding(chunkData->paddingLookup, i)){
        //     continue;
        //   }

          //populate coarse grid
          uint32_t x = i % 64;
          uint32_t y = (i / 64) % 64;
          uint32_t z = i / 4096;

          uint32_t cx = x / 8;
          uint32_t cy = y / 8;
          uint32_t cz = z / 8;
          uint32_t coarse_index = cx + (cy * 8) + (cz * 64);
          coarse.active_count[coarse_index]++;

          
          
          work->noiseVoxels[i] = static_cast<uint8_t>((voxelNoiseValue+.1f) * 10);
          voxelsPopulated++;
          averageNoiseValue += voxelNoiseValue;
        }
    }
    #else

    for(int z = 0; z < pbmr; z++){
        uint32_t cz = z / 8;
        for(int x = 0; x < pbmr; x++){
            uint32_t cx = x / 8;
            float voxelNoiseValue = work->brickmap_noise[z * pbmr + x];
            u8 colorVal = voxelNoiseValue * 255;
            u8 heightVal = voxelNoiseValue * 64;
            for(int y = 0; y < pbmr; y++){
                if(heightVal > y){

                    uint32_t cy = y / 8;
                    uint32_t coarse_index = cx + (cy * 8) + (cz * 64);
                    
                    coarse.active_count[coarse_index]++;
                    voxelsPopulated++;
                    averageNoiseValue += voxelNoiseValue;
                    
                    uint32_t voxel_index = x + (y * 64) + (z * 4096);
                    work->noiseVoxels[voxel_index] = colorVal;
                }
                    
            }


        }
    }
    #endif

    #undef loop3d
    if(voxelsPopulated){
        work->flags |= has_voxels;
        size_t data_size = pbmr3 * sizeof(uint8_t);
        brickmap.active_count = voxelsPopulated;
        memcpy(brickmap.voxels,work->noiseVoxels , data_size);
    }
    // END_TIMED_BLOCK(VoxelizeBrickmap);

    return voxelsPopulated;

}
#endif
/////////////////////////////////////////////////// NOISE POPULATION END //////////////////////////////////////

enum VoxPopResults{
    voxPop_none,
    voxPop_alreadyFilled,
    voxPop_chunkBoundary,
};

inline void AppendMetaChunkInfo(chunk_data* chunkData, u32 chunkID, VoxMetaGenType metaType, VoxelTypes voxType, u32 voxIndex, u32 seedState){
    VoxMetaGenEntry genEntry = {};
    genEntry.type = metaType;
    genEntry.randSeed = seedState;
    genEntry.seed.voxIndex = voxIndex;
    genEntry.seed.type = voxType;
    chunkData->metaGenInfo[chunkID][chunkData->metaGenInfoCount[chunkID]++] = genEntry;

    //TODO: (nate) we never flush these, need to flush them when we do the second pass, otherwise they will keep accumulating and eventually crash us
    Assert(chunkData->metaGenInfoCount[chunkID] < 512);
}

//cuts off voxels in padding, need to handle cross chunk population still
inline VoxPopResults placeTerrainFeatureVoxel(voxel_work* work, VoxelTypes type, int x, int y, int z, u32& voxelsPopulated, CoarseBrickmap64* coarse){
    if((x > 0 && x <= CHUNK_SIZE) && (y > 0 && y <= CHUNK_SIZE) && (z > 0 && z <= CHUNK_SIZE)){
        int voxIndex = x + (y * pbmr) + (z*pbmr*pbmr);
        if(!work->noiseVoxels[voxIndex]){
            work->noiseVoxels[voxIndex] = type; 
            int coarse_index = (x/8) + ((y/8) * 8) + ((z/8) * 64);
            coarse->active_count[coarse_index]++;
            voxelsPopulated++;
            return voxPop_none;
        }else{
            return voxPop_alreadyFilled;
        }
        
    }else{
        return voxPop_chunkBoundary;
    }
}

enum BiomeType{
    biome_none = 0,
    biome_dirt,  //contains trees
    biome_stone, //contains ores
};

/////////////////////////////////////////////////// WORKER THREADS //////////////////////////////////////

    void do_voxel_work(chunk_data* chunkData, voxel_work* work){
        u64 startTime = __rdtsc();

        work->perlinCycles = 0;
        work->perlinHits = 0;
        work->perlinTotal = 0;
        work->accumulateVoxelsTotalCycles = 0;
        work->accumulateVoxelsHits = 0;

        vec3 chunkPos = {}; 
        ivec3 coords = chunkData->coords[work->chunkID]; 
        chunkPos.x = coords.x * CHUNK_SIZE;
        chunkPos.y = coords.y * CHUNK_SIZE;
        chunkPos.z = coords.z * CHUNK_SIZE;
        
        //need to subtract 1 if we are fully sampling
        // chunkPos = get_chunk_bottom_left_back_corner(chunkPos) - 1;//shift it back to account for padding
        chunkPos = get_chunk_bottom_left_back_corner(chunkPos);
        // float scale = 0.0075;
        // float scale = (1/640);
        float scale = 0.04953125f;

        uint32_t seed = 12345;
        seed += hash3d(coords.x, coords.y, coords.z);
        uint32_t rng = 0;
        rng_seed(&rng, seed);

        vec3 pos = {};
        vec3 scaled = {};
        vec3 scaled3d = {};
        u32 xz = 0;
        pos.y = chunkPos.y;
        scaled.z = (pos.y + work->yoffset) * scale;

            
        float averageNoiseValue = 0.0f;
        uint32_t voxelsPopulated = 0;
        Brickmap64* brickmap = chunkData->brickmaps + work->chunkID;
        brickmap->active_count = 0;
        CoarseBrickmap64* coarse = chunkData->coarse_brickmaps + work->chunkID;
        memset(work->noiseVoxels, 0, sizeof(uint8_t) * pbmr3);
        memset(brickmap->voxels, 0 , sizeof(uint8_t) * pbmr3);
        memset(work->isAboveAir, 0 , sizeof(uint8_t) * pbmr*pbmr);
        
        float blendedWeight = 64.0f * coords.y;
        float surfaceLevel = 32.0f;
        float surfaceHeightRange = 255.0f;

        float caveStartDepth = -32.0f;
        float voidDepth = -512.0f;
        int xSample = 8;
        int ySample = 8;
        int zSample = 8;
        s32 sampleReduction = 1;
        //used to calculate the last sample in a chunk
        s32 lastSamplePadding = 2;
        s32 lastTile = 15 + 16 + 16; // = 47
        
        BiomeType biomeType = biome_dirt;
        if(coords.x & 1){
            biomeType = biome_stone;
        }

        //for voxel grass placement


        int tiles = 4;
        // if(coords.x == -1 && coords.y == 0 && coords.z == 0){
        //     __debugbreak();
        // }
        // if(coords.x == 0 && coords.y == 0 && coords.z == 0){
        //     __debugbreak();
        // }
        // if(coords.x == 1 && coords.y == 0 && coords.z == 0){
        //      __debugbreak();
        // }
        
        //to track absolute x y z inside the chunk tiles

        //xTileStep replaces xSample in the switch statements, because 16 is the size of a tile
        int xTileStep = 16;
        int yTileStep = 16;
        int zTileStep = 16;
        work->seedCount = 0;
        for(int zTiles = 0; zTiles < 4; zTiles++){
            int curZ = 0;
            int extraZSample = 0;
            memset(work->coarseSamplePosZ, 0, sizeof(float) * 18 * 18 * 18);
            switch(zTiles){
                case 0: {curZ = -(zSample-sampleReduction);}break;
                case 1: {curZ =  (zTileStep-sampleReduction);}break;
                case 2: 
                case 3: {curZ =  (zTileStep-sampleReduction) + ((zTiles-1) * zTileStep); extraZSample++;}break;
            }
            for(int xTiles = 0; xTiles < 4; xTiles++){
                memset(work->coarseSamplePosX, 0, sizeof(float) * 18 * 18 * 18);
                int curX = 0;
                int extraXSample = 0;
                switch(xTiles){
                    case 0: {curX = -(xSample-sampleReduction);}break;
                    case 1: {curX =  (xTileStep-sampleReduction);}break;
                    case 2: 
                    case 3: {curX =  (xTileStep-sampleReduction) + ((xTiles-1) * xTileStep); extraXSample++;}break;
                }
                // for(int yTiles = 0; yTiles < 4; yTiles++){
                for(int yTiles = 3; yTiles >= 0; yTiles--){
                memset(work->coarseSamplePosY, 0, sizeof(float) * 18 * 18 * 18);
                    int curY = 0;
                    int extraYSample = 0;
                    switch(yTiles){
                        case 0: {curY = -(ySample-sampleReduction);}break;
                        case 1: {curY =  (yTileStep-sampleReduction);}break;
                        case 2: 
                        case 3: {curY =  (yTileStep-sampleReduction) + ((yTiles-1) * yTileStep); extraYSample++;}break;
                    }
                    int zz = 0;

                    //if its less than 6 we use basic heightmaps and skip this
                    if(work->demoEnum > 6){

                    //full sampling experiment


                    int zSamples = 0;
                    int tempZ = curZ;
                    int z = -1;
                    if(tempZ >= 0){z = 0; zSamples = 1;}
                    for (; z < (16 / zSample) + 1 + extraZSample; z++){
                    // for (; z < (16 / zSample) + 1; z++){
                        float posz = (chunkPos.z + tempZ + work->zoffset) * scale;
                        
                        int xSamples = 0;
                        int tempX = curX;
                        int x = -1;
                        if(tempX >= 0){x = 0; xSamples = 1;}
                        for (; x < (16 / xSample) + 1 + extraXSample; x++){
                        // for (; x < (16 / xSample) + 1; x++){
                                float posx = (chunkPos.x + tempX + work->xoffset) * scale;
                                
                                int ySamples = 0;
                                // int ySamples = 16 / ySample;
                                int tempY = curY;
                                int y = -1;
                                if(tempY >= 0){y = 0; ySamples = 1;}
                                for (; y < (16 / ySample) + 1 + extraYSample; y++){
                                // for (; y < (16 / ySample) + 1; y++){
                                        float posy = (chunkPos.y + tempY + work->yoffset) * scale;
                                        //record sample
                                        int index = xSamples + (ySamples * 18) + (zSamples * 324);
                                        float val =  (fnlPerlin3d(chunkData, {posx, posy, posz}) + 1.0f)*0.5f;
                                        
                                        // PRINT("%2d %2d %2d : Pos: %3.1f %3.1f %3.1f Curxyz: %2d %2d %2d Vox: %3.1f %3.1f %3.1f : %5f\n",
                                        //disable this when we multithread
                                        // log_file(&chunkData->logger, LogTypes::Info, "%2d %2d %2d : Pos: %5.1f %5.1f %5.1f Curxyz: %4d %4d %4d Vox: %6.1f %6.1f %6.1f: coarse: %2d, %2d, %2d: %5f", 
                                        // coords.x,coords.y,coords.z, 
                                        // chunkPos.x,chunkPos.y,chunkPos.z,
                                        // tempX, tempY, tempZ, 
                                        // chunkPos.x+tempX, chunkPos.y+tempY,chunkPos.z+tempZ,
                                        // xSamples, ySamples, zSamples,
                                        // val);

                                        work->coarseNoiseTile[index] = val;
                                        float coarseSamplePosX = chunkPos.x + tempX + work->xoffset;
                                        float coarseSamplePosY = chunkPos.y + tempY + work->yoffset;
                                        float coarseSamplePosZ = chunkPos.z + tempZ + work->zoffset;
                                        if(coords.x == 0 && (coarseSamplePosX == -46.00f && coarseSamplePosY == -16.00f && coarseSamplePosZ == 0.0f)){
                                            int fuckthedebugger = 0;
                                        }
                                        if(coords.x == -1 && (coarseSamplePosX == -46.00f && coarseSamplePosY == -16.00f && coarseSamplePosZ == 0.0f)){
                                            int fuckthedebugger = 0;
                                        }
                                        if(coords.x == 0 && (xTiles == 1 && yTiles == 1 && zTiles == 1) && (index == 343 || index == 344)){
                                            int fuckthedebugger = 0;
                                        }
                                        work->coarseSamplePosX[index] = coarseSamplePosX;
                                        work->coarseSamplePosY[index] = coarseSamplePosY;
                                        work->coarseSamplePosZ[index] = coarseSamplePosZ;
                                        
                                        if(tempY <  (ySample-sampleReduction) || (tempY >= (CHUNK_SIZE - (ySample-sampleReduction)))){
                                            tempY += (ySample-sampleReduction);
                                            // tempY -= (ySample-sampleReduction);
                                        }else{
                                            tempY += ySample;
                                            // tempY -= ySample;
                                        }
                                        ySamples++;
                                        // ySamples--;

                                }
                                //reset to -2 after the initial loop
                                
                                if(tempX < (xSample-sampleReduction) || (tempX >= (CHUNK_SIZE - (xSample-sampleReduction)))){
                                    tempX += (xSample-sampleReduction);
                                }else{
                                    tempX += xSample;
                                }
                                xSamples++;

                        }
                        if(tempZ < (zSample-sampleReduction) || (tempZ >= (CHUNK_SIZE - (zSample-sampleReduction)))){
                            tempZ += (zSample-sampleReduction);
                        }else{
                            tempZ += zSample;
                        }
                        zSamples++;

                    }
                    

                    }

                    #if 1
                    float coarseSample = 0.0f;
                    zz = 1;                    
                    float fx = 0;
                    float fy = 0;
                    float fz = 0;
                    //populate the voxels from the noise for each block
                    for(int z = (zTiles * 16); z < (zTiles * 16)+16; z++){
                        uint32_t cz = z / 8;
                        pos.z = chunkPos.z + z;
                        scaled.y = (pos.z + work->zoffset) * scale;
                        scaled3d.z = (pos.z + work->zoffset) * scale;
                        
                        if(z <= (zSample-sampleReduction)){
                            if(z == 0){
                                fz = (float)((zSample - sampleReduction)) / (float)zSample;
                            }else{
                                fz = (float)((z-1)) / (float)zSample;
                            }
                        }
                        else if(z >= (CHUNK_SIZE - (zSample - lastSamplePadding))){
                            if(z == 63){
                                fz = 0;
                            }else{
                            // fz = (float)((z-47)) / (float)zSample;
                                fz = ((float)((z-lastTile)) - (float)((float)( zz-1)*zSample)) / (float)zSample;
                            }

                        }else{
                            fz = (float)((z&15) - (float)((float)( zz-1)*zSample)) / (float)zSample;
                        }


                        int xx = 1;           
                        int zzIndex = zz * 324;         
                        for(int x = (xTiles * 16); x < (xTiles * 16)+16; x++){
                            uint32_t cx = x / 8;
                            pos.x = chunkPos.x + x;
                            scaled.x = (pos.x + work->xoffset) * scale;
                            scaled3d.x = (pos.x + work->xoffset) * scale;
                            xz = x + (z * 64);

                            float val =  (fnlPerlin3d(chunkData, scaled) + 1.0f)*0.5f;


                            if(x <= (xSample-sampleReduction)){
                                if(x == 0){
                                    fx = (float)((xSample - sampleReduction)) / (float)xSample;
                                }else{
                                    fx = (float)((x-1)) / (float)xSample;
                                }
                            }
                            else if(x >= (CHUNK_SIZE - (xSample - lastSamplePadding))){
                                if(x == 63){
                                    fx = 0;
                                }else{
                                // fx = (float)((x-47)) / (float)xSample;
                                    fx = ((float)((x-lastTile)) - (float)((float)( xx-1)*xSample)) / (float)xSample;
                                }
                            }else{
                                fx = (float)((x&15) - (float)((float)( xx-1)*xSample)) / (float)xSample;
                            }

                            int yy = 16 / ySample;                    
                            // int yy = 1;                    
                            int xxzzIndex = xx + zzIndex;

                            for(int y = (yTiles * 16)+(16-1); y >= (yTiles * 16); y--){
                            // for(int y = (yTiles * 16); y < (yTiles * 16)+16; y++){
                                uint32_t cy = y / 8;
                                uint32_t final_voxel_index = x + (y * 64) + (z * 4096);
                                // uint32_t voxel_index = (x&15) + ((y&15) * 16) + ((z&15) * 256);
                                uint32_t voxel_index = ((yy) * 18) + xxzzIndex;

                                //interpolate samples
                                
                                int coarse_x0 = xx;
                                int coarse_y0 = yy;
                                int coarse_z0 = zz;
                                if(coords.x == 0  && ((x == 47) && y == 31 && z == 32)||
                                    (coords.x == 0  && ((x == 48) && y == 31 && z == 32))){
                                // if(coords.x == 0  && ((x == 56) && y == 35 && z == 33)||
                                // (coords.x == 0  && ((x == 55) && y == 31 && z == 33))){
                                // if(coords.x == 0  && ((x == 0 || x == 1) && y == 31 && z == 40)){
                                    int fuckthedebugger = 0;
                                }
                                if(coords.x == -1 && ((x == 62 || x == 63) && y == 31 && z == 40)){
                                    int fuckthedebugger = 0;
                                }


                                
                                //need to sample from the final tile of the adjacent chunk

                                
                                if(x==0)coarse_x0--;
                                else if(x==63)coarse_x0++;

                                if(y==0)coarse_y0--;
                                else if(y==63)coarse_y0++;
                                
                                if(z==0)coarse_z0--;
                                else if(z==63)coarse_z0++;

                                
                                //edge voxels need to be the lerped value of the voxel to the left, so 0 needs to be voxel value 62
                                //edge voxel 63 needs to be the value of voxel 1

                              
                                if(y <= (ySample-sampleReduction)){
                                    if(y == 0){
                                        fy = (float)((ySample - sampleReduction)) / (float)ySample;
                                    }else{
                                        fy = (float)((y-1)) / (float)ySample;
                                    }
                                }
                                else if(y >= (CHUNK_SIZE - (ySample - lastSamplePadding))){
                                    if(y == 63){
                                        fy = 0;
                                    }else{
                                        // fy = (float)((y-47)) / (float)ySample;
                                        fy = ((float)((y-lastTile)) - (float)((float)( yy-1)*ySample)) / (float)ySample;
                                    }
                                }else{
                                    fy = (float)((y&15) - (float)((float)( yy-1)*ySample)) / (float)ySample;
                                }
                                


                                
                                
                                
                                int coarse_x1 = coarse_x0+1;
                                int coarse_y1 = coarse_y0+1;
                                int coarse_z1 = coarse_z0+1;



                                
                                // int index = coarse_x0 + (coarse_y0 * 18) + (coarse_z0 * 324);
                                int index = coarse_x1 + (coarse_y1 * 18) + (coarse_z1 * 324);
                                
                                float c000 = work->coarseNoiseTile[coarse_x0 + (coarse_y0 * 18) + (coarse_z0 * 324)];
                                float c100 = work->coarseNoiseTile[coarse_x1 + (coarse_y0 * 18) + (coarse_z0 * 324)];
                                float c010 = work->coarseNoiseTile[coarse_x0 + (coarse_y1 * 18) + (coarse_z0 * 324)];
                                float c110 = work->coarseNoiseTile[coarse_x1 + (coarse_y1 * 18) + (coarse_z0 * 324)];
                                float c001 = work->coarseNoiseTile[coarse_x0 + (coarse_y0 * 18) + (coarse_z1 * 324)];
                                float c101 = work->coarseNoiseTile[coarse_x1 + (coarse_y0 * 18) + (coarse_z1 * 324)];
                                float c011 = work->coarseNoiseTile[coarse_x0 + (coarse_y1 * 18) + (coarse_z1 * 324)];
                                float c111 = work->coarseNoiseTile[coarse_x1 + (coarse_y1 * 18) + (coarse_z1 * 324)];

                                float samplePosX = work->coarseSamplePosX[coarse_x0 + (coarse_y0 * 18) + (coarse_z0 * 324)];
                                float samplePosY = work->coarseSamplePosY[coarse_x0 + (coarse_y0 * 18) + (coarse_z0 * 324)];
                                float samplePosZ = work->coarseSamplePosZ[coarse_x0 + (coarse_y0 * 18) + (coarse_z0 * 324)];
                     
        
                                float c00 = lerp(fx, c000, c100);
                                float c10 = lerp(fx, c010, c110);
                                float c01 = lerp(fx, c001, c101);
                                float c11 = lerp(fx, c011, c111);

                                float c0 = lerp(fy, c00, c10);
                                float c1 = lerp(fy, c01, c11);
                                float finVal = lerp(fz, c0, c1);

                                // float val = work->noiseTile[voxel_index];
                                // val = work->coarseNoiseTile[voxel_index];



                                if(work->demoEnum > 6)val = finVal;
                                // if(work->demoEnum > 6)val = c00;
                                // if(work->demoEnum > 6)val = c000;
                                // if(work->demoEnum > 6)val = c100;
                                // if(work->demoEnum > 6)val = c010;
                                // if(work->demoEnum > 6)val = c110;
                                // if(work->demoEnum > 6)val = c001;
                                // if(work->demoEnum > 6)val = c101;
                                // if(work->demoEnum > 6)val = c011;
                                // if(work->demoEnum > 6)val = c111;


                                // log_file(&chunkData->logger, LogTypes::Info, "%2d %2d %2d : Vox: %4d %4d %4d Coarse: %4d %4d %4d : %5f", 
                                //         coords.x,coords.y,coords.z, x,y,z, coarse_x0, coarse_y0, coarse_z0,
                                //         val);
                                

                                float heightVal = val*64;
                                float colorVal = val*255;
                                float density = (heightVal - (pos.y+surfaceLevel+y));
                                // density = val;
                                uint32_t coarse_index = cx + (cy * 8) + (cz * 64);

                                bool added = false;
                                // work->brickmap_noise[final_voxel_index] = val;
                                work->brickmap_noise[final_voxel_index] = density;
                                work->samplePosX[final_voxel_index] = samplePosX;
                                work->samplePosY[final_voxel_index] = samplePosY;
                                work->samplePosZ[final_voxel_index] = samplePosZ;
                                work->tileX[final_voxel_index] = xTiles;
                                work->tileY[final_voxel_index] = yTiles;
                                work->tileZ[final_voxel_index] = zTiles;
                                
                                work->sampleRegionX[final_voxel_index] = coarse_x0;
                                work->sampleRegionY[final_voxel_index] = coarse_y0;
                                work->sampleRegionZ[final_voxel_index] = coarse_z0;
                                // if(coords.x == 0 && x == 31 && z == 31){
                                //     int debug = 0;
                                // }

                                if(density > 0.0f){
                                    added = true;
                                    #if TERRAIN_FEATURES
                                    //need to figure out how to seed trees/ores in the chunk generation? or do we seed in the second pass?
                                    //lets seed in the second p ass
                                    // float nextDensity = heightVal - (pos.y + surfaceLevel + y + 1.0f); //failed to determine grass blocks
                                    switch(biomeType){
                                        case biome_dirt: {
                                            // if((density) > 0.0f && (density) < 2.0f){//leave this for now and place trees
                                            if(work->isAboveAir[z * pbmr + x]){//leave this for now and place trees
                                            // if((nextDensity) >= 0.0f){
                                                u32 treeSeed = seed + hash3d(coords.x + x + work->xoffset, coords.y + y + work->yoffset, coords.z + z + work->zoffset);
                                                rng_seed(&rng, treeSeed);
                                                float treeVal = rng_next_f32(&rng);

                                                if(treeVal > 0.98f && treeVal < 0.999f){
                                                    work->noiseVoxels[final_voxel_index] = vox_tree; //tree
                                                    u32 indexSeed = (final_voxel_index << 8) | (vox_tree);
                                                    work->seeds[work->seedCount++] = indexSeed;

                                                }else if(treeVal > 0.88f && treeVal < 0.89f){
                                                    work->noiseVoxels[final_voxel_index] = vox_leaves; //leaves/bush
                                                    u32 indexSeed = (final_voxel_index << 8) | (vox_leaves);
                                                    work->seeds[work->seedCount++] = indexSeed;

                                                }else{
                                                    work->noiseVoxels[final_voxel_index] = vox_grass; //grass

                                                }

                                            }else{
                                                work->noiseVoxels[final_voxel_index] = vox_dirt; //voxel type 1 is dirt
                                            }
                                            
                                        }break;
                                        case biome_stone:{
                                            u32 oreSeed = seed + hash3d(coords.x + x + work->xoffset, coords.y + y + work->yoffset, coords.z + z + work->zoffset);
                                            rng_seed(&rng, oreSeed);
                                            float oreVal = rng_next_f32(&rng);
                                            if(oreVal > 0.9f && oreVal < 0.915f){
                                                work->noiseVoxels[final_voxel_index] = vox_tin; //tin 
                                                u32 indexSeed = (final_voxel_index << 8) | (vox_tin);
                                                work->seeds[work->seedCount++] = indexSeed;

                                            }else if(oreVal > 0.7f && oreVal < 0.707f){
                                                work->noiseVoxels[final_voxel_index] = vox_copper; //copper 
                                                u32 indexSeed = (final_voxel_index << 8) | (vox_copper);
                                                work->seeds[work->seedCount++] = indexSeed;

                                            }else{
                                                work->noiseVoxels[final_voxel_index] = vox_stone; //voxel type 2 is stone
                                            }
                                        }break;
                                    }
                                    work->isAboveAir[z * pbmr + x] = 0;
                                    #else
                                    work->noiseVoxels[final_voxel_index] = colorVal;
                                    #endif
                                    //make a second pass that loops over all generated chunks and expands trees/ore placement across chunk bounds
                                    
                                }else{
                                    work->isAboveAir[z * pbmr + x] = 1;
                                }
                                if(added){
                                            
                                    coarse->active_count[coarse_index]++;
                                    voxelsPopulated++;
                                }
                    
                                
                                // when we went bottom up
                                // if(((y+1) % ySample) == 0)yy++;
                                //going top down
                                if(((y) % ySample) == 0)yy--;
                                // yy++;
                            }
                            if(((x+1) % xSample) == 0)xx++;
                            // if((x % xSample) == 0)xx++;
                            // xx++;
                        }
                        if(((z+1) % zSample) == 0)zz++;
                        // if((z % zSample) == 0)zz++;
                        // zz++;
                    }

                    #endif


                }//end yTile loop

            }//end xTile loop


        }//end zTile loop

        #if TERRAIN_FEATURES
        if(work->demoEnum > 6){
        //seed expansion pass
        for(int i = 0; i < work->seedCount; i++){
            u32 indexSeed = work->seeds[i];
            u32 voxIndex = indexSeed >> 8;
            u8 type = indexSeed & 0xFF;
            s32 y = (voxIndex / 64) % 64;
            s32 x = voxIndex % 64;
            s32 z = voxIndex / 4096;
            VoxPopResults result = voxPop_none;

            uint32_t cx = x / 8;
            uint32_t cy = y / 8;
            uint32_t cz = z / 8;
            uint32_t coarse_index = cx + (cy * 8) + (cz * 64);


            u32 rngseed = seed + hash3d(coords.x + x + work->xoffset, coords.y + y + work->yoffset, coords.z + z + work->zoffset);
            rng_seed(&rng, rngseed);
            u32 randVal = rng_next_u32(&rng);
            u32 dir = randVal % 6;

            //ignore edge seeds for now
            if((x == 0 || x >= VOX_MAX_INDEX) || (y == 0 || y >= VOX_MAX_INDEX) ||(z == 0 || z >= VOX_MAX_INDEX))continue; 

            switch(type){
                case vox_none:{

                }break;
                case vox_dirt:{

                }break;
                case vox_stone:{

                }break;
                case vox_grass:{

                }break;
                case vox_tree:{

                    //better random trees
                    #if 1
                        float branchDirs[3] = {(float)(rng_next_f32(&rng) * 2 * PI), (float)(rng_next_f32(&rng) * 2 * PI), (float)(rng_next_f32(&rng) * 2 * PI)};
                        //0 degrees from +x going counter clockwise
                        int height = (randVal & 7) + 5;
                        int heightLimit = height + 1;
                        int branchCount = ((height >= 8) * 2) + ((height < 8));//either 2 branches or 1 branch
                        float radius = (float)height * 0.664f;
                        float maxDist = radius;
                        float minDist = maxDist * 0.247f;
                        int branchStart = height * 0.3f;
                        float branchSlope = 0.381f;
                        float branchDists[3] = {(minDist + (rng_next_f32(&rng)) * (maxDist - minDist)), (minDist + (rng_next_f32(&rng)) * (maxDist - minDist)), (minDist + (rng_next_f32(&rng)) * (maxDist - minDist))};
                        int startX0 = x;
                        int startX1 = x;
                        int startX2 = x;
                        int startY0 = y + branchStart;
                        int startY1 = y + branchStart;
                        int startY2 = y + branchStart;
                        int startZ0 = z;
                        int startZ1 = z;
                        int startZ2 = z;
                        int endX0 = x + (int)(cosf(branchDirs[0]) * branchDists[0]);
                        int endX1 = x + (int)(cosf(branchDirs[1]) * branchDists[1]);
                        int endX2 = x + (int)(cosf(branchDirs[2]) * branchDists[2]);
                        int endY0 = y + height;
                        int endY1 = y + height;
                        int endY2 = y + height;
                        int endZ0 = z + (int)(sinf(branchDirs[0]) * branchDists[0]);
                        int endZ1 = z + (int)(sinf(branchDirs[1]) * branchDists[1]);
                        int endZ2 = z + (int)(sinf(branchDirs[2]) * branchDists[2]);

                        for(int j = 0; j < height; j++){//generate tree trunk
                            if(result == voxPop_chunkBoundary)continue;
                            int yj = y+j;
                            result = placeTerrainFeatureVoxel(work, vox_tree, x, yj, z, voxelsPopulated, coarse);
                            if(result == voxPop_chunkBoundary){
                                AppendMetaChunkInfo(chunkData, work->chunkID, voxMeta_expandSeed, vox_tree, voxIndex, rngseed);
                            }
                        //         if(j > (branchStart)){//place branch blocks
                        //             int horizontalDisplacement = (j - branchStart) * branchSlope;
                        //             for(int k = 0; k < branchCount; k++){

                        //             }
                        //             voxIndex = x + (yj * pbmr) + (z*pbmr*pbmr);
                        //             if(!work->noiseVoxels[voxIndex]){//if no voxel exists here

                        //             }
                        //         }
                        }
                        //branch generation
                        for(float step = 0.0f; step < 1.0f; step += 0.1f){
                            if(result == voxPop_chunkBoundary)continue;

                            int x0 = startX0 + (endX0 - startX0)*step; 
                            int x1 = startX1 + (endX1 - startX1)*step; 
                            int x2 = startX2 + (endX2 - startX2)*step; 
                            int y0 = startY0 + (endY0 - startY0)*step; 
                            int y1 = startY1 + (endY1 - startY1)*step; 
                            int y2 = startY2 + (endY2 - startY2)*step; 
                            int z0 = startZ0 + (endZ0 - startZ0)*step; 
                            int z1 = startZ1 + (endZ1 - startZ1)*step; 
                            int z2 = startZ2 + (endZ2 - startZ2)*step; 

                            result = placeTerrainFeatureVoxel(work, vox_tree, x0, y0, z0, voxelsPopulated, coarse);
                            if(result == voxPop_chunkBoundary){
                                AppendMetaChunkInfo(chunkData, work->chunkID, voxMeta_expandSeed, vox_tree, voxIndex, rngseed);
                                break;
                            }
                            if(branchCount > 1){
                                result = placeTerrainFeatureVoxel(work, vox_tree, x1, y1, z1, voxelsPopulated, coarse);
                                if(result == voxPop_chunkBoundary){
                                    AppendMetaChunkInfo(chunkData, work->chunkID, voxMeta_expandSeed, vox_tree, voxIndex, rngseed);
                                    break;
                                }
                            }

                        }
                        //leaf generation
                        //sphere around endpoint
                        int leafDistanceLimit = 2;
                        for(int dx = -leafDistanceLimit; dx <= leafDistanceLimit; dx++){
                            if(result == voxPop_chunkBoundary)continue;

                            for(int dy = -leafDistanceLimit; dy <= leafDistanceLimit; dy++){
                                if(result == voxPop_chunkBoundary)continue;

                                for(int dz = -leafDistanceLimit; dz <= leafDistanceLimit; dz++){
                                    if(result == voxPop_chunkBoundary)continue;
    
                                    //check if point is within sphere
                                    float distSq = dx * dx + dy * dy + dz * dz;
                                    if(distSq <= leafDistanceLimit * leafDistanceLimit){
                                        int leafZ0 = endZ0 + dz;
                                        int leafY0 = endY0 + dy;
                                        int leafX0 = endX0 + dx;
                                        int leafZ1 = endZ1 + dz;
                                        int leafY1 = endY1 + dy;
                                        int leafX1 = endX1 + dx;
                                        int leafZCenter = z + dz;
                                        int leafYCenter = (y+(height-1)) + dy;
                                        int leafXCenter = x + dx;

                                        result = placeTerrainFeatureVoxel(work, vox_leaves, leafXCenter, leafYCenter, leafZCenter, voxelsPopulated, coarse);
                                        if(result == voxPop_chunkBoundary){
                                            AppendMetaChunkInfo(chunkData, work->chunkID, voxMeta_expandSeed, vox_tree, voxIndex, rngseed);
                                            break;
                                        }
                                        result = placeTerrainFeatureVoxel(work, vox_leaves, leafX0, leafY0, leafZ0, voxelsPopulated, coarse);
                                        if(result == voxPop_chunkBoundary){
                                            AppendMetaChunkInfo(chunkData, work->chunkID, voxMeta_expandSeed, vox_tree, voxIndex, rngseed);
                                            break;
                                        }
                                        if(branchCount > 1){
                                            result = placeTerrainFeatureVoxel(work, vox_leaves, leafX1, leafY1, leafZ1, voxelsPopulated, coarse);
                                            if(result == voxPop_chunkBoundary){
                                                AppendMetaChunkInfo(chunkData, work->chunkID, voxMeta_expandSeed, vox_tree, voxIndex, rngseed);
                                                break;
                                            }
                                        }

                                    }
                                }
                            }
                        }
                        // for(int yLayer = height; i >= branchStart; yLayer--){
                        //     for(int k = 0; k < branchCount; k++){
                        //         //generate random branch endpoint
                        //         float angle = branchDirs[k];
                        //         float dist = branchDists[k];

                        //         int endX = x + (int)(cosf(angle) * dist);
                        //         int endZ = z + (int)(sinf(angle) * dist);
                        //         int endY = yLayer;

                        //         //find where this branch connects to the trunk using branchSlope
                        //         int trunkConnectionY = endY - (int)(dist / branchSlope);


                        //     }
                        // }
                    #else
                    //shitty static tree
                    for(int j = 0; j < 8; j++){
                        int yj = y+j;
                        if(yj <= CHUNK_SIZE){
                            work->noiseVoxels[x + (yj * pbmr) + (z*pbmr*pbmr)] = vox_tree; 
                            coarse_index = (x/8) + ((yj/8) * 8) + ((z/8) * 64);
                            coarse.active_count[coarse_index]++;
                            voxelsPopulated++;
                            if(j > 4){//place leaves all around the tree at this level
                                if(j == 6){
                                    if(((x-1) >= 0) && ((x-1) < VOX_MAX_INDEX) && ((z-1) >= 0) && ((z-1) < VOX_MAX_INDEX)){ work->noiseVoxels[(x-1) + (yj * pbmr) + ((z-1)*pbmr*pbmr)] = vox_leaves; coarse_index = ((x-1)/8) + ((yj/8) * 8) + (((z-1)/8) * 64); coarse.active_count[coarse_index]++; voxelsPopulated++;}
                                    if(((x+0) >= 0) && ((x+0) < VOX_MAX_INDEX) && ((z-1) >= 0) && ((z-1) < VOX_MAX_INDEX)){ work->noiseVoxels[(x+0) + (yj * pbmr) + ((z-1)*pbmr*pbmr)] = vox_leaves; coarse_index = ((x+0)/8) + ((yj/8) * 8) + (((z-1)/8) * 64); coarse.active_count[coarse_index]++; voxelsPopulated++; }
                                    if(((x+1) >= 0) && ((x+1) < VOX_MAX_INDEX) && ((z-1) >= 0) && ((z-1) < VOX_MAX_INDEX)){ work->noiseVoxels[(x+1) + (yj * pbmr) + ((z-1)*pbmr*pbmr)] = vox_leaves; coarse_index = ((x+1)/8) + ((yj/8) * 8) + (((z-1)/8) * 64); coarse.active_count[coarse_index]++; voxelsPopulated++; }
                                    if(((x-1) >= 0) && ((x-1) < VOX_MAX_INDEX) && ((z+0) >= 0) && ((z+0) < VOX_MAX_INDEX)){ work->noiseVoxels[(x-1) + (yj * pbmr) + ((z+0)*pbmr*pbmr)] = vox_leaves; coarse_index = ((x-1)/8) + ((yj/8) * 8) + (((z+0)/8) * 64); coarse.active_count[coarse_index]++; voxelsPopulated++; }
                                    if(((x+1) >= 0) && ((x+1) < VOX_MAX_INDEX) && ((z+0) >= 0) && ((z+0) < VOX_MAX_INDEX)){ work->noiseVoxels[(x+1) + (yj * pbmr) + ((z+0)*pbmr*pbmr)] = vox_leaves; coarse_index = ((x+1)/8) + ((yj/8) * 8) + (((z+0)/8) * 64); coarse.active_count[coarse_index]++; voxelsPopulated++; }
                                    if(((x-1) >= 0) && ((x-1) < VOX_MAX_INDEX) && ((z+1) >= 0) && ((z+1) < VOX_MAX_INDEX)){ work->noiseVoxels[(x-1) + (yj * pbmr) + ((z+1)*pbmr*pbmr)] = vox_leaves; coarse_index = ((x-1)/8) + ((yj/8) * 8) + (((z+1)/8) * 64); coarse.active_count[coarse_index]++; voxelsPopulated++; }
                                    if(((x+0) >= 0) && ((x+0) < VOX_MAX_INDEX) && ((z+1) >= 0) && ((z+1) < VOX_MAX_INDEX)){ work->noiseVoxels[(x+0) + (yj * pbmr) + ((z+1)*pbmr*pbmr)] = vox_leaves; coarse_index = ((x+0)/8) + ((yj/8) * 8) + (((z+1)/8) * 64); coarse.active_count[coarse_index]++; voxelsPopulated++; }
                                    if(((x+1) >= 0) && ((x+1) < VOX_MAX_INDEX) && ((z+1) >= 0) && ((z+1) < VOX_MAX_INDEX)){ work->noiseVoxels[(x+1) + (yj * pbmr) + ((z+1)*pbmr*pbmr)] = vox_leaves; coarse_index = ((x+1)/8) + ((yj/8) * 8) + (((z+1)/8) * 64); coarse.active_count[coarse_index]++; voxelsPopulated++; }
                                }else if(j == 7){
                                    if(((x+0) >= 0) && ((x+0) < VOX_MAX_INDEX) && ((z-1) >= 0) && ((z-1) < VOX_MAX_INDEX)){ work->noiseVoxels[(x+0) + (yj * pbmr) + ((z-1)*pbmr*pbmr)] = vox_leaves; coarse_index = ((x+0)/8) + ((yj/8) * 8) + (((z-1)/8) * 64);coarse.active_count[coarse_index]++; voxelsPopulated++;}
                                    if(((x-1) >= 0) && ((x-1) < VOX_MAX_INDEX) && ((z+0) >= 0) && ((z+0) < VOX_MAX_INDEX)){ work->noiseVoxels[(x-1) + (yj * pbmr) + ((z+0)*pbmr*pbmr)] = vox_leaves; coarse_index = ((x-1)/8) + ((yj/8) * 8) + (((z+0)/8) * 64);coarse.active_count[coarse_index]++; voxelsPopulated++;}
                                    if(((x+0) >= 0) && ((x+0) < VOX_MAX_INDEX) && ((z+0) >= 0) && ((z+0) < VOX_MAX_INDEX)){ work->noiseVoxels[(x+0) + (yj * pbmr) + ((z+0)*pbmr*pbmr)] = vox_leaves; }//already populated the coarse index}
                                    if(((x+1) >= 0) && ((x+1) < VOX_MAX_INDEX) && ((z+0) >= 0) && ((z+0) < VOX_MAX_INDEX)){ work->noiseVoxels[(x+1) + (yj * pbmr) + ((z+0)*pbmr*pbmr)] = vox_leaves; coarse_index = ((x+1)/8) + ((yj/8) * 8) + (((z+0)/8) * 64);coarse.active_count[coarse_index]++; voxelsPopulated++;}
                                    if(((x+0) >= 0) && ((x+0) < VOX_MAX_INDEX) && ((z+1) >= 0) && ((z+1) < VOX_MAX_INDEX)){ work->noiseVoxels[(x+0) + (yj * pbmr) + ((z+1)*pbmr*pbmr)] = vox_leaves; coarse_index = ((x+0)/8) + ((yj/8) * 8) + (((z+1)/8) * 64);coarse.active_count[coarse_index]++; voxelsPopulated++;}
                                }else{
                                    if(((x+0) >= 0) && ((x+0) < VOX_MAX_INDEX) && ((z-1) >= 0) && ((z-1) < VOX_MAX_INDEX)){ work->noiseVoxels[(x+0) + (yj * pbmr) + ((z-1)*pbmr*pbmr)] = vox_leaves; coarse_index = ((x+0)/8) + ((yj/8) * 8) + (((z-1)/8) * 64);coarse.active_count[coarse_index]++; voxelsPopulated++;}
                                    if(((x-1) >= 0) && ((x-1) < VOX_MAX_INDEX) && ((z+0) >= 0) && ((z+0) < VOX_MAX_INDEX)){ work->noiseVoxels[(x-1) + (yj * pbmr) + ((z+0)*pbmr*pbmr)] = vox_leaves; coarse_index = ((x-1)/8) + ((yj/8) * 8) + (((z+0)/8) * 64);coarse.active_count[coarse_index]++; voxelsPopulated++;}
                                    if(((x+1) >= 0) && ((x+1) < VOX_MAX_INDEX) && ((z+0) >= 0) && ((z+0) < VOX_MAX_INDEX)){ work->noiseVoxels[(x+1) + (yj * pbmr) + ((z+0)*pbmr*pbmr)] = vox_leaves; coarse_index = ((x+1)/8) + ((yj/8) * 8) + (((z+0)/8) * 64);coarse.active_count[coarse_index]++; voxelsPopulated++;}
                                    if(((x+0) >= 0) && ((x+0) < VOX_MAX_INDEX) && ((z+1) >= 0) && ((z+1) < VOX_MAX_INDEX)){ work->noiseVoxels[(x+0) + (yj * pbmr) + ((z+1)*pbmr*pbmr)] = vox_leaves; coarse_index = ((x+0)/8) + ((yj/8) * 8) + (((z+1)/8) * 64);coarse.active_count[coarse_index]++; voxelsPopulated++;}
                                }

                            }
                        }
                    }
                    #endif

                }break;
                
                #if 1
                case vox_leaves:
                case vox_tin:
                case vox_copper:{
                    for(int j = 0; j < 4; j++){
                        // dir = rng_next_u32(&rng) % 6;
                        s32 dirs[6]={x+j, x-j, y+j, y-j, z+j, z-j};
                        s32 varIndex[6] = {
                            (x+j) + ((y  ) * pbmr) + ((z  )*pbmr*pbmr),
                            (x-j) + ((y  ) * pbmr) + ((z  )*pbmr*pbmr),
                            (x  ) + ((y+j) * pbmr) + ((z  )*pbmr*pbmr),
                            (x  ) + ((y-j) * pbmr) + ((z  )*pbmr*pbmr),
                            (x  ) + ((y  ) * pbmr) + ((z+j)*pbmr*pbmr),
                            (x  ) + ((y  ) * pbmr) + ((z-j)*pbmr*pbmr),
                        };
                        s32 coarseVarIndex[6] = {
                            ((x+j)/8) + (((y  )/8)*8) + (((z  )/8)*64),
                            ((x-j)/8) + (((y  )/8)*8) + (((z  )/8)*64),
                            ((x  )/8) + (((y+j)/8)*8) + (((z  )/8)*64),
                            ((x  )/8) + (((y-j)/8)*8) + (((z  )/8)*64),
                            ((x  )/8) + (((y  )/8)*8) + (((z+j)/8)*64),
                            ((x  )/8) + (((y  )/8)*8) + (((z-j)/8)*64),
                        };
                        if(dirs[dir] < 0 || dirs[dir] >= VOX_MAX_INDEX)continue;
                        Assert(dirs[dir] >= 0);
                        Assert(dirs[dir] < VOX_MAX_INDEX);
                        if(!work->noiseVoxels[varIndex[dir]]){
                            work->noiseVoxels[varIndex[dir]] = type; 
                            coarse->active_count[coarseVarIndex[dir]]++;
                            voxelsPopulated++;

                        }

                    }

                }break;
                #endif
                case 8:{

                }break;

                default:{}break;
            }
            

        }
        #endif
        }




        
      
        if(voxelsPopulated){
            size_t data_size = pbmr3 * sizeof(uint8_t);
            brickmap->active_count = voxelsPopulated;
            memcpy(brickmap->voxels,work->noiseVoxels , data_size);
        }
        u64 end = __rdtsc();
        u64 totalCycles = end - startTime;
        work->perlinTotal += totalCycles;
        work->accumulateVoxelsTotalCycles = __rdtsc() - end;
        work->accumulateVoxelsHits++;
        // PRINT("TIMING: %zu\n", __rdtsc() - end);

        if(!voxelsPopulated){
            LOG(Info,"worker %d chunkID: %u no voxels to mesh", work->workerID, work->chunkID);

            return;
        }
        //////spdLOG(Info,"worker {} LOW meshing voxels", work->workerID);


        // vertex_pull_mesh_variable(chunkData->brickmaps[work->chunkID].voxels, work, true, uvec3_create(64), work->faceMemory, chunkData->face_count[work->chunkID], FACE_MAX);
        work->totalCycles = __rdtsc() - startTime;
        LOG(Info, "chunkID: %u faceCount: %u\n", work->chunkID, chunkData->face_count[work->chunkID]);
        // if(chunkData->face_count[work->chunkID] == 0){
            // __debugbreak();
        // }
    }



    void createMeshWork(chunk_data* chunkData, voxel_work* work){
        vertex_pull_mesh_variable(chunkData->brickmaps[work->chunkID].voxels, work, true, uvec3_create(64), work->chunkMeshMemory.faceMemory, work->chunkMeshMemory.faceCount, FACE_MAX);


    }




    void meshWork(voxel_work* work){
        chunk_data* chunkData = work->chunkData;
        work->chunkIDBackup = work->chunkID;
        #if ENABLE_MESH_WORK
            if(work->meshNeedsUploading)__debugbreak();
            work->chunkMeshMemory.faceCount = 0;
            createMeshWork(chunkData, work);
            work->meshNeedsUploading = true;
            PRINT("[THREAD] completed mesh for chunkID: %u\n", work->chunkID);
        #endif
        Assert(work->chunkIDBackup == work->chunkID);
        AtomicExchangeU32(chunkData->volChunkStages + work->chunkID, chunkStage_meshed);
        u32 desired = 0;
        u32 expected = 1;
        u32 previous = AtomicCompareExchange(chunkData->workerThreadOccupations + work->threadIndex, desired, expected);
        Assert(previous == expected);
    }

    void voxelWork(voxel_work* work){
        chunk_data* chunkData = work->chunkData;
        work->chunkIDBackup = work->chunkID;
        #if ENABLE_VOXEL_WORK
            PRINT("[THREAD] START VOXELIZATION for chunkID: %u\n", work->chunkID);
            if(work->meshNeedsUploading)__debugbreak();
            work->chunkMeshMemory.faceCount = 0;
            do_voxel_work(chunkData, work);
            PRINT("[THREAD] completed voxelization for chunkID: %u\n", work->chunkID);
        #endif
        Assert(work->chunkIDBackup == work->chunkID);
        AtomicExchangeU32(chunkData->volChunkStages + work->chunkID, chunkStage_firstPassComplete);
        u32 desired = 0;
        u32 expected = 1;
        u32 previous = AtomicCompareExchange(chunkData->workerThreadOccupations + work->threadIndex, desired, expected);
        Assert(previous == expected);
    }

    void PushChunkMeshToRenderer(game_state* GameState, chunk_data* chunkData, voxel_work* work){
                work->meshNeedsUploading = false;
                chunkData->activeThreadCount--;
                chunkData->safeToEdit[work->chunkID] = true;

                ChunkVisibility chunkVisibility = (ChunkVisibility)AtomicRead(chunkData->volChunkVisibilities + work->chunkID);
                if(chunkVisibility < chunkVisibility_skirt)return;
                
                u32 desired = chunkStage_uploaded;
                u32 expected = chunkStage_meshed;
                u32 previous = AtomicCompareExchange(chunkData->volChunkStages + work->chunkID, desired, expected);
                if(previous != expected){
                    return;//chunk has probably been deleted
                }
                chunkData->testDebugSSBOCount--;

                #if DISABLE_MESHING
                    return;
                #endif
                
                PRINT("chunkID: %u uploading mesh, estimated ssbo count after upload: %u\n", work->chunkID, chunkData->testDebugSSBOCount);
                Assert(chunkData->testDebugSSBOCount <= 27);
                chunkData->face_count[work->chunkID] = work->chunkMeshMemory.faceCount;
                
                //submits a command to copy the mesh into the gpu, and maintain a mapping of that chunkID to that ssbo slot
                //copy the mesh memory over
                ChunkMeshMemory* meshMemory = chunkData->stableChunkMeshMemory + chunkData->stableChunkMeshMemoryCount++;
                memcpy(meshMemory->faceMemory,  work->chunkMeshMemory.faceMemory, sizeof(FaceData) * work->chunkMeshMemory.faceCount);

                u32& commandCount = GameState->RenderCommandData->chunkCreateCommandCount;
                chunk_create_command newCommand = {};
                newCommand.chunkID = work->chunkID;
                ivec3 coords = chunkData->coords[work->chunkID];
                newCommand.faceMemory = meshMemory->faceMemory; //safe to pass a pointer only as long as the current pipeline is unchanged, the pointer only needs to last for this frame
                newCommand.faceCount = work->chunkMeshMemory.faceCount;
                newCommand.edit = false;

                // if((coords.y < 0 || coords.y > 0) && (newCommand.faceCount != 0)){
                //     __debugbreak();
                //     // Assert(!"MESH ERROR! CHUNK HAS FACES WHERE IT SHOULDNT!");
                // }
                if((coords.y == 0) && (newCommand.faceCount == 0)){
                    __debugbreak();
                    
                    // Assert(!"MESH ERROR! CHUNK HAS NO FACES!");
                }
                
                // ivec3 coords = chunkData->coords[work.chunkID];
                // PRINT("PUSHING MESH COMMAND FOR CHUNK COORDS: %d %d %d\n", coords.x, coords.y, coords.z);
                GameState->RenderCommandData->chunkCreateCommands[commandCount++] = newCommand; 
                // PRINT("Appending chunk edit command, faceIndex: %u\n", work.faceMemoryIndex);
                
                //::IMPORTANT::
                //face memory is released from the vulkan side
    }

    void completeVoxelFirstPassWork(game_state* GameState, chunk_data* chunkData){
        for(int i = 0; i < MAX_WORKER_THREADS; i++){
            voxel_work& work = chunkData->workerThreads[i];
            if(!work.hasValidWork)continue;
            // LOG(Warn,"Thread: %d processing chunkID: %d\n", i, work.chunkID);
            if(work.chunkID > MAX_CHUNKS){
                // COLLISION_DEBUG_PRINTF("work.chunkID: %d\n", work.chunkID);
            }
            work.hasValidWork = false;
        }

    }


    void completeSecondPass(game_state* GameState, chunk_data* chunkData){
        // LOG(Debug, "COMPLETE MESH WORK!");
        uint8_t cameraIndex = GameState->entityComponent->entityToCameraMap[GameState->localPlayerEntityIDs[0]];
        CameraComp& cameraComp = GameState->entityComponent->CameraComps[cameraIndex];
        uvec3 drawDistance = chunkData->chunkDrawDistance; 
        int32_t centerX = drawDistance.x / 2;
        int32_t centerY = drawDistance.y / 2;
        int32_t centerZ = drawDistance.z / 2;
    
        for(int i = 0; i < MAX_WORKER_THREADS; i++){
            voxel_work& work = chunkData->workerThreads[i];
            if(!work.hasValidWork)continue;
            // LOG(Warn,"Thread: %d processing chunkID: %d\n", i, work.chunkID);
            if(work.chunkID > MAX_CHUNKS){
                // COLLISION_DEBUG_PRINTF("work.chunkID: %d\n", work.chunkID);
            }
            work.hasValidWork = false;


            ivec3 coords = chunkData->coords[work.chunkID];
            chunkData->safeToEdit[work.chunkID] = true;
            
            //check if chunk is still in the game world/still intersects camera, if not, destroy it
            if(checkChunkDestructionRules(GameState, cameraComp, coords, drawDistance, centerX, centerY, centerZ)){
                //chunk outside of camera grid, destroy/clear/free or whatever we do
                Assert(chunkData->safeToEdit[work.chunkID]);
                    //it will get destroyed when the multithreading task is finished
                
                LOG(Warn, "Destroy chunk in complete mesh work!");
                destroyChunk(GameState, chunkData, work.chunkID);
                continue;
            }

            SetChunkStage(chunkData, work.chunkID, ChunkStage::chunkStage_meshing);

            //technically the chunk has been meshed, even though it has no visible faces
        
            // if(chunkData->face_count[work.chunkID] > 0){
            PushChunkMeshToRenderer(GameState, chunkData, &work);



        

        }
        chunkData->hasVoxWorkQueued = false;
        chunkData->hasMeshWorkQueued = false;
        chunkData->activeThreadCount = 0;
    }


    void completeMeshWork(game_state* GameState, chunk_data* chunkData){
        // LOG(Debug, "COMPLETE MESH WORK!");
        uint8_t cameraIndex = GameState->entityComponent->entityToCameraMap[GameState->localPlayerEntityIDs[0]];
        CameraComp& cameraComp = GameState->entityComponent->CameraComps[cameraIndex];
        uvec3 drawDistance = chunkData->chunkDrawDistance; 
        int32_t centerX = drawDistance.x / 2;
        int32_t centerY = drawDistance.y / 2;
        int32_t centerZ = drawDistance.z / 2;

        chunkData->perlinThreadTotal = 0;
        chunkData->perlinThreadCycles = 0;
        chunkData->perlinThreadHits = 0;

        chunkData->workThreadTotalCycles = 0;
        chunkData->workThreadCycles = 0;
        chunkData->workThreadHits = 0;


        chunkData->workThreadAccumulateVoxelsTotalCycles = 0;
        chunkData->workThreadAccumulateVoxelsHits = 0;
        chunkData->workThreadAccumulateVoxelsCycles = 0;


        for(int i = 0; i < MAX_WORKER_THREADS; i++){
            voxel_work& work = chunkData->workerThreads[i];
            if(!work.hasValidWork)continue;
            work.hasValidWork = false;

            chunkData->perlinThreadTotal += work.perlinTotal;
            chunkData->perlinThreadHits += work.perlinHits;
            chunkData->workThreadTotalCycles += work.totalCycles;
            chunkData->workThreadHits++;
            chunkData->workThreadAccumulateVoxelsHits++;
            chunkData->workThreadAccumulateVoxelsTotalCycles+= work.accumulateVoxelsTotalCycles;

    

            ivec3 coords = chunkData->coords[work.chunkID];
            chunkData->safeToEdit[work.chunkID] = true;
            
            //check if chunk is still in the game world/still intersects camera, if not, destroy it
            if(checkChunkDestructionRules(GameState, cameraComp, coords, drawDistance, centerX, centerY, centerZ)){
                //chunk outside of camera grid, destroy/clear/free or whatever we do
                Assert(chunkData->safeToEdit[work.chunkID]);
                    //it will get destroyed when the multithreading task is finished
                
                LOG(Warn, "Destroy chunk in complete mesh work!");
                destroyChunk(GameState, chunkData, work.chunkID);
                //release face memory index
                // freeFaceMemory(GameState, work.faceMemoryIndex);
                continue;
            }

            //append chunk to the secondStage array
            SetChunkStage(chunkData, work.chunkID, ChunkStage::chunkStage_secondPass);

            //technically the chunk has been meshed, even though it has no visible faces
        
            // if(chunkData->face_count[work.chunkID] > 0){
                // PushChunkMeshToRenderer(GameState, chunkData, &work);




                //total debug slop to let me inspect noise/voxel values
                memcpy(chunkData->chunkVoxels + work.chunkID, work.noiseVoxels, pbmr3*sizeof(u8));
                memcpy(chunkData->chunkNoise  + work.chunkID, work.brickmap_noise, pbmr3*sizeof(float));
                memcpy(chunkData->samplePosX  + (work.chunkID & (DEBUG_CHUNKS-1)), work.samplePosX, pbmr3*sizeof(float));
                memcpy(chunkData->samplePosY  + (work.chunkID & (DEBUG_CHUNKS-1)), work.samplePosY, pbmr3*sizeof(float));
                memcpy(chunkData->samplePosZ  + (work.chunkID & (DEBUG_CHUNKS-1)), work.samplePosZ, pbmr3*sizeof(float));
                memcpy(chunkData->tileX  + (work.chunkID & (DEBUG_CHUNKS-1)), work.tileX, pbmr3*sizeof(u8));
                memcpy(chunkData->tileY  + (work.chunkID & (DEBUG_CHUNKS-1)), work.tileY, pbmr3*sizeof(u8));
                memcpy(chunkData->tileZ  + (work.chunkID & (DEBUG_CHUNKS-1)), work.tileZ, pbmr3*sizeof(u8));

                memcpy(chunkData->sampleRegionX  + (work.chunkID & (DEBUG_CHUNKS-1)), work.sampleRegionX, pbmr3*sizeof(u8));
                memcpy(chunkData->sampleRegionY  + (work.chunkID & (DEBUG_CHUNKS-1)), work.sampleRegionY, pbmr3*sizeof(u8));
                memcpy(chunkData->sampleRegionZ  + (work.chunkID & (DEBUG_CHUNKS-1)), work.sampleRegionZ, pbmr3*sizeof(u8));
                

            // }else{
                // PRINT("NO FACES! NOT APPENDING COMMAND!\n");
                // freeFaceMemory(GameState, work.faceMemoryIndex);
// 
            // }
        

        }
        chunkData->hasVoxWorkQueued = false;
        chunkData->activeThreadCount = 0;
    }

/////////////////////////////////////////////////// WORKER THREADS END //////////////////////////////////////
 
   

    static PLATFORM_WORK_QUEUE_CALLBACK(DoVoxelWork)
    {
        voxel_work* work = (voxel_work *)Data;
        
        voxelWork(work);
    }


    static PLATFORM_WORK_QUEUE_CALLBACK(DoMeshWork)
    {
        voxel_work* work = (voxel_work *)Data;
        chunk_data* chunkData = work->chunkData;
        meshWork(work);
        
    }


    void setupVoxelizeTask(game_state* GameState, chunk_data* chunkData, CameraComp& cameraComp, uint32_t chunkID, int LODTypeOverride = 0){
        TIMED_BLOCK("Setup Voxelize Task");
        fpt_vec3 cameraPos = cameraComp.pos_in_chunk;
        ivec3 coords = chunkData->coords[chunkID];
        fpt_vec3 chunkPos = ivec_to_fpt_vec3(coords * CHUNK_SIZE);


        int LODType = 0;
        if(LODTypeOverride != 0){
            LODType = LODTypeOverride;
        } 
        else{
            LODType = manhattanDistance(chunkData, chunkPos);
        }

        // uint32_t threadIndex = chunkData->threadToAssign;
        uint32_t threadIndex = 0;
        bool foundFreeThread = false;
        for(int i = 0; i < MAX_WORKER_THREADS; i++){
            u32 desired = 1;
            u32 expected = 0;
            u32 previous = AtomicCompareExchange(chunkData->workerThreadOccupations + i, desired, expected);
            if(previous == expected){
                threadIndex = i;
                foundFreeThread = true;
                break;
            }

        }
        Assert(foundFreeThread);


        // LOG(Warn, "assigning Thread: %d with chunkID: %d", threadIndex, chunkID);
        voxel_work* work = &chunkData->workerThreads[threadIndex];
        work->chunkData = chunkData;
        if(work->meshNeedsUploading)PushChunkMeshToRenderer(GameState, chunkData, work);
        
        Assert(chunkData->safeToEdit[chunkID]);
        chunkData->safeToEdit[chunkID] = false;
        work->hasValidWork = true;
        work->chunkID = chunkID;
        work->LODType = LODType;


        //perlin noise parameters, need to copy them due to multithreading
        work->xoffset = GameState->tiledOffsetX;
        work->yoffset = GameState->tiledOffsetY;
        work->zoffset = GameState->tiledOffsetZ;
        work->scaleFactor = GameState->scaleFactor;
        work->demoEnum = GameState->perlinDemoEnum;
        
        // chunkData->threadToAssign++;

        // if(chunkData->threadToAssign >= MAX_WORKER_THREADS){
        //         chunkData->threadToAssign = 0;
        // }
        chunkData->activeThreadCount++;

        chunkData->chunkWorkers[chunkID] = threadIndex;

        PRINT("submitting chunkID: %u | %d %d %d for voxelization\n", work->chunkID, coords.x,coords.y,coords.z);
        #if CHUNK_SINGLE_THREADING //single threaded path
        chunkData->hasVoxWorkQueued = true;
        voxelWork(work);
        #else

        
        // #if 0 //multithreaded path
        chunkData->hasVoxWorkQueued = true;
        chunkData->PlatformAddEntry(chunkData->Queue, DoVoxelWork, work);
        #endif

        int fuckTheDebugger = 0;
    }

    void setupMeshTask(game_state* GameState, chunk_data* chunkData, CameraComp& cameraComp, uint32_t chunkID){


        // uint32_t threadIndex = chunkData->threadToAssign;
        uint32_t threadIndex = 0;
        bool foundFreeThread = false;
        for(int i = 0; i < MAX_WORKER_THREADS; i++){
            u32 desired = 1;
            u32 expected = 0;
            u32 previous = AtomicCompareExchange(chunkData->workerThreadOccupations + i, desired, expected);
            if(previous == expected){
                threadIndex = i;
                foundFreeThread = true;
                break;
            }

        }
        Assert(foundFreeThread);

        // LOG(Warn, "assigning Thread: %d with chunkID: %d", threadIndex, chunkID);
        voxel_work* work = &chunkData->workerThreads[threadIndex];
        work->chunkData = chunkData;
        if(work->meshNeedsUploading)PushChunkMeshToRenderer(GameState, chunkData, work);
        
        ivec3 coords = chunkData->coords[chunkID];
        
        Assert(chunkData->safeToEdit[chunkID]);
        chunkData->safeToEdit[chunkID] = false;
        work->hasValidWork = true;
        work->chunkID = chunkID;
        work->LODType = 3;


        //perlin noise parameters, need to copy them due to multithreading
        work->xoffset = GameState->tiledOffsetX;
        work->yoffset = GameState->tiledOffsetY;
        work->zoffset = GameState->tiledOffsetZ;
        work->scaleFactor = GameState->scaleFactor;
        work->demoEnum = GameState->perlinDemoEnum;
        
        

        // chunkData->threadToAssign++;

        // if(chunkData->threadToAssign >= MAX_WORKER_THREADS){
        //         chunkData->threadToAssign = 0;
        // }

        chunkData->activeThreadCount++;

        GameState->secondGenState = true;
        chunkData->chunkWorkers[chunkID] = threadIndex;
        PRINT("submitting chunkID: %u | %d %d %d for meshing\n", work->chunkID, coords.x,coords.y,coords.z);

        #if CHUNK_SINGLE_THREADING //single threaded path
        chunkData->hasMeshWorkQueued = true;
        meshWork(work);
        #else

        // #if 0 //multithreaded path
        chunkData->hasMeshWorkQueued = true;
        chunkData->PlatformAddEntry(chunkData->Queue, DoMeshWork, work);
        #endif

        int fuckTheDebugger = 0;
    }

   

    void findAndEnqueueChunks(game_state* GameState){
        chunk_data* chunkData = GameState->chunkData;
        uint8_t cameraIndex = GameState->entityComponent->entityToCameraMap[GameState->localPlayerEntityIDs[0]];
        if(cameraIndex == NULL_PLAYER){
            COLLISION_DEBUG_PRINTF("player camera entity cameraIndex is null??? ERROR ERROR RETURNING\n");
            return;
        }
        CameraComp& cameraComp = GameState->entityComponent->CameraComps[cameraIndex];
        
        if(cameraComp.inNewChunk){
            cameraComp.inNewChunk = false;
            LOG(Warn, "CAMERA IN NEW CHUNK: %d %d %d", cameraComp.chunk_coords.x,cameraComp.chunk_coords.y,cameraComp.chunk_coords.z);
            uvec3 drawDistance = chunkData->chunkDrawDistance; 
            
            int32_t centerX = drawDistance.x / 2;
            int32_t centerY = drawDistance.y / 2;
            int32_t centerZ = drawDistance.z / 2;
            // chunkData->cameraGridCenterCoords = {centerX, centerY, centerZ}; //only needs to get set once, or whenever draw distance changes

            //destroy all chunks that fall outside of the previous chunk grid
            for(u32 i = 0; i < chunkData->cameraGridCount; i++){
                u32 chunkID = chunkData->cameraGrid[i];
                ivec3 coords = chunkData->coords[chunkID];
                if(checkChunkDestructionRules(GameState, cameraComp, coords, drawDistance, centerX, centerY, centerZ)){
                    InterlockedExchange(chunkData->volChunkVisibilities + chunkID, chunkVisibility_outsideCameraGrid);
                    //immediately queue voxel mesh removal
                    u32 desired = chunkStage_flaggedForDestruction;
                    u32 expected = chunkStage_uploaded;
                    u32 previous = AtomicCompareExchange(chunkData->volChunkStages + chunkID, desired, expected);
                    if(previous == expected){
                        chunkData->testDebugSSBOCount++;
                        PRINT("chunkID: %u meshed, uploaded, and outside of camera, REMOVE: estimated ssbo count after removal: %u\n", chunkID, chunkData->testDebugSSBOCount);
                        Assert(chunkData->testDebugSSBOCount <= 27);

                        release_brickmap_mesh(GameState, chunkData, chunkID);
                        
                    }else{
                    } 


                }
            }
            //reset camera grid to find new chunks within view
            chunkData->cameraGridCount = 0;
            
            ivec3 coords = {};
            //for now just find all the chunks in the camera grid
            for(u32 i = 0; i < chunkData->cameraGridSize; i++){
                coords.x = i % (drawDistance.x);
                coords.y =(i /  drawDistance.x) % drawDistance.y;
                coords.z = i / (drawDistance.x  * drawDistance.y);
                coords += cameraComp.chunk_coords - chunkData->cameraGridCenterCoords;
                
//can enable toroidal space here, just wrap the indices if they go out of bounds in any direction
#if 0
                if(chunkData->toroidal_space_enabled){

                }
#endif

                //got the coords for a slot in the camera chunk, hash it, get the ID, and slot it into the camera chunk at current index
                u32 chunkID = findOrCreateChunk(chunkData, coords, &GameState->WorldArena, true);
                chunkData->cameraGrid[i] = chunkID;
                InterlockedExchange(chunkData->volChunkVisibilities + chunkID, chunkVisibility_invisible);
                //if the chunk state is null, touch it to start the voxelization process
                u32 desired = chunkStage_touched;
                u32 expected = chunkStage_null;
                u32 previous = AtomicCompareExchange(chunkData->volChunkStages + chunkID, desired, expected);


                chunkData->cameraGridCount++;
            }

        }

        //determine visible chunks here
        chunkData->cameraGridVisibleCount = 0;
        fpt_vec3 chunkPos = {};
        for(u32 i = 0; i < chunkData->cameraGridCount; i++){
            u32 chunkID = chunkData->cameraGrid[i];
            ivec3 coords = chunkData->coords[chunkID];
            chunkPos = fpt_vec3_mul_scalar(ivec_to_fpt_vec3(coords - cameraComp.chunk_coords), FPT_CHUNK_SIZE);

            fpt_vec3 chunkMin = {fpt_sub(chunkPos.x, FPT_HALF_CHUNK_SIZE),  fpt_sub(chunkPos.y, FPT_HALF_CHUNK_SIZE),  fpt_sub(chunkPos.z, FPT_HALF_CHUNK_SIZE)};
            fpt_vec3 chunkMax = {fpt_add(chunkPos.x, FPT_HALF_CHUNK_SIZE),  fpt_add(chunkPos.y, FPT_HALF_CHUNK_SIZE),  fpt_add(chunkPos.z, FPT_HALF_CHUNK_SIZE)};

            if(aabbIntersectTest(chunkMin, chunkMax, cameraComp.fptFrustumAABBMin, cameraComp.fptFrustumAABBMax)){
                chunkData->cameraGridVisible[chunkData->cameraGridVisibleCount++] = chunkID;
                InterlockedExchange(chunkData->volChunkVisibilities + chunkID, chunkVisibility_visible);

            }
                
        }
        // PRINT("visible chunkIDs: %u\n", chunkData->cameraGridVisibleCount);

    }


    void recalculateViewFrustum(game_state* GameState){
        uint8_t cameraIndex = GameState->entityComponent->entityToCameraMap[GameState->localPlayerEntityIDs[0]];
        if(cameraIndex == NULL_PLAYER){
            COLLISION_DEBUG_PRINTF("player camera entity cameraIndex is null??? ERROR ERROR RETURNING\n");
            return;
        }
        CameraComp& cameraComp = GameState->entityComponent->CameraComps[cameraIndex];

        chunk_data* chunkData = GameState->chunkData;
        // PRINT("NEED TO HANDLE FRUSTUM MATH FROM SCRATCH!\n");
        extractFrustumAABB(cameraComp.invProjMatrix,cameraComp.invViewMatrix, cameraComp.frustumAABBMin, cameraComp.frustumAABBMax);
        // PRINT("frustum aabb min: %10.5f %10.5f %10.5f\n", cameraComp.frustumAABBMin.x,cameraComp.frustumAABBMin.y,cameraComp.frustumAABBMin.z);
        // PRINT("frustum aabb max: %10.5f %10.5f %10.5f\n", cameraComp.frustumAABBMax.x,cameraComp.frustumAABBMax.y,cameraComp.frustumAABBMax.z);
        

        cameraComp.fptFrustumAABBMin = flt_to_fpt_vec3(cameraComp.frustumAABBMin);
        cameraComp.fptFrustumAABBMax = flt_to_fpt_vec3(cameraComp.frustumAABBMax);
        //determine which chunks intersect with the frustum
        // getIntersectingChunkCoordinates(chunkData, cameraComp.frustumAABBMin, cameraComp.frustumAABBMax, cameraComp.chunk_coords);

        //for the chunks that intersect the frustum, find existing ones, create new ones
        findAndEnqueueChunks(GameState);

    }



    void chunk_camera_updated(game_state* GameState){
      if(!GameState->chunkData->freezeChunks){
        recalculateViewFrustum(GameState);
      }
    }

  
    bool editVoxels(game_state* GameState, chunk_data* chunkData, bool add, fpt_vec3 brushPos, ivec3 brushChunkCoords, fpt brushSize){
        int brickmapID;
        int voxelIndex;
        ivec3 voxelCoords;
        ivec3 chunk_coords;
        bool anyEdited = false;
        int count = 0;
        chunkData->editedCount = 0;
        int editedBmCount = 0;
        // LOG(Info,"edited brickmap count: %d", chunkData->brushBMIDsCount);
        while(chunkData->brushChunkIDCount > 0){
            chunkData->brushChunkIDCount--;
            int i = count;
            count++;
            uint32_t chunkID = chunkData->brushChunkIDs[i];
            if(chunkID == NULL_CHUNK)continue;
            
            if(!chunkData->safeToEdit[chunkID])continue;


            uint32_t* chunkFaceCount = nullptr;
            uint32_t* maxFaceCount = nullptr;
            Brickmap64* brickmap = nullptr;
            CoarseBrickmap64* coarse = nullptr;

           
            brickmap = &chunkData->brickmaps[chunkID];
            coarse = &chunkData->coarse_brickmaps[chunkID];

            for(int coarseID = 0; coarseID < 512; coarseID++){
                Assert(chunkData->coarse_brickmaps[chunkID].active_count[coarseID] <= 512);
            }

            chunkFaceCount = &chunkData->face_count[chunkID];
            maxFaceCount = &chunkData->max_face_count[chunkID];


            ivec3 bmCoords = ivec3_create(0);
            fpt_vec3 bmPos = fpt_brickmap_local_chunk_position(bmCoords, FPT_CHUNK_SIZE, FPT_HALF_CHUNK_SIZE);

            bool edited = false;

            ivec3 voxMin = chunkData->brushChunkVoxMin[i];//ivec3_create(0);// = calculateAbsoluteVoxelCoordinates(chunkData->brushBounds.min, bmPos, bmScale, invBmScale);
            ivec3 voxMax = chunkData->brushChunkVoxMax[i];//ivec3_create(0);// = calculateAbsoluteVoxelCoordinates(chunkData->brushBounds.max, bmPos, bmScale, invBmScale);
            
            ivec3 chunk_coords = chunkData->coords[chunkID];
            fpt_vec3 relativeChunkOffset = ivec_to_fpt_vec3(chunk_coords - chunkData->brushCenterchunk_coords) * FPT_CHUNK_SIZE;
            fpt_vec3 relative_brushPos = brushPos - relativeChunkOffset;
            for (int x = voxMin.x; x <= voxMax.x; ++x) {
                for (int y = voxMin.y; y <= voxMax.y; ++y) {
                    for (int z = voxMin.z; z <= voxMax.z; ++z) {
                        
                        fpt_vec3 voxPos = fpt_getVoxelWorldPosition(ivec3_create(x,y,z), bmPos - FPT_HALF_CHUNK_SIZE);
                        vec3 debugVoxPos = fpt_to_flt_vec3(voxPos);
                        // LOG(Info,"voxPos:        {} {} {}", debugVoxPos.x,debugVoxPos.y,debugVoxPos.z);

                        if(fpt_vec3_length(voxPos - relative_brushPos) >= brushSize)continue;
                        uint32_t v = x + (y * 64) + (z * 64 * 64);

                        uint32_t cx = x / 8;
                        uint32_t cy = y / 8;
                        uint32_t cz = z / 8;
                        uint32_t coarse_index = cx + (cy * 8) + (cz * 64);


                        if(add){
                            // if(brickmap->voxels[v] & 0x3FF)continue;
                            if(brickmap->voxels[v])continue;
                            brickmap->active_count++;
                            brickmap->voxels[v] = 1;
                            printf("placing voxel %d %d %d in chunk %d %d %d\n", x,y,z,chunk_coords.x,chunk_coords.y,chunk_coords.z);
                            // brickmap->voxels[v] |= (1 << 10);
                            coarse->active_count[coarse_index]++;
                            Assert(coarse->active_count[coarse_index] <= 512 && "active count too high in coarse grid?? ERROR");
                        }   

                        else{
                            // if(!(brickmap->voxels[v] & 0x3FF))continue;
                            if(!(brickmap->voxels[v]))continue;
                            brickmap->active_count--;
                            coarse->active_count[coarse_index]--;
                            brickmap->voxels[v] = 0;
                        }

                        edited = true;

                    }
                }
            }

            if(edited){
                chunkData->editedChunkIDs[chunkData->editedCount] = chunkID;

                
                chunkData->editedBMs[chunkData->editedCount] = brickmap;

                chunkData->editedCount++;
                editedBmCount++;


                anyEdited = true;

            }

            // floodFillLight(chunkData, brickmap);
        }
#if 0
        //clear lighting loop
        for(int i = 0; i < editedBmCount; i++){
            uint32_t chunkID = chunkData->editedChunkIDs[i];
            if(chunkID == NULL_CHUNK)continue;
            
            int LODType = chunkData->LODTypes[chunkID];
            int brickmapRes = 1;
            if(LODType == 0 || !chunkData->safeToEdit[chunkID])continue;

            // vec3 chunkPos = chunkData->positions[chunkID];
            fpt_vec3 chunkPos = chunkData->fptPositions[chunkID];
            ivec3 chunk_coords = chunkData->coords[chunkID];
            // uint32_t brickmapID = chunkData->editedBMIDs[i];

            int resID = 0;
            Brickmap64* brickmap = nullptr;  
            int lodBrickmapCount = 0;
 
                brickmap = &chunkData->brickmaps[ chunkID];

            clearLight(chunkData, brickmap);
            ivec3 bmCoords = chunkData->editedBMCoords[i];
            clearLightingAdjacentBrickmaps(chunkData, bmCoords, chunk_coords, brickmapID, chunkID);
        }
            

        int tempCount = chunkData->editedCount;
        // flood fill lighting loop
        for(int i = 0; i < tempCount; i++){
            uint32_t chunkID = chunkData->editedChunkIDs[i];
            if(chunkID == NULL_CHUNK)continue;
            
            int LODType = chunkData->LODTypes[chunkID];
            int brickmapRes = 1;
            if(LODType == 0 || !chunkData->safeToEdit[chunkID])continue;
            fpt_vec3 chunkPos = chunkData->fptPositions[chunkID];

            // vec3 chunkPos = chunkData->positions[chunkID];
            ivec3 chunk_coords = chunkData->coords[chunkID];
            ivec3 bmCoords = chunkData->editedBMCoords[i];

            uint32_t brickmapID = chunkData->editedBMIDs[i];

            int resID = 0;
            Brickmap64* brickmap = nullptr;  
            int lodBrickmapCount = 0;
                brickmap = &chunkData->brickmaps[chunkID];
            floodFillLightAcrossBrickmaps(chunkData, brickmap, bmCoords, chunk_coords, brickmapID, chunkID);

        }
#endif
        //defer actual chunk updates until after we've processed all the input and we KNOW that all the voxel edits have been processed
            
        return anyEdited;
    }





    void process_mouse_down(game_state* GameState, player_input& currInput, uint32_t playerEntityID){
        // COLLISION_DEBUG_PRINTF("CHUNK SYSTEM MOUSE DOWN NOT UPDATED FOR FIXED POINT YET\n");
        // return;

        EntityComponent& ec = *GameState->entityComponent;
        uint32_t playerIndex = ec.entityToPlayerMap[playerEntityID];
        uint32_t transIndex = ec.entityToTransMap[playerEntityID];
        TransComp& transComp = ec.TransComps[transIndex];
        PlayerComp& PlayerComp = ec.PlayerComps[playerIndex];

        chunk_data* chunkData = GameState->chunkData;
        bool edited = false;

        if(GameState->localPlayerEntityIDs[0] == playerEntityID){
            if(!currInput.consumedMouse.sideBack && WAS_PRESSED(currInput, mouse.sideBack, input_mouse_sideBack )){
                chunkData->lockMouseMotion = !chunkData->lockMouseMotion;
            }
        }
        recalculateBrushAABB(chunkData, PlayerComp.brushPos, PlayerComp.brushChunkCoords, PlayerComp.brushSize);

        //add adjacent voxel
        if((currInput.consumedMouse.left == 0 && WAS_PRESSED(currInput, mouse.left, input_mouse_left )) && chunkData->voxelRayCastResult.chunkID != NULL_CHUNK){
            // edited = editVoxel(chunkData, true); //true for add voxel
            edited = editVoxels(GameState, chunkData, true, PlayerComp.brushPos, PlayerComp.brushChunkCoords, PlayerComp.brushSize); //true for add voxel
            
        }
        //remove voxel
        if(currInput.consumedMouse.right == 0 && WAS_PRESSED(currInput, mouse.right, input_mouse_right ) && chunkData->voxelRayCastResult.chunkID != NULL_CHUNK){
            // edited = editVoxel(chunkData, false); //false for remove voxel
            edited = editVoxels(GameState, chunkData, false, PlayerComp.brushPos, PlayerComp.brushChunkCoords,  PlayerComp.brushSize); //false for remove voxel
        }
        if(edited){
            // fpt_vec3 rayDir = flt_to_fpt_vec3(CameraManager::CastMouseRay(camera, currInput.mouse_x, currInput.mouse_y));
            // fpt_vec3 rayOrigin = cameraComp.fptPos; //cast from entity position

            //   if(fpt_pick_voxel_in_chunk(GameState->chunkData, rayOrigin, rayDir, 
            uint8_t cameraIndex = GameState->entityComponent->entityToCameraMap[GameState->localPlayerEntityIDs[0]];
            if(cameraIndex == NULL_PLAYER){
                COLLISION_DEBUG_PRINTF("player camera entity cameraIndex is null??? ERROR ERROR RETURNING\n");
                return;
            }
            CameraComp& cameraComp = GameState->entityComponent->CameraComps[cameraIndex];

            fpt_vec3 rayDir = flt_to_fpt_vec3(CastMouseRay(*cameraComp.windowWidth, *cameraComp.windowHeight, cameraComp.invProjMatrix, cameraComp.invViewMatrix ,currInput.mouse_x, currInput.mouse_y));

            fpt_vec3 rayOrigin = cameraComp.pos_in_chunk;
            ivec3 raychunk_coords = cameraComp.chunk_coords;  
            // rayOrigin = cameraComp.fptPos; //cast from entity position

              if(fpt_pick_voxel_in_chunk(GameState->chunkData, rayOrigin, raychunk_coords, rayDir, 
                                            chunkData->voxelRayCastResult)){
                PlayerComp.brushPos = chunkData->voxelRayCastResult.pos_in_chunk_hit;
                PlayerComp.brushChunkCoords = chunkData->voxelRayCastResult.chunk_coords_hit;
                
                recalculateBrushAABB(chunkData, PlayerComp.brushPos, PlayerComp.brushChunkCoords,  PlayerComp.brushSize);
            }
        }

        if(currInput.mouse.sideFront && chunkData->selectedEntityIndex < NULL_ENTITY){
            chunkData->dragging_entity = true;
        }
    }

    void process_mouse_up(game_state* GameState, player_input& currInput, uint32_t playerEntityID){
        chunk_data* chunkData = GameState->chunkData;
        
        if(WAS_RELEASED(currInput, mouse.sideFront, input_mouse_sideFront ) && chunkData->selectedEntityIndex < NULL_ENTITY){
            chunkData->dragging_entity = false;//no longer dragging entity
            chunkData->selectedEntityIndex = NULL_ENTITY;
            chunkData->selectedEntityChunkID = NULL_CHUNK;
        }

    }
    
    void process_mouse_wheel(game_state* GameState, player_input& currInput, uint32_t playerEntityID){
        EntityComponent& ec = *GameState->entityComponent;
        uint32_t playerIndex = ec.entityToPlayerMap[playerEntityID];
        uint32_t transIndex = ec.entityToTransMap[playerEntityID];
        TransComp& transComp = ec.TransComps[transIndex];
        PlayerComp& PlayerComp = ec.PlayerComps[playerIndex];

        uint8_t cameraIndex = GameState->entityComponent->entityToCameraMap[GameState->localPlayerEntityIDs[0]];
        if(cameraIndex == NULL_PLAYER){
            COLLISION_DEBUG_PRINTF("player camera entity cameraIndex is null??? ERROR ERROR RETURNING\n");
            return;
        }
        CameraComp& cameraComp = GameState->entityComponent->CameraComps[cameraIndex];

        chunk_data* chunkData = GameState->chunkData;
   
        //SLOP to update the voxel brush sphere, which is the only one we render currently, to remove we first need to start drawing brushes per entity
        if(playerEntityID == GameState->localPlayerEntityIDs[0]){
            if((chunkData->brushSize + ((float)currInput.mouse_wheel*0.125f)) > 0.125f && (chunkData->brushSize + ((float)currInput.mouse_wheel*0.125f)) < 8.0f){
                chunkData->brushSize += ((float)currInput.mouse_wheel*0.125f);
                chunkData->fptBrushSize = fpt_add(chunkData->fptBrushSize, fpt_mul(i2fpt(currInput.mouse_wheel), FPT_EIGHTH)); 
                        recalculateBrushAABB(chunkData, chunkData->fptBrushPos, chunkData->brushCenterchunk_coords,  chunkData->fptBrushSize);
            }
        }

        if((fpt2fl(PlayerComp.brushSize) + ((float)currInput.mouse_wheel*0.125f)) > 0.125f && (fpt2fl(PlayerComp.brushSize) + ((float)currInput.mouse_wheel*0.125f)) < 8.0f){
            PlayerComp.brushSize = fpt_add(PlayerComp.brushSize, fpt_mul(i2fpt(currInput.mouse_wheel), FPT_EIGHTH)); 
            recalculateBrushAABB(chunkData, PlayerComp.brushPos, PlayerComp.brushChunkCoords,  PlayerComp.brushSize);
        }

        // fpt_vec3 rayDir = PlayerComp.rayDir;//flt_to_fpt_vec3(CameraManager::CastMouseRay(camera, currInput.mouse_x, currInput.mouse_y));
        // fpt_vec3 rayOrigin = player_transComp.pos_in_chunk;//cameraComp.fptPos; //cast from entity position
        // ivec3 raychunk_coords = player_transComp.chunk_coords;
        if(chunkData->dragging_entity && chunkData->selectedEntityIndex < NULL_ENTITY){
            chunkData->distance_to_selected_entity += i2fpt(currInput.mouse_wheel);
            // PRINT("CAST MOUSE RAY HERE! NEED INVERSE MAT4 FUNCTION!\n");
            fpt_vec3 rayDir = flt_to_fpt_vec3(CastMouseRay(*cameraComp.windowWidth, *cameraComp.windowHeight, cameraComp.invProjMatrix, cameraComp.invViewMatrix ,currInput.mouse_x, currInput.mouse_y));

            drag_entity(GameState, rayDir, cameraComp.pos_in_chunk, cameraComp.chunk_coords);
        }
    }
    
    void process_mouse_motion(game_state* GameState, CameraComp& cameraComp, player_input& currInput, uint32_t playerEntityID){
        if(GameState->chunkData->lockMouseMotion || cameraComp.freeMode)return;

        if(currInput.consumedMouse.delta || currInput.mouse_dx == 0 && currInput.mouse_dy == 0)return;
        if(currInput.mouse_x < 0 || currInput.mouse_x > *GameState->window_width || currInput.mouse_y < 0 || currInput.mouse_y > *GameState->window_height)return;
      // ////spdLOG(Info,"mouse moved {} {}", event.x, event.y);
            chunk_data* chunkData = GameState->chunkData;


            EntityComponent& ec = *GameState->entityComponent;
            uint32_t playerIndex = ec.entityToPlayerMap[playerEntityID];
            uint32_t transIndex = ec.entityToTransMap[playerEntityID];
            TransComp&  player_transComp = ec.TransComps[transIndex];
            PlayerComp& PlayerComp = ec.PlayerComps[playerIndex];



            // _chunkManager->selectedchunkID = CHUNK_NOT_FOUND;
            vec2 mousePos = vec2_create(currInput.mouse_x, currInput.mouse_y);
            chunkData->mousePos = mousePos;
            int mousePosX = currInput.mouse_x;
            int mousePosY = currInput.mouse_y;
            float closestDistance = 100.f; //this limits the distance technically, if the entity is further than this, it wont be selected
            bool firstHit = false;
            
            if(playerEntityID == GameState->localPlayerEntityIDs[0]){

                chunkData->rayOrigin = fpt_to_flt_vec3(cameraComp.pos_in_chunk);

                chunkData->rayStart = fpt_to_flt_vec3(cameraComp.fptTarget * -FPT_ONE);
                chunkData->raychunk_coords = cameraComp.chunk_coords;
                
                // PRINT("CAST MOUSE RAY HERE! NEED INVERSE MAT4 FUNCTION!\n");
                chunkData->rayDir = CastMouseRay(*cameraComp.windowWidth, *cameraComp.windowHeight, cameraComp.invProjMatrix, cameraComp.invViewMatrix ,mousePosX, mousePosY);
            

                chunkData->fptRayDir = flt_to_fpt_vec3(chunkData->rayDir);

                chunkData->raySpherePos = chunkData->rayOrigin + chunkData->rayDir * vec3_create(chunkData->raySphereStep);
                // chunkData->raySphereVoxelPos = getVoxelWorldPosition(calculateVoxelCoordinates(chunkData->raySpherePos));

                vec3 rayEnd = (chunkData->rayDir * 1000.0f);// - cameraPosition;
                vec3 frontPoint = GetPointInFront(cameraComp, -1);
                
                // frontPoint += fpt_to_flt_vec3(cameraComp.fptPos);
                if(!cameraComp.freeMode){
                    // chunkData->rayLineVertices[0] = Vertex{chunkData->rayOrigin, 0xffaaaaff};
                    // chunkData->rayLineVertices[1] = Vertex{rayEnd + chunkData->rayOrigin, 0xffaaffaa};
                    // PRINT("update chunk ray cast direction here??\n");
                    // chunkData->rayLineVertices[0] = Vertex{vec3_create(0), 0xffaaaaff};
                    // chunkData->rayLineVertices[1] = Vertex{rayEnd, 0xffaaffaa};
                    // DynamicMesh& chunkRayMesh = GameState->meshData->dynamicMeshes[DynamicMeshTypes::dyn_mesh_ChunkSystemRay];
                    // bgfx::update(chunkRayMesh.vbh, 0, bgfx::makeRef(chunkData->rayLineVertices, sizeof( chunkData->rayLineVertices)));
                }
                else{return;}
                chunkData->cameraSnapshot = cameraComp;

            }
          
            chunkData->voxelPathCount = 0;
            // fpt_vec3 rayDir = PlayerComp.rayDir;//flt_to_fpt_vec3(CameraManager::CastMouseRay(camera, currInput.mouse_x, currInput.mouse_y));
            // fpt_vec3 rayOrigin = player_transComp.pos_in_chunk;//cameraComp.fptPos; //cast from entity position
            // ivec3 raychunk_coords = player_transComp.chunk_coords;

            // PRINT("CAST MOUSE RAY HERE! NEED INVERSE MAT4 FUNCTION!\n");
            // fpt_vec3 rayDir = flt_to_fpt_vec3(CastMouseRay(cameraComp, mousePosX, mousePosY)); 
            // fpt_vec3 rayDir = fpt_vec3_create(0);
            fpt_vec3 rayDir  = flt_to_fpt_vec3(CastMouseRay(*cameraComp.windowWidth, *cameraComp.windowHeight, cameraComp.invProjMatrix, cameraComp.invViewMatrix ,mousePosX, mousePosY));

            fpt_vec3 rayOrigin = cameraComp.pos_in_chunk;
            ivec3 raychunk_coords = cameraComp.chunk_coords;  
        

            //if we arent dragging the entity, try to select one
            if(!chunkData->dragging_entity){
            //select entity logic
                #if 1
                for(int i = 0; i < chunkData->cameraGridVisibleCount; i++){
                    uint32_t chunkID = chunkData->cameraGridVisible[i];
                    // vec3 pos = chunkData->positions[chunkID];
                    ivec3 coords = chunkData->coords[chunkID];
                    // if(coords != ivec3_create(0,0,0))continue; //for simple origin chunk test
                    bool searchForEntities = true;


                    ivec3 relative_coords = coords - raychunk_coords;
                    if(vec3_length(vec3_create(relative_coords)) > 2)continue;
                    
                    fpt_vec3 relative_pos = ivec_to_fpt_vec3(relative_coords) * FPT_CHUNK_SIZE;
                    //ray chunk is at 0 0 -1
                    //cords are at 0 0 -2
                    //so ( 0 0 -2) - (0 0 -1) = (0 0 -1)
                    bvh_tree& bvhTree = chunkData->bvhTrees[chunkID];
                    if(bvhTree.rootNode == NULL_NODE){
                        searchForEntities = false;
                    }
                    fpt_vec3 collisionPosition = fpt_vec3_create(0);

                    fpt_vec3 half_size_offset = fpt_vec3_create(FPT_HALF_CHUNK_SIZE);
                    fpt_vec3 min = relative_pos - half_size_offset;
                    fpt_vec3 max = relative_pos + half_size_offset;

                    // vec3 floatRayOrigin = fpt_to_flt_vec3(rayOrigin);
                    // vec3 floatRayDir = fpt_to_flt_vec3(rayDir);

                    bool hitChunk = HitBoundingBox(min, max, 
                                    rayOrigin, rayDir, 
                                    collisionPosition);
                    bool entityFound = false;

                    //floats for now since HitBoundingBox takes in floats, need to change to fixed point
                    fpt closestDistance = i2fpt(2000);
                    if(hitChunk && searchForEntities){ 
                        // if(!firstHit){_chunkManager->selectedchunkID = chunkID; firstHit = true;}
                        // //spdLOG(Info,"ray hit chunk id {}", chunkID);
                        
                        //pick entity
                        // //spdLOG(Warn,"starting root node traverse in chunk: {} {} {}", coords.x,coords.y,coords.z);
                        uint16_t closestEntity = rayTraverseNode(GameState, bvhTree, bvhTree.rootNode, rayOrigin, relative_pos, rayDir, closestDistance, collisionPosition);
                        COLLISION_DEBUG_PRINTF("closest entity: %d\n", closestEntity);
                        // if(playerEntityID != closestEntity){
                            chunkData->selectedEntityIndex = closestEntity;
                            chunkData->selectedEntityChunkID = chunkID;
                        // }
                        if(closestEntity < NULL_ENTITY/*  && playerEntityID != closestEntity */){
                            chunkData->entity_selected = true;
                            // chunkData->distance_to_selected_entity  = closestDistance;
                            uint32_t entity_transIndex = ec.entityToTransMap[closestEntity];
                            TransComp&      transComp       = ec.TransComps[entity_transIndex];

                            //to get the actual distance between the camera and the selected entity's center
                            //if we use closestDistance then we immediately move the entity to the point at which the ray intersected the entity edge
                            //which would make it teleport closer to the camera every time we select and drag it
                            chunkData->distance_to_selected_entity  = fpt_vec3_length((transComp.pos_in_chunk - rayOrigin) + ivec_to_fpt_vec3((transComp.chunk_coords - raychunk_coords)  * CHUNK_SIZE));
        
                            // chunkData->entity_position = transComp.pos_in_chunk;
                            break;
                        }
                        else{
                            chunkData->entity_selected = false;
                            chunkData->selectedEntityChunkID = NULL_CHUNK;
                        }
                        
                    }
                }
                #endif

            }
            else if(chunkData->selectedEntityIndex < NULL_ENTITY){//we are dragging the entity
                drag_entity(GameState, rayDir, rayOrigin, raychunk_coords);

            }



            if(fpt_pick_voxel_in_chunk(GameState->chunkData, rayOrigin, raychunk_coords, rayDir, 
                chunkData->voxelRayCastResult)){
                bool edited = false;
                // PlayerComp.brushPos = chunkData->voxelRayCastResult.fptHitPosition;
                PlayerComp.brushPos = chunkData->voxelRayCastResult.pos_in_chunk_hit;
                PlayerComp.brushChunkCoords = chunkData->voxelRayCastResult.chunk_coords_hit;
                //add adjacent voxel
                recalculateBrushAABB(chunkData, PlayerComp.brushPos, PlayerComp.brushChunkCoords,  PlayerComp.brushSize);

                if(!currInput.consumedMouse.left && currInput.mouse.left && chunkData->voxelRayCastResult.chunkID != NULL_CHUNK){
                    // edited = editVoxel(chunkData, true); //true for add voxel
                    edited = editVoxels(GameState, chunkData, true, PlayerComp.brushPos, PlayerComp.brushChunkCoords,  PlayerComp.brushSize); //true for add voxel
                }
                //remove voxel
                else if(!currInput.consumedMouse.right && currInput.mouse.right  && chunkData->voxelRayCastResult.chunkID != NULL_CHUNK){
                    // edited = editVoxel(chunkData, false); //false for remove voxel
                    edited = editVoxels(GameState, chunkData, false, PlayerComp.brushPos, PlayerComp.brushChunkCoords,  PlayerComp.brushSize); //false for remove voxel
                }
                if(edited){
                    if(fpt_pick_voxel_in_chunk(GameState->chunkData, rayOrigin, raychunk_coords, rayDir, chunkData->voxelRayCastResult));
                }
                // PlayerComp.brushPos = chunkData->voxelRayCastResult.fptHitPosition;
                PlayerComp.brushPos = chunkData->voxelRayCastResult.pos_in_chunk_hit;
                PlayerComp.brushChunkCoords = chunkData->voxelRayCastResult.chunk_coords_hit;
                recalculateBrushAABB(chunkData, PlayerComp.brushPos, PlayerComp.brushChunkCoords,  PlayerComp.brushSize);

            }
            
            //calculate the ray direction from the entity to the brush position of the main camera component
            ivec3 chunk_diff = PlayerComp.brushChunkCoords - player_transComp.chunk_coords; 
            if(abs(chunk_diff.x) >= 2 || abs(chunk_diff.y) >= 2 || abs(chunk_diff.z) >= 2); 
            else PlayerComp.rayDir = fpt_vec3_normalize(distance_diff(PlayerComp.brushPos, player_transComp.pos_in_chunk, PlayerComp.brushChunkCoords, player_transComp.chunk_coords));

    }

    void drag_entity(game_state* GameState, fpt_vec3 rayDir, fpt_vec3 rayOrigin, ivec3 raychunk_coords){
        chunk_data* chunkData = GameState->chunkData;
        if(chunkData->selectedEntityIndex == GameState->localPlayerEntityIDs[0])return;//dont drag the selected entity, cant get it working
        
        // fpt_vec3 rayDir = PlayerComp.rayDir;//flt_to_fpt_vec3(CameraManager::CastMouseRay(camera, currInput.mouse_x, currInput.mouse_y));
        // fpt_vec3 rayOrigin = player_transComp.pos_in_chunk;//cameraComp.fptPos; //cast from entity position
        // ivec3 raychunk_coords = player_transComp.chunk_coords;
        
        //how can we consolidate entity movement into a single function?
        EntityComponent& ec = *GameState->entityComponent;
        
        uint32_t transIndex = ec.entityToTransMap[chunkData->selectedEntityIndex];
        fpt_vec3 movement = fpt_vec3_create(0);


        TransComp&      transComp       = ec.TransComps[transIndex];


        ivec3 new_chunk_coords = ivec3_create(0);
        bool in_new_chunk = false;
        // //spdLOG(Warn,"entity drag calculation start!");
        fpt_vec3 new_pos = (rayOrigin + (rayDir * chunkData->distance_to_selected_entity));// - (flt_to_fpt_vec3(raychunk_coords  - transComp.chunk_coords ) * FPT_CHUNK_SIZE);
        fpt_vec3 new_pos_in_chunk = new_pos;
        
        rebasePosition(new_pos_in_chunk, new_chunk_coords, &in_new_chunk, chunkData->toroidal_space_enabled);
        
        
        ivec3 coords_offset_from_camera = new_chunk_coords;

        
        // movement = new_pos - chunkData->entity_position;

        movement = (new_pos_in_chunk - transComp.pos_in_chunk) + (ivec_to_fpt_vec3((raychunk_coords + new_chunk_coords) - transComp.chunk_coords) * FPT_CHUNK_SIZE);

        fpt_vec3 movement_pos_in_chunk = movement;


        // rebasePosition(movement_pos_in_chunk, new_chunk_coords, in_new_chunk, chunkData->toroidal_space_enabled);
        // movement -= (flt_to_fpt_vec3(new_chunk_coords) * FPT_CHUNK_SIZE);

        COLLISION_DEBUG_PRINT_FPT_VEC3("movement  : ", movement);
        COLLISION_DEBUG_PRINT_FPT_VEC3("posInChunk: ", transComp.pos_in_chunk);

        // vec3 debugMovement     =   fpt_to_flt_vec3(movement);
        // vec3 debugNewPos       =   fpt_to_flt_vec3(new_pos);
        // vec3 debugRayOrigin    =   fpt_to_flt_vec3(rayOrigin);
        // vec3 debugRayDir       =   fpt_to_flt_vec3(rayDir);
        // vec3 debugOrigPos      =   fpt_to_flt_vec3(transComp.pos_in_chunk); 
        // float     debugDistance     =   fpt2fl(chunkData->distance_to_selected_entity); 

        // //spdLOG(Info,"origPos           : {} {} {}", debugOrigPos.x, debugOrigPos.y, debugOrigPos.z);
        // //spdLOG(Info,"newPos            : {} {} {}", debugNewPos.x, debugNewPos.y, debugNewPos.z);
        // //spdLOG(Info,"movement          : {} {} {}", debugMovement.x, debugMovement.y, debugMovement.z);
        // //spdLOG(Info,"chunkCoordOffset  : {} {} {}", new_chunk_coords.x, new_chunk_coords.y, new_chunk_coords.z);
        // //spdLOG(Info,"rayOrigin         : {} {} {}", debugRayOrigin.x, debugRayOrigin.y, debugRayOrigin.z);
        // //spdLOG(Info,"rayDir            : {} {} {}", debugRayDir.x, debugRayDir.y, debugRayDir.z);
        // //spdLOG(Info,"entity Chunk      : {} {} {}", transComp.chunk_coords.x, transComp.chunk_coords.y, transComp.chunk_coords.z);
        // //spdLOG(Info,"camera Chunk      : {} {} {}", raychunk_coords.x, raychunk_coords.y, raychunk_coords.z);
        // //spdLOG(Info,"distance          : {}", debugDistance);
        
        // apply_entity_movement(transComp, movement);
        apply_desired_movement(transComp, movement);
        if(in_new_chunk){
            //spdLOG(Warn,"NEED TO UPDATE SELECTED ENTITY CHUNK ID FOR ENTITY SELECTION!!");
        }
        //will move delta mess us up?
        // transComp.moveDelta +=  movement; 
        // transComp.pos_in_chunk +=  movement;
        // transComp.pos_in_chunk_delta = new_pos_in_chunk - transComp.pos_in_chunk;

        // transComp.pos_in_chunk =  new_pos_in_chunk;
        // ivec3 old_chunk_coords = transComp.chunk_coords;
        // transComp.chunk_coords =  (raychunk_coords + coords_offset_from_camera);
        // vec3 debugpos_in_chunk = fpt_to_flt_vec3(transComp.pos_in_chunk);    
        // //spdLOG(Info,"pos_in_chunk        : {} {} {}", debugpos_in_chunk.x, debugpos_in_chunk.y, debugpos_in_chunk.z);


        // rebasePosition(transComp.pos_in_chunk, transComp.chunk_coords, transComp.inNewChunk, chunkData->toroidal_space_enabled);
        // transComp.chunk_coords_delta += (transComp.chunk_coords - old_chunk_coords);

        // debugpos_in_chunk = fpt_to_flt_vec3(transComp.pos_in_chunk);    

        // //spdLOG(Info,"NEW pos_in_chunk    : {} {} {}", debugpos_in_chunk.x, debugpos_in_chunk.y, debugpos_in_chunk.z);
        // //spdLOG(Info,"NEW CHUNK COORDS    : {} {} {}", transComp.chunk_coords.x, transComp.chunk_coords.y, transComp.chunk_coords.z);
        uint32_t physicsIndex = ec.entityToPhysicsMap[chunkData->selectedEntityIndex];
        if(physicsIndex != NULL_ENTITY){
            PRINT("CALCULATE PHYSICS DERIVED DATA HERE! %d\n", __LINE__);
            // PhysicsComp& physicsComp = ec.PhysicsComps[physicsIndex];
            // calculate_derived_data(physicsComp, transComp);
        }
        // chunkData->entity_position      = transComp.pos_in_chunk;
        // chunkData->entity_chunk_coords  = transComp.chunk_coords;
        
        // ec.transFlags[transIndex] |= pos_dirty;
    }


    void chunk_start(game_state* GameState){

        debug_logger_open_file(&GameState->chunkData->logger, "chunkSystemLog.txt", true, LogTypes::Trace);

    
        COLLISION_DEBUG_PRINTF("ChunkSystem start()\n");
        chunk_data* chunkData = GameState->chunkData;
        chunkData->logger.console_output = true;

        uvec3 drawDistance = chunkData->chunkDrawDistance; 

        int32_t centerX = drawDistance.x / 2;
        int32_t centerY = drawDistance.y / 2;
        int32_t centerZ = drawDistance.z / 2;
        chunkData->cameraGridCenterIndex = centerX + (centerY * drawDistance.x) + (centerZ * drawDistance.x * drawDistance.y);
        chunkData->chunkGridIntersectBounds.x = fpt_mul((drawDistance.x - 2) , FPT_CHUNK_SIZE);
        chunkData->chunkGridIntersectBounds.y = fpt_mul((drawDistance.y - 2) , FPT_CHUNK_SIZE);
        chunkData->chunkGridIntersectBounds.z = fpt_mul((drawDistance.z - 2) , FPT_CHUNK_SIZE);
        //the intersect bounds will always be centered at the camera's current chunk
        chunkData->cameraGridSize = drawDistance.x * drawDistance.y * drawDistance.z;
        chunkData->cameraGridCenterCoords = {centerX, centerY, centerZ};
        //do we need to worry about padding anymore?
        // populate_padding_lookup(chunkData->paddingLookup);

        //set each thread's index
        for(int i = 0; i < MAX_WORKER_THREADS; i++){
            voxel_work* work = &chunkData->workerThreads[i];
            work->threadIndex = i;
        }

        reset(chunkData);

        recalculateViewFrustum(GameState);
        // chunkData->vertexPulled_ibh = BGFX_INVALID_HANDLE;
        // chunkData->vertexPulled_vbh = BGFX_INVALID_HANDLE;
        
        init_vertex_pull_buffers(chunkData);
        
        chunkData->testDebugSSBOCount = 27;
    }


  

    void chunk_update(game_state* GameState){
        TIMED_BLOCK("CHUNK UPDATE");

        // BEGIN_BLOCK("CHUNK UPDATE");
        chunk_data* chunkData = GameState->chunkData;
        
        //reset for the next tick
        chunkData->stableChunkMeshMemoryCount = 0;
        
        // PRINT("visible chunks: %d\n", chunkData->visibleCount);
        uint8_t cameraIndex = GameState->entityComponent->entityToCameraMap[GameState->localPlayerEntityIDs[0]];
        if(cameraIndex == NULL_PLAYER){
            COLLISION_DEBUG_PRINTF("player camera entity cameraIndex is null??? ERROR ERROR RETURNING\n");
            return;
        }
        CameraComp& cameraComp = GameState->entityComponent->CameraComps[cameraIndex];
        //create a single chunk from the top of the queue and add it to the chunkHash
        
        //better multithreading
        #if 1
        //just cycle over ALL the chunks to determine their state and see if we need to transition
        BEGIN_BLOCK("CHUNK CYCLE");
        for(u32 curChunkID = 0; curChunkID < MAX_CHUNKS; curChunkID++){
            ivec3 coords = chunkData->coords[curChunkID];
            ChunkStage chunkStage = (ChunkStage)AtomicRead(chunkData->volChunkStages + curChunkID);
            ChunkVisibility chunkVisibility = (ChunkVisibility)AtomicRead(chunkData->volChunkVisibilities + curChunkID);

            
            switch(chunkStage){//try to voxelize visible chu
                case chunkStage_touched:{    
                    TIMED_BLOCK("Chunk Touched");
                    // BEGIN_BLOCK("PRINTF");
                    PRINT("chunkID: %u touched\n", curChunkID);
                    // END_BLOCK("PRINTF");
                //chunk has not yet been meshed, appened to queue
                    //when the chunk is successfully meshed, we will assign it an index into the persistent SSBO to draw
                    // BEGIN_BLOCK("CHUNK COORDS ASSERT");
                    Assert(coords.x != CHUNK_COORDS_UNINITIALIZED && coords.y != CHUNK_COORDS_UNINITIALIZED && coords.z != CHUNK_COORDS_UNINITIALIZED);
                    // END_BLOCK("CHUNK COORDS ASSERT");
                    // BEGIN_BLOCK("CHUNK TOUCHED VISIBILITY CHECK");
                    if(chunkVisibility > chunkVisibility_skirt){
                        if(chunkData->activeThreadCount < MAX_WORKER_THREADS){
                            {
                                // TIMED_BLOCK("CHUNK TOUCHED ATOMIC EXCHANGE");
                                AtomicExchangeU32(chunkData->volChunkStages + curChunkID, chunkStage_firstPassWorking);

                            }
                            setupVoxelizeTask(GameState, chunkData, cameraComp, curChunkID);
                        }
                    }
                    // END_BLOCK("CHUNK TOUCHED VISIBILITY CHECK");

                }break;
                case chunkStage_firstPassComplete:{
                    TIMED_BLOCK("Chunk First Pass Complete");
                    //voxelization work is done, that thread is no longer active
                    PRINT("chunkID: %u firstPassComplete\n", curChunkID);
                    Assert(coords.x != CHUNK_COORDS_UNINITIALIZED && coords.y != CHUNK_COORDS_UNINITIALIZED && coords.z != CHUNK_COORDS_UNINITIALIZED);
                    chunkData->activeThreadCount--;
                    chunkData->safeToEdit[curChunkID] = true;

                    //try to push it through the second phase, requires all its neighbors having been loaded in
                    if(chunkVisibility > chunkVisibility_skirt){
                        //pretend we expand the terrain here, 
                        chunkData->metaGenInfoCount[curChunkID] = 0;
                        AtomicExchangeU32(chunkData->volChunkStages + curChunkID, chunkStage_secondPassComplete);
                    }else{
                        PRINT("chunkID: %4u | %3d %3d %3d initially voxelized but not not within camera grid!\n", curChunkID, coords.x, coords.y, coords.z);
                    }

                }break;
                case chunkStage_secondPassComplete:{//expand terrain features from all neighbors
                    TIMED_BLOCK("Chunk Second Pass Complete");
                    // chunkData->activeThreadCount--;
                    PRINT("chunkID: %u secondPassComplete\n", curChunkID);
                    // chunkData->safeToEdit[curChunkID] = true;
                    Assert(coords.x != CHUNK_COORDS_UNINITIALIZED && coords.y != CHUNK_COORDS_UNINITIALIZED && coords.z != CHUNK_COORDS_UNINITIALIZED);

                    if(chunkVisibility > chunkVisibility_skirt){
                        //mesh it here
                        if(chunkData->activeThreadCount < MAX_WORKER_THREADS){
                            AtomicExchangeU32(chunkData->volChunkStages + curChunkID, chunkStage_meshing);
                            setupMeshTask(GameState, chunkData, cameraComp, curChunkID);
                        }
                    }else{
                        PRINT("chunkID: %4u | %3d %3d %3d fully voxelized but not not within camera grid!\n", curChunkID, coords.x, coords.y, coords.z);
                    }
                    
                }break;
                case chunkStage_meshed:{//upload the chunk to the GPU
                    #if DISABLE_MESHING
                        break;
                    #endif
                    TIMED_BLOCK("Chunk Meshed");

                    PRINT("chunkID: %u meshed\n", curChunkID);

                    Assert(coords.x != CHUNK_COORDS_UNINITIALIZED && coords.y != CHUNK_COORDS_UNINITIALIZED && coords.z != CHUNK_COORDS_UNINITIALIZED);
                    //meshing work is done, that thread is no longer active
                        
                    voxel_work* work = chunkData->workerThreads + chunkData->chunkWorkers[curChunkID];
                    //mesh could be uploaded already because another chunk needed the worker thread
                    if(work->meshNeedsUploading)PushChunkMeshToRenderer(GameState, chunkData, work);
                        
                    if(chunkVisibility < chunkVisibility_skirt){
                        PRINT("chunkID: %4u | %3d %3d %3d meshed but not not within camera grid!\n", curChunkID, coords.x, coords.y, coords.z);
                    }
                    
                }break;

                
                default:{}break;
            }


            switch(chunkVisibility){
                case chunkVisibility_outsideCameraGrid:{
                    //chunk outside of camera grid, destroy/clear/free or whatever we do
                    if(!chunkData->safeToEdit[curChunkID]){
                        //it will get destroyed when the multithreading task is finished
                        PRINT("cant destroy chunkID: %u\n", curChunkID);
                        // __debugbreak();
                    }
                    else{
                        TIMED_BLOCK("Chunk Visibility Destroy");
                        // BEGIN_BLOCK("Chunk Vis Sub Destroy");
                        ivec3 coords = chunkData->coords[curChunkID];
                        LOG(Trace, "chunk_update() destroying chunkID: %d that now falls outside of camera grid!, coords: %d %d %d, stage: %s", curChunkID, coords.x, coords.y, coords.z, GetChunkStage(chunkStage));
                        
                        AtomicExchangeU32(chunkData->volChunkVisibilities + curChunkID, chunkVisibility_null);
                        AtomicExchangeU32(chunkData->volChunkStages + curChunkID, chunkStage_null);
                        // END_BLOCK("Chunk Vis Sub Destroy");
                        destroyChunk(GameState, chunkData, curChunkID);

                    }

                }break;
                case chunkVisibility_skirt:
                case chunkVisibility_invisible:
                case chunkVisibility_visible:{
                }break;
            }
        }
        END_BLOCK("CHUNK CYCLE");
    
        #endif


        //entity movement
        EntityComponent& ec = *GameState->entityComponent;
        for (int i = 0; i < ec.AabbCount; i ++) {
            uint32_t entityID = ec.AabbToEntityMap[i];
            AabbComp&  aabb     = ec.AabbComps[i];

            if(ec.entityToTransMap[entityID >= NULL_ENTITY])assert(0 && "INVALID ENTITY TRANSFORM COMPONENT");

            TransComp& trans    = ec.TransComps[ec.entityToTransMap[entityID]];
            
            bool isVisible = false;



            if ((aabb.flags & aabb_dirty) != 0) {
                COLLISION_DEBUG_PRINTF("entityID %u aabbFlags is dirty, tick: %u", entityID, GameState->tick);
                COLLISION_DEBUG_PRINT_FPT_VEC3("desired movement   :", trans.desired_movement);
                COLLISION_DEBUG_PRINT_FPT_VEC3("collide movement   :", trans.collide_movement);

                trans.pos_in_chunk +=  trans.desired_movement + trans.collide_movement;  
                // COLLISION_DEBUG_PRINT_FPT_VEC3("pos in chunk       :", trans.pos_in_chunk);
                trans.move_delta  =  trans.desired_movement + trans.collide_movement;

                // if(trans.move_delta.x == 0 && trans.move_delta.y && trans.move_delta.z){
                    // COLLISION_DEBUG_PRINT_FPT_VEC3("trans.move_delta   :", trans.move_delta);
                    // COLLISION_DEBUG_PRINT_FPT_VEC3("aabb min (in chunk):", aabb.min + trans.pos_in_chunk - trans.move_delta);
                //     COLLISION_DEBUG_PRINT_FPT_VEC3("aabb max (in chunk):", aabb.max + trans.pos_in_chunk - trans.move_delta);
                // }
                ///*TAG*/COLLISION_DEBUG_PRINT_FPT_VEC3("desired movement:", trans.desired_movement);
                // COLLISION_DEBUG_PRINT_FPT_VEC3("collide movement:", trans.collide_movement);
                // // COLLISION_DEBUG_PRINT_FPT_VEC3("test mov delta  :", trans.move_delta);
                /*TAG*/COLLISION_DEBUG_PRINT_FPT_VEC3("BEFORE pos in chunk    :", trans.pos_in_chunk);

                // ivec3 old_chunk_coords = trans.chunk_coords;
                rebasePosition(trans.pos_in_chunk, trans.chunk_coords, &trans.inNewChunk, chunkData->toroidal_space_enabled);
                /*TAG*/COLLISION_DEBUG_PRINT_FPT_VEC3("AFTER pos in chunk    :", trans.pos_in_chunk);

                // trans.chunk_coords_delta = (trans.chunk_coords - old_chunk_coords);
                // trans.pos_in_chunk_delta += trans.desired_movement;
                // trans.desired_movement = fpt_vec3_create(0);
                trans.collide_movement = fpt_vec3_create(0);

                //VOXEL SELECTION TEST
                chunkData->testVoxelHemisphereHighlightCount = 0;
                ivec3 min = fpt_calculateAABBVoxelCoordinates(trans.pos_in_chunk - (3 << 16), -FPT_HUNDREDTH);
                ivec3 max = fpt_calculateAABBVoxelCoordinates(trans.pos_in_chunk + (3 << 16) + FPT_HUNDREDTH);
                for(int z = min.z; z < max.z; z++){
                    fpt fptz = ((z - 31) << 16) - (1 << 15) - trans.pos_in_chunk.z;// 1 << 15 is .5 in fixed point
                    for(int y = min.y; y < max.y; y++){
                        fpt fpty = ((y - 31) << 16) - (1 << 15) - trans.pos_in_chunk.y;
                        for(int x = min.x; x < max.x; x++){
                            fpt fptx = ((x - 31) << 16) - (1 << 15) - trans.pos_in_chunk.x;
                            fpt_vec3 relPos = {fptx, fpty, fptz};
                            fpt dotProduct = fpt_vec3_dot(relPos, trans.forward);
                            fpt dist2 = fpt_vec3_dot(relPos,relPos);
                            float floatDotProduct = fpt2fl(dotProduct);
                            if(dotProduct > 0 && dist2 <= (5<<16)){
                                chunkData->testVoxelHemisphereHighlights[chunkData->testVoxelHemisphereHighlightCount++] = z * 4096 + y * 64 + x;
                            }
                        }
                    }
                }

                //update camera component if it exists
                uint16_t cameraIndex = ec.entityToCameraMap[entityID];
                if(cameraIndex != NULL_PLAYER){
                    CameraComp& cameraComp = ec.CameraComps[cameraIndex];
#if DEBUG_CAMERA
#else
                    update_cameraComp_position(GameState, entityID, cameraComp, trans);
#endif
                }

                //aabb.offset is either 0, or some offset within the current enlarged AABB
                fpt_vec3 aabbRelMin = aabb.min - aabb.offset + trans.pos_in_chunk;
                fpt_vec3 aabbRelMax = aabb.max - aabb.offset + trans.pos_in_chunk;

                ///*TAG*/COLLISION_DEBUG_PRINT_FPT_VEC3("aabbRelMin         : ", aabbRelMin);
                ///*TAG*/COLLISION_DEBUG_PRINT_FPT_VEC3("aabbRelMax         : ", aabbRelMax);

     
                ivec3 center_chunk_coords = trans.chunk_coords;
                // getIntersectingChunkCoordinates(chunkData, aabbRelMin, aabbRelMax);

                ivec3* intersectingchunk_coords = chunkData->intersectingchunk_coords;
                int index = 0;
                chunkData->intersectingChunkCount = 0;
                chunkData->intersectingChunkIndicesCount = 0;

                ivec3 chunkMin = calculateFPTChunkCoordinates(aabbRelMin);
                ivec3 chunkMax = calculateFPTChunkCoordinates(aabbRelMax);
                for (int x = chunkMin.x; x <= chunkMax.x; ++x) {
                    for (int y = chunkMin.y; y <= chunkMax.y; ++y) {
                        for (int z = chunkMin.z; z <= chunkMax.z; ++z) {
                            int pre_inc_index = index;
                            intersectingchunk_coords[index] = (ivec3_create(x, y, z) + center_chunk_coords);
                            chunkData->intersectingChunkCount++;
                            index++;
                            if(index >= MAX_CHUNKS){
                                //spdLOG(Warn,"too many intersected chunks, above MAX_CHUNKS");
                                continue;
                            }
                            uint32_t hashSlot = getChunkHash(chunkData, chunkData->intersectingchunk_coords[pre_inc_index]);
                            uint32_t newchunkID = findOrCreateChunk(chunkData, chunkData->intersectingchunk_coords[pre_inc_index], &GameState->WorldArena, false);
                            if(newchunkID != NULL_CHUNK){
                                chunkData->intersectingChunkIndices[chunkData->intersectingChunkIndicesCount] = newchunkID;
                                chunkData->intersectingChunkIndicesCount++;
            
                            }
                        }
                    }
                }
            // COLLISION_DEBUG_PRINTF("intersected chunk coords count: %d\n", chunkData->intersectingChunkIndicesCount); 
                for(int i = 0; i < chunkData->intersectingChunkIndicesCount; i++){
                    uint32_t chunkID = chunkData->intersectingChunkIndices[i];
                    if(!chunkData->safeToEdit[chunkID])continue;
                    ivec3 coords = chunkData->coords[chunkID];// + trans.chunk_coords;
                    // LOG(Info,"entity %s moving in chunk %d %d %d, chunkID: %u", getEntityName(ec, entityID), coords.x,coords.y,coords.z, chunkID);
                    //this loops through all currently intersected/active chunks
                    fpt_vec3 relative_pos = ivec_to_fpt_vec3(coords - center_chunk_coords) * FPT_CHUNK_SIZE;
                    vec3 debugRelPos = fpt_to_flt_vec3(relative_pos);
                    



                    bvh_tree& tree = chunkData->bvhTrees[chunkID];

                    //BVH logics
                    // /*TAG*///spdLOG(Info,"reinstering leaf node in bvh for chunkID: {}, intersecting chunks: {}", chunkID, chunkData->intersectingChunkCount);
                    // /*TAG*///spdLOG(Info,"relative position in chunk: {} {} {} : {} {} {}", coords.x,coords.y,coords.z, debugRelPos.x, debugRelPos.y, debugRelPos.z);
                    int result = ReinsertLeaf(tree, entityID, {aabbRelMin - relative_pos, aabbRelMax - relative_pos}, aabb.primitiveCount);
                    // COLLISION_DEBUG_PRINT_FPT_VEC3("bvh aabb min: ", aabbRelMin - relative_pos);
                    // COLLISION_DEBUG_PRINT_FPT_VEC3("bvh aabb max: ", aabbRelMax - relative_pos);
                    if(!tree.is_dirty){
                        tree.is_dirty = true;
                        chunkData->dirty_chunkIDs[chunkData->dirty_chunkID_count++] = chunkID;
                    }

                    if(result < 0){
                        //spdlog::error("problem reinserting entity into BVH?");
                    //   __debugbreak();
                    }
                    aabb.flags &= ~aabb_dirty;

                }//end of chunk for loop

                chunkData->chunkDifferenceCount = 0;
                // fpt_vec3 oldAABBMin = aabbRelMin - aabb.minDelta - aabb.posDelta;
                // fpt_vec3 oldAABBMax = aabbRelMax - aabb.maxDelta - aabb.posDelta;
                // fpt_vec3 previousPos = trans.pos_in_chunk - aabb.posDelta;
                fpt_vec3 previousPos = trans.pos_in_chunk - trans.move_delta;
                
                // COLLISION_DEBUG_PRINT_FPT_VEC3("pos in chunk  : ", trans.pos_in_chunk);
                // COLLISION_DEBUG_PRINT_FPT_VEC3("aabb delta pos: ", aabb.posDelta);
                // COLLISION_DEBUG_PRINT_FPT_VEC3("relative pos  : ", previousPos);
                // COLLISION_DEBUG_PRINT_FPT_VEC3("pos_in_chunk_delta pos  : ", trans.pos_in_chunk_delta);
                // print_glm_vec3_create("chunk coords delta: ", trans.chunk_coords_delta);
                
                ivec3 chunk_coord_offset = ivec3_create(0);
                bool inNewChunk = false;
                rebasePosition(previousPos, chunk_coord_offset, &inNewChunk,  chunkData->toroidal_space_enabled);
                // if(chunk_coord_offset != ivec3_create(0))COLLISION_DEBUG_PRINTF("chunk coords offset: %d %d %d\n", chunk_coord_offset.x,chunk_coord_offset.y,chunk_coord_offset.z);
                //NEED TO UPDATE PHYSIC S COMPONENT HERE
                uint32_t physicsIndex = ec.entityToPhysicsMap[entityID];
                if(physicsIndex != NULL_ENTITY){
                    // PRINT("CALCULATE DERIVED DATA HERE!\n");
                    // PhysicsComp& physicsComp = ec.PhysicsComps[physicsIndex];
                    // calculate_derived_data(physicsComp, trans);
                }
                // fpt_vec3 oldAABBMin = aabb.min - aabb.minDelta - aabb.posDelta + trans.pos_in_chunk;
                // fpt_vec3 oldAABBMax = aabb.max - aabb.maxDelta - aabb.posDelta + trans.pos_in_chunk;


                //aabb minDelta/maxDelta were used for tracking any changes to the aabb over time, currently disabled
                fpt_vec3 oldAABBMin = aabb.min - aabb.minDelta + previousPos;
                fpt_vec3 oldAABBMax = aabb.max - aabb.maxDelta + previousPos;

                // LOG(Info,"aabb min       : %6.2f %6.2f %6.2f", fpt2fl(aabb.min.x),fpt2fl(aabb.min.y),fpt2fl(aabb.min.z));
                // LOG(Info,"aabb min       : %6.2f %6.2f %6.2f", fpt2fl(aabb.max.x),fpt2fl(aabb.max.y),fpt2fl(aabb.max.z));

                // LOG(Info,"aabb.minDelta  : %6.2f %6.2f %6.2f", fpt2fl(aabb.minDelta.x),fpt2fl(aabb.minDelta.y),fpt2fl(aabb.minDelta.z));
                // LOG(Info,"aabb.maxDelta  : %6.2f %6.2f %6.2f", fpt2fl(aabb.maxDelta.x),fpt2fl(aabb.maxDelta.y),fpt2fl(aabb.maxDelta.z));
                // LOG(Info,"previousPos    : %6.2f %6.2f %6.2f", fpt2fl(previousPos.x),fpt2fl(previousPos.y),fpt2fl(previousPos.z));
                vec3 debugMin = fpt_to_flt_vec3(oldAABBMin);
                vec3 debugMax = fpt_to_flt_vec3(oldAABBMax); 
                vec3 debugPreviousPos = fpt_to_flt_vec3(previousPos); 
                
                //test debug logic for yet more rebasing hell
                // LOG(Info,"oldAABBMin  : %6.2f %6.2f %6.2f", debugMin.x,debugMin.y,debugMin.z);
                // LOG(Info,"oldAABBMax  : %6.2f %6.2f %6.2f", debugMax.x,debugMax.y,debugMax.z);
                // LOG(Info,"previousPos : %6.2f %6.2f %6.2f", debugPreviousPos.x,debugPreviousPos.y,debugPreviousPos.z);
                // LOG(Info,"chunk_offset: %6d %6d %6d", chunk_coord_offset.x,chunk_coord_offset.y,chunk_coord_offset.z);
                // LOG(Info,"new coords  : %6d %6d %6d", trans.chunk_coords.x, trans.chunk_coords.y, trans.chunk_coords.z);
                
                index = 0;
                //Loggers::chunk_logger->trace("getting intersecting chunk coordinates, initializing vector to return");
                // Calculate chunk coordinates for min and max
                chunkMin = calculateFPTChunkCoordinates(oldAABBMin);
                //Loggers::chunk_logger->trace("calculated min chinfounk coordinates");
                chunkMax = calculateFPTChunkCoordinates(oldAABBMax);
                //Loggers::chunk_logger->trace("calculated max chunk coordinates");
                // LOG(Info,"chunkMin    : %6d %6d %6d", chunkMin.x,chunkMin.y,chunkMin.z);
                // LOG(Info,"chunkMax    : %6d %6d %6d", chunkMax.x,chunkMax.y,chunkMax.z);
                
                // Iterate through all chunks that the AABB spans
                for (int x = chunkMin.x; x <= chunkMax.x; ++x) {
                    for (int y = chunkMin.y; y <= chunkMax.y; ++y) {
                        for (int z = chunkMin.z; z <= chunkMax.z; ++z) {
                            ivec3 oldchunk_coords = ivec3_create(x,y,z) + (trans.chunk_coords + chunk_coord_offset);
                            ///*TAG*/COLLISION_DEBUG_PRINTF("old chunk coords        : %d %d %d\n", oldchunk_coords.x,oldchunk_coords.y,oldchunk_coords.z);
                            bool found = false;
                            for(int i = 0; i < chunkData->intersectingChunkCount; i++){
                                if(oldchunk_coords == chunkData->intersectingchunk_coords[i]){
                                    found = true;
                                    break;
                                }
                            }
                            if(!found){
                                // LOG(Info,"removing entity {} from old chunk coords: {} {} {}", entityID, oldchunk_coords.x, oldchunk_coords.y, oldchunk_coords.z);
                                uint32_t chunkID = findOrCreateChunk(chunkData, oldchunk_coords , &GameState->WorldArena, false);
                                if(chunkID == NULL_CHUNK){
                                    //spdlog::error("trying to remove entity {} from null chunk", entityID);
                                    break;
                                }
                                bvh_tree& tree = chunkData->bvhTrees[chunkID];

                                if(!removeLeafNode(tree, entityID)){
                                    //spdlog::error("couldnt remove entity {} from chunkID {}, coords: {} {} {}", entityID, chunkID, oldchunk_coords.x, oldchunk_coords.y, oldchunk_coords.z);

                                }
                            }
                        }
                    }
                }
                trans.chunk_coords_delta = ivec3_create(0);
                
                //only reset the flag if its been processed in an actual chunk
                // ec.aabbFlags[i] &= ~aabb_dirty;
                // COLLISION_DEBUG_PRINTF("entityID %d aabbFlags RESET, tick: %u\n", entityID, GameState->tick);

            }//end of if AabbComp.isDirty


        }//end of entity for loop
      
        //broad phase collision check
        bool collisions = false;
        // COLLISION_DEBUG_PRINTF("dirty chunk count: %d, first %d, second %d, third %d\n", chunkData->dirty_chunkID_count, chunkData->dirty_chunkIDs[0], chunkData->dirty_chunkIDs[1], chunkData->dirty_chunkIDs[2]);
        int i = 0;
        uint32_t chunkID = chunkData->dirty_chunkIDs[i];
        if(chunkData->dirty_chunkID_count > 1){
            int debug = 0;
        }
        if(chunkData->dirty_chunkID_count > 0){
            while(chunkID != NULL_CHUNK && i < MAX_CHUNKS)
            // for (int i = 0; i < chunkData->dirty_chunkID_count; i++)
            {
                // COLLISION_DEBUG_PRINTF("processing dirty chunkID: %d\n", chunkID);
                assert(chunkID != NULL_CHUNK && "DIRTY CHUNK IS NULL??");
                chunkData->dirty_chunkIDs[i] = NULL_CHUNK;
                bvh_tree& tree = chunkData->bvhTrees[chunkID];
                
                entity_interaction_data* entity_interactions = chunkData->entity_interactions[chunkID]; 
                memset(entity_interactions, 0, sizeof(entity_interaction_data) * MAX_ENTITY_INTERACTIONS);
                int& interaction_count = chunkData->entity_interactions_count[chunkID];
                interaction_count = 0;

                if(tree.is_dirty){
                    start_broad_phase_collision_check(GameState, tree, chunkID);
                    tree.is_dirty = false;
                }
                
                //pure slop to clear out the nodes for fine phase actual colliding drawing
                tree.colliding_node_count = 0;
                //fine phase entity collision
                for(int j = 0; j < tree.entity_collision_count; j++){
                    uint16_t entityID1 = tree.entity_collisions[j].entityID;
                    uint16_t entityID2 = tree.entity_collisions[j].other_entityID;
                    uint16_t physicsIndex1 = GameState->entityComponent->entityToPhysicsMap[entityID1];
                    uint16_t physicsIndex2 = GameState->entityComponent->entityToPhysicsMap[entityID2];
                    

                    //entity pickup interaction logic
                    uint16_t dataIndex1 = GameState->entityComponent->entityToDataMap[entityID1];
                    uint16_t dataIndex2 = GameState->entityComponent->entityToDataMap[entityID2];
                    if(dataIndex1 != NULL_ENTITY && dataIndex2 != NULL_ENTITY){
                        if(interaction_count >= MAX_ENTITY_INTERACTIONS)PRINT("TOO MANY ENTITY INTERACTIONS IN CHUNKID %u!", chunkID);
                        else{

                            DataComp& dataComp1 = GameState->entityComponent->DataComps[dataIndex1]; 
                            DataComp& dataComp2 = GameState->entityComponent->DataComps[dataIndex2]; 
                                 if(dataComp1.type == entity_actor && dataComp2.type == entity_item){
                                entity_interactions[interaction_count].interacting_entityID     = entityID1;
                                entity_interactions[interaction_count].interactable_entityID    = entityID2;
                                entity_interactions[interaction_count].interacting_versionID    = GameState->entityComponent->versionIDs[entityID1];
                                entity_interactions[interaction_count].interactable_versionID   = GameState->entityComponent->versionIDs[entityID2];
                                entity_interactions[interaction_count].interacting_type         = dataComp1.type;
                                entity_interactions[interaction_count].interactable_type        = dataComp2.type;
                                dataComp1.entity_interaction_lookup_chunkID = chunkID;
                                dataComp1.entity_interaction_lookup_index   = interaction_count;
                                interaction_count++;
                            
                            }
                            else if(dataComp2.type == entity_actor && dataComp1.type == entity_item){
                                entity_interactions[interaction_count].interacting_entityID     =  entityID2;
                                entity_interactions[interaction_count].interactable_entityID    =  entityID1; 
                                entity_interactions[interaction_count].interacting_versionID    =  GameState->entityComponent->versionIDs[entityID2]; 
                                entity_interactions[interaction_count].interactable_versionID   =  GameState->entityComponent->versionIDs[entityID1];
                                entity_interactions[interaction_count].interacting_type         =  dataComp2.type;
                                entity_interactions[interaction_count].interactable_type        =  dataComp1.type;
                                dataComp2.entity_interaction_lookup_chunkID = chunkID;
                                dataComp2.entity_interaction_lookup_index   = interaction_count;
                                interaction_count++;
                            }                        
                            tree.colliding_node_ids[tree.colliding_node_count++] = tree.entityToNodeID[entityID1];
                            tree.colliding_node_ids[tree.colliding_node_count++] = tree.entityToNodeID[entityID2];
                            collisions = true;

                        }
                    
                    }

                    // uint16_t transIndex1 = GameState->entityComponent->entityToTransMap[entityID1];
                    // uint16_t transIndex2 = GameState->entityComponent->entityToTransMap[entityID2];
                    // TransComp& trans_one = GameState->entityComponent->TransComps[transIndex1];
                    // TransComp& trans_two = GameState->entityComponent->TransComps[transIndex2];
                    PhysicsComp& one = GameState->entityComponent->PhysicsComps[physicsIndex1];
                    PhysicsComp& two = GameState->entityComponent->PhysicsComps[physicsIndex2];
                    // /*TAG*/COLLISION_DEBUG_PRINTF("BOX ONE: pos: %3.3f %3.3f %3.3f, half_size: %3.3f %3.3f %3.3f\n", fpt2fl(trans_one.pos_in_chunk.x),fpt2fl(trans_one.pos_in_chunk.y),fpt2fl(trans_one.pos_in_chunk.z),fpt2fl(one.half_size.x),fpt2fl(one.half_size.y),fpt2fl(one.half_size.z));
                    // /*TAG*/COLLISION_DEBUG_PRINTF("BOX TWO: pos: %3.3f %3.3f %3.3f, half_size: %3.3f %3.3f %3.3f\n", fpt2fl(trans_two.pos_in_chunk.x),fpt2fl(trans_two.pos_in_chunk.y),fpt2fl(trans_two.pos_in_chunk.z),fpt2fl(two.half_size.x),fpt2fl(two.half_size.y),fpt2fl(two.half_size.z));
                    // PRINT("BOX AND BOX PHYSICS HERE! %d\n", __LINE__);
                    // if(box_and_box(GameState, one, two, entityID1, entityID2)){
                        //if they do collide, get the nodes and add them to the array to draw
                        //we dont yet store any of the collision data
                        // tree.colliding_node_ids[tree.colliding_node_count++] = tree.entityToNodeID[entityID1];
                        // tree.colliding_node_ids[tree.colliding_node_count++] = tree.entityToNodeID[entityID2];
                        // collisions = true;
                    // }
    
                }
                Brickmap64* bm = &chunkData->brickmaps[chunkID];
                if(bm){
                    //fine phase voxel collision
                    fpt_vec3 bmPos =  fpt_vec3_create(0);
                    fpt_vec3 bmPos_minCorner =  fpt_vec3_create(-FPT_HALF_CHUNK_SIZE);
                    chunkData->intersected_voxel_count = 0; 

                    for(int j = 0; j < tree.entity_voxel_collision_count; j++){
                        uint16_t entityID = tree.entity_voxel_collisions[j].entityID;
                        
                        //for this chunk, if we've already processed this entity, lets ignore it so we don't do similar work
                        //coarse grid ID is only for the broad phase, in this collision phase we process all the voxels from the whole chunk
                        //so if the entity spans 3 coarse grid cells, then the first pass would resolve all possible collisions, other 2 would duplicate the work
                        //use some flag to detect if the entity has already been processed in this chunk?

                        uint16_t gridID = tree.entity_voxel_collisions[j].coarse_grid_id;
                        uint16_t physicsIndex = GameState->entityComponent->entityToPhysicsMap[entityID];
                        uint16_t aabbIndex = GameState->entityComponent->entityToAabbMap[entityID];
                        uint16_t transIndex = GameState->entityComponent->entityToTransMap[entityID];
                        uint16_t stateIndex = GameState->entityComponent->entityToStateMap[entityID];

                        //physicsComp used to be called 'one' to denote it as the first rigidbody in the collision
                        PhysicsComp* phys = GameState->entityComponent->PhysicsComps + physicsIndex;
                        StateComp* state = GameState->entityComponent->StateComps + stateIndex;

                        AabbComp& aabb = GameState->entityComponent->AabbComps[aabbIndex];
                        TransComp& trans = GameState->entityComponent->TransComps[transIndex];
                        //get range of voxels the entity spans
                        // COLLISION_DEBUG_PRINTF("entityID: %d, coarse grid id: %d\n", entityID, gridID);
                        ivec3 coords = chunkData->coords[chunkID];
                        // //spdLOG(Info,"COLLISION RESOLUTION STEP, GETTING ALL INTERSECTING CHUNK COORDS! chunkID: {}, coords: {} {} {}\n", chunkID, coords.x, coords.y, coords.z);
                        fpt_vec3 adjusted_pos_in_chunk = pos_in_chunk_offset(trans.pos_in_chunk, trans.chunk_coords, coords);
                        
                        COLLISION_DEBUG_PRINT_FPT_VEC3("entity pos         :", adjusted_pos_in_chunk); 
                        COLLISION_DEBUG_PRINT_FPT_VEC3("aabb min           :", aabb.min + adjusted_pos_in_chunk); 
                        COLLISION_DEBUG_PRINT_FPT_VEC3("aabb max           :", aabb.max + adjusted_pos_in_chunk); 


                        fpt yPos_without_movement_for_stepup = fpt_sub(fpt_add(aabb.min.y , adjusted_pos_in_chunk.y) , trans.desired_movement.y);
                        fpt potential_step_y = fpt_sub(fpt_add(fpt_floor(fpt_add(yPos_without_movement_for_stepup , FPT_ONE)) , FPT_HUNDREDTH) , yPos_without_movement_for_stepup);


                        ivec3 voxCoordsMin = fpt_calculateAABBVoxelCoordinates(aabb.min + adjusted_pos_in_chunk, -FPT_HUNDREDTH);
                        ivec3 voxCoordsMax = fpt_calculateAABBVoxelCoordinates(aabb.max + adjusted_pos_in_chunk + fpt_vec3_create(0,potential_step_y, 0),  FPT_HUNDREDTH);

                        // ivec3 PADDEDvoxCoordsMin = fpt_calculatePaddedVoxelCoordinates(aabb.min + adjusted_pos_in_chunk, -FPT_HUNDREDTH);
                        // ivec3 PADDEDvoxCoordsMax = fpt_calculatePaddedVoxelCoordinates(aabb.max + adjusted_pos_in_chunk,  FPT_HUNDREDTH);

                        // COLLISION_DEBUG_PRINTF("PADDED     vox coords min: %d %d %d , coarse grid min: %d %d %d\n", PADDEDvoxCoordsMin.x,PADDEDvoxCoordsMin.y,PADDEDvoxCoordsMin.z, PADDEDvoxCoordsMin.x/8,PADDEDvoxCoordsMin.y/8,PADDEDvoxCoordsMin.z/8);
                        // COLLISION_DEBUG_PRINTF("PADDED     vox coords max: %d %d %d , coarse grid max: %d %d %d\n", PADDEDvoxCoordsMax.x,PADDEDvoxCoordsMax.y,PADDEDvoxCoordsMax.z, PADDEDvoxCoordsMax.x/8,PADDEDvoxCoordsMax.y/8,PADDEDvoxCoordsMax.z/8);

                        // ivec3 NON_PADDED_voxCoordsMin = fpt_calculateVoxelCoordinates(aabb.min + adjusted_pos_in_chunk, -FPT_HUNDREDTH);
                        // ivec3 NON_PADDED_voxCoordsMax = fpt_calculateVoxelCoordinates(aabb.max + adjusted_pos_in_chunk,  FPT_HUNDREDTH);
                        // COLLISION_DEBUG_PRINTF("NON PADDED vox coords min: %d %d %d , coarse grid min: %d %d %d\n", NON_PADDED_voxCoordsMin.x,NON_PADDED_voxCoordsMin.y,NON_PADDED_voxCoordsMin.z, NON_PADDED_voxCoordsMin.x/8,NON_PADDED_voxCoordsMin.y/8,NON_PADDED_voxCoordsMin.z/8);
                        // COLLISION_DEBUG_PRINTF("NON PADDED vox coords max: %d %d %d , coarse grid max: %d %d %d\n", NON_PADDED_voxCoordsMax.x,NON_PADDED_voxCoordsMax.y,NON_PADDED_voxCoordsMax.z, NON_PADDED_voxCoordsMax.x/8,NON_PADDED_voxCoordsMax.y/8,NON_PADDED_voxCoordsMax.z/8);


                        bool voxel_collision = false;
                        fpt_vec3 closest_vox_pos = fpt_vec3_create(0);
                        fpt distance = 655360000;//10000 in fixed point
                        fpt_vec3 normal_direction = fpt_vec3_create(0);
                        uint32_t closest_voxel_index = 0;

                        fpt epsilon = fl2fpt(0.01f);//tiny epsilon for additional offset, to prevent boxes from directly overlapping on resolution
                        // fpt epsilon = FPT_ONE;//tiny epsilon for additional offset, to prevent boxes from directly overlapping on resolution
                        // COLLISION_DEBUG_PRINT_FPT_VEC3("entity min START: ", aabb.min + adjusted_pos_in_chunk - trans.desired_movement);
                        // COLLISION_DEBUG_PRINT_FPT_VEC3("entity max START: ", aabb.max + adjusted_pos_in_chunk - trans.desired_movement);
                        // COLLISION_DEBUG_PRINT_FPT_VEC3("entity min MOVED: ", aabb.min + trans.pos_in_chunk);
                        // COLLISION_DEBUG_PRINT_FPT_VEC3("entity max MOVED: ", aabb.max + trans.pos_in_chunk);
                        // COLLISION_DEBUG_PRINT_FPT_VEC3("movement  : ", trans.desired_movement);
                        
                        //this will be how we track which coordinate, x y z of the voxels to associate with a normal response
                        //the axis components will match the normal response
                        // ivec3 collided_vox_coords = ivec3_create(0);
                        fpt_vec3 total_penetration_resolution = fpt_vec3_create(0);
                        fpt_vec3 total_penetration_resolution_cached = fpt_vec3_create(0);
                        //do we need additional fields just in case?
                        //voxel positions that gave us the largest penetration resolution for each axis
                        // ivec3 x_vox_coords = ivec3_create(0);
                        // ivec3 y_vox_coords = ivec3_create(0);
                        // ivec3 z_vox_coords = ivec3_create(0);
                        
                        fpt smallest_entry_time_x = FPT_ONE;
                        fpt smallest_entry_time_y = FPT_ONE;
                        fpt smallest_entry_time_z = FPT_ONE;


                        fpt_vec3 start_min = aabb.min + adjusted_pos_in_chunk - trans.desired_movement;
                        fpt_vec3 start_max = aabb.max + adjusted_pos_in_chunk - trans.desired_movement;
                        COLLISION_DEBUG_PRINT_FPT_VEC3("start_min: ", start_min);
                        COLLISION_DEBUG_PRINT_FPT_VEC3("start_max: ", start_max);
                        fpt_vec3 stop_min_x = start_min + fpt_vec3_create(trans.desired_movement.x, 0, 0);
                        fpt_vec3 stop_max_x = start_max + fpt_vec3_create(trans.desired_movement.x, 0, 0);

                        //used only as the max for the aabb intersect test
                        //imagine a smaller box above the players head that we use to determine voxel step up collisions. this is that box, doesnt seem to help
                        //because we already expand the possible intersecting voxels with the potential y step amount
                        fpt_vec3 stop_max_x_with_y_step_check = start_max + fpt_vec3_create(trans.desired_movement.x, potential_step_y, 0);
                        fpt_vec3 start_min_x_with_y_step_check = fpt_vec3_create(start_min.x, start_max.y, start_min.z);
                        

                        bool stepX = true;
                        COLLISION_DEBUG_PRINT_FPT_VEC3("stop_min_x: ", stop_min_x);
                        COLLISION_DEBUG_PRINT_FPT_VEC3("stop_max_x: ", stop_max_x);
                        COLLISION_DEBUG_PRINTF("X PASS!, total collision resolution: %10.5f %10.5f %10.5f, vox coords min: %d %d %d, vox coords max: %d %d %d\n", fpt2fl(total_penetration_resolution.x),fpt2fl(total_penetration_resolution.y), fpt2fl(total_penetration_resolution.z), voxCoordsMin.x, voxCoordsMin.y, voxCoordsMin.z, voxCoordsMax.x,voxCoordsMax.y,voxCoordsMax.z );

                        //can optimize this by storing the coordinates of all active voxels and looping through those instead of 3 triple nested loops
                        //X PASS
                        if(trans.desired_movement.x){
                            for(int x = voxCoordsMin.x; x <= voxCoordsMax.x; x++){
                                for(int y = voxCoordsMin.y; y <= voxCoordsMax.y; y++){
                                    for(int z = voxCoordsMin.z; z <= voxCoordsMax.z; z++){
    
                                        uint32_t voxel_index = x + (y * 64) + (z * 4096);
                                        if(bm->voxels[voxel_index]){
                                            fpt_vec3 voxPos = fpt_getVoxelWorldPosition(ivec3_create(x,y,z), bmPos_minCorner);
    
                                            //early out to skip the collision if we don't intersect with it in the final position, does this even work? maybe...
                                            
                                            //hard to tell if this extra check actually helps or not
                                            //the idea is that we cast a smaller box over the players head that we check for intersections along this axis with as well
                                            //to prevent stepping up into voxels we would collide with
                                            // stepX = stepX && !aabbIntersectTest(start_min_x_with_y_step_check, stop_max_x_with_y_step_check, voxPos - FPT_HALF_VOXEL_SCALE, voxPos + FPT_HALF_VOXEL_SCALE);
                                            if((!aabbIntersectTest(stop_min_x, stop_max_x, voxPos - FPT_HALF_VOXEL_SCALE, voxPos + FPT_HALF_VOXEL_SCALE)))continue;
                                            fpt_vec3 penetration = fpt_vec3_create(0);
                                            fpt_vec3 vox_normal = fpt_vec3_create(0);
                                            COLLISION_DEBUG_PRINTF("collides with vox: %d %d %d\n", x,y,z);
                                            fpt dist = swept_aabb(start_min, start_max, voxPos - FPT_HALF_VOXEL_SCALE, voxPos + FPT_HALF_VOXEL_SCALE, fpt_vec3_create(trans.desired_movement.x,0, 0), vox_normal);
                                            if(dist < FPT_ONE){
                                                if(vox_normal.x){
                                                    //there is a collision, check the height of the box and see if its too large to step over
                                                    
                                                    if(fpt_add(voxPos.y, FPT_HALF) > fpt_add(yPos_without_movement_for_stepup, FPT_ONE)){
                                                        COLLISION_DEBUG_PRINTF("collided in x pass with voxel %d %d %d, min character height: %d %d %d\n", x,y,z,voxCoordsMin.x,voxCoordsMin.y,voxCoordsMin.z);
                                                        
                                                        COLLISION_DEBUG_PRINT_FPT_VEC3("voxPos MAX         :", voxPos + FPT_HALF);
                                                        COLLISION_DEBUG_PRINT_FPT_VEC3("aabbMin            :", aabb.min + adjusted_pos_in_chunk);
                                                        stepX = false;//voxel height is too large, can't step
                                                    }
    
                                                    if(dist < smallest_entry_time_x){
                                                        //this voxel shares a coordinate with another penetration resolution step
                                                        // if((collided_vox_coords.y == y && dist >= smallest_entry_time_y) || (collided_vox_coords.z == z && dist >= smallest_entry_time_z))continue; //ignore this collision, we already have a better collision with its other component
                                                        voxel_collision = true;
                                                        // print_fpt("smallest dist X: ", dist);
                                                        chunkData->intersected_voxel_positions[chunkData->intersected_voxel_count++] = fpt_to_flt_vec3(voxPos); 
                                                        smallest_entry_time_x = dist;
                                                        total_penetration_resolution.x = fpt_mul(FPT_ONE , fpt_mul(-trans.desired_movement.x , fpt_add((fpt_sub(FPT_ONE , dist)) , epsilon)));
                                                        // //spdLOG(Info,"latest total penetration resolution x: {}", fpt2fl(total_penetration_resolution.x));
                                                        // total_penetration_resolution.x = FPT_ONE * (-trans.desired_movement.x * ((FPT_ONE - smallest_entry_time_x) + epsilon));
                                                        int fuckthedebugger = 0;
                                                    }
                                                }
                                            }
                                        }
        
                                    }
                                }
                            }
    
                        }
                       
                        if(!total_penetration_resolution.x)stepX = false;
                        total_penetration_resolution_cached.x = total_penetration_resolution.x;//store it in case we need to undo the step

                        total_penetration_resolution.x = stepX ? 0 : total_penetration_resolution.x;
                        fpt_vec3 start_min_z = stop_min_x + fpt_vec3_create(total_penetration_resolution.x, 0, 0) ;
                        fpt_vec3 start_max_z = stop_max_x + fpt_vec3_create(total_penetration_resolution.x, 0, 0) ;
                        fpt_vec3 stop_min_xz = start_min_z +  fpt_vec3_create(0, 0, trans.desired_movement.z) ;
                        fpt_vec3 stop_max_xz = start_max_z +  fpt_vec3_create(0, 0, trans.desired_movement.z) ;
                        fpt_vec3 stop_max_xz_with_y_step_check = start_max_z + fpt_vec3_create(0, potential_step_y, trans.desired_movement.z);
                        fpt_vec3 start_min_xz_with_y_step_check = fpt_vec3_create(start_min_z.x, start_max_z.y, start_min_z.z);

                        bool stepZ = true;
                        COLLISION_DEBUG_PRINT_FPT_VEC3("start_min_z: ",  start_min_z);
                        COLLISION_DEBUG_PRINT_FPT_VEC3("start_max_z: ",  start_max_z);
                        COLLISION_DEBUG_PRINT_FPT_VEC3("stop_min_xz: ",  stop_min_xz);
                        COLLISION_DEBUG_PRINT_FPT_VEC3("stop_min_xz: ",  stop_max_xz);
                           //Z PASS
                        COLLISION_DEBUG_PRINTF("Z PASS!, total collision resolution: %10.5f %10.5f %10.5f\n", fpt2fl(total_penetration_resolution.x),fpt2fl(total_penetration_resolution.y), fpt2fl(total_penetration_resolution.z) );
                        if(trans.desired_movement.z){
                            for(int x = voxCoordsMin.x; x <= voxCoordsMax.x; x++){
                                for(int y = voxCoordsMin.y; y <= voxCoordsMax.y; y++){
                                    for(int z = voxCoordsMin.z; z <= voxCoordsMax.z; z++){
    
                                        uint32_t voxel_index = x + (y * 64) + (z * 4096);
                                        if(bm->voxels[voxel_index]){
                                            fpt_vec3 voxPos = fpt_getVoxelWorldPosition(ivec3_create(x,y,z), bmPos_minCorner);
    
                                            // early out to skip the collision if we don't intersect with it in the final position, does this even work? maybe...
                                            // stepZ = stepZ && !aabbIntersectTest(start_min_xz_with_y_step_check, stop_max_xz_with_y_step_check, voxPos - FPT_HALF_VOXEL_SCALE, voxPos + FPT_HALF_VOXEL_SCALE);
                                            if((!aabbIntersectTest(stop_min_xz, stop_max_xz, voxPos - FPT_HALF_VOXEL_SCALE, voxPos + FPT_HALF_VOXEL_SCALE)))continue;
                                            fpt_vec3 penetration = fpt_vec3_create(0);
                                            fpt_vec3 vox_normal = fpt_vec3_create(0);
                                            // COLLISION_DEBUG_PRINTF("collides with vox: %d %d %d\n", x,y,z);
                                            fpt dist = swept_aabb(start_min_z, start_max_z, voxPos - FPT_HALF_VOXEL_SCALE, voxPos + FPT_HALF_VOXEL_SCALE, fpt_vec3_create(0 ,0, trans.desired_movement.z), vox_normal);
                                            if(dist < FPT_ONE){
                                                if(vox_normal.z){
                                                    //there is a collision, check the height of the box and see if its too large to step over
                                                    // if(y > voxCoordsMin.y){_
                                                    if(fpt_add(voxPos.y, FPT_HALF) > fpt_add(yPos_without_movement_for_stepup, FPT_ONE)){

                                                        COLLISION_DEBUG_PRINTF("collided in z pass with voxel %d %d %d, min character height: %d %d %d\n", x,y,z,voxCoordsMin.x,voxCoordsMin.y,voxCoordsMin.z);
                                                        stepZ = false;//voxel height is too large, can't step
                                                    }
                                                    if(dist < smallest_entry_time_z){
                                                        //this voxel shares a coordinate with another penetration resolution step
                                                        // if((collided_vox_coords.y == y && dist >= smallest_entry_time_y) || (collided_vox_coords.z == z && dist >= smallest_entry_time_z))continue; //ignore this collision, we already have a better collision with its other component
                                                        voxel_collision = true;
                                                        // print_fpt("smallest dist X: ", dist);
                                                        chunkData->intersected_voxel_positions[chunkData->intersected_voxel_count++] = fpt_to_flt_vec3(voxPos); 
                                                        smallest_entry_time_z = dist;
                                                        total_penetration_resolution.z = fpt_mul(FPT_ONE , fpt_mul(-trans.desired_movement.z , fpt_add((fpt_sub(FPT_ONE , dist)) , epsilon)));
                                                        // //spdLOG(Info,"latest total penetration resolution z: {}", fpt2fl(total_penetration_resolution.z));
                                                        // total_penetration_resolution.x = FPT_ONE * (-trans.desired_movement.x * ((FPT_ONE - smallest_entry_time_x) + epsilon));
                                                        int fuckthedebugger = 0;
                                                    }
                                                }
                                            }                                  
                                        }
        
                                    }
                                }
                            }
                        }
                       

                        if(!total_penetration_resolution.z)stepZ = false;
                        total_penetration_resolution_cached.z = total_penetration_resolution.z;//store it in case we need to undo the step

                        total_penetration_resolution.z = stepZ && !stepX ? 0 : total_penetration_resolution.z;
                        int original_y_vox_max = voxCoordsMax.y;
                        fpt_vec3 start_min_y = stop_min_xz + fpt_vec3_create(0, 0, total_penetration_resolution.z);
                        fpt_vec3 start_max_y = stop_max_xz + fpt_vec3_create(0, 0, total_penetration_resolution.z);
                        COLLISION_DEBUG_PRINT_FPT_VEC3("start_min_y: ", start_min_y);
                        COLLISION_DEBUG_PRINT_FPT_VEC3("start_max_y: ", start_max_y);
                        fpt desired_movement_y = trans.desired_movement.y;// + stepX || stepZ ? FPT_ONE : 0;
                        bool stepY = false;
                        bool stepY_failed = false;

                        if(stepZ || stepX){
                            stepY = true;
                            //add the desired movement in the +Y direction for the final collision step
                            //if we detect a collision we undo everything and apply the actual resolution
                            //if it works, we apply a final resolution of the +Y amount
                            //step up to next voxel height
                            // desired_movement_y = fpt_sub(fpt_add(fpt_floor(fpt_add(yPos_without_movement_for_stepup , FPT_ONE)) , FPT_HUNDREDTH) , yPos_without_movement_for_stepup);
                            desired_movement_y = potential_step_y;
                            
                            total_penetration_resolution.y = desired_movement_y - trans.desired_movement.y;
                            COLLISION_DEBUG_PRINTF("STEPPING UP, tick: %u", GameState->tick);

                            COLLISION_DEBUG_PRINTF("stepping up, up movement: %10.5f + original desired movement: %10.5f = %10.5f\n", fpt2fl(total_penetration_resolution.y),fpt2fl(trans.desired_movement.y), fpt2fl(fpt_add(total_penetration_resolution.y,trans.desired_movement.y)));
                            if(voxCoordsMax.y < PADDED_VOXELS)voxCoordsMax.y++;
                            

                        }
                        fpt_vec3 stop_min_xzy = start_min_y + fpt_vec3_create(0, desired_movement_y, 0);
                        fpt_vec3 stop_max_xzy = start_max_y + fpt_vec3_create(0, desired_movement_y, 0);
                        COLLISION_DEBUG_PRINT_FPT_VEC3("stop_min_xzy: ", stop_min_xzy);
                        COLLISION_DEBUG_PRINT_FPT_VEC3("stop_max_xzy: ", stop_max_xzy);
                        // COLLISION_DEBUG_PRINTF("Y PASS!, total collision resolution: %10.5f %10.5f %10.5f\n", fpt2fl(total_penetration_resolution.x),fpt2fl(total_penetration_resolution.y), fpt2fl(total_penetration_resolution.z) );
                        //Y PASS
                        // if(trans.desired_movement.y){
                        if(desired_movement_y){
                            for(int x = voxCoordsMin.x; x <= voxCoordsMax.x; x++){
                                for(int y = voxCoordsMin.y; y <= voxCoordsMax.y; y++){
                                    for(int z = voxCoordsMin.z; z <= voxCoordsMax.z; z++){
    
                                        uint32_t voxel_index = x + (y * 64) + (z * 4096);
                                        if(bm->voxels[voxel_index]){
                                            fpt_vec3 voxPos = fpt_getVoxelWorldPosition(ivec3_create(x,y,z), bmPos_minCorner);
    
                                            //early out to skip the collision if we don't intersect with it in the final position, does this even work? maybe...
                                            if((!aabbIntersectTest(stop_min_xzy, stop_max_xzy, voxPos - FPT_HALF_VOXEL_SCALE, voxPos + FPT_HALF_VOXEL_SCALE)))continue;
                                            fpt_vec3 penetration = fpt_vec3_create(0);
                                            fpt_vec3 vox_normal = fpt_vec3_create(0);
                                            COLLISION_DEBUG_PRINTF("collides with vox: %d %d %d\n", x,y,z);
                                            fpt dist = swept_aabb(start_min_y, start_max_y, voxPos - FPT_HALF_VOXEL_SCALE, voxPos + FPT_HALF_VOXEL_SCALE, fpt_vec3_create(0, desired_movement_y, 0), vox_normal);
                                            if(dist < FPT_ONE){
                                                if(vox_normal.y){
                                                    if(dist < smallest_entry_time_y){
                                                        //this voxel shares a coordinate with another penetration resolution step
                                                        // if((collided_vox_coords.y == y && dist >= smallest_entry_time_y) || (collided_vox_coords.z == z && dist >= smallest_entry_time_z))continue; //ignore this collision, we already have a better collision with its other component
                                                        voxel_collision = true;
                                                        stepY_failed = true;//we cant step up, break out and recalculate everything
                                                        // break;//breaking out doesnt work in a triple loop
                                                        // print_fpt("smallest dist X: ", dist);
                                                        chunkData->intersected_voxel_positions[chunkData->intersected_voxel_count++] = fpt_to_flt_vec3(voxPos); 
                                                        smallest_entry_time_y = dist;
                                                        total_penetration_resolution.y = fpt_mul(FPT_ONE , fpt_mul(-trans.desired_movement.y , fpt_add((fpt_sub(FPT_ONE , dist)) , epsilon)));
                                                        // //spdLOG(Info,"latest total penetration resolution y: {}", fpt2fl(total_penetration_resolution.y));
                                                        // total_penetration_resolution.x = FPT_ONE * (-trans.desired_movement.x * ((FPT_ONE - smallest_entry_time_x) + epsilon));
                                                        int fuckthedebugger = 0;
                                                    }
                                                }
                                            }                                    
                                        }
        
                                    }
                                }
                            }
      
                            if(stepY && stepY_failed){
                                //if it fails lets just do the entire calculation over again fuck it
                                fpt_vec3 start_min = aabb.min + adjusted_pos_in_chunk - trans.desired_movement;
                                fpt_vec3 start_max = aabb.max + adjusted_pos_in_chunk - trans.desired_movement;
                                voxCoordsMax.y = original_y_vox_max;//set them back if the step up failed
                                fpt_vec3 stop_min_x = start_min + fpt_vec3_create(trans.desired_movement.x, 0, 0);
                                fpt_vec3 stop_max_x = start_max + fpt_vec3_create(trans.desired_movement.x, 0, 0);
                                // COLLISION_DEBUG_PRINT_FPT_VEC3("stop_min_x: ", stop_min_x);
                                // COLLISION_DEBUG_PRINT_FPT_VEC3("stop_max_x: ", stop_max_x);
                                // COLLISION_DEBUG_PRINTF("X PASS!, total collision resolution: %10.5f %10.5f %10.5f\n", fpt2fl(total_penetration_resolution.x),fpt2fl(total_penetration_resolution.y), fpt2fl(total_penetration_resolution.z) );
                                //apparently we can skip the x/z checks if we restore the original collision resolution we ignored if we thought we could step
                                total_penetration_resolution.x = total_penetration_resolution_cached.x ;
                                total_penetration_resolution.z = total_penetration_resolution_cached.z ;
                                smallest_entry_time_y = FPT_ONE;
                                //can optimize this by storing the coordinates of all active voxels and looping through those instead of 3 triple nested loops
                                //X PASS
                                // for(int x = voxCoordsMin.x; x <= voxCoordsMax.x; x++){
                                //     for(int y = voxCoordsMin.y; y <= voxCoordsMax.y; y++){
                                //         for(int z = voxCoordsMin.z; z <= voxCoordsMax.z; z++){
        
                                //             uint32_t voxel_index = x + (y * 64) + (z * 4096);
                                //             if(bm->voxels[voxel_index]){
                                //                 fpt_vec3 voxPos = fpt_getVoxelWorldPosition(ivec3_create(x,y,z), bmPos_minCorner, FPT_HALF_VOXEL_SCALE, 1);
        
                                //                 //early out to skip the collision if we don't intersect with it in the final position, does this even work? maybe...
                                //                 if((!aabbIntersectTest(stop_min_x, stop_max_x, voxPos - FPT_HALF_VOXEL_SCALE, voxPos + FPT_HALF_VOXEL_SCALE)))continue;
                                //                 fpt_vec3 penetration = fpt_vec3_create(0);
                                //                 fpt_vec3 vox_normal = fpt_vec3_create(0);
                                //                 COLLISION_DEBUG_PRINTF("collides with vox: %d %d %d\n", x,y,z);
                                //                 fpt dist = swept_aabb(start_min, start_max, voxPos - FPT_HALF_VOXEL_SCALE, voxPos + FPT_HALF_VOXEL_SCALE, fpt_vec3_create(trans.desired_movement.x,0, 0), vox_normal);
                                //                 if(dist < FPT_ONE){
                                //                     if(vox_normal.x){
                                //                         if(dist < smallest_entry_time_x){
                                //                             //this voxel shares a coordinate with another penetration resolution step
                                //                             // if((collided_vox_coords.y == y && dist >= smallest_entry_time_y) || (collided_vox_coords.z == z && dist >= smallest_entry_time_z))continue; //ignore this collision, we already have a better collision with its other component
                                //                             voxel_collision = true;
                                //                             print_fpt("smallest dist X: ", dist);
                                //                             chunkData->intersected_voxel_positions[chunkData->intersected_voxel_count++] = fpt_to_flt_vec3(voxPos); 
                                //                             smallest_entry_time_x = dist;
                                //                             total_penetration_resolution.x = fpt_mul(FPT_ONE , fpt_mul(-trans.desired_movement.x , fpt_add((fpt_sub(FPT_ONE , dist)) , epsilon)));
                                //                             //spdLOG(Info,"latest total penetration resolution x: {}", fpt2fl(total_penetration_resolution.x));
                                //                             // total_penetration_resolution.x = FPT_ONE * (-trans.desired_movement.x * ((FPT_ONE - smallest_entry_time_x) + epsilon));
                                //                             int fuckthedebugger = 0;
                                //                         }
                                //                     }
                                //                 }
                                //             }
            
                                //         }
                                //     }
                                // }
                                fpt_vec3 start_min_z = stop_min_x + fpt_vec3_create(total_penetration_resolution.x, 0, 0) ;
                                fpt_vec3 start_max_z = stop_max_x + fpt_vec3_create(total_penetration_resolution.x, 0, 0) ;
                                fpt_vec3 stop_min_xz = start_min_z +  fpt_vec3_create(0, 0, trans.desired_movement.z) ;
                                fpt_vec3 stop_max_xz = start_max_z +  fpt_vec3_create(0, 0, trans.desired_movement.z) ;
                                // COLLISION_DEBUG_PRINT_FPT_VEC3("start_min_z: ",  start_min_z);
                                // COLLISION_DEBUG_PRINT_FPT_VEC3("start_max_z: ",  start_max_z);
                                // COLLISION_DEBUG_PRINT_FPT_VEC3("stop_min_xyz: ", stop_min_xyz);
                                // COLLISION_DEBUG_PRINT_FPT_VEC3("stop_max_xyz: ", stop_max_xyz);
                                //    //Z PASS
                                // COLLISION_DEBUG_PRINTF("Z PASS!, total collision resolution: %10.5f %10.5f %10.5f\n", fpt2fl(total_penetration_resolution.x),fpt2fl(total_penetration_resolution.y), fpt2fl(total_penetration_resolution.z) );
        
                                // for(int x = voxCoordsMin.x; x <= voxCoordsMax.x; x++){
                                //     for(int y = voxCoordsMin.y; y <= voxCoordsMax.y; y++){
                                //         for(int z = voxCoordsMin.z; z <= voxCoordsMax.z; z++){
        
                                //             uint32_t voxel_index = x + (y * 64) + (z * 4096);
                                //             if(bm->voxels[voxel_index]){
                                //                 fpt_vec3 voxPos = fpt_getVoxelWorldPosition(ivec3_create(x,y,z), bmPos_minCorner, FPT_HALF_VOXEL_SCALE, 1);
        
                                //                 //early out to skip the collision if we don't intersect with it in the final position, does this even work? maybe...
                                //                 if((!aabbIntersectTest(stop_min_xyz, stop_max_xyz, voxPos - FPT_HALF_VOXEL_SCALE, voxPos + FPT_HALF_VOXEL_SCALE)))continue;
                                //                 fpt_vec3 penetration = fpt_vec3_create(0);
                                //                 fpt_vec3 vox_normal = fpt_vec3_create(0);
                                //                 COLLISION_DEBUG_PRINTF("collides with vox: %d %d %d\n", x,y,z);
                                //                 fpt dist = swept_aabb(start_min_z, start_max_z, voxPos - FPT_HALF_VOXEL_SCALE, voxPos + FPT_HALF_VOXEL_SCALE, fpt_vec3_create(0 ,0, trans.desired_movement.z), vox_normal);
                                //                 if(dist < FPT_ONE){
                                //                     if(vox_normal.z){
                                //                         if(dist < smallest_entry_time_z){
                                //                             //this voxel shares a coordinate with another penetration resolution step
                                //                             // if((collided_vox_coords.y == y && dist >= smallest_entry_time_y) || (collided_vox_coords.z == z && dist >= smallest_entry_time_z))continue; //ignore this collision, we already have a better collision with its other component
                                //                             voxel_collision = true;
                                //                             print_fpt("smallest dist Z: ", dist);
                                //                             chunkData->intersected_voxel_positions[chunkData->intersected_voxel_count++] = fpt_to_flt_vec3(voxPos); 
                                //                             smallest_entry_time_z = dist;
                                //                             total_penetration_resolution.z = fpt_mul(FPT_ONE , fpt_mul(-trans.desired_movement.z , fpt_add((fpt_sub(FPT_ONE , dist)) , epsilon)));
                                //                             //spdLOG(Info,"latest total penetration resolution z: {}", fpt2fl(total_penetration_resolution.z));
                                //                             // total_penetration_resolution.x = FPT_ONE * (-trans.desired_movement.x * ((FPT_ONE - smallest_entry_time_x) + epsilon));
                                //                             int fuckthedebugger = 0;
                                //                         }
                                //                     }
                                //                 }                                  
                                //             }
            
                                //         }
                                //     }
                                // }


                                fpt_vec3 start_min_y = stop_min_xz + fpt_vec3_create(0, 0, total_penetration_resolution.z);
                                fpt_vec3 start_max_y = stop_max_xz + fpt_vec3_create(0, 0, total_penetration_resolution.z);
                                // COLLISION_DEBUG_PRINT_FPT_VEC3("start_min_y: ", start_min_y);
                                // COLLISION_DEBUG_PRINT_FPT_VEC3("start_max_y: ", start_max_y);
                         
                                fpt_vec3 stop_min_xzy = start_min_y + fpt_vec3_create(0, trans.desired_movement.y, 0);
                                fpt_vec3 stop_max_xzy = start_max_y + fpt_vec3_create(0, trans.desired_movement.y, 0);
                                // COLLISION_DEBUG_PRINT_FPT_VEC3("stop_min_xzy: ", stop_min_xzy);
                                // COLLISION_DEBUG_PRINT_FPT_VEC3("stop_max_xzy: ", stop_max_xzy);
                                // COLLISION_DEBUG_PRINTF("Y PASS!, total collision resolution: %10.5f %10.5f %10.5f\n", fpt2fl(total_penetration_resolution.x),fpt2fl(total_penetration_resolution.y), fpt2fl(total_penetration_resolution.z) );
                                //spdLOG(Warn,"step up failed, recaulculating from pre step up collision resolution");
                                //Y PASS
                                for(int x = voxCoordsMin.x; x <= voxCoordsMax.x; x++){
                                    for(int y = voxCoordsMin.y; y <= voxCoordsMax.y; y++){
                                        for(int z = voxCoordsMin.z; z <= voxCoordsMax.z; z++){
        
                                            uint32_t voxel_index = x + (y * 64) + (z * 4096);
                                            if(bm->voxels[voxel_index]){
                                                fpt_vec3 voxPos = fpt_getVoxelWorldPosition(ivec3_create(x,y,z), bmPos_minCorner);
        
                                                //early out to skip the collision if we don't intersect with it in the final position, does this even work? maybe...
                                                if((!aabbIntersectTest(stop_min_xzy, stop_max_xzy, voxPos - FPT_HALF_VOXEL_SCALE, voxPos + FPT_HALF_VOXEL_SCALE)))continue;
                                                fpt_vec3 penetration = fpt_vec3_create(0);
                                                fpt_vec3 vox_normal = fpt_vec3_create(0);
                                                // COLLISION_DEBUG_PRINTF("collides with vox: %d %d %d\n", x,y,z);
                                                fpt dist = swept_aabb(start_min_y, start_max_y, voxPos - FPT_HALF_VOXEL_SCALE, voxPos + FPT_HALF_VOXEL_SCALE, fpt_vec3_create(0, trans.desired_movement.y, 0), vox_normal);
                                                if(dist < FPT_ONE){
                                                    if(vox_normal.y){
                                                        if(dist < smallest_entry_time_y){
                                                            //this voxel shares a coordinate with another penetration resolution step
                                                            // if((collided_vox_coords.y == y && dist >= smallest_entry_time_y) || (collided_vox_coords.z == z && dist >= smallest_entry_time_z))continue; //ignore this collision, we already have a better collision with its other component
                                                            voxel_collision = true;
                                                            // print_fpt("smallest dist Y: ", dist);
                                                            chunkData->intersected_voxel_positions[chunkData->intersected_voxel_count++] = fpt_to_flt_vec3(voxPos); 
                                                            smallest_entry_time_y = dist;
                                                            total_penetration_resolution.y = fpt_mul(FPT_ONE , fpt_mul(-trans.desired_movement.y , fpt_add((fpt_sub(FPT_ONE , dist)) , epsilon)));
                                                            // //spdLOG(Info,"latest total penetration resolution y: {}", fpt2fl(total_penetration_resolution.y));
                                                            // total_penetration_resolution.x = FPT_ONE * (-trans.desired_movement.x * ((FPT_ONE - smallest_entry_time_x) + epsilon));
                                                            int fuckthedebugger = 0;
                                                        }
                                                    }
                                                }                                    
                                            }
            
                                        }
                                    }
                                }
                               
                            }
                        }
                        if(voxel_collision){
                            // total_penetration_resolution = normal_direction * (-trans.desired_movement * ((FPT_ONE - smallest_entry_time) + epsilon));
                            // COLLISION_DEBUG_PRINT_FPT_VEC3("total penetration resolution: ", total_penetration_resolution);
                            // apply_entity_movement(trans, total_penetration_resolution);
                            // trans.collide_movement = total_penetration_resolution;

                            if(fpt_abs(total_penetration_resolution.x) > fpt_abs(trans.collide_movement.x)){
                                trans.collide_movement.x = total_penetration_resolution.x;
                                phys->velocity.x = 0;
                                trans.flags |= pos_dirty;
                            }
                            if(fpt_abs(total_penetration_resolution.y) > fpt_abs(trans.collide_movement.y)){
                                trans.collide_movement.y = total_penetration_resolution.y;
                                if( trans.collide_movement.y > 0){
                                    if(!state->grounded){
                                        state->landed = true; //only trigger landing state if we were not previously grounded to prevent infinite looping
                                        state->landingIntensity = -phys->velocity.y; //invert it since its negative
                                    }
                                    phys->velocity.y = 0;//only reset velocity when pushed up from ground
                                    state->grounded = true;
                                }
                                else{
                                    phys->velocity.y -= phys->velocity.y;
                                }
                                trans.flags |= pos_dirty;
                            }
                            if(fpt_abs(total_penetration_resolution.z) > fpt_abs(trans.collide_movement.z)){
                                trans.collide_movement.z = total_penetration_resolution.z;
                                phys->velocity.z = 0;

                                trans.flags |= pos_dirty;
                            }
                            // COLLISION_DEBUG_PRINT_FPT_VEC3("total penetration resolution: ", trans.collide_movement);
                            // COLLISION_DEBUG_PRINT_FPT_VEC3("entity min RESOLVED: ", aabb.min + trans.pos_in_chunk + trans.collide_movement);
                            // COLLISION_DEBUG_PRINT_FPT_VEC3("entity max RESOLVED: ", aabb.max + trans.pos_in_chunk + trans.collide_movement);
                            // COLLISION_DEBUG_PRINT_FPT_VEC3("posInChunk RESOLVED: ", trans.pos_in_chunk + trans.collide_movement);

                            //need to consolidate once its all working, this stops the camera from moving if the collision was resolved
                            uint16_t cameraIndex = ec.entityToCameraMap[entityID];
                            if(cameraIndex != NULL_PLAYER){
                                CameraComp& cameraComp = ec.CameraComps[cameraIndex];
                                update_cameraComp_position(GameState, entityID, cameraComp, trans);
                            }
                      
                            // for(int x = voxCoordsMin.x; x <= voxCoordsMax.x; x++){
                            //     for(int y = voxCoordsMin.y; y <= voxCoordsMax.y; y++){
                            //         for(int z = voxCoordsMin.z; z <= voxCoordsMax.z; z++){
                            //             uint32_t voxel_index = x + (y * 64) + (z * 4096);
                            //             if(bm->voxels[voxel_index]){
                            //                 fpt_vec3 voxPos = fpt_getVoxelWorldPosition(ivec3_create(x,y,z), bmPos_minCorner, FPT_HALF_VOXEL_SCALE, 1);
                            //                 if(aabbIntersectTest(aabb.min + trans.pos_in_chunk + trans.collide_movement, aabb.max + trans.pos_in_chunk + trans.collide_movement, voxPos - FPT_HALF_VOXEL_SCALE, voxPos + FPT_HALF_VOXEL_SCALE)) {
                            //                     COLLISION_DEBUG_PRINTF("entity still colliding?? voxel: %d %d %d\n", x, y, z);
                            //                 }   
                            //             }
                            //         }
                            //     }
                            // }
                      
                        // }else{
                        //     COLLISION_DEBUG_PRINTF("no collision?\n");

                        }
                        //check velocity, if its outside of the -100 * 0.016 timestep epsilon, we are probably falling and thus not grounded
                        else if(phys->velocity.y <= fl2fpt(-1.7f)){//we can just check if its not 0 immediately after being correct for collision
                            //entity is falling
                            state->grounded = false;
                        }
                        
                    }
             
                    
                }
                chunkID = chunkData->dirty_chunkIDs[++i];
    
            }
            if(collisions){
                //used to have resolve collisions here, just going to call it every fixed update from scene for now
            }
            
        }
        
        chunkData->dirty_chunkID_count = 0;

        // END_BLOCK();
    }

    void entityDestroyed(game_state* GameState, uint32_t entityID){
        EntityComponent& ec = *GameState->entityComponent;
        chunk_data* chunkData = GameState->chunkData;
        uint32_t aabbIndex = ec.entityToAabbMap[entityID];
        uint32_t transIndex = ec.entityToTransMap[entityID];

        
            
        if(aabbIndex != NULL_ENTITY && transIndex != NULL_ENTITY){
            //entity has AABB, determine which chunk(s) it intersects and remove it
            int index = 0;
            AabbComp&  aabb  = ec.AabbComps[aabbIndex];
            TransComp& trans = ec.TransComps[transIndex];
            fpt_vec3 aabbRelMin = aabb.min + trans.pos_in_chunk;
            fpt_vec3 aabbRelMax = aabb.max + trans.pos_in_chunk;

            ivec3 chunkMin = calculateFPTChunkCoordinates(aabbRelMin);
            ivec3 chunkMax = calculateFPTChunkCoordinates(aabbRelMax);
            for (int x = chunkMin.x; x <= chunkMax.x; ++x) {
                for (int y = chunkMin.y; y <= chunkMax.y; ++y) {
                    for (int z = chunkMin.z; z <= chunkMax.z; ++z) {
                        //loop through 
                        ivec3 chunk_coords = ivec3_create(x,y,z) + trans.chunk_coords;
                        //spdLOG(Info,"removing entity {} from chunk coords: {} {} {}", entityID, x, y, z);
                        uint32_t chunkID = findOrCreateChunk(chunkData, chunk_coords, &GameState->WorldArena, false);
                        if(chunkID == NULL_CHUNK){LOG(Error,"trying to remove entity %u from null chunkID: %u,  %u %u %u", entityID, chunkID, chunk_coords.x, chunk_coords.y, chunk_coords.z);return;}
                        bvh_tree& tree = chunkData->bvhTrees[chunkID];
                        removeLeafNode(tree, entityID);
                        if(!tree.is_dirty){
                            COLLISION_DEBUG_PRINTF("adding chunkID %d to dirty trees\n", chunkID);
                            tree.is_dirty = true;
                            chunkData->dirty_chunkIDs[chunkData->dirty_chunkID_count++] = chunkID;
                        }
        
                    }
                }
            }
        }
    }



    //TODO: (nate) never gets cleaned up, should we make another function pointer to shutdown?
    void chunk_cleanup(game_state* GameState){
        chunk_data* chunkData = GameState->chunkData;
        debug_logger_close_file(&chunkData->logger);

        chunkData->lockMouseMotion = false;
        #ifdef SERVER_BUILD
          return;
        #endif

    }

    void reload(game_state* GameState){
      // cleanup(GameState);
      // start(GameState);
    }




    void chunk_processInput(game_state* GameState, player_input& currInput, uint32_t playerEntityID){
        uint8_t cameraIndex = GameState->entityComponent->entityToCameraMap[GameState->localPlayerEntityIDs[0]];
        if(cameraIndex == NULL_PLAYER){
            COLLISION_DEBUG_PRINTF("player camera entity cameraIndex is null??? ERROR ERROR RETURNING\n");
            return;
        }
        CameraComp& cameraComp = GameState->entityComponent->CameraComps[cameraIndex];



        if(currInput.mouseButtons)process_mouse_down(GameState, currInput, playerEntityID);
        process_mouse_up(GameState, currInput, playerEntityID);
        
        if(!currInput.consumedMouse.wheel && currInput.mouse_wheel != 0)process_mouse_wheel(GameState, currInput, playerEntityID);
        

        process_mouse_motion(GameState, cameraComp, currInput, playerEntityID);
        chunk_data* chunkData = GameState->chunkData;
        //process all chunk edits here after we know all the voxel edits have been processed (CURRENTLY ONLY INPUTS CAN EDIT VOXELS, NEED TO HANDLE THIS BETTER LATER
        Assert(chunkData->editedCount <= 8);//cant exceed the number of face data buffers, otherwise we will overwrite memory and it will look like voxel soup

        for(int i = 0; i < chunkData->editedCount; i++){
            uint32_t chunkID = chunkData->editedChunkIDs[i];
            ivec3 chunk_coords = chunkData->coords[chunkID];

            uint32_t* maxFaceCount = nullptr;
            uint32_t* chunkFaceCount = nullptr;
            Brickmap64* brickmap = nullptr;  

            brickmap = &chunkData->brickmaps[chunkID];
            chunkFaceCount = &chunkData->face_count[chunkID];
            *chunkFaceCount = 0;
            maxFaceCount = &chunkData->max_face_count[chunkID];

            #ifndef SERVER_BUILD

                chunkData->mainThreadWorker[i].chunkID = chunkID;
                //face count must not be getting updated correctly
                // floodFillLight(chunkData, brickmap);

                vertex_pull_mesh_variable(brickmap->voxels, &chunkData->mainThreadWorker[i], true, uvec3_create(64), chunkData->mainThreadFaceMemory[i], 
                                *chunkFaceCount, FACE_MAX);
           
                //:::VERY IMPORTANT:::
                //make sure that we process all chunk mesh updates AFTER WE KNOW ALL THE EDITS HAVE COMPLETED FOR THIS FRAME
                //otherwise we risk overwriting the same location in memory if we push update commands FROM MORE THAN ONCE LOCATION
                u32& commandCount = GameState->RenderCommandData->chunkCreateCommandCount;
                chunk_create_command newCommand = {};
                newCommand.chunkID = chunkID;
                newCommand.faceMemory = chunkData->mainThreadFaceMemory[i]; //safe to pass a pointer only as long as the current pipeline is unchanged, the pointer only needs to last for this frame
                newCommand.faceCount = *chunkFaceCount;
                newCommand.edit = true;
                newCommand.faceMemoryIndex = MAX_CORES;
                GameState->RenderCommandData->chunkCreateCommands[commandCount++] = newCommand; 
                LOG(Info, "chunkID: %u, %d %d %d, facePointer: %p, mainWorkThreadID: %u, chunkCreateCommandCount: %u\n", chunkID, chunk_coords.x,chunk_coords.y,chunk_coords.z, chunkData->mainThreadFaceMemory[i], i, commandCount);
            #endif
        }
        chunkData->editedCount = 0;
    }

