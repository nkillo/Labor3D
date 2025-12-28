#pragma once

#include "chunkManager.h"

     
        // Manual clamp function
        #define CLAMP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
        
    ivec3 fpt_calculatePaddedVoxelCoordsFloor(fpt_vec3 pos) {

            float normalizedX = ((pos.x >> 16) + HALF_CHUNK_SIZE) * INVERSE_CHUNK_SIZE;
            float normalizedY = ((pos.y >> 16) + HALF_CHUNK_SIZE) * INVERSE_CHUNK_SIZE;
            float normalizedZ = ((pos.z >> 16) + HALF_CHUNK_SIZE) * INVERSE_CHUNK_SIZE;
            
            int scaledX = (normalizedX  * TOTAL_VOXELS);
            int scaledY = (normalizedY  * TOTAL_VOXELS);
            int scaledZ = (normalizedZ  * TOTAL_VOXELS);
            // printf("scaledXYZ    : %10.5f %10.5f %10.5f\n", fpt2fl(scaledX),fpt2fl(scaledY),fpt2fl(scaledZ));
            // printf("v FPT        : %d %d %d\n", scaledX, scaledY, scaledZ);
    
            ivec3 v = {scaledX, scaledY, scaledZ};
            // printf("v            : %d %d %d\n", v.x, v.y, v.z);
    
            v.x = CLAMP(v.x, 0, PADDED_VOXELS);
            v.y = CLAMP(v.y, 0, PADDED_VOXELS);
            v.z = CLAMP(v.z, 0, PADDED_VOXELS);
            // printf("CLAMPED v    : %d %d %d\n", v.x, v.y, v.z);

            return v;
    }

        ivec3 fpt_calculatePaddedVoxelCoordsCeil(fpt_vec3 pos) {

            float normalizedX = ((pos.x >> 16) + 1 + HALF_CHUNK_SIZE) * INVERSE_CHUNK_SIZE;
            float normalizedY = ((pos.y >> 16) + 1 + HALF_CHUNK_SIZE) * INVERSE_CHUNK_SIZE;
            float normalizedZ = ((pos.z >> 16) + 1 + HALF_CHUNK_SIZE) * INVERSE_CHUNK_SIZE;
            
            int scaledX = (normalizedX  * TOTAL_VOXELS);
            int scaledY = (normalizedY  * TOTAL_VOXELS);
            int scaledZ = (normalizedZ  * TOTAL_VOXELS);
            // printf("scaledXYZ    : %10.5f %10.5f %10.5f\n", fpt2fl(scaledX),fpt2fl(scaledY),fpt2fl(scaledZ));
            // printf("v FPT        : %d %d %d\n", scaledX, scaledY, scaledZ);
    
            ivec3 v = {scaledX, scaledY, scaledZ};
            // printf("v            : %d %d %d\n", v.x, v.y, v.z);
    
            v.x = CLAMP(v.x, 0, PADDED_VOXELS);
            v.y = CLAMP(v.y, 0, PADDED_VOXELS);
            v.z = CLAMP(v.z, 0, PADDED_VOXELS);
            // printf("CLAMPED v    : %d %d %d\n", v.x, v.y, v.z);

            return v;
    }




const ivec3 adjacentBrickmaps[26] = {
    ivec3_create( 1, 0, 0),
    ivec3_create(-1, 0, 0),
    ivec3_create( 0, 1, 0),
    ivec3_create( 0,-1, 0),
    ivec3_create( 0, 0, 1),
    ivec3_create( 0, 0,-1),

    ivec3_create( 1, 1, 0),
    ivec3_create( 1,-1, 0),
    ivec3_create(-1, 1, 0),
    ivec3_create(-1,-1, 0),

    ivec3_create( 0, 1, 1),
    ivec3_create( 0, 1,-1),
    ivec3_create( 0,-1, 1),
    ivec3_create( 0,-1,-1),

    ivec3_create( 1, 0, 1),
    ivec3_create( 1, 0,-1),
    ivec3_create(-1, 0, 1),
    ivec3_create(-1, 0,-1),

    ivec3_create( 1, 1, 1),
    ivec3_create( 1, 1,-1),
    ivec3_create( 1,-1, 1),
    ivec3_create( 1,-1,-1),
    ivec3_create(-1, 1, 1),
    ivec3_create(-1, 1,-1),
    ivec3_create(-1,-1, 1),
    ivec3_create(-1,-1,-1),
};

    ivec3 calculateBrickmapCoordinates(vec3 position, int grid_resolution);
    vec3 brickmapPosition_LOD(ivec3 brickmapCoordinates, vec3 chunkPosition, float brickmapScale, float brickmapCenterOffset);
    ivec3 calculatePaddedVoxelCoordinates(vec3 pos, vec3 brickmapPosition, float brickmapScale, float inverseBrickmapScale, float voxelSize);
    ivec3 calculateVoxelCoordinates(vec3 pos, vec3 brickmapPosition = vec3_create(0.0f), float brickmapScale = 64.0f, float inverseBrickmapScale = 1/64.0f);

    vec3 getVoxelWorldPosition(ivec3 voxel, vec3 brickmapMinCorner, float halfVoxelSize, uint32_t LODresolution);

    bool pick_voxel_in_chunk(chunk_data* chunkData, Camera* camera, int mouseX, int mouseY, 
                    rayCastResult& result /*, uint16_t brickmap_index, int& voxel_index
                    ,int& adjacent_chunk_buffer_index, int& adjacent_brickmap_index, int& adjacent_voxel_index, 
                    Chunk** selected_chunk, Chunk** adjacent_chunk,  ivec3& adjacent_chunk_coords*/);


    ////////////////////////////////////// FIXED POINT MATH FUNCTIONS ////////////////////////////
    ivec3 calculateBrickmapCoordinates(fpt_vec3 position, fpt grid_resolution);
    fpt_vec3 fpt_brickmapPosition_LOD(ivec3 brickmapCoordinates, fpt_vec3 chunkPosition, fpt brickmapScale, fpt brickmapCenterOffset);
    fpt_vec3 fpt_brickmap_local_chunk_position(ivec3 brickmapCoordinates, fpt brickmapScale, fpt brickmapCenterOffset);

    fpt_vec3 fpt_voxel_local_position_LOD(int bmResolution, int voxel_index, uint32_t LODresolution, fpt chunkSize);
    fpt_vec3 fpt_voxel_position(int bmResolution, int voxel_local_index, const fpt_vec3& currentBrickmapPosition, uint32_t LODresolution, fpt chunkSize);
    
    ivec3 fpt_calculateVoxelCoordinates(fpt_vec3 pos, fpt epsilon = 0);//epsilon is a fudge factor in collision calculations, FPT_THOUSANDTH usually works
    ivec3 fpt_calculateAABBVoxelCoordinates(fpt_vec3 pos, fpt epsilon = 0);
    ivec3 fpt_calculatePaddedVoxelCoordinates(fpt_vec3 pos, fpt epsilon = 0);
    fpt_vec3 fpt_getVoxelWorldPosition(ivec3 voxel, fpt_vec3 chunkMinCorner);

    bool fpt_pick_voxel_in_chunk(chunk_data* chunkData, fpt_vec3 rayOrigin, ivec3 raychunk_coords, fpt_vec3 rayDir, rayCastResult& result);




  inline void checkAndAddAdjacent(chunk_data* chunkData, ivec3& adjchunk_coords, ivec3& adjBrickmapCoords, ivec3& adjVoxelCoords, uint32_t* chunkBmVoxels, uint32_t& listCount, int brickmapRes){
        uint32_t adjBrickmapIndex = get_index_from_coords(adjBrickmapCoords, uvec3_create(brickmapRes));
        uint32_t adjchunkID = getchunkID(chunkData, adjchunk_coords);
        uint32_t adjVoxelIndex = get_index_from_coords(adjVoxelCoords, uvec3_create(64));
        int index = listCount * 3;
        if(adjchunkID != NULL_CHUNK && chunkData->safeToEdit[adjchunkID]) {
            // //spdlog::info("ADDING ADJACENT CHUNK: {} {} {}", adjchunk_coords.x, adjchunk_coords.y, adjchunk_coords.z);

            chunkBmVoxels[index]   = adjchunkID;
            chunkBmVoxels[index+1] = adjBrickmapIndex;
            chunkBmVoxels[index+2] = adjVoxelIndex;
            listCount++; 
        }
    }


    inline void handleNegativeX(ivec3& voxCoords, ivec3& bmCoords, ivec3& chunk_coords, int bmRes) {
        voxCoords.x = 63;
        if(bmCoords.x == 0) {
            chunk_coords.x -= 1;
            bmCoords.x = (bmRes - 1);
        }
        else bmCoords.x -= 1;
         
    }

    inline void handlePositiveX(ivec3& voxCoords, ivec3& bmCoords, ivec3& chunk_coords, int bmRes) {
        voxCoords.x = 0;
        if(bmCoords.x == (bmRes - 1)) {
            chunk_coords.x += 1;
            bmCoords.x = 0;
        }
        else bmCoords.x += 1;

    }

        inline void handleNegativeY(ivec3& voxCoords, ivec3& bmCoords, ivec3& chunk_coords, int bmRes) {
        voxCoords.y = 63;
        if(bmCoords.y == 0) {
            chunk_coords.y -= 1;
            bmCoords.y = (bmRes - 1);
        }
        else bmCoords.y -= 1;
    }

        inline void handlePositiveY(ivec3& voxCoords, ivec3& bmCoords, ivec3& chunk_coords, int bmRes) {
        voxCoords.y = 0;
        if(bmCoords.y == (bmRes - 1)) {
            chunk_coords.y += 1;
            bmCoords.y = 0;
        }
        else bmCoords.y += 1;
    }

        inline void handleNegativeZ(ivec3& voxCoords, ivec3& bmCoords, ivec3& chunk_coords, int bmRes) {
        voxCoords.z = 63;
        if(bmCoords.z == 0) {
            chunk_coords.z -= 1;
            bmCoords.z = (bmRes - 1);
        }
        else bmCoords.z -= 1;

    }

    inline void handlePositiveZ(ivec3& voxCoords, ivec3& bmCoords, ivec3& chunk_coords, int bmRes) {
         voxCoords.z = 0;
        if(bmCoords.z == (bmRes - 1)) {
            chunk_coords.z += 1;
            bmCoords.z = 0;
        }
        else bmCoords.z += 1;
    }
    
    


    inline void checkAdjacentBrickmaps(chunk_data* chunkData, ivec3& voxelCoords, ivec3& brickmapCoords, ivec3& chunk_coords, int voxelIndex, int brickmapID, int chunkID,
                                        uint32_t* chunkBmVoxels, uint32_t& listCount, int brickmapRes){
        bool nx = (voxelCoords.x == 1);
        bool px = (voxelCoords.x == 62);
        bool ny = (voxelCoords.y == 1);
        bool py = (voxelCoords.y == 62);
        bool nz = (voxelCoords.z == 1);
        bool pz = (voxelCoords.z == 62);


        ivec3 adjacentchunk_coords = chunk_coords;
        ivec3 adjacentBrickmapCoords = brickmapCoords;
        ivec3 adjacentVoxelCoords = voxelCoords;


        if(nx){//voxel is in padding of brickmap in -x direction
            handleNegativeX(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            checkAndAddAdjacent(chunkData, adjacentchunk_coords, adjacentBrickmapCoords, adjacentVoxelCoords, chunkBmVoxels, listCount ,brickmapRes);
        }
        else if(px){
            handlePositiveX(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            checkAndAddAdjacent(chunkData, adjacentchunk_coords, adjacentBrickmapCoords, adjacentVoxelCoords, chunkBmVoxels, listCount ,brickmapRes);
        }

        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        adjacentVoxelCoords = voxelCoords;
        if(ny){
            handleNegativeY(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            checkAndAddAdjacent(chunkData, adjacentchunk_coords, adjacentBrickmapCoords, adjacentVoxelCoords, chunkBmVoxels, listCount ,brickmapRes);
        }

        else if(py){//
            handlePositiveY(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            checkAndAddAdjacent(chunkData, adjacentchunk_coords, adjacentBrickmapCoords, adjacentVoxelCoords, chunkBmVoxels, listCount ,brickmapRes);
        }

        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        adjacentVoxelCoords = voxelCoords;
        if(nz){//
            handleNegativeZ(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            checkAndAddAdjacent(chunkData, adjacentchunk_coords, adjacentBrickmapCoords, adjacentVoxelCoords, chunkBmVoxels, listCount ,brickmapRes);
        }

        else if(pz){//
            handlePositiveZ(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            checkAndAddAdjacent(chunkData, adjacentchunk_coords, adjacentBrickmapCoords, adjacentVoxelCoords, chunkBmVoxels, listCount ,brickmapRes);
        }

        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        adjacentVoxelCoords = voxelCoords;

        if(nx && ny){
            handleNegativeX(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            handleNegativeY(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            checkAndAddAdjacent(chunkData, adjacentchunk_coords, adjacentBrickmapCoords, adjacentVoxelCoords, chunkBmVoxels, listCount ,brickmapRes);
        }

        else if(nx && py){
            handleNegativeX(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            handlePositiveY(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            checkAndAddAdjacent(chunkData, adjacentchunk_coords, adjacentBrickmapCoords, adjacentVoxelCoords, chunkBmVoxels, listCount ,brickmapRes);
        }
        else if(px && ny){
            handlePositiveX(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            handleNegativeY(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            checkAndAddAdjacent(chunkData, adjacentchunk_coords, adjacentBrickmapCoords, adjacentVoxelCoords, chunkBmVoxels, listCount ,brickmapRes);
        }
        else if(px && py){
            handlePositiveX(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            handlePositiveY(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            checkAndAddAdjacent(chunkData, adjacentchunk_coords, adjacentBrickmapCoords, adjacentVoxelCoords, chunkBmVoxels, listCount ,brickmapRes);
        }

        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        adjacentVoxelCoords = voxelCoords;

        if(ny && nz){
            handleNegativeY(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            handleNegativeZ(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            checkAndAddAdjacent(chunkData, adjacentchunk_coords, adjacentBrickmapCoords, adjacentVoxelCoords, chunkBmVoxels, listCount ,brickmapRes);
        }
        else if(ny && pz){
            handleNegativeY(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            handlePositiveZ(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            checkAndAddAdjacent(chunkData, adjacentchunk_coords, adjacentBrickmapCoords, adjacentVoxelCoords, chunkBmVoxels, listCount ,brickmapRes);
        }

        else if(py && nz){
            handlePositiveY(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            handleNegativeZ(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            checkAndAddAdjacent(chunkData, adjacentchunk_coords, adjacentBrickmapCoords, adjacentVoxelCoords, chunkBmVoxels, listCount ,brickmapRes);
        }

        else if(py && pz){
            handlePositiveY(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            handlePositiveZ(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            checkAndAddAdjacent(chunkData, adjacentchunk_coords, adjacentBrickmapCoords, adjacentVoxelCoords, chunkBmVoxels, listCount ,brickmapRes);
        }

        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        adjacentVoxelCoords = voxelCoords;

        if(nx && nz){
            handleNegativeX(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            handleNegativeZ(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            checkAndAddAdjacent(chunkData, adjacentchunk_coords, adjacentBrickmapCoords, adjacentVoxelCoords, chunkBmVoxels, listCount ,brickmapRes);

        }

        else if(nx && pz){
            handleNegativeX(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            handlePositiveZ(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            checkAndAddAdjacent(chunkData, adjacentchunk_coords, adjacentBrickmapCoords, adjacentVoxelCoords, chunkBmVoxels, listCount ,brickmapRes);

        }
        else if(px && nz){
            handlePositiveX(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            handleNegativeZ(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            checkAndAddAdjacent(chunkData, adjacentchunk_coords, adjacentBrickmapCoords, adjacentVoxelCoords, chunkBmVoxels, listCount ,brickmapRes);

        }

        else if(px && pz){
            handlePositiveX(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            handlePositiveZ(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            checkAndAddAdjacent(chunkData, adjacentchunk_coords, adjacentBrickmapCoords, adjacentVoxelCoords, chunkBmVoxels, listCount ,brickmapRes);

        }

        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        adjacentVoxelCoords = voxelCoords;

        if(nx && ny && nz){
            handleNegativeX(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            handleNegativeY(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            handleNegativeZ(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            checkAndAddAdjacent(chunkData, adjacentchunk_coords, adjacentBrickmapCoords, adjacentVoxelCoords, chunkBmVoxels, listCount ,brickmapRes);
        }

        else if(nx && ny && pz){
            handleNegativeX(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            handleNegativeY(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            handlePositiveZ(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            checkAndAddAdjacent(chunkData, adjacentchunk_coords, adjacentBrickmapCoords, adjacentVoxelCoords, chunkBmVoxels, listCount ,brickmapRes);
        }





        else if(nx && py && nz){
            handleNegativeX(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            handlePositiveY(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            handleNegativeZ(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            checkAndAddAdjacent(chunkData, adjacentchunk_coords, adjacentBrickmapCoords, adjacentVoxelCoords, chunkBmVoxels, listCount ,brickmapRes);
        }
        else if(nx && py && pz){
            handleNegativeX(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            handlePositiveY(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            handlePositiveZ(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            checkAndAddAdjacent(chunkData, adjacentchunk_coords, adjacentBrickmapCoords, adjacentVoxelCoords, chunkBmVoxels, listCount ,brickmapRes);
        }

        else if(px && ny && nz){
            handlePositiveX(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            handleNegativeY(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            handleNegativeZ(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            checkAndAddAdjacent(chunkData, adjacentchunk_coords, adjacentBrickmapCoords, adjacentVoxelCoords, chunkBmVoxels, listCount ,brickmapRes);
        }

        else if(px && ny && pz){
            handlePositiveX(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            handleNegativeY(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            handlePositiveZ(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            checkAndAddAdjacent(chunkData, adjacentchunk_coords, adjacentBrickmapCoords, adjacentVoxelCoords, chunkBmVoxels, listCount ,brickmapRes);
        }

        else if(px && py && nz){
            handlePositiveX(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            handlePositiveY(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            handleNegativeZ(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            checkAndAddAdjacent(chunkData, adjacentchunk_coords, adjacentBrickmapCoords, adjacentVoxelCoords, chunkBmVoxels, listCount ,brickmapRes);
        }



        else if(px && py && pz){
            handlePositiveX(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            handlePositiveY(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            handlePositiveZ(adjacentVoxelCoords, adjacentBrickmapCoords, adjacentchunk_coords, brickmapRes);
            checkAndAddAdjacent(chunkData, adjacentchunk_coords, adjacentBrickmapCoords, adjacentVoxelCoords, chunkBmVoxels, listCount ,brickmapRes);

        }


    }


  inline void tryAddAdjacent(chunk_data* chunkData, ivec3& adjchunk_coords){
    #if 1

        uint32_t adjchunkID = getchunkID(chunkData, adjchunk_coords);
        if(adjchunkID != NULL_CHUNK && chunkData->safeToEdit[adjchunkID]) {
            //check that the bm/chunk isnt already in the list

            if(chunkData->brushChunkIDCount >= 64){
                //spdlog::error("tryAddAdjacent() BRUSH INTERSECTS TOO MANY BRICKMAPS, INCREASE ARRAY SIZE");
                return;
            }

            bool found = false;
            for(int i = 0; i < chunkData->brushChunkIDCount; i++){
                if(chunkData->brushChunkIDs[i] == adjchunkID){
                    found = true;
                    break;
                }
            }
            if(found){
                return;
            }
            fpt_vec3 relativeChunkOffset = ivec_to_fpt_vec3(adjchunk_coords - chunkData->brushCenterchunk_coords) * FPT_CHUNK_SIZE;

            fpt_vec3 adjustedMin = chunkData->fptBrushBounds.min - relativeChunkOffset;
            fpt_vec3 adjustedMax = chunkData->fptBrushBounds.max - relativeChunkOffset;
            ivec3 voxCoordsMin = fpt_calculatePaddedVoxelCoordsFloor(adjustedMin);
            ivec3 voxCoordsMax = fpt_calculatePaddedVoxelCoordsCeil(adjustedMax);

            chunkData->brushChunkIDs[chunkData->brushChunkIDCount] = adjchunkID;
            
            //store voxel bounds for later use
            chunkData->brushChunkVoxMin[chunkData->brushChunkIDCount] = voxCoordsMin;
            chunkData->brushChunkVoxMax[chunkData->brushChunkIDCount] = voxCoordsMax;

            chunkData->brushChunkIDCount++;
        }
        #endif
    }
    
    inline void handlenx(ivec3& chunk_coords) {
            chunk_coords.x -= 1;
    }

        inline void handlepx(ivec3& chunk_coords) {
            chunk_coords.x += 1;
    }

        inline void handleny(ivec3& chunk_coords) {
        chunk_coords.y -= 1;
    }

        inline void handlepy(ivec3& chunk_coords) {
            chunk_coords.y += 1;
    }

        inline void handlenz(ivec3& chunk_coords) {
            chunk_coords.z -= 1;
    }

    inline void handlepz(ivec3& chunk_coords) {
            chunk_coords.z += 1;
    }
    
    

    inline void checkPadding(chunk_data* chunkData, ivec3& voxMin, ivec3& voxMax, ivec3& chunk_coords, int chunkID){
        bool nx = (voxMin.x <= 1);
        bool ny = (voxMin.y <= 1);
        bool nz = (voxMin.z <= 1);

        bool px = (voxMax.x >= 62);
        bool py = (voxMax.y >= 62);
        bool pz = (voxMax.z >= 62);


        ivec3 adjacentchunk_coords = chunk_coords;


        if(nx){//voxel is in padding of brickmap in -x direction
            handlenx(adjacentchunk_coords);
            tryAddAdjacent(chunkData, adjacentchunk_coords);
        }
        else if(px){
            handlepx(adjacentchunk_coords);
            tryAddAdjacent(chunkData, adjacentchunk_coords);
        }

        adjacentchunk_coords = chunk_coords;
        
        if(ny){
            handleny(adjacentchunk_coords);
            tryAddAdjacent(chunkData, adjacentchunk_coords);
        }

        else if(py){//
            handlepy(adjacentchunk_coords);
            tryAddAdjacent(chunkData, adjacentchunk_coords);
        }

        adjacentchunk_coords = chunk_coords;
        
        if(nz){//
            handlenz(adjacentchunk_coords);
            tryAddAdjacent(chunkData, adjacentchunk_coords);
        }

        else if(pz){//
            handlepz( adjacentchunk_coords);
            tryAddAdjacent(chunkData, adjacentchunk_coords);
        }

        adjacentchunk_coords = chunk_coords;
        

        if(nx && ny){
            handlenx(adjacentchunk_coords);
            handleny(adjacentchunk_coords);
            tryAddAdjacent(chunkData, adjacentchunk_coords);
        }

        else if(nx && py){
            handlenx(adjacentchunk_coords);
            handlepy(adjacentchunk_coords);
            tryAddAdjacent(chunkData, adjacentchunk_coords);
        }
        else if(px && ny){
            handlepx( adjacentchunk_coords);
            handleny( adjacentchunk_coords);
            tryAddAdjacent(chunkData, adjacentchunk_coords);
        }
        else if(px && py){
            handlepx( adjacentchunk_coords);
            handlepy( adjacentchunk_coords);
            tryAddAdjacent(chunkData, adjacentchunk_coords);
        }

        adjacentchunk_coords = chunk_coords;

        if(ny && nz){
            handleny( adjacentchunk_coords);
            handlenz( adjacentchunk_coords);
            tryAddAdjacent(chunkData, adjacentchunk_coords);
        }
        else if(ny && pz){
            handleny( adjacentchunk_coords);
            handlepz( adjacentchunk_coords);
            tryAddAdjacent(chunkData, adjacentchunk_coords);
        }

        else if(py && nz){
            handlepy( adjacentchunk_coords);
            handlenz( adjacentchunk_coords);
            tryAddAdjacent(chunkData, adjacentchunk_coords);
        }

        else if(py && pz){
            handlepy( adjacentchunk_coords);
            handlepz( adjacentchunk_coords);
            tryAddAdjacent(chunkData, adjacentchunk_coords);
        }

        adjacentchunk_coords = chunk_coords;

        if(nx && nz){
            handlenx( adjacentchunk_coords);
            handlenz( adjacentchunk_coords);
            tryAddAdjacent(chunkData, adjacentchunk_coords);

        }

        else if(nx && pz){
            handlenx( adjacentchunk_coords);
            handlepz( adjacentchunk_coords);
            tryAddAdjacent(chunkData, adjacentchunk_coords);

        }
        else if(px && nz){
            handlepx( adjacentchunk_coords);
            handlenz( adjacentchunk_coords);
            tryAddAdjacent(chunkData, adjacentchunk_coords);

        }

        else if(px && pz){
            handlepx( adjacentchunk_coords);
            handlepz( adjacentchunk_coords);
            tryAddAdjacent(chunkData, adjacentchunk_coords);

        }

        adjacentchunk_coords = chunk_coords;

        if(nx && ny && nz){
            handlenx( adjacentchunk_coords);
            handleny( adjacentchunk_coords);
            handlenz( adjacentchunk_coords);
            tryAddAdjacent(chunkData, adjacentchunk_coords);
        }

        else if(nx && ny && pz){
            handlenx( adjacentchunk_coords);
            handleny( adjacentchunk_coords);
            handlepz( adjacentchunk_coords);
            tryAddAdjacent(chunkData, adjacentchunk_coords);
        }





        else if(nx && py && nz){
            handlenx( adjacentchunk_coords);
            handlepy( adjacentchunk_coords);
            handlenz( adjacentchunk_coords);
            tryAddAdjacent(chunkData, adjacentchunk_coords);
        }
        else if(nx && py && pz){
            handlenx( adjacentchunk_coords);
            handlepy( adjacentchunk_coords);
            handlepz( adjacentchunk_coords);
            tryAddAdjacent(chunkData, adjacentchunk_coords);
        }

        else if(px && ny && nz){
            handlepx( adjacentchunk_coords);
            handleny( adjacentchunk_coords);
            handlenz( adjacentchunk_coords);
            tryAddAdjacent(chunkData, adjacentchunk_coords);
        }

        else if(px && ny && pz){
            handlepx( adjacentchunk_coords);
            handleny( adjacentchunk_coords);
            handlepz( adjacentchunk_coords);
            tryAddAdjacent(chunkData, adjacentchunk_coords);
        }

        else if(px && py && nz){
            handlepx( adjacentchunk_coords);
            handlepy( adjacentchunk_coords);
            handlenz( adjacentchunk_coords);
            tryAddAdjacent(chunkData, adjacentchunk_coords);
        }



        else if(px && py && pz){
            handlepx( adjacentchunk_coords);
            handlepy( adjacentchunk_coords);
            handlepz( adjacentchunk_coords);
            tryAddAdjacent(chunkData, adjacentchunk_coords);

        }


    }
       

  inline void tryLightAdjacent(chunk_data* chunkData, ivec3& adjchunk_coords, ivec3& adjVoxelCoords, uint8_t light){
        uint32_t adjBrickmapIndex = 0;
        uint32_t adjchunkID = getchunkID(chunkData, adjchunk_coords);
        if(adjchunkID != NULL_CHUNK && chunkData->safeToEdit[adjchunkID]) {
            //check that the bm/chunk isnt already in the list

            int LODres = 0;
            Brickmap64* bm = nullptr;
            int resID;

            LODres = 1;
            bm = &chunkData->brickmaps [adjchunkID];

                fpt voxelSize = FPT_VOXEL_SCALE;
            fpt bmScale = FPT_CHUNK_SIZE;
            fpt voxScale = voxelSize;
            fpt totalBmScale = fpt_add(bmScale , fpt_mul(voxScale,131072)); // 131072 = 2 * (2^16)
            fpt invTotalBmScale = fpt_div(FPT_ONE , totalBmScale); 

            if(chunkData->lightBmCount >= 54 || chunkData->lightBmCount >= 54){
                //spdlog::error("tryLightAdjacent() TOO MANY BRICKMAPS, INCREASE ARRAY SIZE");
                return;
            }


            uint32_t v = adjVoxelCoords.x + (adjVoxelCoords.y * 64) + (adjVoxelCoords.z * 64 * 64);
            // uint8_t neighborLightValue = (bm->voxels[v] >> 12) & 0xF;
            uint8_t neighborLightValue = 0xF;
            if(neighborLightValue >= light)return;//skip blocks with equal or greater light values
            neighborLightValue = light; //set neighbors light value, add it to the queue
            // bm->voxels[v] |= (neighborLightValue << 12);
            bm->voxels[v] |= (0xF);
            if(neighborLightValue <= 1)return;//skip solid blocks for now, skip if light value is lowest
            // Or to check if it's air (lower 10 bits are zero)
            // if((bm->voxels[v] & 0x3FF))return; //skip if voxel is solid
            if(bm->voxels[v])return; //skip if voxel is solid

            int index = chunkData->lightBmCount;
            bool found = false;
            for(int i = 0; i < chunkData->lightBmCount; i++){
                if(chunkData->lightBMIDs[i] == adjBrickmapIndex && chunkData->lightChunkIDs[i] == adjchunkID){
                    found = true;
                    index = i;
                    break;
                }
            }
            if(!found){
                chunkData->lightBrickmaps[index] = bm;
                chunkData->lightchunk_coords[index] = adjchunk_coords;
                chunkData->lightChunkIDs[index] = adjchunkID;
                chunkData->lightBMIDs[index] = adjBrickmapIndex;
                chunkData->lightBmCount++;
            }

            uint32_t& queueCount = chunkData->lightBoundaryQueueCounts[index];







            chunkData->lightBoundaryQueueValues[index][queueCount] = light;
            chunkData->lightBoundaryQueuePositions[index][queueCount] = adjVoxelCoords;
            queueCount++;

        }
    }
    
    
    inline void lightnx(ivec3& voxCoords, ivec3& chunk_coords) {
        voxCoords.x = 62;
            chunk_coords.x -= 1;
         
    }

    inline void lightpx(ivec3& voxCoords, ivec3& chunk_coords) {
        voxCoords.x = 1;
            chunk_coords.x += 1;

    }

        inline void lightny(ivec3& voxCoords, ivec3& chunk_coords) {
        voxCoords.y = 62;
            chunk_coords.y -= 1;
    }

        inline void lightpy(ivec3& voxCoords, ivec3& chunk_coords) {
        voxCoords.y = 1;
            chunk_coords.y += 1;
    }

        inline void lightnz(ivec3& voxCoords, ivec3& chunk_coords) {
        voxCoords.z = 62;
            chunk_coords.z -= 1;

    }

    inline void lightpz(ivec3& voxCoords, ivec3& chunk_coords) {
         voxCoords.z = 1;
            chunk_coords.z += 1;
    }
    
    

       
    inline void lightFloodFillBoundaries(chunk_data* chunkData, ivec3& voxCoords, ivec3& chunk_coords, int chunkID, uint8_t light){
        bool nx = (voxCoords.x < 1);
        bool ny = (voxCoords.y < 1);
        bool nz = (voxCoords.z < 1);

        bool px = (voxCoords.x > 62);
        bool py = (voxCoords.y > 62);
        bool pz = (voxCoords.z > 62);


        ivec3 adjacentchunk_coords = chunk_coords;
        ivec3 adjacentVoxelCoords = voxCoords;

        if(nx){//voxel is in padding of brickmap in -x direction
            lightnx(adjacentVoxelCoords, adjacentchunk_coords);
            tryLightAdjacent(chunkData, adjacentchunk_coords, adjacentVoxelCoords, light);
        }
        else if(px){
            lightpx(adjacentVoxelCoords, adjacentchunk_coords);
            tryLightAdjacent(chunkData, adjacentchunk_coords, adjacentVoxelCoords, light);
        }

        adjacentchunk_coords = chunk_coords;
        adjacentVoxelCoords = voxCoords;
        if(ny){
            lightny(adjacentVoxelCoords, adjacentchunk_coords);
            tryLightAdjacent(chunkData, adjacentchunk_coords, adjacentVoxelCoords, light);
        }

        else if(py){//
            lightpy(adjacentVoxelCoords, adjacentchunk_coords);
            tryLightAdjacent(chunkData, adjacentchunk_coords, adjacentVoxelCoords, light);
        }

        adjacentchunk_coords = chunk_coords;
        adjacentVoxelCoords = voxCoords;
        
        if(nz){//
            lightnz(adjacentVoxelCoords, adjacentchunk_coords);
            tryLightAdjacent(chunkData, adjacentchunk_coords, adjacentVoxelCoords, light);
        }

        else if(pz){//
            lightpz(adjacentVoxelCoords, adjacentchunk_coords);
            tryLightAdjacent(chunkData, adjacentchunk_coords, adjacentVoxelCoords, light);
        }

        adjacentchunk_coords = chunk_coords;
        adjacentVoxelCoords = voxCoords;
        

        if(nx && ny){
            lightnx(adjacentVoxelCoords, adjacentchunk_coords);
            lightny(adjacentVoxelCoords, adjacentchunk_coords);
            tryLightAdjacent(chunkData, adjacentchunk_coords, adjacentVoxelCoords, light);
        }

        else if(nx && py){
            lightnx(adjacentVoxelCoords, adjacentchunk_coords);
            lightpy(adjacentVoxelCoords, adjacentchunk_coords);
            tryLightAdjacent(chunkData, adjacentchunk_coords, adjacentVoxelCoords, light);
        }
        else if(px && ny){
            lightpx(adjacentVoxelCoords, adjacentchunk_coords);
            lightny(adjacentVoxelCoords, adjacentchunk_coords);
            tryLightAdjacent(chunkData, adjacentchunk_coords, adjacentVoxelCoords, light);
        }
        else if(px && py){
            lightpx(adjacentVoxelCoords, adjacentchunk_coords);
            lightpy(adjacentVoxelCoords, adjacentchunk_coords);
            tryLightAdjacent(chunkData, adjacentchunk_coords, adjacentVoxelCoords, light);
        }

        adjacentchunk_coords = chunk_coords;
        adjacentVoxelCoords = voxCoords;

        if(ny && nz){
            lightny(adjacentVoxelCoords, adjacentchunk_coords);
            lightnz(adjacentVoxelCoords, adjacentchunk_coords);
            tryLightAdjacent(chunkData, adjacentchunk_coords, adjacentVoxelCoords, light);
        }
        else if(ny && pz){
            lightny(adjacentVoxelCoords, adjacentchunk_coords);
            lightpz(adjacentVoxelCoords, adjacentchunk_coords);
            tryLightAdjacent(chunkData, adjacentchunk_coords, adjacentVoxelCoords, light);
        }

        else if(py && nz){
            lightpy(adjacentVoxelCoords, adjacentchunk_coords);
            lightnz(adjacentVoxelCoords, adjacentchunk_coords);
            tryLightAdjacent(chunkData, adjacentchunk_coords, adjacentVoxelCoords, light);
        }

        else if(py && pz){
            lightpy(adjacentVoxelCoords, adjacentchunk_coords);
            lightpz(adjacentVoxelCoords, adjacentchunk_coords);
            tryLightAdjacent(chunkData, adjacentchunk_coords, adjacentVoxelCoords, light);
        }

        adjacentchunk_coords = chunk_coords;
        adjacentVoxelCoords = voxCoords;

        if(nx && nz){
            lightnx(adjacentVoxelCoords, adjacentchunk_coords);
            lightnz(adjacentVoxelCoords, adjacentchunk_coords);
            tryLightAdjacent(chunkData, adjacentchunk_coords, adjacentVoxelCoords, light);

        }

        else if(nx && pz){
            lightnx(adjacentVoxelCoords, adjacentchunk_coords);
            lightpz(adjacentVoxelCoords, adjacentchunk_coords);
            tryLightAdjacent(chunkData, adjacentchunk_coords, adjacentVoxelCoords, light);

        }
        else if(px && nz){
            lightpx(adjacentVoxelCoords, adjacentchunk_coords);
            lightnz(adjacentVoxelCoords, adjacentchunk_coords);
            tryLightAdjacent(chunkData, adjacentchunk_coords, adjacentVoxelCoords, light);

        }

        else if(px && pz){
            lightpx(adjacentVoxelCoords, adjacentchunk_coords);
            lightpz(adjacentVoxelCoords, adjacentchunk_coords);
            tryLightAdjacent(chunkData, adjacentchunk_coords, adjacentVoxelCoords, light);

        }

        adjacentchunk_coords = chunk_coords;
        adjacentVoxelCoords = voxCoords;

        if(nx && ny && nz){
            lightnx(adjacentVoxelCoords, adjacentchunk_coords);
            lightny(adjacentVoxelCoords, adjacentchunk_coords);
            lightnz(adjacentVoxelCoords, adjacentchunk_coords);
            tryLightAdjacent(chunkData, adjacentchunk_coords, adjacentVoxelCoords, light);
        }

        else if(nx && ny && pz){
            lightnx(adjacentVoxelCoords, adjacentchunk_coords);
            lightny(adjacentVoxelCoords, adjacentchunk_coords);
            lightpz(adjacentVoxelCoords, adjacentchunk_coords);
            tryLightAdjacent(chunkData, adjacentchunk_coords, adjacentVoxelCoords, light);
        }

        else if(nx && py && nz){
            lightnx(adjacentVoxelCoords, adjacentchunk_coords);
            lightpy(adjacentVoxelCoords, adjacentchunk_coords);
            lightnz(adjacentVoxelCoords, adjacentchunk_coords);
            tryLightAdjacent(chunkData, adjacentchunk_coords, adjacentVoxelCoords, light);
        }
        else if(nx && py && pz){
            lightnx(adjacentVoxelCoords, adjacentchunk_coords);
            lightpy(adjacentVoxelCoords, adjacentchunk_coords);
            lightpz(adjacentVoxelCoords, adjacentchunk_coords);
            tryLightAdjacent(chunkData, adjacentchunk_coords, adjacentVoxelCoords, light);
        }

        else if(px && ny && nz){
            lightpx(adjacentVoxelCoords, adjacentchunk_coords);
            lightny(adjacentVoxelCoords, adjacentchunk_coords);
            lightnz(adjacentVoxelCoords, adjacentchunk_coords);
            tryLightAdjacent(chunkData, adjacentchunk_coords, adjacentVoxelCoords, light);
        }

        else if(px && ny && pz){
            lightpx(adjacentVoxelCoords, adjacentchunk_coords);
            lightny(adjacentVoxelCoords, adjacentchunk_coords);
            lightpz(adjacentVoxelCoords, adjacentchunk_coords);
            tryLightAdjacent(chunkData, adjacentchunk_coords, adjacentVoxelCoords, light);
        }

        else if(px && py && nz){
            lightpx(adjacentVoxelCoords, adjacentchunk_coords);
            lightpy(adjacentVoxelCoords, adjacentchunk_coords);
            lightnz(adjacentVoxelCoords, adjacentchunk_coords);
            tryLightAdjacent(chunkData, adjacentchunk_coords, adjacentVoxelCoords, light);
        }



        else if(px && py && pz){
            lightpx(adjacentVoxelCoords, adjacentchunk_coords);
            lightpy(adjacentVoxelCoords, adjacentchunk_coords);
            lightpz(adjacentVoxelCoords, adjacentchunk_coords);
            tryLightAdjacent(chunkData, adjacentchunk_coords, adjacentVoxelCoords, light);

        }


    }




inline bool clearLight(chunk_data* chunkData, Brickmap64* bm){
    bool edited = false;
    for(int x = 1; x < 63; x++){
        for(int y = 1; y < 63; y++){
            for(int z = 1; z < 63; z++){
                ivec3 voxCoords = ivec3_create(x,y,z);
                uint32_t v = x + (y * 64) + (z * 64 * 64);
                if(bm->voxels[v] & 0xF000){
                    bm->voxels[v] &= ~0xF000;//clear lighting in all voxels
                    edited = true;
                }
            }
        }
    }
    return edited;
}




  inline void tryClearLightAdjacent(chunk_data* chunkData, ivec3& adjchunk_coords){
        uint32_t adjBrickmapIndex = 0;
        uint32_t adjchunkID = getchunkID(chunkData, adjchunk_coords);
        if(adjchunkID != NULL_CHUNK && chunkData->safeToEdit[adjchunkID]) {
            //check that the bm/chunk isnt already in the list

            int LODres = 0;
            Brickmap64* bm = nullptr;
         
            bm = &chunkData->brickmaps [adjchunkID];

            if(!bm){
                //spdlog::error("tryClearLightAdjacent() brickmap is null?");
            }
            if(chunkData->editedCount >= 128){
                //spdlog::error("tryClearLightAdjacent() BRUSH INTERSECTS TOO MANY BRICKMAPS, INCREASE ARRAY SIZE");
                return;
            }
            if(clearLight(chunkData, bm)){
                bool found = false;
                for(int i = 0; i < chunkData->editedCount; i++){
                    if(chunkData->editedBMIDs[i] == adjBrickmapIndex && chunkData->editedChunkIDs[i] == adjchunkID){
                        found = true;
                        break;
                    }
                }
                if(found){
                    return;
                }

                chunkData->editedChunkIDs[chunkData->editedCount] = adjchunkID;
                chunkData->editedBMIDs[chunkData->editedCount] = adjBrickmapIndex;
                chunkData->editedBMs[chunkData->editedCount] = bm;
                chunkData->editedCount++;
            }
        }
    }
    

    
  
    
    inline void clearLightingAdjacentBrickmaps(chunk_data* chunkData, ivec3& brickmapCoords, ivec3& chunk_coords, int brickmapID, int chunkID){



        ivec3 adjacentchunk_coords = chunk_coords;
        ivec3 adjacentBrickmapCoords = brickmapCoords;


        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        handlenx(adjacentchunk_coords);
        tryClearLightAdjacent(chunkData, adjacentchunk_coords);
    
        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        handlepx(adjacentchunk_coords);
        tryClearLightAdjacent(chunkData, adjacentchunk_coords);

        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        handleny(adjacentchunk_coords);
        tryClearLightAdjacent(chunkData, adjacentchunk_coords);

        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        handlepy(adjacentchunk_coords);
        tryClearLightAdjacent(chunkData, adjacentchunk_coords);

        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        handlenz(adjacentchunk_coords);
        tryClearLightAdjacent(chunkData, adjacentchunk_coords);

        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        handlepz( adjacentchunk_coords);
        tryClearLightAdjacent(chunkData, adjacentchunk_coords);


        
        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        handlenx(adjacentchunk_coords);
        handleny(adjacentchunk_coords);
        tryClearLightAdjacent(chunkData, adjacentchunk_coords);

        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        handlenx(adjacentchunk_coords);
        handlepy(adjacentchunk_coords);
        tryClearLightAdjacent(chunkData, adjacentchunk_coords);

        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        handlepx( adjacentchunk_coords);
        handleny( adjacentchunk_coords);
        tryClearLightAdjacent(chunkData, adjacentchunk_coords);

        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        handlepx( adjacentchunk_coords);
        handlepy( adjacentchunk_coords);
        tryClearLightAdjacent(chunkData, adjacentchunk_coords);

        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        handleny( adjacentchunk_coords);
        handlenz( adjacentchunk_coords);
        tryClearLightAdjacent(chunkData, adjacentchunk_coords);

        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        handleny( adjacentchunk_coords);
        handlepz( adjacentchunk_coords);
        tryClearLightAdjacent(chunkData, adjacentchunk_coords);

        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        handlepy( adjacentchunk_coords);
        handlenz( adjacentchunk_coords);
        tryClearLightAdjacent(chunkData, adjacentchunk_coords);

        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        handlepy( adjacentchunk_coords);
        handlepz( adjacentchunk_coords);
        tryClearLightAdjacent(chunkData, adjacentchunk_coords);

        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        handlenx( adjacentchunk_coords);
        handlenz( adjacentchunk_coords);
        tryClearLightAdjacent(chunkData, adjacentchunk_coords);

        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        handlenx( adjacentchunk_coords);
        handlepz( adjacentchunk_coords);
        tryClearLightAdjacent(chunkData, adjacentchunk_coords);

        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        handlepx( adjacentchunk_coords);
        handlenz( adjacentchunk_coords);
        tryClearLightAdjacent(chunkData, adjacentchunk_coords);

        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        handlepx( adjacentchunk_coords);
        handlepz( adjacentchunk_coords);
        tryClearLightAdjacent(chunkData, adjacentchunk_coords);


        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        handlenx( adjacentchunk_coords);
        handleny( adjacentchunk_coords);
        handlenz( adjacentchunk_coords);
        tryClearLightAdjacent(chunkData, adjacentchunk_coords);

        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        handlenx( adjacentchunk_coords);
        handleny( adjacentchunk_coords);
        handlepz( adjacentchunk_coords);
        tryClearLightAdjacent(chunkData, adjacentchunk_coords);


        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        handlenx( adjacentchunk_coords);
        handlepy( adjacentchunk_coords);
        handlenz( adjacentchunk_coords);
        tryClearLightAdjacent(chunkData, adjacentchunk_coords);

        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        handlenx( adjacentchunk_coords);
        handlepy( adjacentchunk_coords);
        handlepz( adjacentchunk_coords);
        tryClearLightAdjacent(chunkData, adjacentchunk_coords);

        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        handlepx( adjacentchunk_coords);
        handleny( adjacentchunk_coords);
        handlenz( adjacentchunk_coords);
        tryClearLightAdjacent(chunkData, adjacentchunk_coords);

        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        handlepx( adjacentchunk_coords);
        handleny( adjacentchunk_coords);
        handlepz( adjacentchunk_coords);
        tryClearLightAdjacent(chunkData, adjacentchunk_coords);

        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        handlepx( adjacentchunk_coords);
        handlepy( adjacentchunk_coords);
        handlenz( adjacentchunk_coords);
        tryClearLightAdjacent(chunkData, adjacentchunk_coords);

        adjacentchunk_coords = chunk_coords;
        adjacentBrickmapCoords = brickmapCoords;
        handlepx( adjacentchunk_coords);
        handlepy( adjacentchunk_coords);
        handlepz( adjacentchunk_coords);
        tryClearLightAdjacent(chunkData, adjacentchunk_coords);


    }