#include "chunkManager.h"
#include "boxIntersect.h"
#include "bvh.h"
// #include "labour/managers/fontManager.h"

    //pop and swap
    inline void RemoveChunkStage(u32 chunkID, uint32_t* pass_chunkIDs, uint32_t* pass_chunkIDMap, int32_t* pass_count){
        (*pass_count)--;
        Assert(*pass_count >= 0);
        int32_t index = pass_chunkIDMap[chunkID];
        int32_t lastIndex = (*pass_count);
        uint32_t lastChunkID = pass_chunkIDs[lastIndex];
        if(index != lastIndex){
            pass_chunkIDs[index] = lastChunkID;
            pass_chunkIDMap[lastChunkID] = index;
        }
    }

    inline void SetChunkStage(chunk_data* chunkData, u32 chunkID, ChunkStage stage){
        ChunkStage prevStage = chunkData->chunkStages[chunkID];
        switch(prevStage){
            case chunkStage_firstPass: {
                Assert(chunkData->firstPass_chunkID_count >= 0);
                RemoveChunkStage(chunkID, chunkData->firstPass_chunkIDs, chunkData->firstPass_chunkIDMap, &chunkData->firstPass_chunkID_count);
                int debug = 0;
            }break;
            case chunkStage_secondPass:{
                Assert(chunkData->secondPass_chunkID_count >= 0);
                PRINT("removing chunkID: %u from secondPass, secondPass count: %d\n", chunkID, chunkData->secondPass_chunkID_count);
                RemoveChunkStage(chunkID, chunkData->secondPass_chunkIDs, chunkData->secondPass_chunkIDMap, &chunkData->secondPass_chunkID_count);
                int debug = 0;
            }break;
            default:{}break;
        }

        switch(stage){
            case chunkStage_firstPass: {
                Assert(chunkData->firstPass_chunkID_count < MAX_CHUNKS);
                if(chunkData->chunkStages[chunkID] != stage){
                    int32_t index = chunkData->firstPass_chunkID_count++;
                    chunkData->firstPass_chunkIDMap[chunkID] = index;
                    chunkData->firstPass_chunkIDs[index] = chunkID;
                    chunkData->chunkStages[chunkID] = stage;
                }
            }break;
            case chunkStage_secondPass:{
                Assert(chunkData->secondPass_chunkID_count < MAX_CHUNKS);
                if(chunkData->chunkStages[chunkID] != stage){
                    int32_t index = chunkData->secondPass_chunkID_count++;
                    chunkData->secondPass_chunkIDMap[chunkID] = index;
                    chunkData->secondPass_chunkIDs[index] = chunkID;
                    chunkData->chunkStages[chunkID] = stage;
                }
            }break;
            //if we forget to set it here, we will keep removing from the last set stage
            //if the stage is none, then we would never otherwise reset it, so we would keep removing from the last set stage
            default:{
                    chunkData->chunkStages[chunkID] = stage;
            }break;
        }

    }


    float valueNoiseHash(ivec3 p){
        //optional hash operation
        #if 1 //faster
            //3d -> 1d
            int n = p.x*23 + p.y*113 + p.z*311;
            //1d hash by hugo elias
            n = (n << 13) ^ n;
            n = n * (n * n * 15731 + 789221) + 1376312589;
            return -1.0f+2.0f*(float)(n & 0x0fffffff)/(float)(0x0fffffff);
        #else
            vec3 temp = {0.71f, 0.113f, 0.419f};
            vec3 pf = {(float)p.x, (float)p.y,(float)p.z};
            pf = 50.0f*vec3_fract(pf*0.3183099f + temp);
            return -1.0f+2.0f*fract(pf.x*pf.y*pf.z*(pf.x+pf.y+pf.z));
        #endif
    }
    
    void release_brickmap_mesh(game_state* GameState, chunk_data* chunkData, uint32_t chunkID){
    TIMED_BLOCK("Release Brickmap Mesh");

        #ifndef SERVER_BUILD
            //reqlinquish SSBO index here
            chunk_destroy_command newCommand = {};
            newCommand.chunkID = chunkID;
            u32& commandCount = GameState->RenderCommandData->chunkDestroyCommandCount;
            GameState->RenderCommandData->chunkDestroyCommands[commandCount++] = newCommand; 
        
        #endif


    }


void releaseChunkResources(game_state* GameState, chunk_data* chunkData, uint32_t chunkID){
    TIMED_BLOCK("Release Chunk Resources");



    memset(chunkData->brickmaps[chunkID].voxels, 0 , sizeof(uint8_t) * pbmr3);
    memset(chunkData->coarse_brickmaps[chunkID].active_count , 0, sizeof(uint16_t) * coarse3);

    chunkData->brickmaps[chunkID].active_count = 0;

}

    
    ivec3 calculateChunkCoordinates(vec3 position){
        TIMED_BLOCK("Calculate Chunk Coordinates");
        // //PRINT("chunk position: (%f, %f, %f)\n", position.x, position.y, position.z);
        auto chunkCoord = [](float pos) -> int {
        // Shift position so the range -chunkSize/2 to chunkSize/2 maps to 0
            // Then divide by chunkSize and floor the result
            // //PRINT("halfChunkSize: %f, inverseChunkSize: %f\n", halfChunkSize, inverseChunkSize);
            // //PRINT("pos: %f, int: %f\n", pos, (floor((pos + halfChunkSize) * inverseChunkSize)));
            return static_cast<int>(floor((pos + g_HalfChunkSize) * g_InverseChunkSize));
        };

        ivec3 coords = {};
        coords.x = chunkCoord(position.x);
        coords.y = chunkCoord(position.y);
        coords.z = chunkCoord(position.z);
        // ////spdlog::info("calculateChunkCoordinates() {} {} {}", coords.x,coords.y,coords.z);
        return coords;
    }

    ivec3 calculateFPTChunkCoordinates(fpt_vec3 position) {
        TIMED_BLOCK("Calculate FPT Chunk Coordinates");
        // vec3 debugPos = fpt_to_glm_vec3(position);
        // //spdlog::info("calculate chunk coords of pos: {} {} {}", debugPos.x,debugPos.y,debugPos.z);
        ivec3 scaled = {
            fpt2i(fpt_floor(fpt_mul(fpt_add(position.x, FPT_HALF_CHUNK_SIZE), FPT_INVERSE_CHUNK_SIZE))),
            fpt2i(fpt_floor(fpt_mul(fpt_add(position.y, FPT_HALF_CHUNK_SIZE), FPT_INVERSE_CHUNK_SIZE))),
            fpt2i(fpt_floor(fpt_mul(fpt_add(position.z, FPT_HALF_CHUNK_SIZE), FPT_INVERSE_CHUNK_SIZE)))
        };
        // //spdlog::info("chunk coords:                  {} {} {}", scaled.x, scaled.y, scaled.z);
        return (scaled);
    }

    ivec3 calculateFPTChunkCoordinatesPadded(fpt_vec3 position) {
        TIMED_BLOCK("Calculate FPT Chunk Coordinates");
        // vec3 debugPos = fpt_to_glm_vec3(position);
        // //spdlog::info("calculate chunk coords of pos: {} {} {}", debugPos.x,debugPos.y,debugPos.z);
        ivec3 scaled = {
            fpt2i(fpt_floor(fpt_mul(fpt_add(position.x, FPT_HALF_CHUNK_SIZE_PADDED), FPT_INVERSE_CHUNK_SIZE_PADDED))),
            fpt2i(fpt_floor(fpt_mul(fpt_add(position.y, FPT_HALF_CHUNK_SIZE_PADDED), FPT_INVERSE_CHUNK_SIZE_PADDED))),
            fpt2i(fpt_floor(fpt_mul(fpt_add(position.z, FPT_HALF_CHUNK_SIZE_PADDED), FPT_INVERSE_CHUNK_SIZE_PADDED)))
        };
        // //spdlog::info("chunk coords:                  {} {} {}", scaled.x, scaled.y, scaled.z);
        return (scaled);
    }


    void intersectChunks(chunk_data* chunkData,const vec3& aabbMin, const vec3& aabbMax, ivec3* chunkArray, uint32_t arrayMaxSize, uint32_t& chunkCount) {
        TIMED_BLOCK("Intersect Chunks");
        auto& intersectingchunk_coords = chunkData->intersectingchunk_coords;
        ivec3 chunkMin = calculateChunkCoordinates(aabbMin);
        ivec3 chunkMax = calculateChunkCoordinates(aabbMax);
        for (int x = chunkMin.x; x <= chunkMax.x; ++x) {
            for (int y = chunkMin.y; y <= chunkMax.y; ++y) {
                for (int z = chunkMin.z; z <= chunkMax.z; ++z) {
                    if(chunkCount >= arrayMaxSize){
                        //spdlog::warn("too many intersected chunks, above {}, return from getIntersectingChunkCoordinates()", arrayMaxSize);
                        return;
                    }
                    chunkArray[chunkCount] = (ivec3_create(x, y, z));
                    chunkCount++;
                }
            }
        }
    }

    void intersectChunks(chunk_data* chunkData,const fpt_vec3& aabbMin, const fpt_vec3& aabbMax, const ivec3& startchunk_coords, ivec3* chunkArray, uint32_t arrayMaxSize, uint32_t& chunkCount) {
        TIMED_BLOCK("FPT Intersect Chunks");
        auto& intersectingchunk_coords = chunkData->intersectingchunk_coords;
        ivec3 chunkMin = calculateFPTChunkCoordinates(aabbMin) + startchunk_coords;
        ivec3 chunkMax = calculateFPTChunkCoordinates(aabbMax) + startchunk_coords;
        for (int x = chunkMin.x; x <= chunkMax.x; ++x) {
            for (int y = chunkMin.y; y <= chunkMax.y; ++y) {
                for (int z = chunkMin.z; z <= chunkMax.z; ++z) {
                    if(chunkCount >= arrayMaxSize){
                        //spdlog::warn("too many intersected chunks, above {}, return from getIntersectingChunkCoordinates()", arrayMaxSize);
                        return;
                    }
                    chunkArray[chunkCount] = (ivec3_create(x, y, z));
                    chunkCount++;
                }
            }
        }
    }

    void intersectChunksPadded(chunk_data* chunkData,const fpt_vec3& aabbMin, const fpt_vec3& aabbMax, const ivec3& startchunk_coords, ivec3* chunkArray, uint32_t arrayMaxSize, uint32_t& chunkCount) {
        TIMED_BLOCK("FPT Intersect Padded Chunks");
        auto& intersectingchunk_coords = chunkData->intersectingchunk_coords;
        ivec3 chunkMin = calculateFPTChunkCoordinatesPadded(aabbMin) + startchunk_coords;
        ivec3 chunkMax = calculateFPTChunkCoordinatesPadded(aabbMax) + startchunk_coords;
        for (int x = chunkMin.x; x <= chunkMax.x; ++x) {
            for (int y = chunkMin.y; y <= chunkMax.y; ++y) {
                for (int z = chunkMin.z; z <= chunkMax.z; ++z) {
                    if(chunkCount >= arrayMaxSize){
                        //spdlog::warn("too many intersected chunks, above {}, return from getIntersectingChunkCoordinates()", arrayMaxSize);
                        return;
                    }
                    chunkArray[chunkCount] = (ivec3_create(x, y, z));
                    chunkCount++;
                }
            }
        }
    }


    //populates intersectingChunkCoordVector with chunk_coords of intersecting chunks (of the tested aabb)
    void getIntersectingChunkCoordinates(chunk_data* chunkData,const vec3& aabbMin, const vec3& aabbMax, ivec3 start_chunk_coords) {
        TIMED_BLOCK("Get Intersecting Chunk Coords");
        // ////spdlog::info("getIntersectingChunkCoordinates()");
        // ////spdlog::info("camera min: {} {} {}", aabbMin.x,aabbMin.y,aabbMin.z);
        // ////spdlog::info("camera max: {} {} {}", aabbMax.x,aabbMax.y,aabbMax.z);
        auto& intersectingchunk_coords = chunkData->intersectingchunk_coords;
        int index = 0;
        chunkData->intersectingChunkCount = 0;
        //Loggers::chunk_logger->trace("getting intersecting chunk coordinates, initializing vector to return");
        // Calculate chunk coordinates for min and max
        ivec3 chunkMin = calculateChunkCoordinates(aabbMin);
        //Loggers::chunk_logger->trace("calculated min chunk coordinates");
        ivec3 chunkMax = calculateChunkCoordinates(aabbMax);
        //Loggers::chunk_logger->trace("calculated max chunk coordinates");
        // ////spdlog::info("chunkMin: {} {} {}", chunkMin.x,chunkMin.y,chunkMin.z);
        // ////spdlog::info("chunkMax: {} {} {}", chunkMax.x,chunkMax.y,chunkMax.z);
        // Iterate through all chunks that the AABB spans
        for (int x = chunkMin.x; x <= chunkMax.x; ++x) {
            for (int y = chunkMin.y; y <= chunkMax.y; ++y) {
                for (int z = chunkMin.z; z <= chunkMax.z; ++z) {
                    // if(x == 2 && y == -1 && z == -4){
                    //     //PRINT("CHUNK 2 -1 -4 ADDED in getIntersectingChunkCoordinates()\n");
                    // }
                    intersectingchunk_coords[index] = (ivec3_create(x, y, z) + start_chunk_coords);
                    chunkData->intersectingChunkCount++;
                    index++;
                    if(index >= MAX_CHUNKS){
                        //spdlog::warn("too many intersected chunks, above MAX_CHUNKS, return from getIntersectingChunkCoordinates()");
                        return;
                    }
                    ////PRINT("pushed intersecting chunk: ({}, {}, {}) into vector", x, y, z);
                }
            }
        }
    }

        //populates intersectingChunkCoordVector with chunk_coords of intersecting chunks (of the tested aabb)
    void getIntersectingChunkCoordinates(chunk_data* chunkData,const fpt_vec3& aabbMin, const fpt_vec3& aabbMax, ivec3 start_chunk_coords) {
        TIMED_BLOCK("FPT Get Intersecting Chunk Coords");
        // ////spdlog::info("getIntersectingChunkCoordinates()");
        // ////spdlog::info("camera min: {} {} {}", aabbMin.x,aabbMin.y,aabbMin.z);
        // ////spdlog::info("camera max: {} {} {}", aabbMax.x,aabbMax.y,aabbMax.z);
        auto& intersectingchunk_coords = chunkData->intersectingchunk_coords;
        int index = 0;
        chunkData->intersectingChunkCount = 0;
        //Loggers::chunk_logger->trace("getting intersecting chunk coordinates, initializing vector to return");
        // Calculate chunk coordinates for min and max
        ivec3 chunkMin = calculateFPTChunkCoordinates(aabbMin);
        //Loggers::chunk_logger->trace("calculated min chunk coordinates");
        ivec3 chunkMax = calculateFPTChunkCoordinates(aabbMax);
        //Loggers::chunk_logger->trace("calculated max chunk coordinates");
        // ////spdlog::info("chunkMin: {} {} {}", chunkMin.x,chunkMin.y,chunkMin.z);
        // ////spdlog::info("chunkMax: {} {} {}", chunkMax.x,chunkMax.y,chunkMax.z);
        // Iterate through all chunks that the AABB spans
        for (int x = chunkMin.x; x <= chunkMax.x; ++x) {
            for (int y = chunkMin.y; y <= chunkMax.y; ++y) {
                for (int z = chunkMin.z; z <= chunkMax.z; ++z) {
                    // if(x == 2 && y == -1 && z == -4){
                    //     //PRINT("CHUNK 2 -1 -4 ADDED in getIntersectingChunkCoordinates()\n");
                    // }
                    intersectingchunk_coords[index] = (ivec3_create(x, y, z) + start_chunk_coords);
                    chunkData->intersectingChunkCount++;
                    index++;
                    if(index >= MAX_CHUNKS){
                        //spdlog::warn("too many intersected chunks, above MAX_CHUNKS, return from getIntersectingChunkCoordinates()");
                        return;
                    }
                    ////PRINT("pushed intersecting chunk: ({}, {}, {}) into vector", x, y, z);
                }
            }
        }
    }

    //passes by reference a vector for chunk indices
    //getIntersectingchunk_coordsVector contains the chunk_coords
    //why dont we directly pass in the chunk_coords? and return the intersectingChunkIndices??
    void findOrCreateChunks(chunk_data* chunkData, memory_arena *Arena){
        TIMED_BLOCK("Find or create Chunks");

        chunkData->intersectingChunkIndicesCount = 0;
        bool create = true;
        for(int i = 0; i < chunkData->intersectingChunkCount; i++){

            uint32_t hashSlot = getChunkHash(chunkData, chunkData->intersectingchunk_coords[i]);
            // bool removed = ChunkManager::removeChunkCreationQueue(chunkData, chunkData->intersectingchunk_coords[i]);
            //in case chunk was in queue, we remove it before creating

            uint32_t newchunkID = findOrCreateChunk(chunkData, chunkData->intersectingchunk_coords[i], Arena, create);
            chunkData->intersectingChunkIndices[chunkData->intersectingChunkIndicesCount] = newchunkID;
            chunkData->intersectingChunkIndicesCount++;
        }
    }

    uint32_t getChunkHash(chunk_data* chunkData, const ivec3& newchunk_coords){
        // TIMED_BLOCK("Get Chunk Hash");
        uint32_t hashValue = (newchunk_coords.x*19) + ((newchunk_coords.y) *7) + ((newchunk_coords.z) *3);
        
        uint32_t hashSlot = hashValue & (ArrayCount(chunkData->chunkHash) - 1); //same as module ONLY IF chunkHas is power of 2

        assert(hashSlot < ArrayCount(chunkData->chunkHash));
        return hashSlot;
    }




    uint32_t getchunkID(chunk_data* chunkData, const ivec3& chunk_coords){
        // TIMED_BLOCK("Get Chunk ID");
        uint32_t chunkID = NULL_CHUNK;
        uint32_t hashSlot = getChunkHash(chunkData, chunk_coords);
        chunk_hash *chunk = chunkData->chunkHash + hashSlot;
        do {
            if (chunk->coords == chunk_coords) {
                chunkID = chunk->index;
                break;  // Found the chunk, exit loop
            }
            chunk = chunk->NextInHash;
        } while (chunk);  // Continue while there are more chunks in the chain
        return chunkID;
    }


    uint32_t findOrCreateChunk(chunk_data* chunkData, const ivec3& newchunk_coords, memory_arena *Arena, bool create/* , bool test */){
        TIMED_BLOCK("Find or create Chunk");
                
        uint32_t chunkID = NULL_CHUNK;

        uint32_t hashSlot = getChunkHash(chunkData, newchunk_coords);

        chunk_hash *chunk = chunkData->chunkHash + hashSlot;
        do {
            // Check if current chunk matches the coordinates we're looking for
            if (chunk->coords == newchunk_coords) {

                if(chunkID != NULL_CHUNK){
                    //spdlog::warn("REDUNDENT CHUNK INDEX CREATED {} for coords: {} {} {}", chunkID, newchunk_coords.x,newchunk_coords.y,newchunk_coords.z);
                }

                chunkID = chunk->index;
                // if(test)PRINT("found chunk index: %d, %d %d %d in findOrCreateChunk()\n", chunkID, chunk->coords.x, chunk->coords.y, chunk->coords.z);
                break;  // Found the chunk, exit loop
            }
            
            // If we have an arena, the current chunk is initialized, and there's no next chunk
            if (Arena && (chunk->coords.x != CHUNK_COORDS_UNINITIALIZED) && (!chunk->NextInHash) && create) {
                // Create a new chunk in the chain
                if(hashSlot == 19){
                    //PRINT("slot 19 MAYBE GETTING NULLIFIED\n");
                }
                // if(test)PRINT("inserting chunk %d %d %d to hash %d, pushing new chunk to the chain\n", newchunk_coords.x,newchunk_coords.y,newchunk_coords.z, hashSlot);
                chunk->NextInHash = PushStruct(Arena, chunk_hash);
                chunk = chunk->NextInHash;
                chunk->coords.x = CHUNK_COORDS_UNINITIALIZED;
                chunk->NextInHash = 0;

            }
            
            // If we have an arena and the current chunk is uninitialized
            if (Arena && (chunk->coords.x == CHUNK_COORDS_UNINITIALIZED) && create) {
                // Initialize this chunk with the new coordinates
                //new index logic here
                bool takenFromFreeList = false;
                if(chunkData->chunkFreeListSize > 0){
                    //handle index recycling logic
                    chunkData->chunkFreeListSize--;
                    chunkID = chunkData->chunkFreeList[chunkData->chunkFreeListSize];
                    chunkData->activeChunkCount++;
                    takenFromFreeList = true;


                }
                else if(chunkData->activeChunkCount < MAX_CHUNKS - 1){
                    chunkID = chunkData->activeChunkCount;
                    chunkData->activeChunkCount++;
                }
                else{
                    //PRINT("activeChunkCount is %d\n", chunkData->activeChunkCount);
                    //PRINT("MAX_ACTIVE_CHUNKS is %d\n", MAX_CHUNKS);
                    PRINT("not enough space for a new chunk, returning NULL_CHUNK\n");
                    return NULL_CHUNK;
                }
                
                chunk->coords = newchunk_coords;
                chunk->index = chunkID;
                u32 version =  chunkData->versions[chunkID];


                PRINT("initializing new chunk %3d %3d %3d with chunkID: %3d | Version: %3u in hash %4d\n", newchunk_coords.x,newchunk_coords.y,newchunk_coords.z, chunkID, version, hashSlot);

                chunkData->coords  [chunkID] = newchunk_coords;
                chunkData->bvhTrees[chunkID].chunk_coords = newchunk_coords;

                break;  // Exit loop
            }

            // Move to the next chunk in the chain
            // if(test)PRINT("HASH COLLISION with currently visited chunk coords: %d %d %d\n", chunk->coords.x,chunk->coords.y,chunk->coords.z);
            chunk = chunk->NextInHash;
        } while (chunk);  // Continue while there are more chunks in the chain

        return chunkID;
    }

    uint32_t reusechunkID(){
        return 0;
    }
    void recyclechunkID(uint32_t chunkID){

     
    }
        
    void reset(chunk_data* chunkData){
        ivec3 uninitialized = ivec3_create(CHUNK_COORDS_UNINITIALIZED);
        for(int i = 0; i < MAX_CHUNKS; i++){
            // chunkData->textBuffers[i] = BGFX_INVALID_HANDLE;
            chunkData->chunkHash[i].coords = uninitialized;
            chunkData->bvhTrees[i].rootNode = NULL_NODE;
            chunkData->dirty_chunkIDs[i] = NULL_CHUNK;
            // chunkData->idToHighRes[i] = NULL_LOD_INDEX;
            // chunkData->idToMidRes[i] = NULL_LOD_INDEX;
            chunkData->safeToEdit[i] = true;
            // chunkData->brickmap_mesh_handles[i] = BGFX_INVALID_HANDLE;

            reset_bvh(chunkData, i);

            memset(chunkData->entity_interactions[i], 0, sizeof(entity_interaction_data) * MAX_ENTITY_INTERACTIONS);
            chunkData->entity_interactions_count[i] = 0;

        }
        // for(int i = 0; i < MAX_HIGH_RES; i++) {chunkData->highResQueue[i] = i;}
        // for(int i = 0; i < MAX_MID_RES; i++){chunkData->midResQueue[i] = i; }
        
        #ifndef SERVER_BUILD
            // for(int i = 0; i < MAX_HIGH_RES*HIGH_RES_BRICKMAPS; i++) {chunkData->highResHandles[i] = BGFX_INVALID_HANDLE;}
            // for(int i = 0; i < MAX_MID_RES*MID_RES_BRICKMAPS; i++){chunkData->midResHandles[i] = BGFX_INVALID_HANDLE; }
        #endif
        
        for(int i = 0; i < MAX_WORKER_THREADS; i++){chunkData->workerThreads[i].workerID = i; chunkData->workerThreads[i].hasValidWork = false;}
        
        chunkData->chunk_coord_bounds = ivec3_create(0);//TOROIDAL SPACE TEST
        chunkData->toroidal_space_enabled = false;
        chunkData->dirty_chunkID_count = 0;
        chunkData->chunkFreeListSize = 0;
        chunkData->voxelizeQueueCount = 0;
        chunkData->intersectingChunkCount = 0;
        chunkData->chunkDifferenceCount = 0;
        chunkData->bufferCount = 0;
        chunkData->threadToAssign = 0;
        chunkData->voxelizeQueueCount = 0;
        chunkData->hasVoxWorkQueued = false;
        chunkData->freezeChunks = false;
        chunkData->voxelRayCastResult.chunkID = NULL_CHUNK;
        chunkData->selectedEntityChunkID = NULL_CHUNK;
        chunkData->new_visible_chunk_coords_count = 0;
        //voxel logic
        chunkData->voxelPathSearchIndex = 0;
        chunkData->brushSize = 1.0f;
        chunkData->fptBrushSize = FPT_ONE;
        chunkData->brushChunkIDCount = 0;
        chunkData->editedCount = 0;
        chunkData->cameraGridFreeListCount = 0;
        chunkData->cameraGridCount = 0;
        chunkData->cameraGridVisibleCount = 0;

        u8 tempTable[256] = {//ken perlin random noise table
            151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
            190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
            88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,134,139,48,27,166,
            77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
            102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,200,196,
            135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,250,124,123,
            5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
            223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,
            129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,228,
            251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,107,
            49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
            138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
        };

        float tempGradients3dTable[256] = 
        {
            0, 1, 1, 0,  0,-1, 1, 0,  0, 1,-1, 0,  0,-1,-1, 0,
            1, 0, 1, 0, -1, 0, 1, 0,  1, 0,-1, 0, -1, 0,-1, 0,
            1, 1, 0, 0, -1, 1, 0, 0,  1,-1, 0, 0, -1,-1, 0, 0,
            0, 1, 1, 0,  0,-1, 1, 0,  0, 1,-1, 0,  0,-1,-1, 0,
            1, 0, 1, 0, -1, 0, 1, 0,  1, 0,-1, 0, -1, 0,-1, 0,
            1, 1, 0, 0, -1, 1, 0, 0,  1,-1, 0, 0, -1,-1, 0, 0,
            0, 1, 1, 0,  0,-1, 1, 0,  0, 1,-1, 0,  0,-1,-1, 0,
            1, 0, 1, 0, -1, 0, 1, 0,  1, 0,-1, 0, -1, 0,-1, 0,
            1, 1, 0, 0, -1, 1, 0, 0,  1,-1, 0, 0, -1,-1, 0, 0,
            0, 1, 1, 0,  0,-1, 1, 0,  0, 1,-1, 0,  0,-1,-1, 0,
            1, 0, 1, 0, -1, 0, 1, 0,  1, 0,-1, 0, -1, 0,-1, 0,
            1, 1, 0, 0, -1, 1, 0, 0,  1,-1, 0, 0, -1,-1, 0, 0,
            0, 1, 1, 0,  0,-1, 1, 0,  0, 1,-1, 0,  0,-1,-1, 0,
            1, 0, 1, 0, -1, 0, 1, 0,  1, 0,-1, 0, -1, 0,-1, 0,
            1, 1, 0, 0, -1, 1, 0, 0,  1,-1, 0, 0, -1,-1, 0, 0,
            1, 1, 0, 0,  0,-1, 1, 0, -1, 1, 0, 0,  0,-1,-1, 0
        };

                
        float tempGradients2dTable[256] = 
        {
            0.130526192220052f, 0.99144486137381f, 0.38268343236509f, 0.923879532511287f, 0.608761429008721f, 0.793353340291235f, 0.793353340291235f, 0.608761429008721f,
            0.923879532511287f, 0.38268343236509f, 0.99144486137381f, 0.130526192220051f, 0.99144486137381f, -0.130526192220051f, 0.923879532511287f, -0.38268343236509f,
            0.793353340291235f, -0.60876142900872f, 0.608761429008721f, -0.793353340291235f, 0.38268343236509f, -0.923879532511287f, 0.130526192220052f, -0.99144486137381f,
            -0.130526192220052f, -0.99144486137381f, -0.38268343236509f, -0.923879532511287f, -0.608761429008721f, -0.793353340291235f, -0.793353340291235f, -0.608761429008721f,
            -0.923879532511287f, -0.38268343236509f, -0.99144486137381f, -0.130526192220052f, -0.99144486137381f, 0.130526192220051f, -0.923879532511287f, 0.38268343236509f,
            -0.793353340291235f, 0.608761429008721f, -0.608761429008721f, 0.793353340291235f, -0.38268343236509f, 0.923879532511287f, -0.130526192220052f, 0.99144486137381f,
            0.130526192220052f, 0.99144486137381f, 0.38268343236509f, 0.923879532511287f, 0.608761429008721f, 0.793353340291235f, 0.793353340291235f, 0.608761429008721f,
            0.923879532511287f, 0.38268343236509f, 0.99144486137381f, 0.130526192220051f, 0.99144486137381f, -0.130526192220051f, 0.923879532511287f, -0.38268343236509f,
            0.793353340291235f, -0.60876142900872f, 0.608761429008721f, -0.793353340291235f, 0.38268343236509f, -0.923879532511287f, 0.130526192220052f, -0.99144486137381f,
            -0.130526192220052f, -0.99144486137381f, -0.38268343236509f, -0.923879532511287f, -0.608761429008721f, -0.793353340291235f, -0.793353340291235f, -0.608761429008721f,
            -0.923879532511287f, -0.38268343236509f, -0.99144486137381f, -0.130526192220052f, -0.99144486137381f, 0.130526192220051f, -0.923879532511287f, 0.38268343236509f,
            -0.793353340291235f, 0.608761429008721f, -0.608761429008721f, 0.793353340291235f, -0.38268343236509f, 0.923879532511287f, -0.130526192220052f, 0.99144486137381f,
            0.130526192220052f, 0.99144486137381f, 0.38268343236509f, 0.923879532511287f, 0.608761429008721f, 0.793353340291235f, 0.793353340291235f, 0.608761429008721f,
            0.923879532511287f, 0.38268343236509f, 0.99144486137381f, 0.130526192220051f, 0.99144486137381f, -0.130526192220051f, 0.923879532511287f, -0.38268343236509f,
            0.793353340291235f, -0.60876142900872f, 0.608761429008721f, -0.793353340291235f, 0.38268343236509f, -0.923879532511287f, 0.130526192220052f, -0.99144486137381f,
            -0.130526192220052f, -0.99144486137381f, -0.38268343236509f, -0.923879532511287f, -0.608761429008721f, -0.793353340291235f, -0.793353340291235f, -0.608761429008721f,
            -0.923879532511287f, -0.38268343236509f, -0.99144486137381f, -0.130526192220052f, -0.99144486137381f, 0.130526192220051f, -0.923879532511287f, 0.38268343236509f,
            -0.793353340291235f, 0.608761429008721f, -0.608761429008721f, 0.793353340291235f, -0.38268343236509f, 0.923879532511287f, -0.130526192220052f, 0.99144486137381f,
            0.130526192220052f, 0.99144486137381f, 0.38268343236509f, 0.923879532511287f, 0.608761429008721f, 0.793353340291235f, 0.793353340291235f, 0.608761429008721f,
            0.923879532511287f, 0.38268343236509f, 0.99144486137381f, 0.130526192220051f, 0.99144486137381f, -0.130526192220051f, 0.923879532511287f, -0.38268343236509f,
            0.793353340291235f, -0.60876142900872f, 0.608761429008721f, -0.793353340291235f, 0.38268343236509f, -0.923879532511287f, 0.130526192220052f, -0.99144486137381f,
            -0.130526192220052f, -0.99144486137381f, -0.38268343236509f, -0.923879532511287f, -0.608761429008721f, -0.793353340291235f, -0.793353340291235f, -0.608761429008721f,
            -0.923879532511287f, -0.38268343236509f, -0.99144486137381f, -0.130526192220052f, -0.99144486137381f, 0.130526192220051f, -0.923879532511287f, 0.38268343236509f,
            -0.793353340291235f, 0.608761429008721f, -0.608761429008721f, 0.793353340291235f, -0.38268343236509f, 0.923879532511287f, -0.130526192220052f, 0.99144486137381f,
            0.130526192220052f, 0.99144486137381f, 0.38268343236509f, 0.923879532511287f, 0.608761429008721f, 0.793353340291235f, 0.793353340291235f, 0.608761429008721f,
            0.923879532511287f, 0.38268343236509f, 0.99144486137381f, 0.130526192220051f, 0.99144486137381f, -0.130526192220051f, 0.923879532511287f, -0.38268343236509f,
            0.793353340291235f, -0.60876142900872f, 0.608761429008721f, -0.793353340291235f, 0.38268343236509f, -0.923879532511287f, 0.130526192220052f, -0.99144486137381f,
            -0.130526192220052f, -0.99144486137381f, -0.38268343236509f, -0.923879532511287f, -0.608761429008721f, -0.793353340291235f, -0.793353340291235f, -0.608761429008721f,
            -0.923879532511287f, -0.38268343236509f, -0.99144486137381f, -0.130526192220052f, -0.99144486137381f, 0.130526192220051f, -0.923879532511287f, 0.38268343236509f,
            -0.793353340291235f, 0.608761429008721f, -0.608761429008721f, 0.793353340291235f, -0.38268343236509f, 0.923879532511287f, -0.130526192220052f, 0.99144486137381f,
            0.38268343236509f, 0.923879532511287f, 0.923879532511287f, 0.38268343236509f, 0.923879532511287f, -0.38268343236509f, 0.38268343236509f, -0.923879532511287f,
            -0.38268343236509f, -0.923879532511287f, -0.923879532511287f, -0.38268343236509f, -0.923879532511287f, 0.38268343236509f, -0.38268343236509f, 0.923879532511287f,
        };
        memcpy(chunkData->gradients3d, tempGradients3dTable, sizeof(float) * 256);
        memcpy(chunkData->gradients2d, tempGradients2dTable, sizeof(float) * 256);

        // for (int i = 0; i < 512; i++) {
        //     chunkData->perlinPermTable[i] = tempTable[i & 255];
        // }
        memcpy(chunkData->perlinPermTable + 0  , tempTable, sizeof(u8) * 256);
        memcpy(chunkData->perlinPermTable + 256, tempTable, sizeof(u8) * 256);



        vec3 gradients3d[12] = {
            {1,1,0}, {-1,1,0}, {1,-1,0}, {-1,-1,0},
            {1,0,1}, {-1,0,1}, {1,0,-1}, {-1,0,-1},
            {0,1,1}, {0,-1,1}, {0,1,-1}, {0,-1,-1}
        };
        chunkData->gradientX[0 ] = 1; chunkData->gradientX[1 ] =-1; chunkData->gradientX[2 ] = 1; chunkData->gradientX[3 ] =-1; 
        chunkData->gradientX[4 ] = 1; chunkData->gradientX[5 ] =-1; chunkData->gradientX[6 ] = 1; chunkData->gradientX[7 ] =-1; 
        chunkData->gradientX[8 ] = 0; chunkData->gradientX[9 ] = 0; chunkData->gradientX[10] = 0; chunkData->gradientX[11] = 0; 

        chunkData->gradientY[0 ] = 1; chunkData->gradientY[1 ] = 1; chunkData->gradientY[2 ] =-1; chunkData->gradientY[3 ] =-1; 
        chunkData->gradientY[4 ] = 0; chunkData->gradientY[5 ] = 0; chunkData->gradientY[6 ] = 0; chunkData->gradientY[7 ] = 0; 
        chunkData->gradientY[8 ] = 1; chunkData->gradientY[9 ] =-1; chunkData->gradientY[10] = 1; chunkData->gradientY[11] =-1; 

        chunkData->gradientZ[0 ] = 0; chunkData->gradientZ[1 ] = 0; chunkData->gradientZ[2 ] = 0; chunkData->gradientZ[3 ] = 0; 
        chunkData->gradientZ[4 ] = 1; chunkData->gradientZ[5 ] = 1; chunkData->gradientZ[6 ] =-1; chunkData->gradientZ[7 ] =-1; 
        chunkData->gradientZ[8 ] = 1; chunkData->gradientZ[9 ] = 1; chunkData->gradientZ[10] =-1; chunkData->gradientZ[11] =-1; 

        vec4 gradients4d[32] = {
            { 1, 1, 1, 0}, {-1, 1, 1, 0}, { 1,-1, 1, 0}, {-1,-1, 1, 0},
            { 1, 1,-1, 0}, {-1, 1,-1, 0}, { 1,-1,-1, 0}, {-1,-1,-1, 0},
            { 1, 1, 0, 1}, {-1, 1, 0, 1}, { 1,-1, 0, 1}, {-1,-1, 0, 1},
            { 1, 1, 0,-1}, {-1, 1, 0,-1}, { 1,-1, 0,-1}, {-1,-1, 0,-1},
            { 1, 0, 1, 1}, {-1, 0, 1, 1}, { 1, 0,-1, 1}, {-1, 0,-1, 1},
            { 1, 0, 1,-1}, {-1, 0, 1,-1}, { 1, 0,-1,-1}, {-1, 0,-1,-1},
            { 0, 1, 1, 1}, { 0,-1, 1, 1}, { 0, 1,-1, 1}, { 0,-1,-1, 1},
            { 0, 1, 1,-1}, { 0,-1, 1,-1}, { 0, 1,-1,-1}, { 0,-1,-1,-1}
        };

        int seed = 12345;
        u32 state = 0;
        rng_seed(&state, seed);
        
        for(int i = 0; i < 256; i++){
            chunkData->permutations2d[i] = i;
            chunkData->permutations3d[i] = i;
            chunkData->permutations4d[i] = i;

            float angle = (float)(i * 2.0 * PI / 256.0f);
            chunkData->directionGradients2d[i].x = cosf(angle);
            chunkData->directionGradients2d[i].y = sinf(angle);
            chunkData->directionGradients3d[i] = gradients3d[i % 12];
            chunkData->directionGradients4d[i] = gradients4d[i % 32];
        }
        //shuffle perm
        for(int i = 255; i > 0; i--){
            int j = rng_next_u32(&state) % (i + 1);
            u8 temp = chunkData->permutations2d[i];
            chunkData->permutations2d[i] = chunkData->permutations2d[j];
            chunkData->permutations2d[j] = temp;    
            chunkData->permutations3d[i] = chunkData->permutations3d[j];
            chunkData->permutations3d[j] = temp;
            chunkData->permutations4d[i] = chunkData->permutations4d[j];
            chunkData->permutations4d[j] = temp;
            
        }
        for(int i = 0; i < 256; i++){
            chunkData->permutations2d[256 + i] = chunkData->permutations2d[i];
            chunkData->permutations3d[256 + i] = chunkData->permutations3d[i];
            chunkData->permutations4d[256 + i] = chunkData->permutations4d[i];
        }
        for(int z = 0; z < VALUE_NOISE_TILE; z++){
            for(int y = 0; y < VALUE_NOISE_TILE; y++){
                for(int x = 0; x < VALUE_NOISE_TILE; x++){
                    chunkData->valueNoise3d[x][y][z] = valueNoiseHash({x,y,z});
                    PRINT("%d,%d,%d : %f\n", x,y,z,chunkData->valueNoise3d[x][y][z]);
                }

            }

        }



    }


    void destroyChunk(game_state* GameState, chunk_data* chunkData, u32 chunkID){
            TIMED_BLOCK("Destroy Chunk");
            ivec3 null_coords = ivec3_create(CHUNK_COORDS_UNINITIALIZED);
        
            // ////spdlog::info("destroying chunkID {}", chunkID);
            ivec3 chunk_coords = chunkData->coords[chunkID];
            uint32_t hashSlot = getChunkHash(chunkData, chunk_coords);

            bvh_tree& tree = chunkData->bvhTrees[chunkID];
            if(tree.entityToNodeID[0] != NULL_NODE){
                PRINT("DELETING CHUNK WITH AN ENTITY INSIDE?? ID: %d, %d %d %d\n", chunkID, chunk_coords.x, chunk_coords.y, chunk_coords.z);
            }
            // if(aabbIntersectTest(pos - g_HalfChunkSize, pos + g_HalfChunkSize, cameraMin,  cameraMax)){              
            //     ////spdlog::info("camera min: {} {} {}", cameraMin.x,cameraMin.y,cameraMin.z);
            //     ////spdlog::info("camera max: {} {} {}", cameraMax.x,cameraMax.y,cameraMax.z);
            //     // ////spdlog::info("camera SHRUNK min: {} {} {}", cameraShrunkMin.x,cameraShrunkMin.y,cameraShrunkMin.z);
            //     // ////spdlog::info("camera SHRUNK max: {} {} {}", cameraShrunkMax.x,cameraShrunkMax.y,cameraShrunkMax.z);
            //     //PRINT("destroying chunk %d %d %d WITHIN CAMERA FRUSTUM?? index: %d\n", chunk_coords.x,chunk_coords.y,chunk_coords.z, chunkID);
            //     //PRINT("destroying chunk %d %d %d WITHIN CAMERA FRUSTUM?? index: %d\n", chunk_coords.x,chunk_coords.y,chunk_coords.z, chunkID);
            // }
            // assert(!isChunkVisible(*chunkData, chunkID) && "DESTROYING VISIBLE CHUNK??");
            // removeVisibleChunk(*chunkData, chunkID);
            // cancelChunkDestruction(*chunkData, chunkID);
        
            //need to move the values around to keep the linked list packed to prevent double entries
            chunk_hash* current = chunkData->chunkHash + hashSlot;
            chunk_hash* last_valid = nullptr;
            chunk_hash* chunk_to_remove = nullptr;
            if(current && current->coords == chunk_coords)chunk_to_remove = current;

            while(current && current->NextInHash){
                //check if next node is the one to remove
                if(current->NextInHash->coords == chunk_coords)chunk_to_remove = current->NextInHash;
                if(current->NextInHash->coords.x != CHUNK_COORDS_UNINITIALIZED){
                    last_valid = current->NextInHash;
                }
                current = current->NextInHash;
            }
            if(!chunk_to_remove){
                PRINT("FAILED TO DESTROY chunkID: %d coords: %d %d %d\n", chunkID, chunkData->coords[chunkID].x, chunkData->coords[chunkID].y, chunkData->coords[chunkID].z);
            }
            Assert(chunk_to_remove && "CANT FIND CHUNK TO REMOVE IN DESTROY CHUNKS??");

            if(chunk_to_remove == last_valid || !last_valid){
                chunk_to_remove->coords.x = CHUNK_COORDS_UNINITIALIZED;
                chunk_to_remove->index = NULL_CHUNK;
            }else{
                chunk_to_remove->coords   = last_valid->coords;
                chunk_to_remove->index    = last_valid->index;
                last_valid->coords.x = CHUNK_COORDS_UNINITIALIZED;
                last_valid->index    = NULL_CHUNK;
            }

            SetChunkStage(chunkData, chunkID, ChunkStage::chunkStage_null);
            releaseChunkResources(GameState, chunkData, chunkID);
            PRINT("CLEARED chunkID: %d coords: %d %d %d\n", chunkID, chunkData->coords[chunkID].x, chunkData->coords[chunkID].y, chunkData->coords[chunkID].z);
            chunkData->coords[chunkID] = null_coords;

            reset_bvh(chunkData, chunkID);
            
            entity_interaction_data* entity_interactions = chunkData->entity_interactions[chunkID]; 
            memset(entity_interactions, 0, sizeof(entity_interaction_data) * MAX_ENTITY_INTERACTIONS);
            int& interaction_count = chunkData->entity_interactions_count[chunkID];
            interaction_count = 0;

            chunkData->idRecycledCount[chunkID]++;
            chunkData->chunkFreeList[chunkData->chunkFreeListSize] = chunkID;
            chunkData->versions[chunkID]++;
            chunkData->chunkFreeListSize++;
            chunkData->activeChunkCount--;
    }
    
    
    
    #if 0
    void destroyChunks(chunk_data* chunkData, vec3 cameraMax, vec3 cameraMin){
        ivec3 null_coords = ivec3_create(CHUNK_COORDS_UNINITIALIZED);
        uint32_t numToDestroy = chunkData->destroyCount;
        //iterate in reverse to avoid pop and swap expense
        for(int i = numToDestroy - 1; i >= 0; i--){

            uint32_t chunkID = chunkData->destroyToChunkIDMap[i]; 

            // ////spdlog::info("destroying chunkID {}", chunkID);
            ivec3 chunk_coords = chunkData->coords[chunkID];
            uint32_t hashSlot = getChunkHash(chunkData, chunk_coords);

            bvh_tree& tree = chunkData->bvhTrees[chunkID];
            if(tree.entityToNodeID[0] != NULL_NODE){
                PRINT("DELETING CHUNK WITH AN ENTITY INSIDE?? ID: %d, %d %d %d\n", chunkID, chunk_coords.x, chunk_coords.y, chunk_coords.z);
            }
            // if(aabbIntersectTest(pos - g_HalfChunkSize, pos + g_HalfChunkSize, cameraMin,  cameraMax)){              
            //     ////spdlog::info("camera min: {} {} {}", cameraMin.x,cameraMin.y,cameraMin.z);
            //     ////spdlog::info("camera max: {} {} {}", cameraMax.x,cameraMax.y,cameraMax.z);
            //     // ////spdlog::info("camera SHRUNK min: {} {} {}", cameraShrunkMin.x,cameraShrunkMin.y,cameraShrunkMin.z);
            //     // ////spdlog::info("camera SHRUNK max: {} {} {}", cameraShrunkMax.x,cameraShrunkMax.y,cameraShrunkMax.z);
            //     //PRINT("destroying chunk %d %d %d WITHIN CAMERA FRUSTUM?? index: %d\n", chunk_coords.x,chunk_coords.y,chunk_coords.z, chunkID);
            //     //PRINT("destroying chunk %d %d %d WITHIN CAMERA FRUSTUM?? index: %d\n", chunk_coords.x,chunk_coords.y,chunk_coords.z, chunkID);
            // }
            assert(!isChunkVisible(*chunkData, chunkID) && "DESTROYING VISIBLE CHUNK??");
            removeVisibleChunk(*chunkData, chunkID);
            cancelChunkDestruction(*chunkData, chunkID);
        
            //need to move the values around to keep the linked list packed to prevent double entries
            chunk_hash* current = chunkData->chunkHash + hashSlot;
            chunk_hash* last_valid = nullptr;
            chunk_hash* chunk_to_remove = nullptr;
            if(current && current->coords == chunk_coords)chunk_to_remove = current;

            while(current && current->NextInHash){
                //check if next node is the one to remove
                if(current->NextInHash->coords == chunk_coords)chunk_to_remove = current->NextInHash;
                if(current->NextInHash->coords.x != CHUNK_COORDS_UNINITIALIZED){
                    last_valid = current->NextInHash;
                }
                current = current->NextInHash;
            }
            assert(chunk_to_remove && "CANT FIND CHUNK TO REMOVE IN DESTROY CHUNKS??");

            if(chunk_to_remove == last_valid || !last_valid){
                chunk_to_remove->coords.x = CHUNK_COORDS_UNINITIALIZED;
                chunk_to_remove->index = NULL_CHUNK;
            }else{
                chunk_to_remove->coords   = last_valid->coords;
                chunk_to_remove->index    = last_valid->index;
                last_valid->coords.x = CHUNK_COORDS_UNINITIALIZED;
                last_valid->index    = NULL_CHUNK;
            }


            releaseChunkResources(chunkData, chunkID);
            // PRINT("CLEARED chunkID: %d coords: %d %d %d\n", chunkID, chunkData->coords[chunkID].x, chunkData->coords[chunkID].y, chunkData->coords[chunkID].z);
            chunkData->coords[chunkID] = null_coords;

            chunkData->flags[chunkID] = 0x00;
            reset_bvh(chunkData, chunkID);
            
            entity_interaction_data* entity_interactions = chunkData->entity_interactions[chunkID]; 
            memset(entity_interactions, 0, sizeof(entity_interaction_data) * MAX_ENTITY_INTERACTIONS);
            int& interaction_count = chunkData->entity_interactions_count[chunkID];
            interaction_count = 0;

            chunkData->idRecycledCount[chunkID]++;
            chunkData->chunkFreeList[chunkData->chunkFreeListSize] = chunkID;
            chunkData->chunkFreeListSize++;
            chunkData->activeChunkCount--;
        }
    }

    bool isChunkVisible(chunk_data& chunkData, uint32_t chunkID){
        if (chunkData.chunkIDToVisibleMap[chunkID] != NULL_CHUNK){
            return true;
        } 
        return false;
    }


  void addVisibleChunk(chunk_data& chunkData, uint32_t chunkID) {
        if (chunkData.visibleCount >= MAX_CHUNKS || chunkID >= MAX_CHUNKS){
            ////spdlog::info("visibleChunks is full, or chunkID too large");
            return;
        } 
        if (chunkData.chunkIDToVisibleMap[chunkID] != NULL_CHUNK){
            // ////spdlog::info("chunkID {} already visible at position: {}",chunkID,chunkData.chunkIDToVisibleMap[chunkID]);
            return;
        } 
        //PRINT("adding visible chunkID : %d, visibleIndex: %d\n", chunkID, chunkData.visibleCount);
        chunkData.visibleChunks[chunkData.visibleCount] = chunkID;

        chunkData.chunkIDToVisibleMap[chunkID] = chunkData.visibleCount;
        chunkData.visibleToChunkIDMap[chunkData.visibleCount] = chunkID;

        chunkData.visibleCount++;



    }

    void removeVisibleChunk(chunk_data& chunkData, uint32_t chunkID) {
        if (chunkID >= MAX_CHUNKS || chunkData.chunkIDToVisibleMap[chunkID] == NULL_CHUNK) return;

        uint32_t lastIndex = chunkData.visibleCount - 1;
        

        if(lastIndex != chunkData.chunkIDToVisibleMap[chunkID] && lastIndex != 0){
            uint32_t removedIndex = chunkData.chunkIDToVisibleMap[chunkID];

            chunkData.visibleChunks[removedIndex] = chunkData.visibleChunks[lastIndex];

            uint32_t lastchunkID = chunkData.visibleToChunkIDMap[lastIndex];
            chunkData.chunkIDToVisibleMap[lastchunkID] = removedIndex;
            chunkData.visibleToChunkIDMap[removedIndex] = chunkData.visibleToChunkIDMap[lastIndex];
            


        }

        chunkData.chunkIDToVisibleMap[chunkID] = NULL_CHUNK;
        chunkData.visibleToChunkIDMap[lastIndex] = NULL_CHUNK;
        chunkData.visibleCount--;
    }

    void queueChunkDestruction(chunk_data& chunkData, uint32_t chunkID) {
        if (chunkData.destroyCount >= MAX_CHUNKS || chunkID >= MAX_CHUNKS){
            //PRINT("destroyChunks is full, or chunkID too large\n");
            return;
        } 
        if (chunkData.chunkIDToDestroyMap[chunkID] != NULL_CHUNK){
            ////spdlog::info("chunkID {} already destroy at position: {}",chunkID,chunkData.chunkIDToDestroyMap[chunkID]);
            return;
        } 
        if(!chunkData.safeToEdit[chunkID]){
            //spdlog::error("queueChunkDestruction() chunkID {} NOT SAFE TO EDIT", chunkID);
            //spdlog::error("queueChunkDestruction() chunkID {} NOT SAFE TO EDIT", chunkID);
        }

        chunkData.destroyChunks[chunkData.destroyCount] = chunkID;

        chunkData.chunkIDToDestroyMap[chunkID] = chunkData.destroyCount;
        chunkData.destroyToChunkIDMap[chunkData.destroyCount] = chunkID;

        chunkData.destroyCount++;
    }

    void cancelChunkDestruction(chunk_data& chunkData, uint32_t chunkID) {
        if (chunkID >= MAX_CHUNKS || chunkData.chunkIDToDestroyMap[chunkID] == NULL_CHUNK) return;

        uint32_t lastIndex = chunkData.destroyCount - 1;

        if(lastIndex != chunkData.chunkIDToDestroyMap[chunkID] && lastIndex != 0){
            uint32_t removedIndex = chunkData.chunkIDToDestroyMap[chunkID];

            chunkData.destroyChunks[removedIndex] = chunkData.destroyChunks[lastIndex];

            uint32_t lastchunkID = chunkData.destroyToChunkIDMap[lastIndex];
            chunkData.chunkIDToDestroyMap[lastchunkID] = removedIndex;
            chunkData.destroyToChunkIDMap[removedIndex] = chunkData.destroyToChunkIDMap[lastIndex];

        }

        chunkData.chunkIDToDestroyMap[chunkID] = NULL_CHUNK;
        chunkData.destroyToChunkIDMap[lastIndex] = NULL_CHUNK;
        chunkData.destroyCount--;
    }

    //if there is no chunk in that slot, in that index for the visible coords, it means that chunk is no longer visible
    //if the newSlotCoords are invalid, it means there was no chunk inserted there during the last visibility test, so the chunk isnt visible
    void compareIntersectingChunkArrays(chunk_data& chunkData){
        //compare the old visible chunks and queue the missing ones for destruction
        ivec3 null = ivec3_create(CHUNK_COORDS_UNINITIALIZED);
// 
        for(int i = 0; i < chunkData.visibleCount; i++)
        {
            uint32_t chunkID = chunkData.visibleToChunkIDMap[i];
            if(!chunkData.safeToEdit[chunkID])continue;
            ivec3 newSlotCoords = chunkData.newVisiblechunk_coords[chunkID];
            chunkData.newVisiblechunk_coords[chunkID] = null;

            // if(newSlotCoords == null)continue;
            // assert(newSlotCoords.x != INT32_MAX ||newSlotCoords.y != INT32_MAX ||newSlotCoords.z != INT32_MAX && "REPLACING CHUNK COORDS WITH INVALID DATA!");
            ivec3 potentiallyInvisibleCoords = chunkData.coords[chunkID];
            if (potentiallyInvisibleCoords != newSlotCoords){ //the new visible chunk doesn't match, this chunk is no longer visible
                removeVisibleChunk(chunkData, chunkID);
                queueChunkDestruction(chunkData, chunkID);  
                // //spdlog::info("queueing chunkID {}, coords: {} {} {} for deletion, new slot coords: {} {} {}", chunkID, potentiallyInvisibleCoords.x,potentiallyInvisibleCoords.y,potentiallyInvisibleCoords.z, newSlotCoords.x,newSlotCoords.y,newSlotCoords.z);
                // if(newSlotCoords.x == 2147483647 || newSlotCoords.y == 2147483647 || newSlotCoords.z == 2147483647){
                    // if(newSlotCoords.x == INT32_MAX || newSlotCoords.y == INT32_MAX || newSlotCoords.z == INT32_MAX){
                    //     PRINT("NEW COORDS ARE NOT VALID!\n");
                    // }
            } 
  
        }

        // for(int i = 0; i < MAX_CHUNKS; i++) {
        //     chunkData.newVisiblechunk_coords[i] = null;
        // }

    }
    #endif



