#include "voxelHelpers.h"
#include "brickmap.h"





    constexpr int drawDistance = 5; 
    constexpr int chunk_grid_resolution = 2 * drawDistance + 1;
    constexpr int maxVal = pbmr - 1;

    constexpr float VOXEL_SCALE_PADDING_COMPENSATION = 1.0f;//(float)TOTAL_VOXELS/(float)USABLE_VOXELS;

    constexpr fpt FPT_TOTAL_VOXELS  = 4194304;
    constexpr fpt FPT_PADDED_VOXELS = 4128768;
    constexpr fpt FPT_USABLE_VOXELS = 4063232;
    constexpr fpt FPT_VOXEL_SCALE_PADDING_COMPENSATION = 65536;//67650;
    constexpr fpt FPT_SAFE_RAYCAST_MAX = 1966080000; // testing to see if its safe, its 30000 << 16



    //we dont need the scaled chunkSize in here, do we? we want to constrain the ray to the regular chunk size...
    ivec3 calculateBrickmapCoordinates(vec3 position, int grid_resolution) {
        vec3 normalizedPosition = { //dont need the chunk position since its relative and thus will be considered 0 0 0
            position.x - chunk_min_corner.x,
            position.y - chunk_min_corner.y,
            position.z - chunk_min_corner.z
        };

        // Manual clamp function
        #define CLAMP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
        
        // For negative numbers, floor can be implemented by:
        // (int)(x - 0.5f)
        // Or even: (int)x - (x < 0)
        
        ivec3 result;
        const int max_val = grid_resolution - 1;
        
        float scaledX = (normalizedPosition.x * inverse_chunk_size) * grid_resolution;
        float scaledY = (normalizedPosition.y * inverse_chunk_size) * grid_resolution;
        float scaledZ = (normalizedPosition.z * inverse_chunk_size) * grid_resolution;
        
        result.x = CLAMP((int)(scaledX), 0, max_val);
        result.y = CLAMP((int)(scaledY), 0, max_val);
        result.z = CLAMP((int)(scaledZ), 0, max_val);
        return result;
    }


    vec3 brickmapPosition_LOD(ivec3 brickmapCoordinates, vec3 chunkPosition, float brickmapScale, float brickmapCenterOffset) {
        vec3 brickmapPosition = (chunkPosition + chunk_min_corner) + (vec3_create(brickmapCoordinates) * brickmapScale) + brickmapCenterOffset;
        return brickmapPosition;
    }
    
    vec3 voxel_local_position_LOD(int bmResolution, int voxel_index, uint32_t LODresolution, float chunkSize) {
        ivec3 position = get_coords_from_index(voxel_index, uvec3_create(bmResolution));
        vec3 voxel_local_position_vec3 = vec3_create(position);
        float voxel_size = chunkSize / (bmResolution * LODresolution);
        vec3 voxel_local_position = voxel_local_position_vec3 * voxel_size;
        float voxel_center_offset = voxel_size / 2.0f;
        return voxel_local_position + voxel_center_offset;
    }

    vec3 voxel_position(int bmResolution, int voxel_local_index, const vec3& currentBrickmapPosition, uint32_t LODresolution, float chunkSize){
        vec3  voxel_local_position = voxel_local_position_LOD(bmResolution, voxel_local_index, LODresolution, chunkSize);
        vec3 absolute_voxel_position = voxel_local_position + (currentBrickmapPosition - ((chunkSize / LODresolution) / 2.0f));
        return absolute_voxel_position;
    }

    
    ivec3 calculateVoxelCoordinates(vec3 pos, vec3 brickmapPosition, float brickmapScale, float inverseBrickmapScale) {

        // First normalize position to 0-1 range
        float halfSize = brickmapScale * 0.5f;
        
        // Move from -halfSize..+halfSize to 0..1
        float normalizedX = (pos.x - brickmapPosition.x + halfSize) * inverseBrickmapScale;
        float normalizedY = (pos.y - brickmapPosition.y + halfSize) * inverseBrickmapScale;
        float normalizedZ = (pos.z - brickmapPosition.z + halfSize) * inverseBrickmapScale;
        
        float scaledX = normalizedX * USABLE_VOXELS + 1.0f;
        float scaledY = normalizedY * USABLE_VOXELS + 1.0f;
        float scaledZ = normalizedZ * USABLE_VOXELS + 1.0f;

        ivec3 v;
        v.x = (int)(scaledX);
        v.y = (int)(scaledY);
        v.z = (int)(scaledZ);

        // Clamp to usable range
        v.x = CLAMP(v.x, 1, USABLE_VOXELS);
        v.y = CLAMP(v.y, 1, USABLE_VOXELS);
        v.z = CLAMP(v.z, 1, USABLE_VOXELS);

        return v;
    }

        ivec3 calculatePaddedVoxelCoordinates(vec3 pos, vec3 brickmapPosition, float brickmapScale, float inverseBrickmapScale, float voxelSize) {

        // First normalize position to 0-1 range
        float halfSize = brickmapScale * 0.5f;
        
        // Move from -halfSize..+halfSize to 0..1
        float normalizedX = (pos.x - brickmapPosition.x /* - voxelSize */ + halfSize) * inverseBrickmapScale;
        float normalizedY = (pos.y - brickmapPosition.y /* - voxelSize */ + halfSize) * inverseBrickmapScale;
        float normalizedZ = (pos.z - brickmapPosition.z /* - voxelSize */ + halfSize) * inverseBrickmapScale;
        
        float scaledX = normalizedX * TOTAL_VOXELS;
        float scaledY = normalizedY * TOTAL_VOXELS;
        float scaledZ = normalizedZ * TOTAL_VOXELS;

        ivec3 v;
        v.x = (int)(scaledX);
        v.y = (int)(scaledY);
        v.z = (int)(scaledZ);

        // Clamp to usable range
        v.x = CLAMP(v.x, 0, PADDED_VOXELS);
        v.y = CLAMP(v.y, 0, PADDED_VOXELS);
        v.z = CLAMP(v.z, 0, PADDED_VOXELS);

        return v;
    }




    //     float scaledX = (pos.x + 32.0f) * (pbmr - 2) / 64.0f + 1.0f;
    //     float scaledY = (pos.y + 32.0f) * (pbmr - 2) / 64.0f + 1.0f;
    //     float scaledZ = (pos.z + 32.0f) * (pbmr - 2) / 64.0f + 1.0f;

    //     ivec3 v;
    //     v.x = (int)(scaledX);// + 0.5f);
    //     v.y = (int)(scaledY);// + 0.5f);
    //     v.z = (int)(scaledZ);// + 0.5f);

    //     #define CLAMP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
    //     v.x = CLAMP(v.x, 1, pbmr - 2);
    //     v.y = CLAMP(v.y, 1, pbmr - 2);
    //     v.z = CLAMP(v.z, 1, pbmr - 2);
    //     // printf("voxel coords: %f %f %f\n", v.x,v.y,v.z);

    //     return v;
    // }


    vec3 getVoxelWorldPosition(ivec3 voxel, vec3 brickmapMinCorner, float halfVoxelSize, uint32_t LODresolution) {
        float worldX = ((voxel.x - 1.0f) * (VOXEL_SCALE_PADDING_COMPENSATION / LODresolution)) + brickmapMinCorner.x + (halfVoxelSize);//add half voxel size to it (adjust for LOD later)
        float worldY = ((voxel.y - 1.0f) * (VOXEL_SCALE_PADDING_COMPENSATION / LODresolution)) + brickmapMinCorner.y + (halfVoxelSize);//add half voxel size to it (adjust for LOD later)
        float worldZ = ((voxel.z - 1.0f) * (VOXEL_SCALE_PADDING_COMPENSATION / LODresolution)) + brickmapMinCorner.z + (halfVoxelSize);//add half voxel size to it (adjust for LOD later)

        return vec3_create(worldX, worldY, worldZ);
    }


    bool pick_voxel_in_chunk(chunk_data* chunkData, CameraComp& camera, int mouseX, int mouseY, 
                    rayCastResult& result /*, uint16_t brickmap_index, int& voxel_index
                ,int& adjacent_chunk_buffer_index, int& adjacent_brickmap_index, int& adjacent_voxel_index, 
                  Chunk** selected_chunk, Chunk** adjacent_chunk,  ivec3& adjacent_chunk_coords*/){

    
    uint32_t brickmap_index = 0;
    uint32_t voxel_index = 0;
    uint32_t adjacent_chunk_index = 0;
    uint32_t adjacent_brickmap_index = 0;
    uint32_t adjacent_voxel_index = 0;
    uint8_t voxel_type = 0;
    int currentVoxelIndex = 0;
    int currentBrickmapIndex = 0;
    ivec3 adjacent_chunk_coords = ivec3_create(0);
    ivec3 selected_chunk_coords = ivec3_create(0);

    // vec3 rayOrigin = camera.m_position;
    vec3 rayOrigin = fpt_to_flt_vec3(camera.pos_in_chunk);
    printf("CAST MOUSE RAY HERE!\n");
    // vec3 rayDir = CastMouseRay(camera, mouseX, mouseY);
    vec3 rayDir = vec3_create(1);
    float voxelSize = 1;
    float halfVoxelSize = voxelSize * .5f;
    vec3 voxelCenter = vec3_create(0.0f);
    vec3 hitNormal = vec3_create(0.0f);
    int selectedchunkID = 0;
    float nearPlane = camera.nearPlane;
    float farPlane = camera.farPlane;
    ivec3 cameraCoords = calculateChunkCoordinates(rayOrigin);
    ivec3 rayCurrentChunk = ivec3_create(drawDistance); //should be 5,5,5 since ray starts at camera position
    int currentGridIndex = get_index_from_coords(rayCurrentChunk, uvec3_create(chunk_grid_resolution)); //should be 665
    ivec3 rayCurrentWorldChunk = cameraCoords;
    //calculate the world space coordinates of the current cell's minimum corner
    vec3 cellMinCorner = vec3_create(rayCurrentWorldChunk) * chunk_size - (chunk_size/2.0f);

    ivec3 step = {};
    step.x = sign(rayDir.x);
    step.y = sign(rayDir.y);
    step.z = sign(rayDir.z);
    
    // The world space position of the next cell boundary
    vec3 stepGreaterThanZero = vec3_create(step.x > 0 ? 1.0 : 0.0, step.y > 0 ? 1.0 : 0.0, step.z > 0 ? 1.0 : 0.0);
    vec3 nextBoundary = cellMinCorner + (vec3_create(step) * chunk_size * stepGreaterThanZero);

    // Calculate the distance from ray origin to the next boundary along each axis
    vec3 toNextBoundary = nextBoundary - rayOrigin;

    // Initialize tMax and tDelta with large values
    vec3 tMax = vec3_create(FLT_MAX);
    vec3 tDelta = vec3_create(FLT_MAX);

    vec3 brickmapTMax = vec3_create(FLT_MAX);
    vec3 brickmapTDelta = vec3_create(FLT_MAX);

    vec3 voxelTMax = vec3_create(FLT_MAX);
    vec3 voxelTDelta = vec3_create(FLT_MAX);

    // Only perform calculations for non-zero components of rayDir
    if (rayDir.x != 0.0) {
        tMax.x = toNextBoundary.x / rayDir.x;
        tDelta.x = fabs(chunk_size / rayDir.x);
        brickmapTDelta.x = fabs(pbmr / rayDir.x);
        voxelTDelta.x = fabs(voxel_size / rayDir.x);
    }
    if (rayDir.y != 0.0) {
        tMax.y = toNextBoundary.y / rayDir.y;
        tDelta.y = fabs(chunk_size / rayDir.y);
        brickmapTDelta.y = fabs(pbmr / rayDir.y);
        voxelTDelta.y = fabs(voxel_size / rayDir.y);
    }
    if (rayDir.z != 0.0) {
        tMax.z = toNextBoundary.z / rayDir.z;
        tDelta.z = fabs(chunk_size / rayDir.z);
        brickmapTDelta.z = fabs(pbmr / rayDir.z);
        voxelTDelta.z = fabs(voxel_size / rayDir.z);
    }


    float distanceToHit = 0.0f;
    vec3 chunkHitPosition = vec3_create(0.0f);
    int maxChunkSteps =     20;
    int maxBrickmapSteps =  20;
    int maxVoxelSteps =     20;
    bool hit = false; 
    bool brickmapHit = false; 
    float debugBrickmapDistanceToHit = 0.0f;
    float debugChunkDistanceToHit = 0.0f;

    float brickmapDistanceToHit = 0.0f;
    float voxelDistanceToHit = 0.0f;
    ivec3 rayCurrentBrickmapCell;
    ivec3 rayCurrentVoxelCell;
    uint8_t selectedVoxelColor;
    uint32_t LODresolution = 0;
    float brickmapScale = 0.0f;
    float adjustedBrickmapScale = 0.0f; //to account for the lost voxels from padding *(64/62) or * 1.032258064516129f
    float brickmapCenterOffset = 0.0f;
    float adjustedBrickmapCenterOffset = 0.0f;
    float inverseBrickmapScale = 0.0f;
    float adjustedInverseBrickmapScale = 0.0f;
    uint16_t LODIndex = NULL_LOD_INDEX;
    vec3 currentChunkPos = {};
    Brickmap64* brickmapsArray = nullptr;
    vec3 currentBrickmapPosition;
    uint32_t chunkID = getchunkID(chunkData, cameraCoords);

    // printf("raycast coords %f %f %f, chunkID %f\n", cameraCoords.x,cameraCoords.y,cameraCoords.z, chunkID);
    printf("ray cast start, cameraPos: %f %f %f\n", rayOrigin.x,rayOrigin.y,rayOrigin.z);
    printf("rayDir: %f %f %f\n", rayDir.x,rayDir.y,rayDir.z);
    printf("tMax: %f %f %f\n", tMax.x,tMax.y,tMax.z);


    // if (rayDir.x != 0.0) {
    //     tDelta.x = abs(chunk_size / rayDir.x);
    //     // brickmapTDelta.x = abs(brickmapScale / rayDir.x);
    //     brickmapTDelta.x = abs(64.0f / rayDir.x);
    //     voxelTDelta.x = abs(1.0322f / rayDir.x); 
    // }
    // if (rayDir.y != 0.0) {
    //     tDelta.y = abs(chunk_size / rayDir.y);
    //     brickmapTDelta.y = abs(64.0f / rayDir.y);
    //     // voxelTDelta.y = abs(voxelSize / rayDir.y);
    //     voxelTDelta.y = abs(1.0322f / rayDir.y);
    // }
    // if (rayDir.z != 0.0) {
    //     tDelta.z = abs(chunk_size / rayDir.z);
    //     brickmapTDelta.z = abs(64.0f / rayDir.z);
    //     voxelTDelta.z = abs(1.0322f / rayDir.z);
    // }
    printf("brickmapTDelta: %f %f %f\n", brickmapTDelta.x,brickmapTDelta.y,brickmapTDelta.z);
    printf("voxelTDelta: %f %f %f\n", voxelTDelta.x,voxelTDelta.y,voxelTDelta.z);
    

    // // Now you can loop through the voxels along the ray's path
    // //////////////////////////////////////////////////////////////// CHUNK GRID LOOP ////////////////////////////////////////////////////////////////
    while (!hit) { 
        rayCurrentWorldChunk = rayCurrentChunk - ivec3_create(drawDistance) + cameraCoords;

        bool chunkExists = false;
        bool hasVoxels = false;
        int currentchunkID = 0;
        chunkID = getchunkID(chunkData, rayCurrentWorldChunk);
        float chunkSize = chunk_size;// * 1.015 ;

        if(chunkID != NULL_CHUNK && chunkData->safeToEdit[chunkID]){
            chunkExists = true;
            selectedchunkID = chunkID;

            LODresolution = 1;
            brickmapsArray = &chunkData->brickmaps[chunkID];
            // currentChunkPos -= 1.0f;
            // chunkSize *= 1.0322;
            brickmapScale = chunkSize;
            inverseBrickmapScale = 1 / brickmapScale;
            // voxelSize = (chunkSize / (brickmapScale * LODresolution));// * 1.0322;
            // voxelSize = 1.032258064516129f; //i'm guessing this will work the same
            voxelSize = LOW_VOXEL_SCALE;

            halfVoxelSize = voxelSize * 0.5f;
            brickmapCenterOffset = brickmapScale * 0.5f;
            
            // brickmapScale = chunkSize / LODresolution;
            // inverseBrickmapScale = 1 / brickmapScale;
            // voxelSize = (chunkSize / (brickmapScale * LODresolution));// * 1.0322;
            if (rayDir.x != 0.0) {
                tDelta.x = fabs(chunk_size / rayDir.x);
                // brickmapTDelta.x = abs(brickmapScale / rayDir.x);
                brickmapTDelta.x = fabs(brickmapScale / rayDir.x);
                voxelTDelta.x = fabs(voxelSize / rayDir.x); 
            }
            if (rayDir.y != 0.0) {
                tDelta.y = fabs(chunk_size / rayDir.y);
                brickmapTDelta.y = fabs(brickmapScale / rayDir.y);
                // voxelTDelta.y = abs(voxelSize / rayDir.y);
                voxelTDelta.y = fabs(voxelSize / rayDir.y);
            }
            if (rayDir.z != 0.0) {
                tDelta.z = fabs(chunk_size / rayDir.z);
                brickmapTDelta.z = fabs(brickmapScale / rayDir.z);
                voxelTDelta.z = fabs(voxelSize / rayDir.z);
            }
        }



        if (chunkExists) {

            ChunkStage chunkStage = (ChunkStage)AtomicRead(chunkData->volChunkStages + chunkID);
            if(chunkStage == chunkStage_secondPassComplete || chunkStage_meshed || chunkStage_uploaded)hasVoxels = true;
            if(hasVoxels){
              
                // printf("chunkID %f has voxels!", chunkID);
                chunkHitPosition = rayOrigin + (rayDir * distanceToHit);
    
                maxBrickmapSteps = 30;
                rayCurrentBrickmapCell = calculateBrickmapCoordinates(chunkHitPosition, LODresolution);

                printf("chunkHitPosition: %f %f %f\n", chunkHitPosition.x, chunkHitPosition.y, chunkHitPosition.z);

                
                currentBrickmapIndex = get_index_from_coords(rayCurrentBrickmapCell, uvec3_create(LODresolution));


                //calculate the world space coordinates of the current cell's minimum corner
                vec3 brickCellMinCorner = brickmapPosition_LOD(rayCurrentBrickmapCell, currentChunkPos, brickmapScale, brickmapCenterOffset) - (brickmapScale*0.5f);
                printf("brickCellMinCorner: %f %f %f\n", brickCellMinCorner.x, brickCellMinCorner.y, brickCellMinCorner.z);

                // The world space position of the next cell boundary
                vec3 nextBrickBoundary = brickCellMinCorner + (vec3_create(step) * brickmapScale * stepGreaterThanZero);
                printf("nextBrickBoundary: %f %f %f\n", nextBrickBoundary.x, nextBrickBoundary.y, nextBrickBoundary.z);

                // Calculate the distance from ray origin to the next boundary along each axis
                vec3 toNextBrickBoundary = nextBrickBoundary - chunkHitPosition;
                printf("toNextBrickBoundary: %f %f %f\n", toNextBrickBoundary.x, toNextBrickBoundary.y, toNextBrickBoundary.z);

                brickmapTMax = toNextBrickBoundary / rayDir;
                printf("brickmapTMax: %f %f %f\n", brickmapTMax.x,brickmapTMax.y,brickmapTMax.z);

                //////////////////////////////////////////////////////////////// BRICKMAP GRID LOOP ////////////////////////////////////////////////////////////////
                uint16_t brickmapStepCount = 0;
                while (!hit) {
                    // Update currentGridIndex
                    printf("brickmap loop, step: %d, current brickmap: %d %d %d\n",brickmapStepCount, rayCurrentBrickmapCell.x,rayCurrentBrickmapCell.y,rayCurrentBrickmapCell.z);
                    currentBrickmapIndex = get_index_from_coords(rayCurrentBrickmapCell, uvec3_create(LODresolution));
                    Brickmap64* bm = &brickmapsArray[currentBrickmapIndex];
                    if(currentBrickmapIndex < 0 || currentBrickmapIndex >= (LODresolution * LODresolution * LODresolution)){
                        brickmapDistanceToHit = 0.0f;
                        break;
                    }
                    bool containsVoxels = false;

                    if(bm->active_count > 0){
                        maxVoxelSteps = 200;
                        //TODO: adjust step size dynamically to ensure ray hits every single voxel
                        printf("brickmap has voxels");
                        vec3 brickmapHitPosition = rayOrigin + (rayDir * (distanceToHit + brickmapDistanceToHit));
                        printf("brickmap hit position: %f,%f,%f\n", brickmapHitPosition.x, brickmapHitPosition.y, brickmapHitPosition.z);

                        currentBrickmapPosition = brickmapPosition_LOD(rayCurrentBrickmapCell, currentChunkPos, brickmapScale, brickmapCenterOffset);
                        printf("currentBrickmapPosition: %f %f %f\n", currentBrickmapPosition.x, currentBrickmapPosition.y, currentBrickmapPosition.z);

                        brickCellMinCorner = brickmapPosition_LOD(rayCurrentBrickmapCell, currentChunkPos, brickmapScale, brickmapCenterOffset) - (brickmapScale*0.5f);
                        printf("brickCellMinCorner: %f %f %f\n", brickCellMinCorner.x, brickCellMinCorner.y, brickCellMinCorner.z);
                        //position the brickmap so the non padding starts at the bottom left
                        // currentBrickmapPosition -= voxelSize;
                        // printf("shift -%f currentBrickmapPosition: %f %f %f\n",voxelSize, currentBrickmapPosition.x, currentBrickmapPosition.y, currentBrickmapPosition.z);

                        rayCurrentVoxelCell = calculateVoxelCoordinates(brickmapHitPosition, currentBrickmapPosition, brickmapScale, inverseBrickmapScale);
                        printf("rayCurrentVoxelCell: %d %d %d\n", rayCurrentVoxelCell.x, rayCurrentVoxelCell.y, rayCurrentVoxelCell.z);
                        
                        currentVoxelIndex = get_index_from_coords(rayCurrentVoxelCell, uvec3_create(pbmr));
                        printf("currentVoxelIndex: %d\n", currentVoxelIndex);

                        //calculate the world space coordinates of the current cell's minimum corner
                        // vec3 voxelCellMinCorner = voxel_position(pbmr, currentVoxelIndex, currentBrickmapPosition, LODresolution, chunkSize) - (float(voxelSize)/2.0f);
                        vec3 voxelCellMinCorner = getVoxelWorldPosition(rayCurrentVoxelCell, brickCellMinCorner, halfVoxelSize, LODresolution) - (halfVoxelSize);
                        printf("voxelCellMinCorner: %f %f %f\n", voxelCellMinCorner.x, voxelCellMinCorner.y, voxelCellMinCorner.z);

                        // The world space position of the next cell boundary
                        vec3 nextVoxelBoundary = voxelCellMinCorner + (vec3_create(step) * voxelSize * stepGreaterThanZero);

                        // Calculate the distance from ray origin to the next boundary along each axis
                        vec3 toNextVoxelBoundary = nextVoxelBoundary - brickmapHitPosition;
                        printf("toNextVoxelBoundary: %f %f %f\n", toNextVoxelBoundary.x,toNextVoxelBoundary.y,toNextVoxelBoundary.z);

                        voxelTMax = toNextVoxelBoundary / rayDir;
                        printf("voxelTMax: %f %f %f\n", voxelTMax.x,voxelTMax.y,voxelTMax.z);
                        
                        uint16_t voxelStep = 0;
                        //////////////////////////////////////////////////////////////// VOXEL GRID LOOP ////////////////////////////////////////////////////////////////
                        while (!hit) {
                            // Update currentGridIndex
                            printf("voxel grid loop, step %d\n", voxelStep);

                            currentVoxelIndex = get_index_from_coords(rayCurrentVoxelCell, uvec3_create(pbmr));
                            printf("rayCurrentVoxelCell: %d %d %d, index %d\n", rayCurrentVoxelCell.x, rayCurrentVoxelCell.y, rayCurrentVoxelCell.z, currentVoxelIndex);

                            if(currentVoxelIndex < 0 || currentVoxelIndex >= (pbmr * pbmr * pbmr)){
                                voxelDistanceToHit = 0.0f;
                                break;
                            }
                            //for finding if the specific voxel the ray intersects is set
                            // if(isBitSet(currentChunk->brickgrid.brickmaps[currentBrickmapIndex]->bitmask, currentVoxelIndex)){
                            bool notInPadding = ((rayCurrentVoxelCell.x > 0 &&  rayCurrentVoxelCell.x < 63) && (rayCurrentVoxelCell.y > 0 &&  rayCurrentVoxelCell.y < 63) && (rayCurrentVoxelCell.z > 0 &&  rayCurrentVoxelCell.z < 63));
                            
                            uint32_t voxelPathIndex = chunkData->voxelPathCount;
                            // chunkData->voxelPath[voxelPathIndex].voxel_position = voxel_position(pbmr, currentVoxelIndex, currentBrickmapPosition + voxelSize, LODresolution, chunkSize);// * ((pbmr/brickmap_resolution));
                            chunkData->voxelPath[voxelPathIndex].voxel_position = getVoxelWorldPosition(rayCurrentVoxelCell, brickCellMinCorner, halfVoxelSize, LODresolution);
                            chunkData->voxelPath[voxelPathIndex].voxel_coords = rayCurrentVoxelCell;
                            chunkData->voxelPath[voxelPathIndex].voxel_index = currentVoxelIndex;
                            chunkData->voxelPath[voxelPathIndex].voxel_scale = voxelSize;
                            chunkData->voxelPath[voxelPathIndex].step = voxelStep;
                            chunkData->voxelPath[voxelPathIndex].hit = false;
                            chunkData->voxelPathCount++;
                            if(chunkData->voxelPathCount >= 1024){
                                chunkData->voxelPathCount = 0; //reset count if its too large
                            }
                            voxelStep++;
                            // if(notInPadding && bm->voxels[currentVoxelIndex] & 0x3FF){
                            if(notInPadding && bm->voxels[currentVoxelIndex]){
                                printf("VOXEL HIT");

                                brickmap_index = currentBrickmapIndex;
                                voxel_index = currentVoxelIndex;
                                // voxelCenter = voxel_position(pbmr, currentVoxelIndex, currentBrickmapPosition + voxelSize, LODresolution, chunkSize) * 1.032258064516129f;
                                voxelCenter = getVoxelWorldPosition(rayCurrentVoxelCell, brickCellMinCorner, halfVoxelSize, LODresolution) ;
                                hit = true;
                                voxel_type = bm->voxels[currentVoxelIndex];
                                chunkData->voxelRayCastResult.voxel_coords = rayCurrentVoxelCell;
                                printf("hit voxel coords: %d %d %d\n", rayCurrentVoxelCell.x,rayCurrentVoxelCell.y,rayCurrentVoxelCell.z);
                                chunkData->voxelRayCastResult.voxel_index = currentVoxelIndex;
                                chunkData->voxelPath[voxelPathIndex].hit = true;
                                selectedVoxelColor = bm->voxels[currentVoxelIndex];
                                // printf("hit, brickmap_index: %i, voxel_index: %i\n", brickmap_index, voxel_index);
                                break;
                            }
                            if(hit){break;}
                            if(--maxVoxelSteps <= 0 || distanceToHit + brickmapDistanceToHit + voxelDistanceToHit > farPlane){
                                // printf("breaking out of voxel ray loop\n");
                                voxelDistanceToHit = 0.0f;
                                break;
                            }
                            // Move to the next voxel along the ray
                            if (voxelTMax.x < voxelTMax.y && voxelTMax.x < voxelTMax.z) {
                                // The ray will hit the X plane next
                                rayCurrentVoxelCell.x += step.x;
                                voxelDistanceToHit = voxelTMax.x;
                                voxelTMax.x += voxelTDelta.x;
                                printf("stepping X");

                            } else if (voxelTMax.y < voxelTMax.z) {
                                // The ray will hit the Y plane next
                                rayCurrentVoxelCell.y += step.y;
                                voxelDistanceToHit = voxelTMax.y;
                                voxelTMax.y += voxelTDelta.y;
                                printf("stepping Y");

                            } else {
                                // The ray will hit the Z plane next
                                rayCurrentVoxelCell.z += step.z;
                                voxelDistanceToHit = voxelTMax.z;
                                voxelTMax.z += voxelTDelta.z;
                                printf("stepping Z");

                            }

                            // Check if the current cell is within the bounds of your grid
                            if (rayCurrentVoxelCell.x < 1 || rayCurrentVoxelCell.x >= maxVal ||
                                rayCurrentVoxelCell.y < 1 || rayCurrentVoxelCell.y >= maxVal ||
                                rayCurrentVoxelCell.z < 1 || rayCurrentVoxelCell.z >= maxVal) {
                                voxelDistanceToHit = 0.0f;
                                break; // Exit the loop if the ray is out of bounds
                            }

                        }
                        //*/
                    }
                    brickmapStepCount++;
                    if(hit){break;}
                    if(--maxBrickmapSteps <= 0 || distanceToHit + brickmapDistanceToHit > farPlane){
                        brickmapDistanceToHit = 0.0f;
                        // printf("breaking out of brickmap ray loop\n");
                        break;
                    } 
                    // Move to the next voxel along the ray
                    if (brickmapTMax.x < brickmapTMax.y && brickmapTMax.x < brickmapTMax.z) {
                        // The ray will hit the X plane next
                        rayCurrentBrickmapCell.x += step.x;
                        brickmapDistanceToHit = brickmapTMax.x;
                        brickmapTMax.x += brickmapTDelta.x;
                    } else if (brickmapTMax.y < brickmapTMax.z) {
                        // The ray will hit the Y plane next
                        rayCurrentBrickmapCell.y += step.y;
                        brickmapDistanceToHit = brickmapTMax.y;
                        brickmapTMax.y += brickmapTDelta.y;
                    } else {
                        // The ray will hit the Z plane next
                        rayCurrentBrickmapCell.z += step.z;
                        brickmapDistanceToHit = brickmapTMax.z;
                        brickmapTMax.z += brickmapTDelta.z;
                    }

                    // Check if the current cell is within the bounds of your grid
                    if (rayCurrentBrickmapCell.x < 0 || rayCurrentBrickmapCell.x >= LODresolution ||
                        rayCurrentBrickmapCell.y < 0 || rayCurrentBrickmapCell.y >= LODresolution ||
                        rayCurrentBrickmapCell.z < 0 || rayCurrentBrickmapCell.z >= LODresolution) {
                        brickmapDistanceToHit = 0.0f;
                        break; // Exit the loop if the ray is out of bounds
                    }

                }

            }
        }

        if(hit){break;}

        if(--maxChunkSteps <= 0 || distanceToHit > farPlane){
            distanceToHit = 0.0f;
            break;
        }
        // Move to the next voxel along the ray
        if (tMax.x < tMax.y && tMax.x < tMax.z) {
            // The ray will hit the X plane next
            rayCurrentChunk.x += step.x;
            distanceToHit = tMax.x;
            tMax.x += tDelta.x;
        } else if (tMax.y < tMax.z) {
            // The ray will hit the Y plane next
            rayCurrentChunk.y += step.y;
            distanceToHit = tMax.y;
            tMax.y += tDelta.y;
        } else {
            // The ray will hit the Z plane next
            rayCurrentChunk.z += step.z;
            distanceToHit = tMax.z;
            tMax.z += tDelta.z;
        }

        // Check if the current cell is within the bounds of your grid
        if (rayCurrentChunk.x < 0 || rayCurrentChunk.x >= chunk_grid_resolution ||
            rayCurrentChunk.y < 0 || rayCurrentChunk.y >= chunk_grid_resolution ||
            rayCurrentChunk.z < 0 || rayCurrentChunk.z >= chunk_grid_resolution) {
            distanceToHit = 0.0f;
            break; // Exit the loop if the ray is out of bounds
        }

        // Update currentGridIndex
        currentGridIndex = get_index_from_coords(rayCurrentChunk, uvec3_create(chunk_grid_resolution));
        if(currentGridIndex < 0 || currentGridIndex >= 1331){
            distanceToHit = 0.0f;
            printf("breaking from chunk loop, ray went out of bounds\n");
            break;
        }

        
        // Further checks and operations
        //check if the current grid index is in the mega chunk bitmask
        //if it is, the chunk has voxels, so we need to check the brickmaps
    
    }

    if(hit){
        vec3 finalHitPosition = rayOrigin + (rayDir * (distanceToHit + brickmapDistanceToHit + voxelDistanceToHit));
        result.hitPosition = finalHitPosition;

        ivec3 hitPosVox = calculateVoxelCoordinates(finalHitPosition,currentBrickmapPosition, brickmapScale, inverseBrickmapScale);
        //determine normal/adjacent voxel
        vec3 diff = finalHitPosition - voxelCenter;
        vec3 normalizedDiff = vec3_normalize(diff);
        float maxComponent = max(max(fabs(normalizedDiff.x), fabs(normalizedDiff.y)), fabs(normalizedDiff.z));
        if (maxComponent == fabs(normalizedDiff.x)) {
            hitNormal = sign(normalizedDiff.x) * vec3_create(1, 0, 0);
        } else if (maxComponent == fabs(normalizedDiff.y)) {
            hitNormal = sign(normalizedDiff.y) * vec3_create(0, 1, 0);
        } else {
            hitNormal = sign(normalizedDiff.z) * vec3_create(0, 0, 1);
        }

        ivec3 adjacent_voxel_coords = rayCurrentVoxelCell + ivec3_create(hitNormal);
        ivec3 adjacent_brickmap_coords = rayCurrentBrickmapCell;
        
        // printf("set adjacent chunk coords here\n");
        // adjacent_chunk_coords = selectedChunk->chunk_coords;
        adjacent_chunk_coords = rayCurrentWorldChunk;
        adjacent_voxel_index = get_index_from_coords(adjacent_voxel_coords, uvec3_create(pbmr));
        adjacent_brickmap_index = get_index_from_coords(adjacent_brickmap_coords, uvec3_create(LODresolution));;
        adjacent_chunk_index = chunkID;
        
    //     // printf("set selected chunk here\n");
    //     // *selected_chunk = selectedChunk;

        //adjacent variables are for when we want to place a voxel, we determine the normal of the currently selected voxel and place a voxel in that direction
        // Check if the adjacent voxel is outside the current brickmap, and adjust the brickmap and chunk index if necessary
        if     (adjacent_voxel_coords.x < 1 ) adjacent_voxel_coords.x = 62;
        else if(adjacent_voxel_coords.x > 62) adjacent_voxel_coords.x = 1;

        if     (adjacent_voxel_coords.y < 1 ) adjacent_voxel_coords.y = 62;
        else if(adjacent_voxel_coords.y > 62) adjacent_voxel_coords.y = 1;
        
        if     (adjacent_voxel_coords.z < 1 ) adjacent_voxel_coords.z = 62;
        else if(adjacent_voxel_coords.z > 62) adjacent_voxel_coords.z = 1;

        if (adjacent_voxel_coords != (rayCurrentVoxelCell + ivec3_create(hitNormal))) {
            adjacent_voxel_index = get_index_from_coords(adjacent_voxel_coords, uvec3_create(pbmr));
            
            // Calculate the brickmap index that the adjacent voxel would fall into
            adjacent_brickmap_coords = rayCurrentBrickmapCell + ivec3_create(hitNormal);
            adjacent_brickmap_index = get_index_from_coords(adjacent_brickmap_coords, uvec3_create(LODresolution));

            if     (adjacent_brickmap_coords.x < 0) adjacent_brickmap_coords.x = static_cast<int>(LODresolution) - 1;
            else if(adjacent_brickmap_coords.x >= static_cast<int>(LODresolution)) adjacent_brickmap_coords.x = 0;
            if     (adjacent_brickmap_coords.y < 0) adjacent_brickmap_coords.y = static_cast<int>(LODresolution) - 1;
            else if(adjacent_brickmap_coords.y >= static_cast<int>(LODresolution)) adjacent_brickmap_coords.y = 0;
            if     (adjacent_brickmap_coords.z < 0) adjacent_brickmap_coords.z = static_cast<int>(LODresolution) - 1;
            else if(adjacent_brickmap_coords.z >= static_cast<int>(LODresolution)) adjacent_brickmap_coords.z = 0;

            if (adjacent_brickmap_coords != (rayCurrentBrickmapCell + ivec3_create(hitNormal))) {

                adjacent_brickmap_index = get_index_from_coords(adjacent_brickmap_coords, uvec3_create(LODresolution));
                
                // Calculate the brickmap index that the adjacent voxel would fall into
                adjacent_chunk_coords = rayCurrentWorldChunk + ivec3_create(hitNormal);
                bool chunkExists = false;
                uint32_t adjacentChunkID = getchunkID(chunkData, adjacent_chunk_coords);
                if(adjacentChunkID != chunkID && adjacentChunkID !=NULL_CHUNK){
                    //adjacent chunk exists
                    if(!chunkData->safeToEdit[adjacentChunkID]){
                        adjacentChunkID = NULL_CHUNK;
                    }
                    adjacent_chunk_index = adjacentChunkID;
                }



            }


        }

        result.adjacent_voxel_coords = adjacent_voxel_coords;
        result.voxel_coords = rayCurrentVoxelCell;
        result.chunk_coords = rayCurrentWorldChunk;
        result.adjacent_chunk_coords = adjacent_chunk_coords;
        result.voxel_type = voxel_type;
        result.adjacent_chunk_index = adjacent_chunk_index;
        result.adjacent_voxel_index = adjacent_voxel_index;
        result.selected = true;
        result.voxel_position = voxelCenter;
        result.chunkID = chunkID;

        return true;
    }
    result.chunk_coords = ivec3_create(0);
    result.adjacent_chunk_coords = ivec3_create(0);
    result.adjacent_voxel_coords = ivec3_create(0);
    result.voxel_coords = ivec3_create(0);
    result.voxel_type = 0;
    result.chunkID = NULL_CHUNK;
    result.selected = false;
    result.voxel_index = 0;
    result.chunk_coords = selected_chunk_coords;
    result.voxel_color = 0;


    return false;
}







//////////////////////////////////////////////// FPT OPERATIONS /////////////////////////////////////////////////////////////////


    //we dont need the scaled chunkSize in here, do we? we want to constrain the ray to the regular chunk size...
    ivec3 calculateBrickmapCoordinates(fpt_vec3 position, fpt grid_resolution) {
        fpt_vec3 normalizedPosition = (position) + FPT_HALF_CHUNK_SIZE;
        

        // For negative numbers, floor can be implemented by:
        // (int)(x - 0.5f)
        // Or even: (int)x - (x < 0)
        
        ivec3 result;
        fpt max_val = fpt_sub(grid_resolution , FPT_ONE);
        
        fpt scaledX = fpt_mul(fpt_mul(normalizedPosition.x, FPT_INVERSE_CHUNK_SIZE), grid_resolution);
        fpt scaledY = fpt_mul(fpt_mul(normalizedPosition.y, FPT_INVERSE_CHUNK_SIZE), grid_resolution);
        fpt scaledZ = fpt_mul(fpt_mul(normalizedPosition.z, FPT_INVERSE_CHUNK_SIZE), grid_resolution);
        
        result.x = fpt2i(CLAMP(scaledX, 0, max_val));
        result.y = fpt2i(CLAMP(scaledY, 0, max_val));
        result.z = fpt2i(CLAMP(scaledZ, 0, max_val));
        return result;
    }


    fpt_vec3 fpt_brickmapPosition_LOD(ivec3 brickmapCoordinates, fpt_vec3 chunkPosition, fpt brickmapScale, fpt brickmapCenterOffset) {
        fpt_vec3 brickmapPosition = (chunkPosition - FPT_HALF_CHUNK_SIZE) + 
                                    (fpt_vec3_create(i2fpt(brickmapCoordinates.x), i2fpt(brickmapCoordinates.y), i2fpt(brickmapCoordinates.z)) * brickmapScale) + brickmapCenterOffset;
        return brickmapPosition;
    }
    
    fpt_vec3 fpt_voxel_local_position_LOD(int bmResolution, int voxel_index, uint32_t LODresolution, fpt chunkSize) {
        ivec3 position = get_coords_from_index(voxel_index, uvec3_create(bmResolution));
        fpt_vec3 voxel_local_position_vec3 = fpt_vec3_create(i2fpt(position.x),i2fpt(position.y),i2fpt(position.z));
        fpt voxel_size = fpt_div(chunkSize , (fpt_mul(i2fpt(bmResolution), i2fpt(LODresolution))));
        fpt_vec3 voxel_local_position = voxel_local_position_vec3 * voxel_size;
        fpt voxel_center_offset = fpt_div(voxel_size , 131072); //2 * 2^16
        return voxel_local_position + voxel_center_offset;
    }

    fpt_vec3 fpt_voxel_position(int bmResolution, int voxel_local_index, const fpt_vec3& currentBrickmapPosition, uint32_t LODresolution, fpt chunkSize){
        fpt_vec3  voxel_local_position = fpt_voxel_local_position_LOD(bmResolution, voxel_local_index, LODresolution, chunkSize);
        fpt_vec3 absolute_voxel_position = voxel_local_position + (currentBrickmapPosition - fpt_div(fpt_div(chunkSize,  i2fpt(LODresolution)) , 131072));
        return absolute_voxel_position;
    }

    
    ivec3 fpt_calculateVoxelCoordinates(fpt_vec3 pos, fpt epsilon) {
        pos += epsilon;

        // First normalize position to 0-1 range
        // fpt halfSizeEpsilon = fpt_add(brickmapScale, 32768); //0.5 * 2^16

        // printf("pos          : %10.5f %10.5f %10.5f, epsilon/fudge factor: %10.5f \n", fpt2fl(pos.x),fpt2fl(pos.y),fpt2fl(pos.z), fpt2fl(epsilon));
        
        // Move from -halfSize..+halfSize to 0..1
        
        fpt normalizedX = fpt_mul(fpt_add(pos.x , FPT_HALF_CHUNK_SIZE) , FPT_INVERSE_CHUNK_SIZE);
        fpt normalizedY = fpt_mul(fpt_add(pos.y , FPT_HALF_CHUNK_SIZE) , FPT_INVERSE_CHUNK_SIZE);
        fpt normalizedZ = fpt_mul(fpt_add(pos.z , FPT_HALF_CHUNK_SIZE) , FPT_INVERSE_CHUNK_SIZE);
        // printf("NON PADDED normalizedXYZ: %10.5f %10.5f %10.5f\n", fpt2fl(normalizedX),fpt2fl(normalizedY),fpt2fl(normalizedZ));
        
        fpt scaledX = fpt_add(fpt_mul(normalizedX , FPT_USABLE_VOXELS) , FPT_ONE);
        fpt scaledY = fpt_add(fpt_mul(normalizedY , FPT_USABLE_VOXELS) , FPT_ONE);
        fpt scaledZ = fpt_add(fpt_mul(normalizedZ , FPT_USABLE_VOXELS) , FPT_ONE);
        // printf("scaledXYZ    : %10.5f %10.5f %10.5f\n", fpt2fl(scaledX),fpt2fl(scaledY),fpt2fl(scaledZ));
        // printf("v FPT        : %d %d %d\n", scaledX, scaledY, scaledZ);

        ivec3 v;
        v.x = (int)(fpt2i(scaledX));
        v.y = (int)(fpt2i(scaledY));
        v.z = (int)(fpt2i(scaledZ));
        // printf("v            : %d %d %d\n", v.x, v.y, v.z);

        // Clamp to usable range
        v.x = CLAMP(v.x, 1, USABLE_VOXELS);
        v.y = CLAMP(v.y, 1, USABLE_VOXELS);
        v.z = CLAMP(v.z, 1, USABLE_VOXELS);
        // printf("CLAMPED v    : %d %d %d\n", v.x, v.y, v.z);

        return v;
    }


    //approximates the world space grid if it were all voxel coords, and just clamps them to the current chunk between 0 and 63
    //used for accurate cross chunk voxel collision testing
    ivec3 fpt_calculateAABBVoxelCoordinates(fpt_vec3 pos, fpt epsilon) {
        pos += epsilon;
        
        fpt normalizedX = fpt_mul(fpt_add(pos.x , FPT_HALF_CHUNK_SIZE) , FPT_INVERSE_CHUNK_SIZE);
        fpt normalizedY = fpt_mul(fpt_add(pos.y , FPT_HALF_CHUNK_SIZE) , FPT_INVERSE_CHUNK_SIZE);
        fpt normalizedZ = fpt_mul(fpt_add(pos.z , FPT_HALF_CHUNK_SIZE) , FPT_INVERSE_CHUNK_SIZE);
        // printf("normalizedXYZ: %10.5f %10.5f %10.5f\n", fpt2fl(normalizedX),fpt2fl(normalizedY),fpt2fl(normalizedZ));
        
        fpt scaledX = fpt_add(fpt_mul(normalizedX , FPT_USABLE_VOXELS) , FPT_ONE);
        fpt scaledY = fpt_add(fpt_mul(normalizedY , FPT_USABLE_VOXELS) , FPT_ONE);
        fpt scaledZ = fpt_add(fpt_mul(normalizedZ , FPT_USABLE_VOXELS) , FPT_ONE);
        // printf("scaledXYZ    : %10.5f %10.5f %10.5f\n", fpt2fl(scaledX),fpt2fl(scaledY),fpt2fl(scaledZ));
        // printf("v FPT        : %d %d %d\n", scaledX, scaledY, scaledZ);

        ivec3 v;
        v.x = (int)(fpt2i(scaledX));
        v.y = (int)(fpt2i(scaledY));
        v.z = (int)(fpt2i(scaledZ));
        // printf("v            : %d %d %d\n", v.x, v.y, v.z);

        // Clamp to usable range
        v.x = CLAMP(v.x, 0, PADDED_VOXELS);
        v.y = CLAMP(v.y, 0, PADDED_VOXELS);
        v.z = CLAMP(v.z, 0, PADDED_VOXELS);
        // printf("CLAMPED v    : %d %d %d\n", v.x, v.y, v.z);

        return v;
    }


        ivec3 fpt_calculatePaddedVoxelCoordinates(fpt_vec3 pos, fpt epsilon) {

            pos += epsilon;

            // First normalize position to 0-1 range
            // fpt halfSizeEpsilon = fpt_add(brickmapScale, 32768); //0.5 * 2^16
    
            // printf("pos          : %10.5f %10.5f %10.5f, epsilon/fudge factor: %10.5f \n", fpt2fl(pos.x),fpt2fl(pos.y),fpt2fl(pos.z), fpt2fl(epsilon));
            
            // Move from -halfSize..+halfSize to 0..1
            fpt normalizedX = fpt_mul(fpt_add(pos.x , FPT_HALF_CHUNK_SIZE) , FPT_INVERSE_CHUNK_SIZE);
            fpt normalizedY = fpt_mul(fpt_add(pos.y , FPT_HALF_CHUNK_SIZE) , FPT_INVERSE_CHUNK_SIZE);
            fpt normalizedZ = fpt_mul(fpt_add(pos.z , FPT_HALF_CHUNK_SIZE) , FPT_INVERSE_CHUNK_SIZE);
            // printf("PADDED normalizedXYZ: %10.5f %10.5f %10.5f\n", fpt2fl(normalizedX),fpt2fl(normalizedY),fpt2fl(normalizedZ));
            
            fpt scaledX = fpt_mul(normalizedX , FPT_TOTAL_VOXELS);
            fpt scaledY = fpt_mul(normalizedY , FPT_TOTAL_VOXELS);
            fpt scaledZ = fpt_mul(normalizedZ , FPT_TOTAL_VOXELS);
            // printf("scaledXYZ    : %10.5f %10.5f %10.5f\n", fpt2fl(scaledX),fpt2fl(scaledY),fpt2fl(scaledZ));
            // printf("v FPT        : %d %d %d\n", scaledX, scaledY, scaledZ);
    
            ivec3 v;
            v.x = (int)(fpt2i(scaledX));
            v.y = (int)(fpt2i(scaledY));
            v.z = (int)(fpt2i(scaledZ));
            // printf("v            : %d %d %d\n", v.x, v.y, v.z);
    
            v.x = CLAMP(v.x, 0, PADDED_VOXELS);
            v.y = CLAMP(v.y, 0, PADDED_VOXELS);
            v.z = CLAMP(v.z, 0, PADDED_VOXELS);
            // printf("CLAMPED v    : %d %d %d\n", v.x, v.y, v.z);

            return v;
    }



    fpt_vec3 fpt_brickmap_local_chunk_position(ivec3 brickmapCoordinates, fpt brickmapScale, fpt brickmapCenterOffset) {
        fpt_vec3 brickmapPosition = (fpt_vec3_create(i2fpt(brickmapCoordinates.x), i2fpt(brickmapCoordinates.y), i2fpt(brickmapCoordinates.z)) * brickmapScale) + brickmapCenterOffset - FPT_HALF_CHUNK_SIZE;
        return brickmapPosition;
    }

    //this will also give us the local chunk position if we give it the local chunk position of the brickmap
    fpt_vec3 fpt_getVoxelWorldPosition(ivec3 voxel, fpt_vec3 chunkMinCorner) {
        fpt worldX = fpt_add( fpt_mul( fpt_sub(i2fpt(voxel.x), FPT_ONE) ,FPT_VOXEL_SCALE_PADDING_COMPENSATION) , fpt_add(chunkMinCorner.x, FPT_HALF_VOXEL_SCALE));
        fpt worldY = fpt_add( fpt_mul( fpt_sub(i2fpt(voxel.y), FPT_ONE) ,FPT_VOXEL_SCALE_PADDING_COMPENSATION) , fpt_add(chunkMinCorner.y, FPT_HALF_VOXEL_SCALE));
        fpt worldZ = fpt_add( fpt_mul( fpt_sub(i2fpt(voxel.z), FPT_ONE) ,FPT_VOXEL_SCALE_PADDING_COMPENSATION) , fpt_add(chunkMinCorner.z, FPT_HALF_VOXEL_SCALE));
        // printf("voxel coords: %d %d %d, pos: %10.5f %10.5f %10.5f \n", voxel.x,voxel.y,voxel.z, fpt2fl(worldX),fpt2fl(worldY),fpt2fl(worldZ));
        return fpt_vec3_create(worldX, worldY, worldZ);
    }


    bool fpt_pick_voxel_in_chunk(chunk_data* chunkData, fpt_vec3 rayOrigin, ivec3 raychunk_coords, fpt_vec3 rayDir, rayCastResult& result){

    result = {};
    uint32_t voxel_index = 0;
    uint32_t adjacent_chunk_index = 0;
    uint32_t adjacent_voxel_index = 0;
    uint8_t voxel_type = 0;
    int currentVoxelIndex = 0;
    ivec3 adjacent_chunk_coords = {};
    ivec3 selected_chunk_coords = {};

    // vec3 rayOrigin = camera.m_position;
    //add small offset in case a value is 0 to prevent division by 0
    if(rayDir.x == 0){
        rayDir.x = fpt_add(rayDir.x, 655);
        printf("rayDir X IS 0, adding 655");
    }
    if(rayDir.y == 0){
        rayDir.y = fpt_add(rayDir.y, 655);
        printf("rayDir Y IS 0, adding 655");
            
    }
    if(rayDir.z == 0){
        rayDir.z = fpt_add(rayDir.z, 655);
        printf("rayDir Z IS 0, adding 655");
            
    }
    
    fpt voxelSize = FPT_ONE;
    fpt_vec3 voxelCenter = {};
    int selectedchunkID = 0;
    fpt farPlane =  i2fpt(150);//hardcoded, used to pass in camera info
    // ivec3 cameraCoords = calculateFPTChunkCoordinates(rayOrigin);
    ivec3 cameraCoords = raychunk_coords;

    ivec3 rayCurrentChunk = ivec3_create(drawDistance); //should be 5,5,5 since ray starts at camera position
    int currentGridIndex = get_index_from_coords(rayCurrentChunk, uvec3_create(chunk_grid_resolution)); //should be 665
    ivec3 rayCurrentWorldChunk = cameraCoords;
    fpt_vec3 relativeChunkPosition = {};

    //calculate the world space coordinates of the current cell's minimum corner
    // fpt_vec3 cellMinCorner = (fpt_vec3_create(i2fpt(rayCurrentWorldChunk.x), i2fpt(rayCurrentWorldChunk.y), i2fpt(rayCurrentWorldChunk.z)) * FPT_CHUNK_SIZE) - (FPT_HALF_CHUNK_SIZE);
    
    //making it relative
    fpt_vec3 cellMinCorner = (relativeChunkPosition) - (FPT_HALF_CHUNK_SIZE);

    ivec3 step = {};
    step.x = rayDir.x > 0 ? 1 : (rayDir.x < 0 ? -1 : 0);
    step.y = rayDir.y > 0 ? 1 : (rayDir.y < 0 ? -1 : 0);
    step.z = rayDir.z > 0 ? 1 : (rayDir.z < 0 ? -1 : 0);

    // The world space position of the next cell boundary
    fpt_vec3 stepGreaterThanZero = fpt_vec3_create(step.x > 0 ? FPT_ONE : 0, step.y > 0 ? FPT_ONE : 0, step.z > 0 ? FPT_ONE : 0);
    #if 1
    fpt_vec3 nextBoundary = cellMinCorner + (fpt_vec3_create(i2fpt(step.x), i2fpt(step.y), i2fpt(step.z)) * FPT_CHUNK_SIZE * stepGreaterThanZero);
    #else
    fpt_vec3 nextBoundary = {};
    // nextBoundary.x = (step.x > 0) ? (cellMinCorner.x + FPT_CHUNK_SIZE) : cellMinCorner.x;
    // nextBoundary.y = (step.y > 0) ? (cellMinCorner.y + FPT_CHUNK_SIZE) : cellMinCorner.y;
    // nextBoundary.z = (step.z > 0) ? (cellMinCorner.z + FPT_CHUNK_SIZE) : cellMinCorner.z;
    nextBoundary.x = (step.x > 0) ? (fpt_ceil(rayOrigin.x / FPT_CHUNK_SIZE) * FPT_CHUNK_SIZE) : (fpt_floor(rayOrigin.x / FPT_CHUNK_SIZE) * FPT_CHUNK_SIZE);
    nextBoundary.y = (step.y > 0) ? (fpt_ceil(rayOrigin.y / FPT_CHUNK_SIZE) * FPT_CHUNK_SIZE) : (fpt_floor(rayOrigin.y / FPT_CHUNK_SIZE) * FPT_CHUNK_SIZE);
    nextBoundary.z = (step.z > 0) ? (fpt_ceil(rayOrigin.z / FPT_CHUNK_SIZE) * FPT_CHUNK_SIZE) : (fpt_floor(rayOrigin.z / FPT_CHUNK_SIZE) * FPT_CHUNK_SIZE);
    #endif
    // Calculate the distance from ray origin to the next boundary along each axis
    fpt_vec3 toNextBoundary = nextBoundary - rayOrigin;

    // Initialize tMax and tDelta with large values
    fpt_vec3 tMax =     fpt_vec3_create(FPT_MAX);
    fpt_vec3 tDelta =   fpt_vec3_create(FPT_MAX);


    fpt_vec3 voxelTMax = fpt_vec3_create(FPT_MAX);
    fpt_vec3 voxelTDelta = fpt_vec3_create(FPT_MAX);

    fpt voxel_size = FPT_VOXEL_SCALE; //1 in fpt
    
    // Only perform calculations for non-zero components of rayDir
    if (rayDir.x != 0) {
        tMax.x =            fpt_div(toNextBoundary.x , rayDir.x);
        tDelta.x =          fpt_abs(fpt_div(FPT_CHUNK_SIZE , rayDir.x));
        voxelTDelta.x =     fpt_abs(fpt_div(voxel_size , rayDir.x));
    } else {
        tMax.x = FPT_SAFE_RAYCAST_MAX;
        tDelta.x = FPT_SAFE_RAYCAST_MAX;
        voxelTDelta.x = FPT_SAFE_RAYCAST_MAX;
    }
    if (rayDir.y != 0) {
        tMax.y =            fpt_div(toNextBoundary.y , rayDir.y);
        tDelta.y =          fpt_abs(fpt_div(FPT_CHUNK_SIZE , rayDir.y));
        voxelTDelta.y =     fpt_abs(fpt_div(voxel_size , rayDir.y));
    } else {
        tMax.y = FPT_SAFE_RAYCAST_MAX;
        tDelta.y = FPT_SAFE_RAYCAST_MAX;
        voxelTDelta.y = FPT_SAFE_RAYCAST_MAX;
    }
    if (rayDir.z != 0) {
        tMax.z =            fpt_div(toNextBoundary.z , rayDir.z);
        tDelta.z =          fpt_abs(fpt_div(FPT_CHUNK_SIZE , rayDir.z));
        voxelTDelta.z =     fpt_abs(fpt_div(voxel_size , rayDir.z));
    } else {
        tMax.z = FPT_SAFE_RAYCAST_MAX;
        tDelta.z = FPT_SAFE_RAYCAST_MAX;
        voxelTDelta.z = FPT_SAFE_RAYCAST_MAX;
    }
    vec3 debugtMax = fpt_to_flt_vec3(tMax);
    vec3 debugtDelta = fpt_to_flt_vec3(tDelta);
    vec3 debugRayDir = fpt_to_flt_vec3(rayDir);
    vec3 debugToNextBoundary = fpt_to_flt_vec3(toNextBoundary);

    printf("rayDir       %f %f %f\n"  , debugRayDir.x, debugRayDir.y, debugRayDir.z);
    printf("toNextBound  %f %f %f\n"  , debugToNextBoundary.x, debugToNextBoundary.y, debugToNextBoundary.z);
    printf("tMax   start %f %f %f\n"  , debugtMax.x, debugtMax.y, debugtMax.z);
    printf("tDelta start %f %f %f\n"  , debugtDelta.x, debugtDelta.y, debugtDelta.z);

    fpt distanceToHit = 0;
    
    //for wrapping back around for local chunk position calculations (rebasing)
    fpt relative_distance_to_hit = 0;

    fpt_vec3 chunkHitPosition = {};
    int maxChunkSteps =     20;
    int maxVoxelSteps =     20;
    bool hit = false; 

    fpt voxelDistanceToHit = 0;
    ivec3 rayCurrentVoxelCell = {};
    uint8_t selectedVoxelColor;
    fpt_vec3 currentChunkPos = {};
    Brickmap64* bm = nullptr;
    uint32_t chunkID = getchunkID(chunkData, cameraCoords);
    uint16_t voxelStep = 0;

    ivec3 rayRelativeChunk = ivec3_create(drawDistance); //we subtract drawDisntance from it every loop, this way it starts as 0 0 0 

    vec3 floatStartPos = fpt_to_flt_vec3(rayOrigin);
    printf("ray start position: %f %f %f\n", floatStartPos.x,floatStartPos.y,floatStartPos.z);

    printf("cameraCoords      : %d %d %d\n", cameraCoords.x,cameraCoords.y,cameraCoords.z);
    int chunkSteps = 0;
    // //////////////////////////////////////////////////////////////// CHUNK GRID LOOP ////////////////////////////////////////////////////////////////
    while (!hit) { 
        rayCurrentWorldChunk = rayCurrentChunk - ivec3_create(drawDistance) + cameraCoords;
        rayRelativeChunk = rayCurrentChunk - ivec3_create(drawDistance);
        relativeChunkPosition = ivec_to_fpt_vec3(rayRelativeChunk) * FPT_CHUNK_SIZE;
        printf("chunkSteps: %d  curr Coords: %d %d %d\n", chunkSteps, rayCurrentWorldChunk.x,rayCurrentWorldChunk.y,rayCurrentWorldChunk.z);

        bool chunkExists = false;
        bool hasVoxels = false;
        int currentchunkID = 0;
        chunkID = getchunkID(chunkData, rayCurrentWorldChunk);

        if(chunkID != NULL_CHUNK && chunkData->safeToEdit[chunkID]){
            chunkExists = true;
            selectedchunkID = chunkID;
            // currentChunkPos = chunkData->fptPositions[chunkID];

            bm = chunkData->brickmaps + chunkID;

        }
        else if(chunkID != NULL_CHUNK){
            printf("CHUNK %d %d %d UNSAFE TO EDIT! %d\n",rayCurrentWorldChunk.x, rayCurrentWorldChunk.y, rayCurrentWorldChunk.z,chunkData->safeToEdit[chunkID]);
        }

        if (chunkExists) {
            ChunkStage chunkStage = (ChunkStage)AtomicRead(chunkData->volChunkStages + chunkID);
            if(chunkStage == chunkStage_secondPassComplete || chunkStage_meshed || chunkStage_uploaded)hasVoxels = true;
            // hasVoxels = !!(chunkData->flags[chunkID] & has_voxels);
            if(hasVoxels){
              
                //rayOrigin is relative to the chunk
                chunkHitPosition = rayOrigin + (rayDir * distanceToHit) - (relativeChunkPosition);
                vec3 debugRelChunkPos = fpt_to_flt_vec3(relativeChunkPosition);
                vec3 debugRayDir = fpt_to_flt_vec3(rayDir);
                vec3 debugRayDirDist = fpt_to_flt_vec3(rayDir * distanceToHit);
                float debugDistanceToHit = fpt2fl(distanceToHit);
                printf("relative chunk position: %3.1f %3.1f %3.1f\n", debugRelChunkPos.x,debugRelChunkPos.y,debugRelChunkPos.z);
                printf("rayDir %f %f %f, distanceToHit %f, total %f %f %f\n", debugRayDir.x,debugRayDir.y,debugRayDir.z,
                debugDistanceToHit, debugRayDirDist.x, debugRayDirDist.y, debugRayDirDist.z);

                bool containsVoxels = false;

                if(bm->active_count > 0){
                    maxVoxelSteps = 200;
                    //TODO: adjust step size dynamically to ensure ray hits every single voxel
                    printf("brickmap has voxels");

                    rayCurrentVoxelCell = fpt_calculateVoxelCoordinates(chunkHitPosition);
                    printf("rayCurrentVoxelCell: %d %d %d\n", rayCurrentVoxelCell.x, rayCurrentVoxelCell.y, rayCurrentVoxelCell.z);
                

                    fpt_vec3 local_chunk_min = currentChunkPos - FPT_HALF_CHUNK_SIZE;
                    
                    //local to chunk
                    fpt_vec3 voxelCellMinCorner = fpt_getVoxelWorldPosition(rayCurrentVoxelCell, local_chunk_min) - (FPT_HALF_VOXEL_SCALE);
                    printf("voxelCellMinCorner: %3.1f %3.1f %3.1f\n", fpt2fl(voxelCellMinCorner.x), fpt2fl(voxelCellMinCorner.y), fpt2fl(voxelCellMinCorner.z));

                    // The world space position of the next cell boundary
                    fpt_vec3 nextVoxelBoundary = voxelCellMinCorner + (fpt_vec3_create(i2fpt(step.x), i2fpt(step.y), i2fpt(step.z)) * voxelSize * stepGreaterThanZero);

                    // Calculate the distance from ray origin to the next boundary along each axis
                    fpt_vec3 toNextVoxelBoundary = nextVoxelBoundary - chunkHitPosition;
                    vec3 debugNextVoxelBoundary = fpt_to_flt_vec3(toNextVoxelBoundary);
                    printf("toNextVoxelBoundary: %f %f %f\n", debugNextVoxelBoundary.x,debugNextVoxelBoundary.y,debugNextVoxelBoundary.z);

                    
                    voxelTMax = toNextVoxelBoundary / rayDir;
                    // vec3 debugVoxelTMax = fpt_to_flt_vec3(voxelTMax);
                    // printf("voxelTMax: %f %f %f\n", debugVoxelTMax.x,debugVoxelTMax.y,debugVoxelTMax.z);
                    
                    voxelStep = 0;
                    //////////////////////////////////////////////////////////////// VOXEL GRID LOOP ////////////////////////////////////////////////////////////////
                    while (!hit) {
                        // Update currentGridIndex
                        printf("voxel grid loop, step %d\n", voxelStep);

                        currentVoxelIndex = get_index_from_coords(rayCurrentVoxelCell, uvec3_create(TOTAL_VOXELS));
                        printf("rayCurrentVoxelCell: %d %d %d, index %d\n", rayCurrentVoxelCell.x, rayCurrentVoxelCell.y, rayCurrentVoxelCell.z, currentVoxelIndex);

                        if(currentVoxelIndex < 0 || currentVoxelIndex >= (TOTAL_VOXELS * TOTAL_VOXELS * TOTAL_VOXELS)){
                            voxelDistanceToHit = 0;
                            break;
                        }
                        //for finding if the specific voxel the ray intersects is set
                        bool notInPadding = ((rayCurrentVoxelCell.x > 0 &&  rayCurrentVoxelCell.x < 63) && (rayCurrentVoxelCell.y > 0 &&  rayCurrentVoxelCell.y < 63) && (rayCurrentVoxelCell.z > 0 &&  rayCurrentVoxelCell.z < 63));
                        
                        fpt_vec3 path_vox_pos = fpt_getVoxelWorldPosition(rayCurrentVoxelCell, local_chunk_min);
                        
                        // [13:34:34] Chitali: norre: welcome back, here's a banger
                        // [13:34:35] norre: fusedforms: this episode sucks ass
                        // [13:34:38] fusedforms: :dance7:
                        // [13:34:39] wideposter: KINO
                        // [13:34:40] AgitatedSkellington: norre: you suck ass
                        // [13:34:42] norre: Chitali: ty ty 
                        // [13:34:47] glad to be back
                        // [13:34:48] Tendertaps2022: taht laugh
                        // [13:34:50] AgitatedSkellington: :trollestia;
                        // [13:34:55] norre: AgitatedSkellington: shut it filly fiddler 
                        // [13:34:55] 037: source hit sound
                        // [13:34:56] Tendertaps2022: that
                        // [13:35:01] AgitatedSkellington: norre: shut it you homosexual

                        uint32_t voxelPathIndex = chunkData->voxelPathCount++;
                        chunkData->voxelPath[voxelPathIndex].fpt_voxel_position = path_vox_pos;
                        chunkData->voxelPath[voxelPathIndex].voxel_coords = rayCurrentVoxelCell;
                        chunkData->voxelPath[voxelPathIndex].chunk_coords = rayCurrentWorldChunk;
                        chunkData->voxelPath[voxelPathIndex].voxel_index = currentVoxelIndex;
                        chunkData->voxelPath[voxelPathIndex].step = voxelStep;
                        chunkData->voxelPath[voxelPathIndex].hit = false;
                        if(chunkData->voxelPathCount >= 1024){
                            chunkData->voxelPathCount = 0; //reset count if its too large
                        }
                        vec3 floatVoxPos = fpt_to_flt_vec3(path_vox_pos);
                        printf("vox path position : %3.1f %3.1f %3.1f\n", floatVoxPos.x, floatVoxPos.y, floatVoxPos.z);
                        
                        if(notInPadding && bm->voxels[currentVoxelIndex]){
                            printf("VOXEL HIT");

                            voxel_index = currentVoxelIndex;
                            voxelCenter = fpt_getVoxelWorldPosition(rayCurrentVoxelCell, local_chunk_min) ;
                            hit = true;
                            voxel_type = bm->voxels[currentVoxelIndex];
                            chunkData->voxelRayCastResult.voxel_coords = rayCurrentVoxelCell;
                            printf("hit voxel coords: %d %d %d\n", rayCurrentVoxelCell.x,rayCurrentVoxelCell.y,rayCurrentVoxelCell.z);
                            chunkData->voxelRayCastResult.voxel_index = currentVoxelIndex;
                            chunkData->voxelRayCastResult.selected_voxel_render_pos = fpt_to_flt_vec3(voxelCenter);
                            chunkData->voxelPath[voxelPathIndex].hit = true;
                            selectedVoxelColor = bm->voxels[currentVoxelIndex];
                            // printf("hit, brickmap_index: %i, voxel_index: %i\n", brickmap_index, voxel_index);
                            break;
                        }
                        if(hit){break;}
                        if(--maxVoxelSteps <= 0 || distanceToHit + voxelDistanceToHit > farPlane){
                            // printf("breaking out of voxel ray loop\n");
                            voxelDistanceToHit = 0;
                            break;
                        }
                        // Move to the next voxel along the ray
                        if (voxelTMax.x < voxelTMax.y && voxelTMax.x < voxelTMax.z) {
                            // The ray will hit the X plane next
                            rayCurrentVoxelCell.x += step.x;
                            voxelDistanceToHit = voxelTMax.x;
                            voxelTMax.x = fpt_add(voxelTMax.x ,voxelTDelta.x);
                            printf("stepping X\n");

                        } else if (voxelTMax.y < voxelTMax.z) {
                            // The ray will hit the Y plane next
                            rayCurrentVoxelCell.y += step.y;
                            voxelDistanceToHit = voxelTMax.y;
                            voxelTMax.y = fpt_add(voxelTMax.y ,voxelTDelta.y);
                            printf("stepping Y\n");

                        } else {
                            // The ray will hit the Z plane next
                            rayCurrentVoxelCell.z += step.z;
                            voxelDistanceToHit = voxelTMax.z;
                            voxelTMax.z = fpt_add(voxelTMax.z ,voxelTDelta.z);
                            printf("stepping Z\n");

                        }

                        // Check if the current cell is within the bounds of your grid
                        if (rayCurrentVoxelCell.x < 1 || rayCurrentVoxelCell.x >= maxVal ||
                            rayCurrentVoxelCell.y < 1 || rayCurrentVoxelCell.y >= maxVal ||
                            rayCurrentVoxelCell.z < 1 || rayCurrentVoxelCell.z >= maxVal) {
                            voxelDistanceToHit = 0;
                            break; // Exit the loop if the ray is out of bounds
                        }
                        voxelStep++;

                    }
                    //*/

                }

            }
        }

        if(hit){break;}

        if(--maxChunkSteps <= 0 || distanceToHit > farPlane){
            distanceToHit = 0;
            break;
        }
        // Move to the next chunk along the ray
        if (tMax.x < tMax.y && tMax.x < tMax.z) {
            // The ray will hit the X plane next
            rayCurrentChunk.x += step.x;
            distanceToHit = tMax.x;
            float debugMax = fpt2fl(tMax.x);
            float debugDelta = fpt2fl(tDelta.x);
            printf("chunk stepping x, distance to hit = %f, tDelta.x: %f\n", debugMax, debugDelta);
            // if(fabs(debugDelta) >= 1000)__debugbreak();
            tMax.x = fpt_add(tMax.x, tDelta.x);
        } else if (tMax.y < tMax.z) {
            // The ray will hit the Y plane next
            rayCurrentChunk.y += step.y;
            distanceToHit = tMax.y;
            float debugMax = tMax.y;
            float debugDelta = fpt2fl(tDelta.y);
            printf("chunk stepping y, distance to hit = %f, tDelta.y: %f\n", debugMax, debugDelta);
            // if(fabs(debugDelta) >= 1000)__debugbreak();
            tMax.y = fpt_add(tMax.y, tDelta.y);
        } else {
            // The ray will hit the Z plane next
            rayCurrentChunk.z += step.z;
            distanceToHit = tMax.z;
            float debugMax = fpt2fl(tMax.z);
            float debugDelta = fpt2fl(tDelta.z);
            printf("chunk stepping z, distance to hit = %f, tDelta.z: %f\n", debugMax, debugDelta);
            // if(fabs(debugDelta) >= 1000)__debugbreak();
            tMax.z = fpt_add(tMax.z, tDelta.z);
        }

        // Check if the current cell is within the bounds of your grid
        if (rayCurrentChunk.x < 0 || rayCurrentChunk.x >= chunk_grid_resolution ||
            rayCurrentChunk.y < 0 || rayCurrentChunk.y >= chunk_grid_resolution ||
            rayCurrentChunk.z < 0 || rayCurrentChunk.z >= chunk_grid_resolution) {
            distanceToHit = 0;
            break; // Exit the loop if the ray is out of bounds
        }

        // Update currentGridIndex
        currentGridIndex = get_index_from_coords(rayCurrentChunk, uvec3_create(chunk_grid_resolution));
        if(currentGridIndex < 0 || currentGridIndex >= 1331){
            distanceToHit = 0;
            printf("breaking from chunk loop, ray went out of bounds\n");
            break;
        }
        chunkSteps++;

        
        // Further checks and operations
        //check if the current grid index is in the mega chunk bitmask
        //if it is, the chunk has voxels, so we need to check the brickmaps
    
    }

    if(hit){
        fpt_vec3 finalHitPosition = rayOrigin + (rayDir * (distanceToHit + voxelDistanceToHit));
        result.hitPosition = fpt_to_flt_vec3(finalHitPosition);
        result.fptHitPosition = finalHitPosition;

        vec3 floatFinalHitPosition = fpt_to_flt_vec3(finalHitPosition);
        printf("final hit position: %f %f %f\n", floatFinalHitPosition.x,floatFinalHitPosition.y,floatFinalHitPosition.z);
        printf("hit chunk coords  : %d %d %d\n", rayCurrentWorldChunk.x,rayCurrentWorldChunk.y,rayCurrentWorldChunk.z);


        // fpt_vec3 pos_in_chunk = fpt_vec3_create(0);
        fpt_vec3 pos_in_chunk = result.fptHitPosition;
        ivec3 hitChunkCoords = rayCurrentWorldChunk;
        if(voxelStep == 0){//hit the chunk right at the edge, add a tiny offset to the hit position so we rebase it correctly
        //otherwise if we cast from a null chunk, and intersect the edge of a neighboring chunk, we wont mod the value and the brush will appear on the other side
                 if(pos_in_chunk.x ==  FPT_HALF_CHUNK_SIZE)pos_in_chunk.x += FPT_HALF_VOXEL_SCALE;
            else if(pos_in_chunk.x == -FPT_HALF_CHUNK_SIZE)pos_in_chunk.x -= FPT_HALF_VOXEL_SCALE;
                 if(pos_in_chunk.y ==  FPT_HALF_CHUNK_SIZE)pos_in_chunk.y += FPT_HALF_VOXEL_SCALE;
            else if(pos_in_chunk.y == -FPT_HALF_CHUNK_SIZE)pos_in_chunk.y -= FPT_HALF_VOXEL_SCALE;
                 if(pos_in_chunk.z ==  FPT_HALF_CHUNK_SIZE)pos_in_chunk.z += FPT_HALF_VOXEL_SCALE;
            else if(pos_in_chunk.z == -FPT_HALF_CHUNK_SIZE)pos_in_chunk.z -= FPT_HALF_VOXEL_SCALE;
        }

        bool in_new_chunk = false;

        rebasePosition(pos_in_chunk, rayCurrentWorldChunk, &in_new_chunk, chunkData->toroidal_space_enabled);

        vec3 after_rebase = fpt_to_flt_vec3(pos_in_chunk);
        printf("after rebase: %f %f %f, in new chunk %d\n", after_rebase.x,after_rebase.y,after_rebase.z, in_new_chunk);

        fpt_vec3 relativeHitPosition = finalHitPosition - relativeChunkPosition;
        //determine normal/adjacent voxel
        fpt_vec3 diff = relativeHitPosition - voxelCenter;
        
        fpt_vec3 normalizedDiff = fpt_vec3_normalize(diff);
        fpt maxComponent = fpt_vec3_max_component(normalizedDiff);
        ivec3 hitNormal = {};

        if (maxComponent == fpt_abs(normalizedDiff.x)) {
            hitNormal.x = fpt2i(fpt_sign(normalizedDiff.x));
        } else if (maxComponent == fpt_abs(normalizedDiff.y)) {
            hitNormal.y = fpt2i(fpt_sign(normalizedDiff.y));
        } else {
            hitNormal.z = fpt2i(fpt_sign(normalizedDiff.z));
        }

        ivec3 adjacent_voxel_coords = rayCurrentVoxelCell +  hitNormal;
        
        // printf("set adjacent chunk coords here\n");
        // adjacent_chunk_coords = selectedChunk->chunk_coords;
        adjacent_chunk_coords = rayCurrentWorldChunk;
        adjacent_voxel_index = get_index_from_coords(adjacent_voxel_coords, uvec3_create(TOTAL_VOXELS));
        adjacent_chunk_index = chunkID;
        

        //adjacent variables are for when we want to place a voxel, we determine the normal of the currently selected voxel and place a voxel in that direction
        // Check if the adjacent voxel is outside the current brickmap, and adjust the brickmap and chunk index if necessary
        if     (adjacent_voxel_coords.x < 1 ) adjacent_voxel_coords.x = 62;
        else if(adjacent_voxel_coords.x > 62) adjacent_voxel_coords.x = 1;

        if     (adjacent_voxel_coords.y < 1 ) adjacent_voxel_coords.y = 62;
        else if(adjacent_voxel_coords.y > 62) adjacent_voxel_coords.y = 1;
        
        if     (adjacent_voxel_coords.z < 1 ) adjacent_voxel_coords.z = 62;
        else if(adjacent_voxel_coords.z > 62) adjacent_voxel_coords.z = 1;

        if (adjacent_voxel_coords != (rayCurrentVoxelCell + hitNormal)) {
            adjacent_voxel_index = get_index_from_coords(adjacent_voxel_coords, uvec3_create(TOTAL_VOXELS));
            
                // Calculate the chunk coords that the adjacent voxel would fall into
                adjacent_chunk_coords = rayCurrentWorldChunk + hitNormal;
                //calculate the actual brush position
                //brush position should actually be in the adjacent chunk
                hitChunkCoords = adjacent_chunk_coords;
                bool chunkExists = false;
                uint32_t adjacentChunkID = getchunkID(chunkData, adjacent_chunk_coords);
                if(adjacentChunkID != chunkID && adjacentChunkID !=NULL_CHUNK){
                    //adjacent chunk exists
                    if(!chunkData->safeToEdit[adjacentChunkID]){
                        adjacentChunkID = NULL_CHUNK;
                    }
                    adjacent_chunk_index = adjacentChunkID;
                }

        }

        result.adjacent_voxel_coords = adjacent_voxel_coords;
        result.voxel_coords = rayCurrentVoxelCell;
        // if(result.voxel_coords.x == 62 && result.voxel_coords.y == 62 && result.voxel_coords.z == 62)__debugbreak();
        result.chunk_coords = rayRelativeChunk + cameraCoords;
        result.adjacent_chunk_coords = adjacent_chunk_coords;
        result.voxel_type = voxel_type;
        result.adjacent_chunk_index = adjacent_chunk_index;
        result.adjacent_voxel_index = adjacent_voxel_index;
        result.selected = true;
        result.fpt_voxel_position = voxelCenter;
        result.chunkID = chunkID;
        result.pos_in_chunk_hit = pos_in_chunk;
        result.chunk_coords_hit = hitChunkCoords;

        return true;
    }
    result.chunk_coords             = {};
    result.adjacent_chunk_coords    = {};
    result.adjacent_voxel_coords    = {};
    result.voxel_coords             = {};
    result.chunk_coords_hit         = {};
    result.voxel_type = 0;
    result.chunkID = NULL_CHUNK;
    result.selected = false;
    result.voxel_index = 0;
    result.chunk_coords = selected_chunk_coords;
    result.voxel_color = 0;

    return false;
}

