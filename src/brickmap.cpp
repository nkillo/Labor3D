#include "brickmap.h"
// #include "labour/voxel/brickmap.h"


    Brickmap* create_brickmap(uint32_t res_x, uint32_t res_y, uint32_t res_z) {
        // Check if each dimension is a power of 2
        // assert(res_x > 0 && (res_x & (res_x - 1)) == 0 &&
        //     res_y > 0 && (res_y & (res_y - 1)) == 0 &&
        //     res_z > 0 && (res_z & (res_z - 1)) == 0 &&
        //     "create_brickmap(): Each resolution must be a positive power of 2.");

        Brickmap* bm = new Brickmap();
        bm->resolution = uvec3_create(res_x, res_y, res_z);
        bm->total_voxel_count = res_x * res_y * res_z;
        bm->colors = new uint8_t[bm->total_voxel_count];
        memset(bm->colors, 0, sizeof(uint8_t) * bm->total_voxel_count);
        // for(int i = 0; i < bm->total_voxel_count; i++){
        //     printf("index %d color: %d\n", i, bm->colors[i]);
        //     if(bm->colors[i] == 52685){
        //         printf("total voxel count: %d\n", bm->total_voxel_count);
        //     }
        // }
        assert(bm && "brickmap is nullptr in create_brickmap()?");
        return bm;
    }

    void clear_brickmap(Brickmap* bm){
        if(!bm){
            printf("clear_brickmap(), brickmap is null, returning\n");
            return;
        }

        memset(bm->colors, 0, sizeof(uint8_t) * bm->total_voxel_count);
        bm->active_count = 0;
    }

    void destroy_brickmap(Brickmap* bm) {
        if(!bm){
            printf("destroy_brickmap(), brickmap is null, returning\n");
            return;
        }

        ::operator delete[](bm->colors, sizeof(uint8_t) * bm->total_voxel_count);
        // delete[] bm->colors;
        delete bm;
    }

    uint32_t get_index_from_coords(const ivec3& coords, const uvec3& resolution) {
        return coords.x + (coords.y * resolution.x) + (coords.z * resolution.x * resolution.y);
    }

    ivec3 get_coords_from_index(uint32_t index, const uvec3& resolution) {
        // printf("resolution: %i %i %i\n", resolution.x, resolution.y, resolution.z);
        int x = index % resolution.x;
        int y = (index / resolution.x) % resolution.y;
        int z = index / (resolution.x * resolution.y);
        return ivec3_create(x, y, z);
    }

    //for coords
    bool set_voxel(Brickmap* bm, const ivec3& coords, uint8_t color, bool padded_voxel) {

        if(!bm){
            printf("set_voxel(), brickmap is null, returning\n");
            return false;
        }

        if (!is_within_bounds(bm, coords)) return false;
        uint32_t index = get_index_from_coords(coords, bm->resolution);
        
        if (bm->colors[index] == 0 && color != 0) {
            // bm->active_voxels.insert(index);
            if(!padded_voxel)bm->active_count++;
        } else if (bm->colors[index] != 0 && color == 0) {
            // bm->active_voxels.erase(index);
            if(!padded_voxel)bm->active_count--;
        }
        else if(bm->colors[index] == color) {
            printf("set_voxel() called with same color, returning false\n");
            return false;
        }
        
        bm->colors[index] = color;
        return true;
    }
    //for index
    bool set_voxel(Brickmap* bm, uint32_t index, uint8_t color, bool padded_voxel) {

        if(!bm){
            printf("set_voxel(), brickmap is null, returning\n");
            return false;
        }
        
        if (bm->colors[index] == 0 && color != 0) {
            // bm->active_voxels.insert(index);
            if(!padded_voxel)bm->active_count++;
        } else if (bm->colors[index] != 0 && color == 0) {
            // bm->active_voxels.erase(index);
            if(!padded_voxel)bm->active_count--;
        }
        else if(bm->colors[index] == color) {
            printf("set_voxel() called with same color, returning false\n");
            return false;
        }
        
        bm->colors[index] = color;
        return true;
    }


    void clear_voxel(Brickmap* bm, const ivec3& coords) {
        if(!bm){
            printf("clear_voxel(), brickmap is null, returning\n");
            return;
        }
        
        uint32_t index = get_index_from_coords(coords, bm->resolution);
        // bm->active_voxels.erase(index);
        bm->colors[index] = 0;
    }

    uint8_t get_voxel_color(const Brickmap* bm, const ivec3& coords) {
        if(!bm){
            printf("get_voxel_color(), brickmap is null, returning\n");
            return 0;
        }

        uint32_t index = get_index_from_coords(coords, bm->resolution);
        return bm->colors[index];
    }

    bool is_within_bounds(const Brickmap* bm, const ivec3& coords) {
        if(!bm){
            // printf("is_within_bounds(), brickmap is null, returning\n");
            return false;
        }

        return coords.x >= 0 && coords.x < static_cast<int>(bm->resolution.x) &&
            coords.y >= 0 && coords.y < static_cast<int>(bm->resolution.y) &&
            coords.z >= 0 && coords.z < static_cast<int>(bm->resolution.z);
    }


    vec3 get_voxel_local_position(int bmResolution, int voxel_index) {
        ivec3 position = get_coords_from_index(voxel_index, uvec3_create(bmResolution));
        // printf("Voxel index: %d, position: (%d, %d, %d)\n", voxel_index, position.x, position.y, position.z);
        vec3 voxel_local_position_vec3 = vec3_create(position);
        // Scale the grid index by the size of the voxel to get local voxel position
        float voxel_size = chunk_size / voxel_resolution_per_chunk;
        vec3 voxel_local_position = voxel_local_position_vec3 * voxel_size;
        float voxel_center_offset = voxel_size / 2.0f;
        // Add the chunk's world position to get the voxel's world position
        return voxel_local_position + voxel_center_offset;
    }

    vec3 get_absolute_voxel_position(int bmResolution, int voxel_local_index, int brickmap_index, const vec3& chunk_position){
        
        vec3  voxel_local_position = get_voxel_local_position(bmResolution, voxel_local_index);
        

        vec3 absolute_brickmap_position = (get_absolute_brickmap_position(brickmap_index, chunk_position) - brickmap_center_offset);
        // printf("voxel local position: %f, %f, %f\n", voxel_local_position.x, voxel_local_position.y, voxel_local_position.z);
        // printf("bm_index: %d, absolute_brickmap_position: %f, %f, %f\n", brickmap_index, absolute_brickmap_position.x, absolute_brickmap_position.y, absolute_brickmap_position.z);
 
        vec3 absolute_voxel_position = voxel_local_position + absolute_brickmap_position;
        //brickmap offset is chunk size / brickmap resolution
        // printf("voxel index: %d, brickmap_index: %d\n", voxel_local_index, brickmap_index);
        // printf("absolute voxel position:  %f, %f, %f\n", absolute_voxel_position.x, absolute_voxel_position.y, absolute_voxel_position.z);
        
        return absolute_voxel_position;
    }
    
    //retrieves brickmap position within chunk
    vec3 get_absolute_brickmap_position(int index, const vec3& chunk_position){
        assert(index < (brickgrid_resolution*brickgrid_resolution*brickgrid_resolution) && "given index is larger than brickgrid res cubed");
        ivec3 local_brickmap_position = get_coords_from_index(index, uvec3_create((uint32_t)brickgrid_resolution));
        // printf("local brickmap coords within chunk: (%d,%d,%d)\n", local_brickmap_position.x, local_brickmap_position.y, local_brickmap_position.z);

        vec3 local_brickmap_position_vec3 = vec3_create(local_brickmap_position);
        // printf("local brickmap position within chunk: (%f,%f,%f)\n", local_brickmap_position_vec3.x, local_brickmap_position_vec3.y, local_brickmap_position_vec3.z);


        // Calculate the world position of the brickmap's center
        // The local brickmap position is scaled by the size of each brickmap (which is the brickmap_offset)
        // and then offset by half of that size to get to the center of the brickmap.
        //brickmap offset is chunk size / brickmap resolution

        // printf("brickmap_offset: %f, brickmap_center_offset: %f\n", brickmap_offset, brickmap_center_offset);
        vec3 chunk_bottom_left_back_corner = get_chunk_bottom_left_back_corner(chunk_position);

        vec3 result = chunk_bottom_left_back_corner + (local_brickmap_position_vec3 * brickmap_offset) + brickmap_center_offset;
        // printf("absolute brickmap position for index: %d: (%f, %f, %f)\n", index, result.x, result.y, result.z);
        
        return result;

    }

   
    
    vec3 get_chunk_bottom_left_back_corner(const vec3& chunk_position)
    {
        //TODO: replace the magic number later with a constant
        //32 is the half size of a chunk, so chunk size is 64
        vec3 result = chunk_position - vec3_create(chunk_size/2);

        // printf("chunk_position: (%f,%f,%f), brickgrid_resolution: %f\n", chunk_position.x,chunk_position.y,chunk_position.z, brickgrid_resolution);

        // printf("get_chunk_bottom_left_back_corner(): result: %f, %f, %f\n", result.x, result.y, result.z);

        return result;
    }

void memcpy_voxel_noise(Brickmap* bm, const uint8_t* colors, uint32_t active_voxel_count) {
    if (!bm) {
        printf("set_voxels_batch(), brickmap is null, returning\n");
        return;
    }

    // Calculate the size of the data to copy
    size_t data_size = brickmap_resolution_cubed * sizeof(uint8_t);
    bm->active_count+= active_voxel_count;
    // Copy the colors data to bm->colors
    memcpy(bm->colors, colors, data_size);

}

void memcpy_padded_voxel_noise(Brickmap* bm, const uint8_t* colors, uint32_t active_voxel_count) {
    if (!bm) {
        printf("set_voxels_batch(), brickmap is null, returning\n");
        return;
    }
    uint32_t resolutionCubed = bm->resolution.x * bm->resolution.y * bm->resolution.z;
    // Calculate the size of the data to copy
    size_t data_size = resolutionCubed * sizeof(uint8_t);
    bm->active_count+= active_voxel_count;
    // Copy the colors data to bm->colors
    memcpy(bm->colors, colors, data_size);
}

size_t set_voxels_batch(Brickmap* bm, const uint32_t* indices, const uint8_t* colors, size_t count) {
    if (!bm) {
        printf("set_voxels_batch(), brickmap is null, returning\n");
        return 0;
    }

    size_t changed_voxels = 0;

    for (size_t i = 0; i < count; ++i) {
        uint32_t index = indices[i];
        uint8_t color = colors[i];

        if (bm->colors[index] == 0 && color != 0) {
            // bm->active_voxels.insert(index);
            bm->active_count++;
            changed_voxels++;
        } else if (bm->colors[index] != 0 && color == 0) {
            // bm->active_voxels.erase(index);
            bm->active_count--;
            changed_voxels++;
        } else if (bm->colors[index] != color) {
            changed_voxels++;
        }
        // if(color != 1){
        //     printf("non 1 color: %d\n", color);
        // }
        bm->colors[index] = color;
    }

    return changed_voxels;
}




  // Populate the lookup array

// Function to populate the lookup array (XYZ order)
void populate_padding_lookup(VoxelLookup& padding_lookup) {
    for (uint32_t i = 0; i < pbmr3; ++i) {
        uint32_t x = i % pbmr;
        uint32_t y = (i / pbmr) % pbmr;
        uint32_t z = i / (pbmr * pbmr);
        
        // Set to true if the voxel is in the padding
        padding_lookup[i] = (x < 1 || x >= (int)brickmap_resolution + 1 ||
                     y < 1 || y >= (int)brickmap_resolution + 1 ||
                     z < 1 || z >= (int)brickmap_resolution + 1);
    }
}

// Function to check if a voxel is in padding
bool is_in_padding(VoxelLookup& padding_lookup, uint32_t voxel_index) {
    if (voxel_index >= pbmr3) {
        // Handle out-of-bounds access
        assert((voxel_index < pbmr3) && " is_in_padding(uint32_t voxel_index) Voxel index is out of bounds");
        return true; // or throw an exception, depending on your error handling strategy
    }
    return padding_lookup[voxel_index];
}


// Function to populate the lookup array (XYZ order)
void populate_padding_lookup_half_res(VoxelLookupHalfRes& padding_lookup_half_res) {
    for (uint32_t i = 0; i < hpbmr3; ++i) {
        uint32_t x = i % hpbmr;
        uint32_t y = (i / hpbmr) % hpbmr;
        uint32_t z = i / (hpbmr * hpbmr);
        
        // Set to true if the voxel is in the padding
        padding_lookup_half_res[i] = (x < 1 || x >= (int)half_brickmap_resolution - 1 ||
                     y < 1 || y >= (int)half_brickmap_resolution - 1 ||
                     z < 1 || z >= (int)half_brickmap_resolution - 1);
    }
}

// Function to check if a voxel is in padding
bool is_in_padding_half_res(VoxelLookupHalfRes& padding_lookup_half_res, uint32_t voxel_index) {
    if (voxel_index >= hpbmr3) {
        // Handle out-of-bounds access
        assert((voxel_index < hpbmr3) && " is_in_padding(uint32_t voxel_index) Voxel index is out of bounds");
        return true; // or throw an exception, depending on your error handling strategy
    }
    return padding_lookup_half_res[voxel_index];
}











