#pragma once
#include "chunkManager.h"



    #ifdef _MSC_VER
    inline const int CTZ(uint64_t &x) {
    unsigned long index;
    _BitScanForward64(&index, x);
    return (int)index;
    }
    #else
    inline const int CTZ(uint64_t x) {
    return __builtin_ctzll(x);
    }
    #endif

    inline const int get_axis_i(const int &axis, const int &a, const int &b, const int &c) {
    //may need to swap all the axes around
    //original
    //0 is Y
    //1 is X
    //2 is Z
    //because its a YXZ format
    //WE use a ZYX format instead
    // printf("Calculating index for axis %d, with coords a:%d, b:%d, c:%d\n", axis, a, b, c);
    if (axis == 0){
        // int result = b + (a * CS_P) + (c * CS_P2);
        // printf("result is: %d\n", result);
        return       b + (a * CS_P) + (c * CS_P2);
    } 
    else if (axis == 1){
        // int result = a + (c * CS_P) + (b* CS_P2);
        // printf("result is: %d\n", result);
        return       a + (c * CS_P) + (b* CS_P2);
    } 
    else{
        // int result = c + (b * CS_P) + (a * CS_P2);
        // printf("result is: %d\n", result);
        return       c + (b * CS_P) + (a * CS_P2);
    } 
    return 0;
    }

    // Add checks to this function to skip culling against grass for example
    inline const bool solid_check(int voxel) {
        return voxel; //3FF for lower 10 bits of voxel, if any is set its solid
    }

    // inline constexpr ivec2 ao_dirs[8] = {
    //    ivec2(0, -1),
    //    ivec2(0, 1),
    //    ivec2(-1, 0),
    //    ivec2(1, 0),
    //    ivec2(-1, -1),
    //    ivec2(-1, 1),
    //    ivec2(1, -1),
    //    ivec2(1, 1),
    // };

    ivec2 get_ao_dir(int i) {
        switch(i) {
            case 0: return { 0, -1 };
            case 1: return { 0,  1 };
            case 2: return {-1,  0 };
            case 3: return { 1,  0 };
            case 4: return {-1, -1 };
            case 5: return {-1,  1 };
            case 6: return { 1, -1 };
            case 7: return { 1,  1 };
        }
        return{};
    }

    inline const int vertexAO(int side1, int side2, int corner) {
    if (side1 && side2) {
        return 0;
    }
    return 3 - (side1 + side2 + corner);
    }

    inline const bool compare_ao(uint8_t* voxels, int axis, int forward, int right, int c, int forward_offset, int right_offset) {
    // printf("compare_ao() axis: %d, forward: %d, right: %d, c: %d, forward_offset: %d, right_offset: %d\n", axis, forward, right, c, forward_offset, right_offset);
    for (int i = 0;  i < 8; i ++) {
        ivec2 ao_dir = get_ao_dir(i);
        if (solid_check(voxels[get_axis_i(axis, right + ao_dir.e[0], forward + ao_dir.e[1], c)]) !=
        solid_check(voxels[get_axis_i(axis, right + right_offset + ao_dir.e[0], forward + forward_offset + ao_dir.e[1], c)])
        ) {
        return false;
        }
    }
    return true;
    }

    inline const bool compare_forward(uint8_t* voxels, int axis, int forward, int right, int bit_pos, int air_dir, bool bake_ao) {
    // printf("compare_forward() axis: %d, forward: %d, right: %d, bit_pos: %d, air_dir: %d\n", axis, forward, right, bit_pos, air_dir);
    return
        voxels[get_axis_i(axis, right, forward, bit_pos)] == voxels[get_axis_i(axis, right, forward + 1, bit_pos)] &&
        (!bake_ao || compare_ao(voxels, axis, forward, right, bit_pos + air_dir, 1, 0))
    ;
    }

    inline const bool compare_right(uint8_t* voxels, int axis, int forward, int right, int bit_pos, int air_dir, bool bake_ao) {
    // printf("compare_right() axis: %d, forward: %d, right: %d, bit_pos: %d, air_dir: %d\n", axis, forward, right, bit_pos, air_dir);
    return
        voxels[get_axis_i(axis, right, forward, bit_pos)] == voxels[get_axis_i(axis, right + 1, forward, bit_pos)] &&
        (!bake_ao || compare_ao(voxels, axis, forward, right, bit_pos + air_dir, 0, 1))
    ;
    }

    constexpr uint64_t CULL_MASK = (1ULL << (CS_P - 1));
    constexpr uint64_t BORDER_MASK = (1ULL | (1ULL <<  (CS_P - 1)));


    inline uint8_t pack_ao_values(uint32_t v0ao, uint32_t v1ao, uint32_t v2ao, uint32_t v3ao) {
        return (v3ao << 6) | (v2ao << 4) | (v1ao << 2) | v0ao;
    }

    inline FaceData get_face(uint32_t x, uint32_t y, uint32_t z, uint32_t width, uint32_t height, uint32_t flipped,
                            uint8_t packedAO, uint8_t type, uint8_t type2 , uint32_t face, uint8_t light = 15) {
        uint32_t faceInfo1 = (x & 0x3F) | ((y & 0x3F) << 6) | ((z & 0x3F) << 12) |
                            ((width & 0x3F) << 18) | ((height & 0x3F) << 24) | ((flipped & 0x1) << 30);
        
        uint32_t faceInfo2 = packedAO | ((type & 0xFF) << 8) | ((type2 & 0xFF) << 16) |  ((light & 0xF) << 24) | ((face & 0x07) << 29);
        
        return {faceInfo1, faceInfo2};
    }

    inline const void insert_face(FaceData* faces, uint32_t x, uint32_t y, uint32_t z, uint32_t width, uint32_t height, bool flipped, int& faceI, uint32_t maxFaces,
                                                                    uint8_t packedAO, uint8_t type, uint8_t type2, uint32_t face, uint8_t light = 15) 
    {
    //   if (faceI >= maxFaces) {
    //     FaceData dummyfacedata = {};
    //     faces.resize(maxFaces * 2, dummyfacedata);
    //     maxFaces *= 2;
    //   }
    if(faceI >= maxFaces){
    printf("FACE COUNT TOO HIGH!: %d GREATER THAN MAX FACES: %d\n", faceI, maxFaces);
    printf("ITS SO OVER\n");
    }
    assert(faceI < maxFaces);

    faces[faceI] = get_face( x,  y,  z,  width,  height,  flipped,
                            packedAO,  type, type2, face, light);
    faceI++;
    }




    void vertex_pull_mesh_variable(uint8_t* voxels, voxel_work* worker, bool bake_ao,
                                    FaceData* faces, uint32_t& faceCount, uint32_t maxFaces);
    static inline const uint32_t get_vertex(uint32_t x, uint32_t y, uint32_t z, uint32_t type, uint32_t norm, uint32_t ao) {
    // printf("x: %d, y: %d, z: %d, type: %d, norm: %d, ao: %d\n", x-1, y-1, z-1, type, norm, ao);
    return (ao << 29) | (norm << 26) | (type << 18) | ((z - 1) << 12) | ((y - 1) << 6) | (x - 1);
    }



    static inline const void insert_quad(uint32_t* vertices, uint32_t v1, uint32_t v2, uint32_t v3, uint32_t v4, bool flipped, int& vertexI, int& maxVertices) {
    //   if (vertexI >= maxVertices - 6) {
    //     vertices.resize(maxVertices * 2, 0);
    //     maxVertices *= 2;
    //   }

    if (flipped) {
        // printf("insert_quad() FLIPPED\n");
        vertices[vertexI] = v1;
        vertices[vertexI + 1] = v2;
        vertices[vertexI + 2] = v3;
        vertices[vertexI + 3] = v3;
        vertices[vertexI + 4] = v4;
        vertices[vertexI + 5] = v1;
    }
    else {
        vertices[vertexI] = v1;
        vertices[vertexI + 1] = v2;
        vertices[vertexI + 2] = v4;
        vertices[vertexI + 3] = v4;
        vertices[vertexI + 4] = v2;
        vertices[vertexI + 5] = v3;
    }

    vertexI += 6;
    }






    inline const int get_axis_ivar(const int &axis, const int &a, const int &b, const int &c, uint64_t var_CS_P, uint64_t var_CS_P2, int xRes = 1, int yRes = 1, int zRes = 1) {
    //may need to swap all the axes around
    //original
    //0 is Y
    //1 is X
    //2 is Z
    //because its a YXZ format
    //WE use a ZYX format instead
    // printf("Calculating index for axis %d, with coords a:%d, b:%d, c:%d\n", axis, a, b, c);
        if (axis == 0){
        //for some reason, switching var_CS_P with xRes (which is what it already is)
        //and var_CS_P2 with yRes*xRes (which it already is) gives different AO results?
        // int result = b + (a * var_CS_P) + (c * var_CS_P2);
        // printf("get_axis_ivar() for axis %d:  %d + (%d * %d) + (%d * %d) = %d\n", axis, b , a , var_CS_P, c , var_CS_P2, result);
        // printf("%d + (%d * %llu) + (%d * %llu)\n", b, a, var_CS_P, c, var_CS_P2);
        return       b + (a * xRes) + (c * yRes*xRes);
        } 
        else if (axis == 1){
        //y, var_CS_P is xRes, var_CS_P2 is zRes*xRes. change to xRes * yRes for var_CS_P2 here
        // int result = a + (c * var_CS_P) + (b* var_CS_P2);
        // int result = a + (c * xRes) + (b* xRes * yRes);
        // printf("get_axis_ivar() for axis %d:  %d + (%d * %d) + (%d * %d * %d) = %d\n",axis, a , c , xRes, b ,xRes, yRes, result);
        // return       a + (c * var_CS_P) + (b* var_CS_P2);
        return       a + (c * xRes) + (b* xRes * yRes);
        } 
        else{//x axis
        {     //forward is Y+, right is Z+
            // var_CS_P2 = yRes * zRes;
            // var_CS_P = zRes;
        // int result = c + (b * xRes) + (a * xRes * yRes);
        // printf("get_axis_ivar() for axis %d:  %d + (%d * %d) + (%d * %d * %d) = %d\n",axis, c , b , xRes, a , xRes, yRes, result);
        return       c + (b * xRes) + (a * xRes * yRes);
        } 
    }
    }

    inline const bool compare_aovar(uint8_t* voxels, int axis, int forward, int right, int c, int forward_offset, int right_offset, uint64_t var_CS_P, uint64_t var_CS_P2, int xRes = 1, int yRes = 1, int zRes = 1) {
    // printf("compare_aovar() axis: %d, forward: %d, right: %d, c: %d, forward_offset: %d, right_offset: %d\n", axis, forward, right, c, forward_offset, right_offset);
    for (int i = 0;  i < 8; i ++) {
        ivec2 ao_dir = get_ao_dir(i);
        if (solid_check(voxels[get_axis_ivar(axis, right + ao_dir.e[0], forward + ao_dir.e[1], c, var_CS_P, var_CS_P2, xRes, yRes, zRes)]) !=
        solid_check(voxels[get_axis_ivar(axis, right + right_offset + ao_dir.e[0], forward + forward_offset + ao_dir.e[1], c, var_CS_P, var_CS_P2, xRes, yRes, zRes)])
        ) {
        return false;
        }
    }
    return true;
    }

    inline const bool compare_forwardvar(uint8_t* voxels, int axis, int forward, int right, int bit_pos, int air_dir, bool bake_ao, uint64_t var_CS_P, uint64_t var_CS_P2, int xRes = 1, int yRes = 1, int zRes = 1) {
    // printf("compare_forward() axis: %d, forward: %d, right: %d, bit_pos: %d, air_dir: %d\n", axis, forward, right, bit_pos, air_dir);
    return
        voxels[get_axis_ivar(axis, right, forward, bit_pos, var_CS_P, var_CS_P2, xRes, yRes, zRes)] == voxels[get_axis_ivar(axis, right, forward + 1, bit_pos, var_CS_P, var_CS_P2, xRes, yRes, zRes)] &&
        (!bake_ao || compare_aovar(voxels, axis, forward, right, bit_pos + air_dir, 1, 0, var_CS_P, var_CS_P2, xRes, yRes, zRes))
    ;
    }

    inline const bool compare_rightvar(uint8_t* voxels, int axis, int forward, int right, int bit_pos, int air_dir, bool bake_ao, uint64_t var_CS_P, uint64_t var_CS_P2, int xRes = 1, int yRes = 1, int zRes = 1) {
    // printf("compare_right() axis: %d, forward: %d, right: %d, bit_pos: %d, air_dir: %d\n", axis, forward, right, bit_pos, air_dir);
    return
        voxels[get_axis_ivar(axis, right, forward, bit_pos, var_CS_P, var_CS_P2, xRes, yRes, zRes)] == voxels[get_axis_ivar(axis, right + 1, forward, bit_pos, var_CS_P, var_CS_P2, xRes, yRes, zRes)] &&
        (!bake_ao || compare_aovar(voxels, axis, forward, right, bit_pos + air_dir, 0, 1, var_CS_P, var_CS_P2, xRes, yRes, zRes));
    }
    void vertex_pull_mesh_variable(uint8_t* voxels, voxel_work* worker, bool bake_ao, uvec3 resolution,
                                    FaceData* faces, uint32_t& faceCount, uint32_t maxFaces);

    // voxels - 64^3 (includes neighboring voxels)
    // vertices - pre-allocated array of vertices that will be poplulated. Can be re-used between runs and does not need to be clared.
    // vertexLength - output  number of vertices to read from vertices
    // void mesh(uint8_t* voxels, chunk_data* chunkData, bool bake_ao);
    void init_vertex_pull_buffers(chunk_data* chunkData);




// //////////////////////////////////////////////////// DUMMY VERTEX PULLER START ////////////////////////////////////////////////////

// void pop_triangle_buffers(chunk_data* chunkData);

// void update_tri_buffers(chunk_data *chunkData);

struct vertexAOstruct{
    vec3 pos;
    float ao;
};

constexpr int flipLookup[6] = {1, -1, -1, 1, 1, -1};

vertexAOstruct print_vertexao_pulled_vertices_correct_faces(FaceData facedata, int vertexID);

inline uint32_t getColor(vec3 color){
    return 0xFF000000 | 
        (uint32_t(color.z * 255) << 16) | 
        (uint32_t(color.y * 255) << 8) | 
         uint32_t(color.x * 255);

}


// void convert_binary_mesh_faces_to_test_quads(chunk_data* chunkData);
// //////////////////////////////////////////////////// DUMMY VERTEX PULLER END ////////////////////////////////////////////////////







// void create_packed_quad_test_buffers(chunk_data* chunkData);
// void update_packed_quad_test_buffers(chunk_data* chunkData);

