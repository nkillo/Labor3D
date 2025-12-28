#pragma once
#include "AABB.h"
// #include "labour/util/mesh.h"
// #include "labour/core/memory.h"
#include "brickmap.h"
#include "constants.h"
#include "entity.h"

#define VALUE_NOISE_TILE 8
#define MAX_ACTIVE_CHUNKS   256
#define VOXEL_DEQUE_FRONT_START   MAX_CHUNKS / 2
#define VOXEL_DEQUE_BACK_START   (MAX_CHUNKS / 2) - 1
#define MAX_BVHNODES (MAX_ENTITIES * 2) //we only have as many nodes per tree as all entities, so only have max entities can fit in any chunk (max nodes = leaf nodes * 2)
#define CHUNK_UNINITIALIZED UINT32_MAX
#define NULL_NODE MAX_BVHNODES//probably won't exceed this since MAX_ENTITIES is only 2048, and we will only have max_nodes = 2 * leaf_nodes
#define NULL_LOD_TYPE 0
#define NULL_LOD_INDEX UINT16_MAX
#define MAX_WORKER_THREADS 20

#define BITS_PER_WORD 32
#define NUM_WORDS (MAX_ENTITIES / BITS_PER_WORD) //128 words per chunk

#define CHUNK_COORDS_UNINITIALIZED INT32_MAX

#define MAX_ENTITY_INTERACTIONS 64 //64 max interactions per chunk, cleared and repopulated whenever the chunk performs a broad phase


#if 0
#define LOG(logLevel, format, ...) log_console(&chunkData->logger, logLevel, format, ##__VA_ARGS__)
#else
#define LOG(logLevel, format, ...) 
#endif

struct chunk_hash
{
    ivec3 coords;
    uint32_t index;
    
    chunk_hash *NextInHash;
};

// Platform-agnostic bit operations
    #if defined(_MSC_VER) // Visual Studio
        #include <intrin.h>
        inline uint32_t countLeadingZeros(uint64_t x) {
            unsigned long index;
            return _BitScanReverse64(&index, x) ? 63 - index : 64;
        }
        
        inline uint32_t countTrailingZeros(uint64_t x) {
            unsigned long index;
            return _BitScanForward64(&index, x) ? index : 64;
        }
        
        inline uint32_t popCount(uint64_t x) {
            return static_cast<uint32_t>(__popcnt64(x));
        }
        
    #elif defined(__GNUC__) || defined(__clang__) // GCC, Clang, or compatible
        inline uint32_t countLeadingZeros(uint64_t x) {
            return x ? __builtin_clzll(x) : 64;
        }
        
        inline uint32_t countTrailingZeros(uint64_t x) {
            return x ? __builtin_ctzll(x) : 64;
        }
        
        inline uint32_t popCount(uint64_t x) {
            return static_cast<uint32_t>(__builtin_popcountll(x));
        }
        
    #else // Fallback implementations
        inline uint32_t countLeadingZeros(uint64_t x) {
            if (x == 0) return 64;
            
            uint32_t n = 0;
            if (x <= 0x00000000FFFFFFFF) { n += 32; x <<= 32; }
            if (x <= 0x0000FFFFFFFFFFFF) { n += 16; x <<= 16; }
            if (x <= 0x00FFFFFFFFFFFFFF) { n += 8;  x <<= 8;  }
            if (x <= 0x0FFFFFFFFFFFFFFF) { n += 4;  x <<= 4;  }
            if (x <= 0x3FFFFFFFFFFFFFFF) { n += 2;  x <<= 2;  }
            if (x <= 0x7FFFFFFFFFFFFFFF) { n += 1;  x <<= 1;  }
            return n;
        }
        
        inline uint32_t countTrailingZeros(uint64_t x) {
            if (x == 0) return 64;
            
            uint32_t n = 0;
            if ((x & 0x00000000FFFFFFFF) == 0) { n += 32; x >>= 32; }
            if ((x & 0x000000000000FFFF) == 0) { n += 16; x >>= 16; }
            if ((x & 0x00000000000000FF) == 0) { n += 8;  x >>= 8;  }
            if ((x & 0x000000000000000F) == 0) { n += 4;  x >>= 4;  }
            if ((x & 0x0000000000000003) == 0) { n += 2;  x >>= 2;  }
            if ((x & 0x0000000000000001) == 0) { n += 1;  x >>= 1;  }
            return n;
        }
        
        inline uint32_t popCount(uint64_t x) {
            x = x - ((x >> 1) & 0x5555555555555555);
            x = (x & 0x3333333333333333) + ((x >> 2) & 0x3333333333333333);
            x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0F;
            x = x + (x >> 8);
            x = x + (x >> 16);
            x = x + (x >> 32);
            return static_cast<uint32_t>(x & 0x7F);
        }
    #endif

enum ChunkStage{
    chunkStage_null,
    chunkStage_unloaded,
    chunkStage_touched,//some mechanism has initiated the creation/voxelization/meshing process
    chunkStage_firstPass,//multithreaded initial voxel gen
    chunkStage_firstPassQueued,
    chunkStage_firstPassWorking,
    chunkStage_firstPassComplete,
    chunkStage_secondPass,//single threaded terrain features
    chunkStage_secondPassQueued,
    chunkStage_secondPassWorking,
    chunkStage_secondPassComplete,
    chunkStage_meshing,//multithreaded meshing
    chunkStage_meshed,//multithreaded meshing
    chunkStage_upload,//upload the mesh to the GPU
    chunkStage_uploaded,//upload the mesh to the GPU
    chunkStage_flaggedForDestruction,
};
enum ChunkVisibility{
    chunkVisibility_null,
    chunkVisibility_unloaded,
    chunkVisibility_outsideCameraGrid,
    chunkVisibility_skirt,
    chunkVisibility_invisible,
    chunkVisibility_visible,
};

enum VoxMetaGenType{
    voxMeta_none,
    voxMeta_expandSeed,
    voxMeta_expandFeature,
};

struct VoxMetaExpandSeed{
    u32 voxIndex;
    VoxelTypes type;
};

struct VoxMetaGenEntry{
    VoxMetaGenType type;
    u32 randSeed;
    union{
        VoxMetaExpandSeed seed;
    };

};

struct rayPathStep{
    vec3 voxel_position;
    fpt_vec3 fpt_voxel_position;
    ivec3 voxel_coords;
    ivec3 chunk_coords;
    uint32_t voxel_index;
    uint16_t step;
    float voxel_scale;
    fpt fpt_voxel_scale;
    bool hit;
};

struct rayCastResult
{
    bool selected;
    uint32_t chunkID;
    uint32_t voxel_index;
    vec3 selected_voxel_render_pos;
    ivec3 chunk_coords;
    ivec3 adjacent_chunk_coords;
    ivec3 voxel_coords;
    ivec3 adjacent_voxel_coords;
    uint8_t voxel_color;
    vec3 voxel_position;
    fpt_vec3 fpt_voxel_position;
    vec3 hitPosition;
    fpt_vec3 fptHitPosition;

    fpt_vec3 pos_in_chunk_hit;
    ivec3 chunk_coords_hit;

    uint32_t adjacent_chunk_index;
    uint32_t adjacent_voxel_index;

    uint8_t voxel_type;



};


// CS = chunk size (max 62)
constexpr int CS = 62;

// Padded chunk size
constexpr int CS_P = CS + 2;
constexpr int CS_2 = CS * CS;
constexpr int CS_P2 = CS_P * CS_P;
constexpr int CS_P3 = CS_P * CS_P * CS_P;


//to determine maximum translation difference across chunk boundaries
//same as the relative_position calculation function in the renderSystem
static inline fpt_vec3 distance_diff(fpt_vec3& new_pos, fpt_vec3& old_pos, ivec3& new_coords, ivec3& old_coords){
    return (new_pos - old_pos) + (ivec_to_fpt_vec3(new_coords - old_coords)  * FPT_CHUNK_SIZE);
}

static inline fpt_vec3 pos_in_chunk_offset(fpt_vec3& pos_in_chunk, ivec3& current_chunk, ivec3& intersected_chunk){
    return (pos_in_chunk) + (ivec_to_fpt_vec3(current_chunk - intersected_chunk)  * FPT_CHUNK_SIZE);
}

static inline void update_cameraComp_position(game_state* GameState, uint32_t entityID, CameraComp& cameraComp, TransComp& transComp){
    if(cameraComp.freeMode || entityID != GameState->localPlayerEntityIDs[0])return;
    ivec3 dummy_coords = transComp.chunk_coords;
    fpt_vec3 new_pos_in_chunk = (transComp.pos_in_chunk + transComp.collide_movement + fpt_vec3_create(0, FPT_ONE, 0)) + cameraComp.fptTarget * -cameraComp.third_person_offset;
    rebasePosition(new_pos_in_chunk, dummy_coords);
    fpt_vec3 movement = distance_diff(new_pos_in_chunk, cameraComp.pos_in_chunk, dummy_coords, cameraComp.chunk_coords);
    cameraComp.pos_in_chunk += movement;
    rebasePosition(cameraComp.pos_in_chunk, cameraComp.chunk_coords, &cameraComp.inNewChunk);
}


    struct chunk_data;

    static const uint32_t CHUNK_NOT_FOUND = UINT32_MAX;
    // static constexpr uint8_t isChunkActive = 1 << 0; //if chunk is not active, its essentially deleted and its index can be recycled
        
    static constexpr float g_ChunkSize = 62.0f;//64.f;
    static constexpr float g_CoarseCellSize = 62.0f / 8.0f;
    static constexpr float g_CoarseCellOffset = g_CoarseCellSize / 2.0f;
    static constexpr float g_HalfChunkSize = g_ChunkSize/2;//chunk size = halfsize * 2
    static constexpr float g_InverseChunkSize = 1/g_ChunkSize;

    static constexpr uint8_t maxDepth = 2;//max 512, so we use short for indices
    static constexpr uint8_t maxEntitiesPerNode = 1;


struct entity_v_entity_collision{
    uint16_t entityID;
    uint16_t other_entityID;
};

struct entity_v_voxel_collision{
    uint16_t entityID;
    uint16_t coarse_grid_id;
};


struct bvh_tree {
    // Fixed size arrays - group similar sized members
    AABB     box[MAX_BVHNODES];         // Largest alignment (likely 16 bytes)


    // 16-bit arrays
    uint16_t nodeIDQueue[MAX_BVHNODES];
    uint16_t entityToNodeID[MAX_BVHNODES];
    uint16_t nodeToEntityID[MAX_BVHNODES];
    uint16_t left[MAX_BVHNODES];
    uint16_t right[MAX_BVHNODES];
    uint16_t parent[MAX_BVHNODES];
    uint16_t primitiveCount[MAX_BVHNODES];
    uint16_t nodesToDraw[MAX_BVHNODES];
    uint16_t nodeToDrawPos[MAX_BVHNODES];
    uint16_t colliding_node_ids[MAX_BVHNODES];
    uint8_t  node_children_checked[MAX_BVHNODES];


    entity_v_entity_collision entity_collisions[MAX_ENTITIES];
    entity_v_voxel_collision entity_voxel_collisions[MAX_ENTITIES];
    uint16_t entity_collision_count;
    uint16_t entity_voxel_collision_count;

    uint16_t colliding_node_count;
    // Pack small counts/flags together (32-bit group)
    uint16_t nodeCount;
    uint16_t rootNode;
    
    // 32-bit counters group
    uint32_t activeNodeCount;
    uint32_t bvhFreeListSize;
    uint32_t insertionCount;
    uint32_t insertions_to_rotation;
    uint32_t removalCount;
    
    // Bool array (could be bitpacked)
    bool activeNodes[MAX_BVHNODES];
    
    struct {
        bool imGuiReDraw : 1;     // Uses only 1 bit
        bool bvh_Debug_Log : 1;   // Uses only 1 bit
        bool enable_First_Fit : 1; // Uses only 1 bit
    } flags;      
    
    // Float debug data
    float total_Pick_Best_Cost;
    float average_Best_Cost;
    
    // Integer debug counters
    int32_t total_Node_Pairings;
    int32_t total_Allocated_Nodes;
    int32_t total_Internal_Nodes;
    int32_t total_Leaf_Nodes;
    ivec3 chunk_coords;//debug value

    int total_collision_checks;
    bool is_dirty; //dirty if a node has updated, set to false after performing broad phase collision check

};
    struct entity_interaction_data{
        uint16_t     interacting_entityID;
        uint16_t     interactable_entityID;
        uint8_t      interacting_versionID;
        uint8_t      interactable_versionID;
        entity_types interacting_type;
        entity_types interactable_type;
    };

    struct chunk_queue_hash{
        ivec3 coords;
        chunk_queue_hash* NextInHash;
    };

    struct ChunkMeshMemory{
        //64*64*32*6
        FaceData faceMemory[pbmr*pbmr*hpbmr*6];
        u32 faceCount;
    };

    struct voxel_work{
        u32* thorns; //for texture heightmap reading
        u32* cracked;
        bool meshNeedsUploading;
        bool hasValidWork;
        // FastNoise::Generator* gen;
        uint32_t chunkID;
        uint32_t chunkIDBackup;
        uint8_t LODType;
        uint8_t workerID;
        ChunkMeshMemory chunkMeshMemory;
        uint8_t noiseVoxels[pbmr3];
        float samplePosX[pbmr3];
        float samplePosY[pbmr3];
        float samplePosZ[pbmr3];
        u8 tileX[pbmr3];
        u8 tileY[pbmr3];
        u8 tileZ[pbmr3];
        u8 sampleRegionX[pbmr3];
        u8 sampleRegionY[pbmr3];
        u8 sampleRegionZ[pbmr3];
        float brickmap_noise[pbmr3];
        float noise3d[pbmr3];
        u8 isAboveAir[pbmr * pbmr];

        u32 seeds[pbmr*pbmr];
        u32 seedCount;

        //16+2=18 to allow for sample rates of 1
        float noiseTile[18*18*18];
        float coarseNoiseTile[18*18*18];
        float coarseSamplePosX[18*18*18];
        float coarseSamplePosY[18*18*18];
        float coarseSamplePosZ[18*18*18];

        float xoffset;
        float yoffset;
        float zoffset;
        float scaleFactor;
        u32 demoEnum;

        uint64_t col_face_masks_x[CS_P2 * 2];// CS_P2 * 6
        uint64_t col_face_masks_y[CS_P2 * 2];// CS_P2 * 6
        uint64_t col_face_masks_z[CS_P2 * 2];// CS_P2 * 6
        uint64_t col_face_masks[CS_P2 * 6];
        // uint64_t col_face_masks[CS_P2 * 6];// CS_P2 * 6
        uint64_t a_axis_cols   [CS_P2]; // CS_P2
        uint64_t b_axis_cols   [CS_P]; // CS_P
        uint64_t merged_right  [CS_P]; // CS_P
        uint64_t merged_forward[CS_P2]; // CS_P2
        chunk_data * chunkData;

        uint64_t perlinCycles;
        uint64_t perlinHits;
        uint64_t perlinTotal;

        uint64_t totalCycles;

        u64 accumulateVoxelsTotalCycles;
        u64 accumulateVoxelsHits;
        u32 threadIndex;

    };

    struct voxel_queue_entry{
        uint32_t chunkID;
        uint32_t version;
    };    

    struct chunk_data{
        debug_logger logger;
        chunk_hash chunkHash[MAX_CHUNKS];
        uint32_t activeChunkCount = 0;
        uint32_t chunkFreeListSize = 0;
        bool freezeChunks = false;
        bool freezeWhenFinished = false;
        ivec3 coords   [MAX_CHUNKS];
        u32 versions[MAX_CHUNKS];
        fpt_vec3 chunkGridIntersectBounds;
        uvec3 chunkDrawDistance;
        ivec3 cameraGridCenterCoords;
        uint32_t cameraGrid[MAX_CHUNK_DRAW_DISTANCE]; //grid that follows the camera, contains chunk indices, 0 = bottom left back of the chunk, increments x->y->z like voxels
        uint32_t cameraGridCenterIndex;
        u32 cameraGridSize;
        u32 cameraGridCount;
        u32 cameraGridFreeList[MAX_CHUNK_DRAW_DISTANCE];
        u32 cameraGridFreeListCount;

        u32 chunkRemeshQueue[MAX_CHUNK_DRAW_DISTANCE];
        u32 chunkRemeshQueueCount;

        u32 freshChunks[MAX_CHUNK_DRAW_DISTANCE];
        u32 freshChunkCount;

        u32 treeSeeds[512];
        u32 treeSeedCount;
        u32 oreSeeds[512];
        u32 oreSeedCount;

        u32 cameraGridVisible[MAX_CHUNK_DRAW_DISTANCE];
        u32 cameraGridVisibleCount;

        u32 chunkIDToVisibleMap[MAX_CHUNKS];
        
        VoxMetaGenEntry metaGenInfo[MAX_CHUNKS][512];
        u32 metaGenInfoCount[MAX_CHUNKS];

        uint32_t   chunkFreeList [MAX_CHUNKS];

        uint32_t idRecycledCount[MAX_CHUNKS];
        uint32_t visibleCount;

        ivec3 visiblechunk_coords       [MAX_CHUNKS]; //use ID to index
        ivec3 newVisiblechunk_coords    [MAX_CHUNKS]; 
        uint32_t new_visible_chunk_coords_count;
        //for font rendering, copied from GameState->shaderData
        
        
        voxel_queue_entry voxelizeQueue[MAX_CHUNKS];
        uint32_t voxelizeQueueCount;

        uint8_t chunkVoxels[MAX_CHUNKS][pbmr3];
        float   chunkNoise [MAX_CHUNKS][pbmr3];
        //for very awful debugging. just modulo the chunkID and store the sampled position
        float samplePosX [DEBUG_CHUNKS][pbmr3];
        float samplePosY [DEBUG_CHUNKS][pbmr3];
        float samplePosZ [DEBUG_CHUNKS][pbmr3];
        u8 tileX[DEBUG_CHUNKS][pbmr3];
        u8 tileY[DEBUG_CHUNKS][pbmr3];
        u8 tileZ[DEBUG_CHUNKS][pbmr3];
        u8 sampleRegionX[DEBUG_CHUNKS][pbmr3];
        u8 sampleRegionY[DEBUG_CHUNKS][pbmr3];
        u8 sampleRegionZ[DEBUG_CHUNKS][pbmr3];


        //helper chunk data
        ivec3 intersectingchunk_coords[MAX_CHUNKS];

        uint32_t intersectingChunkIndices[MAX_CHUNKS]; 
        uint32_t intersectingChunkIndicesCount = 0;
        uint32_t chunkDifference[MAX_CHUNKS];
        uint32_t intersectingChunkCount = 0;

        uint32_t dirty_chunkIDs[MAX_CHUNKS];
        uint32_t dirty_chunkID_count;

        //old system, old code relies on this or it will break, remove once we finish the rewrite
        ChunkStage chunkStages[MAX_CHUNKS];
        ChunkVisibility chunkVisibilities[MAX_CHUNKS];

        //better multithreaded approach since chunks need to be streamed
        atomic_uint32 volChunkStages[MAX_CHUNKS];
        atomic_uint32 volChunkVisibilities[MAX_CHUNKS];


        uint32_t secondPass_chunkIDs[MAX_CHUNKS];
        uint32_t secondPass_chunkIDMap[MAX_CHUNKS];
        int32_t secondPass_chunkID_count;

        uint32_t firstPass_chunkIDs[MAX_CHUNKS];
        uint32_t firstPass_chunkIDMap[MAX_CHUNKS];//stores index in to the array
        int32_t firstPass_chunkID_count;

        ivec3 intersectingChunkDifference[MAX_CHUNKS];

        uint32_t chunkDifferenceCount;

        //entity bvh TREE SOA representation
        bvh_tree bvhTrees[MAX_CHUNKS];





        


        ivec3 chunk_coord_bounds;//TOROIDAL SPACE TEST
        bool toroidal_space_enabled;

        // FastNoise::Generator** noiseGenerators;
        uint32_t generatorCount;
        uint32_t threadCount;
        uint32_t threadToAssign;
        uint32_t activeThreadCount;
        bool hasVoxWorkQueued;
        bool hasMeshWorkQueued;
        uint32_t secondPassCount;
        voxel_work workerThreads[MAX_WORKER_THREADS];
        atomic_uint32 workerThreadOccupations[MAX_WORKER_THREADS];
        ChunkMeshMemory stableChunkMeshMemory[MAX_WORKER_THREADS];
        u32 stableChunkMeshMemoryCount;

        uint32_t chunkWorkers[MAX_CHUNKS];//stores the latest worker thread to process this chunk

        voxel_work mainThreadWorker[8];
        FaceData mainThreadFaceMemory[8][FACE_MAX];
        
        platform_work_queue *Queue;
        platform_add_entry *PlatformAddEntry;
        platform_complete_all_work *PlatformCompleteAllWork;
        platform_check_all_work_completed *PlatformCheckAllWorkCompleted;
        // GenNoiseFunc *PlatformGenerateNoise;

        // Vertex data is packed into one unsigned integer:
        // - x, y, z: 6 bit each (0-63)
        // - Type: 8 bit (0-255)
        // - Normal: 3 bit (0-5)
        // - AO: 2 bit
        //

        uint32_t vertices[100000];

        bool safeToEdit[MAX_CHUNKS];


        //voxel testing
        rayCastResult voxelRayCastResult;
        rayPathStep voxelPath[MAX_CHUNKS];
        uint32_t voxelPathCount;
        uint32_t voxelPathSearchIndex;
        bool lockMouseMotion;
        CameraComp cameraSnapshot;
        vec2 mousePos;
        vec3 raySpherePos;
        fpt_vec3 fptRaySpherePos;
        vec3 raySphereVoxelPos;
        fpt_vec3 fptRaySphereVoxelPos;
        vec3 raySphereScale;
        fpt_vec3 fptRaySphereScale;
        quat raySphereRot;
        float raySphereStep;
        vec3 rayOrigin;
        vec3 rayStart;
        ivec3 raychunk_coords;
        fpt_vec3 fptRayOrigin;
        vec3 rayDir;
        fpt_vec3 fptRayDir;
        float raySphereTotalRotation = 0.0f;
        vec3 brushPos;
        fpt_vec3 fptBrushPos;
        ivec3 brushCenterchunk_coords;
        fpt fptBrushSize;
        float brushSize;
        AABB brushBounds;
        FPT_AABB fptBrushBounds;
        // Vertex rayLineVertices[2];
        uint32_t brushChunkIDs[64];
        uint32_t brushChunkIDCount;
        ivec3 brushChunkCoords[64];
        uint32_t brushChunkCoordsCount;
        ivec3 brushChunkVoxMin[64];
        ivec3 brushChunkVoxMax[64];
        
        //flood fill lighting test
        //could eventually create compactVec3 of 1 byte per x y z since we only store 0 to 63 in any axis
        ivec3 lightQueuePositions[pbmr3];
        fpt_vec3 fptLightQueuePositions[pbmr3];
        uint8_t lightQueueValues[pbmr3];
        uint32_t lightQueueCount;
        ivec3 lightBoundaryQueuePositions[54][pbmr3];
        uint8_t lightBoundaryQueueValues[54][pbmr3];
        Brickmap64* lightBrickmaps[54];
        ivec3 lightchunk_coords[54];
        uint32_t lightChunkIDs[54];
        uint32_t lightBMIDs[54];
        uint32_t lightBmCount;

        Brickmap64* lightBoundaryBrickmapQueue[54];
        uint32_t lightBoundaryActiveQueueCount;
        uint32_t floodFillTotalBrickmaps;
        uint32_t lightBoundaryQueueCounts[54];

        //final pass after editing voxels and updating lighting
        uint32_t editedChunkIDs[128];
        uint32_t editedBMIDs[128];
        Brickmap64* editedBMs[128];
        fpt_vec3 fptEditedBMCoords[128];
        uint32_t editedCount;

        int manhattanOverride = 0;


        uint32_t bmToBufferMap[MAX_CHUNKS * 64];
        uint32_t bufferToBmMap[64 * 32];
        uint32_t bufferCount = 0;

        uint32_t faceIndices[1200000]; //shared between all buffers
        

        // bgfx::DynamicVertexBufferHandle brickmap_mesh_handles [MAX_CHUNKS];
        // bgfx::VertexBufferHandle        textBuffers[MAX_CHUNKS];


        bool voxel_buffer_snapshotted[MAX_CHUNKS];
        bool text_buffer_snapshotted[MAX_CHUNKS];

        //these probably wont change, don't need to worry about them
        // bgfx::IndexBufferHandle vertexPulled_ibh;
        // bgfx::DynamicVertexBufferHandle vertexPulled_vbh;

        //maps pixels from 0 to 4095 (64x64) to voxel indices in the brickmap
        uint32_t testProjectionMap[64*64]; //for mapping pixel colors onto surface voxels of a brickmap in any given axis/direction
        VoxelLookup paddingLookup;

        Brickmap64       brickmaps [MAX_CHUNKS];
        CoarseBrickmap64 coarse_brickmaps[MAX_CHUNKS];

        uint32_t face_count[MAX_CHUNKS];
        uint32_t max_face_count [MAX_CHUNKS];




        //entity selection/dragging data
        uint16_t selectedEntityIndex;
        int      selectedEntityChunkID;
        bool entity_selected;
        bool dragging_entity; //if an entity is selected, and we are holding the specific button, this bool is active
        fpt distance_to_selected_entity;
        fpt_vec3 entity_drag_vector;
        fpt_vec3 entity_position;
        ivec3 entity_chunk_coords;

        fpt_vec3 last_camera_update_pos;
        fpt_vec3 last_camera_update_chunk_coords;

        ivec3 test_entity_chunk_coords;

        vec3 intersected_voxel_positions[MAX_CHUNKS];
        int intersected_voxel_count;

        fpt_vec3 test_hitbox_pos;

        entity_interaction_data entity_interactions[MAX_CHUNKS][MAX_ENTITY_INTERACTIONS];
        // entity_interaction_data entity_interactions[MAX_CHUNKS * MAX_ENTITY_INTERACTION];
        int entity_interactions_count[MAX_CHUNKS];

        //ken perlin permutation table
        u8 perlinPermTable[512];
        float gradients3d[256];
        float gradients2d[256];

        vec2 directionGradients2d[256];
        u8    permutations2d[512];

        vec3 directionGradients3d[256];
        u8    permutations3d[512];

        vec4 directionGradients4d[256];
        u8    permutations4d[512];
        float valueNoise3d[VALUE_NOISE_TILE][VALUE_NOISE_TILE][VALUE_NOISE_TILE];

        float gradientX[12];
        float gradientY[12];
        float gradientZ[12];

        u64 perlinNoise3dCycles;
        u64 perlinNoise3dCyclesTotal;
        u64 perlinNoise3dHits;

        u64 perlinNoise3dScalarLookup;
        u64 perlinNoise3dScalarLookupTotal;

        u64 perlinNoise3dSIMDLookup;
        u64 perlinNoise3dSIMDLookupTotal;
        
        u64 perlinNoise3dScalar;
        u64 perlinNoise3dScalarTotal;

        u64 perlinNoise3dSIMD;
        u64 perlinNoise3dSIMDTotal;

        u64 grad4x;
        u64 grad4xTotal;
        u64 grad4xHits;

        u64 perlinThreadCycles;
        u64 perlinThreadHits;
        u64 perlinThreadTotal;

        u64 workThreadTotalCycles;
        u64 workThreadHits;
        u64 workThreadCycles;

        u64 workThreadAccumulateVoxelsTotalCycles;
        u64 workThreadAccumulateVoxelsHits;
        u64 workThreadAccumulateVoxelsCycles;

        u32* thorns;
        u32* cracked;

        u32 testDebugSSBOCount;

        u32 testVoxelHemisphereHighlights[512];
        u32 testVoxelHemisphereHighlightCount;
    };



        
        void releaseHighID(chunk_data* chunkData, uint32_t chunkID);
        void releaseMidID(chunk_data* chunkData, uint32_t chunkID);
        uint16_t getHighResID(chunk_data* chunkData, uint32_t chunkID);
        uint16_t getMidResID(chunk_data* chunkData, uint32_t chunkID);
        uint16_t getLowResID(chunk_data* chunkData, uint32_t chunkID);
        

        ivec3 calculateChunkCoordinates(vec3 position);
        ivec3 calculateFPTChunkCoordinates(fpt_vec3 position);
        void intersectChunks(chunk_data* chunkData, const vec3& aabbMin, const vec3& aabbMax, ivec3* chunkArray, uint32_t arrayMaxSize, uint32_t& chunkCount);
        void intersectChunks(chunk_data* chunkData, const fpt_vec3&  aabbMin, const fpt_vec3&  aabbMax, const ivec3& startchunk_coords, ivec3* chunkArray, uint32_t arrayMaxSize, uint32_t& chunkCount);

        void getIntersectingChunkCoordinates(chunk_data* chunkData, const vec3& aabbMin, const vec3& aabbMax, ivec3 start_chunk_coords);
        void getIntersectingChunkCoordinates(chunk_data* chunkData,const fpt_vec3& aabbMin, const fpt_vec3& aabbMax, ivec3 start_chunk_coords);
        void updateChunkText(chunk_data* chunkData, uint32_t chunkID);

        uint32_t getChunkHash(chunk_data* chunkData, const ivec3& newchunk_coords);
        void findOrCreateChunks(chunk_data* chunkData, memory_arena *Arena = 0);
        uint32_t findOrCreateChunk(chunk_data* chunkData, const ivec3& newchunk_coords, memory_arena *Arena = 0, bool create = false/* , bool test = false */);
        uint32_t getchunkID(chunk_data* chunkData, const ivec3& chunk_coords);
        uint32_t reusechunkID();
        void recyclechunkID(uint32_t chunkID);
        void reset(chunk_data* chunkData);
        void destroyChunk(game_state* GameState, chunk_data* chunkData, u32 chunkID);
            

        void removeVisibleChunk(chunk_data& chunkData, uint32_t chunkID);
        void queueChunkDestruction(chunk_data& chunkData, uint32_t chunkID);

        void cancelChunkDestruction(chunk_data& chunkData, uint32_t chunkID);
        
        void compareIntersectingChunkArrays(chunk_data& chunkData);

     
