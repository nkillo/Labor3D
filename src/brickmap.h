#pragma once
// #define VOXEL_RESOLUTION 8 // Or any other compile-time constant
// #define CHUNK_BRICKMAPS 512 // Or any other compile-time constant
// #define MAX_CHUNKS 128 // Adjust based on your needs

// #define VOXEL_RESOLUTION 8 // Or any other compile-time constant
// #define CHUNK_BRICKMAPS 512 // Or any other compile-time constant
// #define MAX_CHUNKS 128 // Adjust based on your needs



// static VoxelLookup padding_lookup;
// static VoxelLookupHalfRes padding_lookup_half_res;

constexpr float chunk_size = 62.0f;
constexpr float inverse_chunk_size = 1/62.0f;
// constexpr float chunk_size = 64.0f;
// constexpr float inverse_chunk_size = 1/64.0f;
constexpr float brickgrid_resolution = 4.0f; //the number of brickmaps in a single axis of the brickgrid
constexpr vec3 chunk_min_corner = {-chunk_size/2.0f, -chunk_size/2.0f, -chunk_size/2.0f};

//need to add constexpr to fpt_vec3_create for this to work, i can just use -FPT_HALF_CHUNK_SIZE instead
// constexpr fpt_vec3 fpt_chunk_min_corner = fpt_vec3_create(-FPT_HALF_CHUNK_SIZE,-FPT_HALF_CHUNK_SIZE,-FPT_HALF_CHUNK_SIZE);
constexpr fpt fpt_brickgrid_resolution = 262144; // 4 * (2^16)

// constexpr float voxel_resolution_per_chunk = 256.0f;
//if i want chunks to have size 62 i need to do 62*4 = 248
constexpr float voxel_resolution_per_chunk = 248.0f;
constexpr float voxel_size = chunk_size / voxel_resolution_per_chunk; 
constexpr float brickmap_scale = chunk_size / brickgrid_resolution;

constexpr float brickmap_offset = chunk_size / brickgrid_resolution;
constexpr float brickmap_center_offset = brickmap_offset / 2.0f;
//number of voxels per axis in a single brickmap. if we want 256 voxels in a chunk, 32 brickmaps, 256/32 = 8
// 256 / 16 = 16, so we get larger brickmaps
constexpr float brickmap_resolution = voxel_resolution_per_chunk / brickgrid_resolution;
constexpr float inverse_brickmap_resolution = 1 / brickmap_resolution;
constexpr size_t brickmap_resolution_cubed = static_cast<size_t>(brickmap_resolution * brickmap_resolution * brickmap_resolution);


//padding
constexpr int padded_voxel_resolution_per_chunk = 256;

constexpr int pbmr = (int)(padded_voxel_resolution_per_chunk / brickgrid_resolution);
constexpr int npbmr = (int)((padded_voxel_resolution_per_chunk / brickgrid_resolution) - 2); //non padded size
constexpr size_t pbmr3 = static_cast<size_t>(pbmr * pbmr * pbmr);
constexpr size_t coarse3 = static_cast<size_t>(8*8*8);

// ============================= HALF RESOLUTION BRICKMAP =====================================

constexpr float hchunk_size = 32.0f;

constexpr float hvoxel_resolution_per_chunk = 124.0f;
constexpr float hvoxel_size = hchunk_size / hvoxel_resolution_per_chunk; 

constexpr float hbrickmap_offset = hchunk_size / brickgrid_resolution;
constexpr float hbrickmap_center_offset = hbrickmap_offset / 2.0f;
//number of voxels per axis in a single brickmap. if we want 256 voxels in a chunk, 32 brickmaps, 256/32 = 8
// 256 / 16 = 16, so we get larger brickmaps
constexpr float hbrickmap_resolution = hvoxel_resolution_per_chunk / brickgrid_resolution;
constexpr size_t hbrickmap_resolution_cubed = static_cast<size_t>(hbrickmap_resolution * hbrickmap_resolution * hbrickmap_resolution);

constexpr int half_brickmap_resolution = 32;
//padding
constexpr int hpadded_voxel_resolution_per_chunk = 128;
constexpr int hpbmr = half_brickmap_resolution;
constexpr size_t hpbmr3 = static_cast<size_t>(hpbmr * hpbmr * hpbmr);
constexpr float half_res_voxel_size = voxel_size * 2;//i dunno, maybe it will work, its twice the size of a high res voxel


using VoxelLookup = bool[pbmr3];
using VoxelLookupHalfRes = bool[hpbmr];




struct Brickmap64{
    uint32_t active_count;
    uint8_t voxels[64*64*64];
};
struct Brickmap32{
    uint32_t active_count;
    uint8_t voxels[32*32*32];
};
struct Brickmap16{
    uint32_t active_count;
    uint8_t voxels[16*16*16];
};
struct Brickmap8{
    uint32_t active_count;
    uint8_t voxels[8*8*8];
};
struct CoarseBrickmap64{
    uint16_t active_count[8*8*8];//count of voxels within the current cell
};

    struct Brickmap {
        //variable power of 2 resolution per axis
        uvec3 resolution;
        uint32_t total_voxel_count;
        
        //IF YOU CHANGE THIS YOU NEED TO ADJUST sizeof(uint16_t) IN BRICKMAP CREATE AND DESTROY
        uint8_t*  colors;

        // std::unordered_set<uint32_t> active_voxels;
        uint32_t  active_count;
        bool isVisible = true;
        bool hasEmptyNeighbor = true;
        // uint32_t bitmask[brickmap_resolution * 2]; // C style array, slightly less safe than std::array
        // A 32-bit pointer for color data.
        // Assuming color data is stored elsewhere, we can use a uint32_t to represent the pointer.
        // Alternatively, if you're actually storing the pointer, use an appropriate pointer type.
        uint32_t colorPointer;

        // A 24-bit LOD color pointer.
        // Since there's no 24-bit integer type in C++, we can use a uint32_t to store it.
        // The extra 8 bits can be ignored or used as padding.
        uint32_t lodColorPointer;
    };

    void clear_brickmap(Brickmap* bm);

    //function declarations
    Brickmap* create_brickmap(uint32_t res_x, uint32_t res_y, uint32_t res_z);
    void destroy_brickmap(Brickmap* bm);

    //maybe put in a grid.h file later
    uint32_t get_index_from_coords(const ivec3& coords, const uvec3& resolution);
    ivec3 get_coords_from_index(uint32_t index, const uvec3& resolution);

    //returns true if voxel was changed, false otherwise
    bool set_voxel(Brickmap* bm, const ivec3& coords, uint8_t color, bool padded_voxel = false);
    bool set_voxel(Brickmap* bm, uint32_t index, uint8_t color, bool padded_voxel = false);

    void clear_voxel(Brickmap* bm, const ivec3& coords);
    uint8_t get_voxel_color(const Brickmap* bm, const ivec3& coords);
    bool is_within_bounds(const Brickmap* bm, const ivec3& coords); 

    
    vec3 get_voxel_local_position(int bmResolution, int voxel_index);
    vec3 get_absolute_voxel_position(int bmResolution, int voxel_local_index, int brickmap_index, const vec3& chunk_position);
    vec3 get_absolute_brickmap_position(int index, const vec3& chunk_position);
    
    vec3 get_chunk_bottom_left_back_corner(const vec3& chunk_position);

    void memcpy_voxel_noise(Brickmap* bm, const uint8_t* colors, uint32_t active_voxel_count);
    void memcpy_padded_voxel_noise(Brickmap* bm, const uint8_t* colors, uint32_t active_voxel_count);
    
    size_t set_voxels_batch(Brickmap* bm, const uint32_t* indices, const uint8_t* colors, size_t count);



    void populate_padding_lookup(VoxelLookup& padding_lookup);

    // Function to check if a voxel is in padding
    bool is_in_padding(VoxelLookup& padding_lookup, uint32_t voxel_index);

    void populate_padding_lookup_half_res();
    bool is_in_padding_half_res(uint32_t voxel_index);
