#include "binaryMesher.h"


    void vertex_pull_mesh_variable(uint8_t* voxels, voxel_work* worker, bool bake_ao,
                                    FaceData* faces, uint32_t& faceCount, uint32_t maxFaces) {
    
    
    // binary_meshColGenTimer.reset();
    // printf("Generate initial axis cols took: %f seconds\n", binary_meshColGenTimer.elapsed());
    // binary_meshColGenTimer.stop();

    //   binary_meshTimer.reset();

    int faceI = 0;
        
        // Use pointers directly instead of dereferencing
        uint64_t* col_face_masks = worker->col_face_masks;//[brickmapIndex];
        uint64_t* a_axis_cols = worker->a_axis_cols;//[brickmapIndex];
        uint64_t* b_axis_cols = worker->b_axis_cols;//[brickmapIndex];
        uint64_t* merged_right = worker->merged_right;//[brickmapIndex];
        uint64_t* merged_forward = worker->merged_forward;//[brickmapIndex];

        memset(col_face_masks, 0, sizeof(uint64_t) * CS_P2 * 6);

    // Begin culling faces
    auto p = voxels;
    memset(a_axis_cols, 0, sizeof(uint64_t) * CS_P2);
    for (int a = 0; a < CS_P; a++) {
        memset(b_axis_cols, 0, sizeof(uint64_t) * CS_P);

        for (int b = 0; b < CS_P; b++) {
        uint64_t cb = 0;

        for (int c = 0; c < CS_P; c++) {
            if (solid_check(*p)) {
                // printf("solid voxel found\n");
            a_axis_cols[b + (c * CS_P)] |= 1ULL << a;
            b_axis_cols[c] |= 1ULL << b;
            cb |= 1ULL << c;
            }
            p++;
        }

        // Cull third axis faces
        col_face_masks[a + (b * CS_P) + (4 * CS_P2)] = cb & ~((cb >> 1) | CULL_MASK);
        col_face_masks[a + (b * CS_P) + (5 * CS_P2)] = cb & ~((cb << 1) | 1ULL);
        }

        // Cull second axis faces
        int faceIndex = (a * CS_P) + (2 * CS_P2);
        for (int b = 1; b < CS_P - 1; b++) {
        uint64_t col = b_axis_cols[b];
        col_face_masks[faceIndex + b] = col & ~((col >> 1) | CULL_MASK);
        col_face_masks[faceIndex + b + CS_P2] = col & ~((col << 1) | 1ULL);
        }
    }

    // Cull first axis faces
    for (int a = 1; a < CS_P - 1; a++) {
        int faceIndex = a * CS_P;
        for (int b = 1; b < CS_P - 1; b++) {
        uint64_t col = a_axis_cols[faceIndex + b];

        col_face_masks[faceIndex + b] = col & ~((col >> 1) | CULL_MASK);
        col_face_masks[faceIndex + b + CS_P2] = col & ~((col << 1) | 1ULL);
        }
    }
    
    
    // Greedy meshing
    for (uint8_t face = 0; face < 6; face++) {
        int axis = face / 2;
        int air_dir = face % 2 == 0 ? 1 : -1;

        memset(merged_forward, 0, sizeof(uint64_t) * CS_P2);

        for (int forward = 1; forward < CS_P - 1; forward++) {
        uint64_t bits_walking_right = 0;
        int forwardIndex = (forward * CS_P) + (face * CS_P2);

        memset(merged_right, 0, sizeof(uint64_t) * CS_P);

        for (int right = 1; right < CS_P - 1; right++) {
            int rightxCS_P = right * CS_P;

            uint64_t bits_here = col_face_masks[forwardIndex + right] &~ BORDER_MASK;
            uint64_t bits_right = right >= CS ? 0 : col_face_masks[forwardIndex + right + 1];
            uint64_t bits_forward = forward >= CS ? 0 : col_face_masks[forwardIndex + right + CS_P];

            uint64_t bits_merging_forward = bits_here & bits_forward & ~bits_walking_right;
            uint64_t bits_merging_right = bits_here & bits_right;

            unsigned long bit_pos;

            uint64_t copy_front = bits_merging_forward;
            while (copy_front) {
            #ifdef _MSC_VER
                _BitScanForward64(&bit_pos, copy_front);
            #else
                bit_pos = __builtin_ctzll(copy_front);
            #endif

            copy_front &= ~(1ULL << bit_pos);

            if(
                voxels[get_axis_i(axis, right, forward, bit_pos)] == voxels[get_axis_i(axis, right, forward + 1, bit_pos)] &&
                (!bake_ao || compare_ao(voxels, axis, forward, right, bit_pos + air_dir, 1, 0))
            ) {
                merged_forward[(right * CS_P) + bit_pos]++;
            }
            else {
                bits_merging_forward &= ~(1ULL << bit_pos);
            }
            }

            uint64_t bits_stopped_forward = bits_here & ~bits_merging_forward;
            while (bits_stopped_forward) {
            #ifdef _MSC_VER
                _BitScanForward64(&bit_pos, bits_stopped_forward);
            #else
                bit_pos = __builtin_ctzll(bits_stopped_forward);
            #endif

            bits_stopped_forward &= ~(1ULL << bit_pos);
            uint32_t voxelIndex = get_axis_i(axis, right, forward, bit_pos);
            uint8_t type = voxels[voxelIndex];
            
            //for debugging the mesh generation
            // type = forward * 4;

            uint8_t type2 = right * 4;
            // uint8_t light = (voxels[voxelIndex] >> 12) & 0xF;
            uint8_t light = 0;
            if (
                (bits_merging_right & (1ULL << bit_pos)) != 0 &&
                (merged_forward[(right * CS_P) + bit_pos] == merged_forward[(right + 1) * CS_P + bit_pos]) &&
                (type == voxels[get_axis_i(axis, right + 1, forward, bit_pos)]) &&
                (!bake_ao || compare_ao(voxels, axis, forward, right, bit_pos + air_dir, 0, 1))
                ) {
                bits_walking_right |= 1ULL << bit_pos;
                merged_right[bit_pos]++;
                merged_forward[rightxCS_P + bit_pos] = 0;
                continue;
            }

            bits_walking_right &= ~(1ULL << bit_pos);

            uint8_t mesh_left =  (uint8_t)(right - merged_right[bit_pos]);
            uint8_t mesh_right = (uint8_t)(right + 1);
            uint8_t mesh_front = (uint8_t)(forward - merged_forward[rightxCS_P + bit_pos]);
            uint8_t mesh_back =  (uint8_t)(forward + 1);
            uint8_t mesh_up =    (uint8_t)(bit_pos + (face % 2 == 0 ? 1 : 0));

            uint8_t ao_LB = 3, ao_RB = 3, ao_RF = 3, ao_LF = 3;
            if (bake_ao) {
                int c = bit_pos + air_dir;
                uint8_t ao_F = solid_check(voxels[get_axis_i(axis, right, forward - 1, c)]);
                uint8_t ao_B = solid_check(voxels[get_axis_i(axis, right, forward + 1, c)]);
                uint8_t ao_L = solid_check(voxels[get_axis_i(axis, right - 1, forward, c)]);
                uint8_t ao_R = solid_check(voxels[get_axis_i(axis, right + 1, forward, c)]);

                uint8_t ao_LFC = !ao_L && !ao_F && solid_check(voxels[get_axis_i(axis, right - 1, forward - 1, c)]);
                uint8_t ao_LBC = !ao_L && !ao_B && solid_check(voxels[get_axis_i(axis, right - 1, forward + 1, c)]);
                uint8_t ao_RFC = !ao_R && !ao_F && solid_check(voxels[get_axis_i(axis, right + 1, forward - 1, c)]);
                uint8_t ao_RBC = !ao_R && !ao_B && solid_check(voxels[get_axis_i(axis, right + 1, forward + 1, c)]);

                ao_LB = vertexAO(ao_L, ao_B, ao_LBC);
                ao_RB = vertexAO(ao_R, ao_B, ao_RBC);
                ao_RF = vertexAO(ao_R, ao_F, ao_RFC);
                ao_LF = vertexAO(ao_L, ao_F, ao_LFC);
            }

            merged_forward[rightxCS_P + bit_pos] = 0;
            merged_right[bit_pos] = 0;

            uint32_t x, y, z, width, height;
            uint32_t v0ao, v1ao, v2ao, v3ao;
            uint8_t packedAO;
            if (face == 0) { //Z+
                // printf("Face 0:\n");
                x = mesh_front - 1;
                y = mesh_left - 1;
                z = mesh_up - 1;
                width  = (mesh_back - 1) - ( mesh_front - 1);
                height = (mesh_right - 1) - ( mesh_left - 1);
                v0ao = ao_LB;
                v1ao = ao_RB;
                v2ao = ao_RF;
                v3ao = ao_LF;
                packedAO = pack_ao_values(v3ao, v2ao, v0ao, v1ao);


            }
            else if (face == 1) { //Z-
                // printf("Face 1:\n");
                x = mesh_back - 1;
                y = mesh_left - 1;
                z = mesh_up - 1;
                width  = (mesh_back - 1) - ( mesh_front - 1);
                height = (mesh_right - 1) - ( mesh_left - 1);
                v0ao = ao_LB;
                v1ao = ao_LF;
                v2ao = ao_RF;
                v3ao = ao_RB;
                packedAO = pack_ao_values(v0ao, v3ao, v1ao, v2ao);

    
            }
            else if (face == 2) { //Y+
                // printf("Face 2:\n");
                x = mesh_right - 1;
                y = mesh_up - 1;
                z = mesh_front - 1;
                width  = (mesh_right - 1) - ( mesh_left - 1);
                height = (mesh_back - 1) - ( mesh_front - 1);
                v0ao = ao_LB;
                v1ao = ao_RB;
                v2ao = ao_RF;
                v3ao = ao_LF;
                packedAO = pack_ao_values(v2ao, v1ao,  v3ao, v0ao);

            }
            else if (face == 3) { //Y-
                // printf("Face 3:\n");
                x = mesh_left - 1;
                y = mesh_up - 1;
                z = mesh_front - 1;
                width  = (mesh_right - 1) - ( mesh_left - 1);
                height = (mesh_back - 1) - ( mesh_front - 1);

                v0ao = ao_LB;
                v1ao = ao_LF;
                v2ao = ao_RF;
                v3ao = ao_RB;
                packedAO = pack_ao_values(v1ao, v0ao,  v2ao, v3ao);


            }
            else if (face == 4) {//X+
                // printf("Face 4:\n");            
                x = mesh_up - 1;
                y = mesh_front - 1;
                z = mesh_left - 1;
                width  = (mesh_back - 1) - ( mesh_front - 1);
                height = (mesh_right - 1) - ( mesh_left - 1);
                v0ao = ao_LB;
                v1ao = ao_RB;
                v2ao = ao_RF;
                v3ao = ao_LF;
                packedAO = pack_ao_values(v3ao, v2ao, v0ao, v1ao);


            }
            else if (face == 5) {//X-
                // printf("Face 5:\n");
                x = mesh_up - 1;
                y = mesh_back - 1;
                z = mesh_left - 1;
                width  = (mesh_back - 1) - ( mesh_front - 1);
                height = (mesh_right - 1) - ( mesh_left - 1);
                v0ao = ao_LB;
                v1ao = ao_LF;
                v2ao = ao_RF;
                v3ao = ao_RB;
                packedAO = pack_ao_values(v0ao, v3ao, v1ao, v2ao);

            }
            // if(brickmapIndex == 1){
            //   printf("debug case\n");
            // }
            insert_face(faces, x, y, z, width, height, (ao_LB + ao_RF > ao_RB + ao_LF), faceI,  maxFaces,
                                                                        packedAO,  type, type2, face, light);
            }
        }
        }
    }
    faceCount += faceI;
    // chunkData->faceIndexCount[brickmapIndex] = faceI * 6;
    // printf("meshing complete, faceCount: %d, indexCount: %d\n",faceI, chunkData->faceIndexCount[brickmapIndex] );
    if(faceI > 15000){
        printf("FACE COUNT EXCEEDS 15,000: %d \n", faceI);
    }
    
        // printf("Binary meshing VERTEX PULLING for test pbm took: %f seconds\n", binary_meshTimer.elapsed());
        // binary_meshTimer.stop();
    }

    void vertex_pull_mesh_variable(uint8_t* voxels, voxel_work* worker, bool bake_ao, uvec3 resolution,
                                    FaceData* faces, uint32_t& faceCount, uint32_t maxFaces) {
    
    faceCount = 0;
    // binary_meshColGenTimer.reset();
    // //printf("Generate initial axis cols took: %f seconds\n", binary_meshColGenTimer.elapsed());
    // binary_meshColGenTimer.stop();
    int xRes = (int)resolution.x;
    int yRes = (int)resolution.y;
    int zRes = (int)resolution.z;
    int xyRes = xRes * yRes;
    int zyRes = zRes * yRes;
    int zxRes = zRes * xRes;
    // binary_meshTimer.reset();

    int faceI = 0;

    uint64_t* a_axis_cols = worker->a_axis_cols;
    uint64_t* b_axis_cols = worker->b_axis_cols;
    uint64_t* merged_right = worker->merged_right;
    uint64_t* merged_forward = worker->merged_forward;
    
    uint64_t* col_face_masks_x = worker->col_face_masks_x;
    uint64_t* col_face_masks_y = worker->col_face_masks_y;
    uint64_t* col_face_masks_z = worker->col_face_masks_z;

        memset(col_face_masks_x, 0, sizeof(uint64_t) * CS_P2 * 2);
        memset(col_face_masks_y, 0, sizeof(uint64_t) * CS_P2 * 2);
        memset(col_face_masks_z, 0, sizeof(uint64_t) * CS_P2 * 2);

    // std::vector<uint64_t> a_axis_cols(xyRes, 0);
    // std::vector<uint64_t> b_axis_cols(xRes, 0);

    // std::vector<uint64_t> col_face_masks_z(xyRes*2, 0);
    // std::vector<uint64_t> col_face_masks_x(zyRes*2, 0);
    // std::vector<uint64_t> col_face_masks_y(zxRes*2, 0);

    uint64_t CULL_MASKX = (1ULL << (xRes - 1));
    uint64_t CULL_MASKY = (1ULL << (yRes - 1));
    uint64_t CULL_MASKZ = (1ULL << (zRes - 1));

    uint64_t BORDER_MASKX = (1ULL | (1ULL <<  (xRes - 1)));
    uint64_t BORDER_MASKY = (1ULL | (1ULL <<  (yRes - 1)));
    uint64_t BORDER_MASKZ = (1ULL | (1ULL <<  (zRes - 1)));

    //a = z, b = y, c = x
    // Begin culling faces
    //printf("printing out voxels solid 1 empty 0\n");
    auto p = voxels;
    memset(a_axis_cols, 0, sizeof(uint64_t) * CS_P2);
    for (int a = 0; a < zRes; a++) {
        memset(b_axis_cols, 0, sizeof(uint64_t) * CS_P);
        
        for (int b = 0; b < yRes; b++) {
        uint64_t cb = 0;

        for (int c = 0; c < xRes; c++) {
            if (solid_check(*p)) {
            a_axis_cols[b + (c * yRes)] |= 1ULL << a;
            b_axis_cols[c] |= 1ULL << b;
            cb |= 1ULL << c;
            }
            else{
            //printf("0 ");
            }
            p++;
        }
        //printf("\n");

        //x axis
        // Cull third axis faces
        col_face_masks_x[a + (b * zRes) + (0 * zyRes)] = cb & ~((cb >> 1) | CULL_MASKX);
        col_face_masks_x[a + (b * zRes) + (1 * zyRes)] = cb & ~((cb << 1) | 1ULL);
        }

        //y axis
        // Cull second axis faces
        int faceIndex = (a * xRes);// + (2 * xyRes);
        for (int b = 1; b < xRes - 1; b++) {//a is z, b is x
        uint64_t col = b_axis_cols[b];
        col_face_masks_y[faceIndex + b] = col & ~((col >> 1) | CULL_MASKY);
        col_face_masks_y[faceIndex + b + zxRes] = col & ~((col << 1) | 1ULL);
        }
    }

    //z axis
    // Cull first axis faces
    // //printf("z axis populating\n");
    for (int a = 1; a < xRes - 1; a++) { //a = y
        int faceIndex = a * yRes;
        for (int b = 1; b < yRes - 1; b++) { //b = x
        uint64_t col = a_axis_cols[faceIndex + b];
        if(col!= 0)//printf("col non zero at: a_axis_col index: %d. faceIndex: %d, b: %d\n", faceIndex + b, faceIndex , b);
        col_face_masks_z[faceIndex + b] = col & ~((col >> 1) | CULL_MASKZ);
        col_face_masks_z[faceIndex + b + xyRes] = col & ~((col << 1) | 1ULL);
        }
    }
    

    // Greedy meshing
    int var_CS_P2 = 0;
    int var_CS_P = 0;
    int var_CS = 0;
    int forward_max = 0;
    int forward_bounded = 0;
    int right_max = 0;
    int right_bounded = 0;

    for (uint8_t face = 0; face < 6; face++) {
        //printf("FACE LOOP START: FACE: %d\n", face);
        int axis = face / 2;
        int air_dir = face % 2 == 0 ? 1 : -1;
        //z axis
        if(axis == 0){//forward is X+, right is Y+
        // //printf("Axis is 0, forward is X+, right is Y+\n");
        var_CS_P2 = xRes * yRes;
        var_CS_P = yRes;
        var_CS = xRes - 2;
        forward_max = xRes;
        right_max = yRes;
        forward_bounded =  xRes - 2;
        right_bounded = yRes - 2;
        }
        //y axis
        if(axis == 1){//forward is Z+, right is X+
        // //printf("Axis is 1, forward is Z+, right is X+\n");
        var_CS_P2 = zRes * xRes;
        var_CS_P = xRes;
        var_CS = xRes - 2;
        forward_max = zRes;
        right_max = xRes;
        forward_bounded =  zRes - 2;
        right_bounded = xRes - 2;
        }
        //x axis
        if(axis == 2){//forward is Y+, right is Z+
        // //printf("Axis is 2, forward is Y+, right is Z+\n");
        var_CS_P2 = yRes * zRes;
        var_CS_P = yRes;
        var_CS = yRes - 2;
        forward_max = yRes;
        right_max = zRes;
        forward_bounded =  yRes - 2;
        right_bounded = zRes - 2;
        }

        //printf("axis: %d, var_CS: %d, var_CS_P: %d, var_CS_P2: %d, forward_max: %d, right_max: %d\n", axis, var_CS, var_CS_P, var_CS_P2, forward_max, right_max);

        memset(merged_forward, 0, sizeof(uint64_t) * CS_P2);

        for (int forward = 1; forward < forward_max - 1; forward++) {
        uint64_t bits_walking_right = 0;
        int array_offset = face % 2;
                                    //was var_CS_P
        int forwardIndex = (forward * right_max) + (array_offset * var_CS_P2);

        memset(merged_right, 0, sizeof(uint64_t) * CS_P);

        for (int right = 1; right < right_max - 1; right++) {
            int rightxCS_P = right * var_CS_P;
            
            //z
            uint64_t bits_here = 0;
            uint64_t bits_right = 0;
            uint64_t bits_forward = 0;
            if(axis == 0){
            bits_here = col_face_masks_z[forwardIndex + right] &~ BORDER_MASKZ;
            //printf("bits_here: %llu, col_face_masks_z[%d + %d] &~ %llu\n", bits_here, forwardIndex ,right ,BORDER_MASKZ);
            if(bits_here == 0) continue;
            

            bits_right = right >= right_bounded ? 0 : col_face_masks_z[forwardIndex + right + 1];
            //printf("bits_right: %llu, %d >= %d, col_face_masks_z[%d + %d + 1] \n", bits_right, right, right_bounded, forwardIndex, right);
            bits_forward = forward >= forward_bounded ? 0 : col_face_masks_z[forwardIndex + right + var_CS_P];
            //printf("bits_forward: %llu, %d >= %d, col_face_masks_z[%d + %d + %d] \n", bits_right, forward, forward_bounded, forwardIndex, right, var_CS_P);

            }
            else if(axis == 1){//y
            bits_here = col_face_masks_y[forwardIndex + right] &~ BORDER_MASKY;
            //printf("bits_here: %llu, col_face_masks_y[%d + %d] &~ %llu\n", bits_here, forwardIndex ,right ,BORDER_MASKY);
            if(bits_here == 0) continue;

            bits_right = right >= right_bounded ? 0 : col_face_masks_y[forwardIndex + right + 1];
            //printf("bits_right: %llu, %d >= %d, col_face_masks_y[%d + %d + 1] \n", bits_right, right, right_bounded, forwardIndex, right);
            bits_forward = forward >= forward_bounded ? 0 : col_face_masks_y[forwardIndex + right + var_CS_P];
            //printf("bits_forward: %llu, %d >= %d, col_face_masks_y[%d + %d + %d] \n", bits_right, forward, forward_bounded, forwardIndex, right, var_CS_P);
            
            }
            else if(axis == 2){//x
            bits_here = col_face_masks_x[forwardIndex + right] &~ BORDER_MASKX;
            if(bits_here == 0) continue;
            

            bits_right = right >= right_bounded ? 0 : col_face_masks_x[forwardIndex + right + 1];
            bits_forward = forward >= forward_bounded ? 0 : col_face_masks_x[forwardIndex + right + zRes]; //forward would have been y
            }

            uint64_t bits_merging_forward = bits_here & bits_forward & ~bits_walking_right;
            uint64_t bits_merging_right = bits_here & bits_right;
            //printf("bits here: %llu, bits_merging_forward %llu, bits_merging_right: %llu, forward: %d, right: %d\n",bits_here, bits_merging_forward, bits_merging_right, forward, right);

            unsigned long bit_pos;

            uint64_t copy_front = bits_merging_forward;
            while (copy_front) {
            #ifdef _MSC_VER
                _BitScanForward64(&bit_pos, copy_front);
            #else
                bit_pos = __builtin_ctzll(copy_front);
            #endif

            copy_front &= ~(1ULL << bit_pos);
            
            //printf("copy_front loop: copy_front pos: %llu bit_pos: %llu\n", copy_front, bit_pos);

            if(
                voxels[get_axis_ivar(axis, right, forward, bit_pos, var_CS_P, var_CS_P2, xRes, yRes, zRes)] == voxels[get_axis_ivar(axis, right, forward + 1, bit_pos, var_CS_P, var_CS_P2, xRes, yRes, zRes)] &&
                (!bake_ao || compare_aovar(voxels, axis, forward, right, bit_pos + air_dir, 1, 0, var_CS_P, var_CS_P2, xRes, yRes, zRes))
            ) {
                merged_forward[(right * var_CS_P) + bit_pos]++;
            }
            else {
                bits_merging_forward &= ~(1ULL << bit_pos);
            }
            }

            uint64_t bits_stopped_forward = bits_here & ~bits_merging_forward;
            while (bits_stopped_forward) {
            #ifdef _MSC_VER
                _BitScanForward64(&bit_pos, bits_stopped_forward);
            #else
                bit_pos = __builtin_ctzll(bits_stopped_forward);
            #endif

            bits_stopped_forward &= ~(1ULL << bit_pos);

            //printf("bits_stopped_forward loop: bits_stopped_forward: %llu bit_pos: %llu\n", bits_stopped_forward, bit_pos);

            uint32_t voxelIndex = get_axis_ivar(axis, right, forward, bit_pos, var_CS_P, var_CS_P2, xRes, yRes, zRes);
            uint8_t type = voxels[voxelIndex];

            //for visualizing the meshing process
            // type = forward * 4;
            
            uint8_t type2 = right * 4;
            // uint8_t light = (voxels[voxelIndex] >> 12) & 0xF;
            uint8_t light = 0;

            //printf("axis: %d, right: %d, forward: %d, bit_pos: %d, type: %d\n", axis, right, forward, bit_pos, type);
            //printf("current voxel: %d\n", get_axis_ivar(axis, right, forward, bit_pos, var_CS_P, var_CS_P2, xRes, yRes, zRes));

            if (
                (bits_merging_right & (1ULL << bit_pos)) != 0 &&
                (merged_forward[(right * var_CS_P) + bit_pos] == merged_forward[(right + 1) * var_CS_P + bit_pos]) &&
                (type == voxels[get_axis_ivar(axis, right + 1, forward, bit_pos, var_CS_P, var_CS_P2, xRes, yRes, zRes)]) &&
                (!bake_ao || compare_aovar(voxels, axis, forward, right, bit_pos + air_dir, 0, 1, var_CS_P, var_CS_P2, xRes, yRes, zRes))
                ) {
                bits_walking_right |= 1ULL << bit_pos;
                merged_right[bit_pos]++;
                merged_forward[rightxCS_P + bit_pos] = 0;
                continue;
            }

            bits_walking_right &= ~(1ULL << bit_pos);

            uint8_t mesh_left =  (uint8_t)(right - merged_right[bit_pos]);
            uint8_t mesh_right = (uint8_t)(right + 1);
            uint8_t mesh_front = (uint8_t)(forward - merged_forward[rightxCS_P + bit_pos]);
            uint8_t mesh_back =  (uint8_t)(forward + 1);
            uint8_t mesh_up =    (uint8_t)(bit_pos + (face % 2 == 0 ? 1 : 0));
            
            // if(face == 0){
            //   //printf("face 0, right %d, forward %d\n", right, forward);
            //   //printf("rightxCS_P: %d, bit_pos: %d\n",rightxCS_P, bit_pos);
            //   //printf("mesh_back: %d, mesh_front: %d\n", mesh_back, mesh_front);
            //   //printf("mergedforward[%d]: %d\n", rightxCS_P + bit_pos, merged_forward[rightxCS_P + bit_pos]);
            // }

            //printf("face %d, right %d, forward %d, bit_pos %d\n", face, right, forward, bit_pos);

            uint8_t ao_LB = 3, ao_RB = 3, ao_RF = 3, ao_LF = 3;
            if (bake_ao) {
                //printf("Checking AO for mesh at right: %d, forward: %d, bit_pos: %d\n", right, forward, bit_pos);
                int c = bit_pos + air_dir;
                uint8_t ao_F = solid_check(voxels[get_axis_ivar(axis, right, forward - 1, c, var_CS_P, var_CS_P2, xRes, yRes, zRes)]);
                //printf("ao_F: %d, checking voxel: %d\n", ao_F, get_axis_ivar(axis, right, forward - 1, c, var_CS_P, var_CS_P2, xRes, yRes, zRes));

                uint8_t ao_B = solid_check(voxels[get_axis_ivar(axis, right, forward + 1, c, var_CS_P, var_CS_P2, xRes, yRes, zRes)]);
                //printf("ao_B: %d, checking voxel: %d\n", ao_B, get_axis_ivar(axis, right, forward + 1, c, var_CS_P, var_CS_P2, xRes, yRes, zRes));

                uint8_t ao_L = solid_check(voxels[get_axis_ivar(axis, right - 1, forward, c, var_CS_P, var_CS_P2, xRes, yRes, zRes)]);
                //printf("ao_L: %d, checking voxel: %d\n", ao_L, get_axis_ivar(axis, right - 1, forward, c, var_CS_P, var_CS_P2, xRes, yRes, zRes));

                uint8_t ao_R = solid_check(voxels[get_axis_ivar(axis, right + 1, forward, c, var_CS_P, var_CS_P2, xRes, yRes, zRes)]);
                //printf("ao_R: %d, checking voxel: %d\n", ao_R, get_axis_ivar(axis, right + 1, forward, c, var_CS_P, var_CS_P2, xRes, yRes, zRes));


                uint8_t ao_LFC = !ao_L && !ao_F && solid_check(voxels[get_axis_ivar(axis, right - 1, forward - 1, c, var_CS_P, var_CS_P2, xRes, yRes, zRes)]);
                //printf("ao_LFC: %d, checking voxel: %d\n", ao_LFC, get_axis_ivar(axis, right - 1, forward - 1, c, var_CS_P, var_CS_P2, xRes, yRes, zRes));

                uint8_t ao_LBC = !ao_L && !ao_B && solid_check(voxels[get_axis_ivar(axis, right - 1, forward + 1, c, var_CS_P, var_CS_P2, xRes, yRes, zRes)]);
                // //printf("ao_LBC: %d, checking voxel: %d\n", ao_LBC, get_axis_ivar(axis, right - 1, forward + 1, c, var_CS_P, var_CS_P2, xRes, yRes, zRes));

                uint8_t ao_RFC = !ao_R && !ao_F && solid_check(voxels[get_axis_ivar(axis, right + 1, forward - 1, c, var_CS_P, var_CS_P2, xRes, yRes, zRes)]);
                // //printf("ao_RFC: %d, checking voxel: %d\n", ao_RFC, get_axis_ivar(axis, right + 1, forward - 1, c, var_CS_P, var_CS_P2, xRes, yRes, zRes));

                uint8_t ao_RBC = !ao_R && !ao_B && solid_check(voxels[get_axis_ivar(axis, right + 1, forward + 1, c, var_CS_P, var_CS_P2, xRes, yRes, zRes)]);
                // //printf("ao_RBC: %d, checking voxel: %d\n", ao_RBC, get_axis_ivar(axis, right + 1, forward + 1, c, var_CS_P, var_CS_P2, xRes, yRes, zRes));


                ao_LB = vertexAO(ao_L, ao_B, ao_LBC);
                ao_RB = vertexAO(ao_R, ao_B, ao_RBC);
                ao_RF = vertexAO(ao_R, ao_F, ao_RFC);
                ao_LF = vertexAO(ao_L, ao_F, ao_LFC);
            }

            merged_forward[rightxCS_P + bit_pos] = 0;
            merged_right[bit_pos] = 0;

    uint32_t x, y, z, width, height;
            uint32_t v0ao, v1ao, v2ao, v3ao;
            uint8_t packedAO;
            if (face == 0) { //Z+
                // printf("Face 0:\n");
                x = mesh_front - 1;
                y = mesh_left - 1;
                z = mesh_up - 1;
                width  = (mesh_back - 1) - ( mesh_front - 1);
                height = (mesh_right - 1) - ( mesh_left - 1);
                v0ao = ao_LB;
                v1ao = ao_RB;
                v2ao = ao_RF;
                v3ao = ao_LF;
                packedAO = pack_ao_values(v3ao, v2ao, v0ao, v1ao);


            }
            else if (face == 1) { //Z-
                // printf("Face 1:\n");
                x = mesh_back - 1;
                y = mesh_left - 1;
                z = mesh_up - 1;
                width  = (mesh_back - 1) - ( mesh_front - 1);
                height = (mesh_right - 1) - ( mesh_left - 1);
                v0ao = ao_LB;
                v1ao = ao_LF;
                v2ao = ao_RF;
                v3ao = ao_RB;
                packedAO = pack_ao_values(v0ao, v3ao, v1ao, v2ao);

    
            }
            else if (face == 2) { //Y+
                // printf("Face 2:\n");
                x = mesh_right - 1;
                y = mesh_up - 1;
                z = mesh_front - 1;
                width  = (mesh_right - 1) - ( mesh_left - 1);
                height = (mesh_back - 1) - ( mesh_front - 1);
                v0ao = ao_LB;
                v1ao = ao_RB;
                v2ao = ao_RF;
                v3ao = ao_LF;
                packedAO = pack_ao_values(v2ao, v1ao,  v3ao, v0ao);

            }
            else if (face == 3) { //Y-
                // printf("Face 3:\n");
                x = mesh_left - 1;
                y = mesh_up - 1;
                z = mesh_front - 1;
                width  = (mesh_right - 1) - ( mesh_left - 1);
                height = (mesh_back - 1) - ( mesh_front - 1);

                v0ao = ao_LB;
                v1ao = ao_LF;
                v2ao = ao_RF;
                v3ao = ao_RB;
                packedAO = pack_ao_values(v1ao, v0ao,  v2ao, v3ao);


            }
            else if (face == 4) {//X+
                // printf("Face 4:\n");            
                x = mesh_up - 1;
                y = mesh_front - 1;
                z = mesh_left - 1;
                width  = (mesh_back - 1) - ( mesh_front - 1);
                height = (mesh_right - 1) - ( mesh_left - 1);
                v0ao = ao_LB;
                v1ao = ao_RB;
                v2ao = ao_RF;
                v3ao = ao_LF;
                packedAO = pack_ao_values(v3ao, v2ao, v0ao, v1ao);


            }
            else if (face == 5) {//X-
                // printf("Face 5:\n");
                x = mesh_up - 1;
                y = mesh_back - 1;
                z = mesh_left - 1;
                width  = (mesh_back - 1) - ( mesh_front - 1);
                height = (mesh_right - 1) - ( mesh_left - 1);
                v0ao = ao_LB;
                v1ao = ao_LF;
                v2ao = ao_RF;
                v3ao = ao_RB;
                packedAO = pack_ao_values(v0ao, v3ao, v1ao, v2ao);

            }
            // if(brickmapIndex == 1){
            //   printf("debug case\n");
            // }
            insert_face(faces, x, y, z, width, height, ao_LB + ao_RF > ao_RB + ao_LF, faceI,  maxFaces,
                                                                        packedAO,  type, type2, face, light);
            }
        }
        }
    }

    faceCount += faceI;
    // chunkData->faceIndexCount[brickmapIndex] = faceI * 6;
    // printf("meshing complete, faceCount: %d\n",faceI);
    if(faceI > FACE_MAX){
        printf("FACE COUNT EXCEEDS %d: %d \n",FACE_MAX, faceI);

    }
    
        // printf("Binary meshing for VARIABLE SIZED pbm took: %f seconds\n", binary_meshTimer.elapsed());
        // binary_meshTimer.stop();
    };



    // voxels - 64^3 (includes neighboring voxels)
    // vertices - pre-allocated array of vertices that will be poplulated. Can be re-used between runs and does not need to be clared.
    // vertexLength - output  number of vertices to read from vertices
    // void mesh(uint16_t* voxels, chunk_data* chunkData, bool bake_ao) {
    
    
    // // binary_meshColGenTimer.reset();
    // // printf("Generate initial axis cols took: %f seconds\n", binary_meshColGenTimer.elapsed());
    // // binary_meshColGenTimer.stop();


    // chunkData->vertexCount = 0;
    // int vertexI = 0;

    // uint64_t* col_face_masks = chunkData->col_face_masks;
    // uint64_t* a_axis_cols = chunkData->a_axis_cols;
    // uint64_t* b_axis_cols = chunkData->b_axis_cols;
    // uint64_t* merged_right = chunkData->merged_right;
    // uint64_t* merged_forward = chunkData->merged_forward;

    // memset(col_face_masks, 0, sizeof(uint64_t) * CS_P2 * 6);

    // // Begin culling faces
    // auto p = voxels;
    // memset(a_axis_cols, 0, sizeof(uint64_t) * CS_P2);
    // for (int a = 0; a < CS_P; a++) {
    //     memset(b_axis_cols, 0, sizeof(uint64_t) * CS_P);

    //     for (int b = 0; b < CS_P; b++) {
    //     uint64_t cb = 0;

    //     for (int c = 0; c < CS_P; c++) {
    //         if (solid_check(*p)) {
    //         a_axis_cols[b + (c * CS_P)] |= 1ULL << a;
    //         b_axis_cols[c] |= 1ULL << b;
    //         cb |= 1ULL << c;
    //         }
    //         p++;
    //     }

    //     //x axis
    //     // Cull third axis faces
    //     col_face_masks[a + (b * CS_P) + (4 * CS_P2)] = cb & ~((cb >> 1) | CULL_MASK);
    //     col_face_masks[a + (b * CS_P) + (5 * CS_P2)] = cb & ~((cb << 1) | 1ULL);
    //     }

    //     //y axis
    //     // Cull second axis faces
    //     int faceIndex = (a * CS_P) + (2 * CS_P2);
    //     for (int b = 1; b < CS_P - 1; b++) {
    //     uint64_t col = b_axis_cols[b];
    //     col_face_masks[faceIndex + b] = col & ~((col >> 1) | CULL_MASK);
    //     col_face_masks[faceIndex + b + CS_P2] = col & ~((col << 1) | 1ULL);
    //     }
    // }

    // //z axis
    // // Cull first axis faces
    // for (int a = 1; a < CS_P - 1; a++) {
    //     int faceIndex = a * CS_P;
    //     for (int b = 1; b < CS_P - 1; b++) {
    //     uint64_t col = a_axis_cols[faceIndex + b];

    //     col_face_masks[faceIndex + b] = col & ~((col >> 1) | CULL_MASK);
    //     col_face_masks[faceIndex + b + CS_P2] = col & ~((col << 1) | 1ULL);
    //     }
    // }
    
    // //its how i imagined, but rotate it right, the top of the visible brickmap is along the rightside. why is that?
    // // printf("print a_axis_cols:\n");
    // // for(int i = 0; i < CS_P; i++){
    // //   for(int j = 0; j < CS_P; j++){
    // //     printf("%llu ", a_axis_cols[i*CS_P + j]);
    // //   }
    // //   printf("\n");
    // // }
    // // Greedy meshing
    // for (uint8_t face = 0; face < 6; face++) {
    //     int axis = face / 2;
    //     int air_dir = face % 2 == 0 ? 1 : -1;

    //     memset(merged_forward, 0, sizeof(uint64_t) * CS_P2);

    //     for (int forward = 1; forward < CS_P - 1; forward++) {
    //     uint64_t bits_walking_right = 0;
    //     int forwardIndex = (forward * CS_P) + (face * CS_P2);

    //     memset(merged_right, 0, sizeof(uint64_t) * CS_P);

    //     for (int right = 1; right < CS_P - 1; right++) {
    //         int rightxCS_P = right * CS_P;

    //         uint64_t bits_here = col_face_masks[forwardIndex + right] &~ BORDER_MASK;
    //         uint64_t bits_right = right >= CS ? 0 : col_face_masks[forwardIndex + right + 1];
    //         uint64_t bits_forward = forward >= CS ? 0 : col_face_masks[forwardIndex + right + CS_P];

    //         uint64_t bits_merging_forward = bits_here & bits_forward & ~bits_walking_right;
    //         uint64_t bits_merging_right = bits_here & bits_right;

    //         unsigned long bit_pos;

    //         uint64_t copy_front = bits_merging_forward;
    //         while (copy_front) {
    //         #ifdef _MSC_VER
    //             _BitScanForward64(&bit_pos, copy_front);
    //         #else
    //             bit_pos = __builtin_ctzll(copy_front);
    //         #endif

    //         copy_front &= ~(1ULL << bit_pos);

    //         if(
    //             voxels[get_axis_i(axis, right, forward, bit_pos)] == voxels[get_axis_i(axis, right, forward + 1, bit_pos)] &&
    //             (!bake_ao || compare_ao(voxels, axis, forward, right, bit_pos + air_dir, 1, 0))
    //         ) {
    //             merged_forward[(right * CS_P) + bit_pos]++;
    //         }
    //         else {
    //             bits_merging_forward &= ~(1ULL << bit_pos);
    //         }
    //         }

    //         uint64_t bits_stopped_forward = bits_here & ~bits_merging_forward;
    //         while (bits_stopped_forward) {
    //         #ifdef _MSC_VER
    //             _BitScanForward64(&bit_pos, bits_stopped_forward);
    //         #else
    //             bit_pos = __builtin_ctzll(bits_stopped_forward);
    //         #endif

    //         bits_stopped_forward &= ~(1ULL << bit_pos);

    //         uint8_t type = (uint8_t)(voxels[get_axis_i(axis, right, forward, bit_pos)]);

    //         if (
    //             (bits_merging_right & (1ULL << bit_pos)) != 0 &&
    //             (merged_forward[(right * CS_P) + bit_pos] == merged_forward[(right + 1) * CS_P + bit_pos]) &&
    //             (type == voxels[get_axis_i(axis, right + 1, forward, bit_pos)]) &&
    //             (!bake_ao || compare_ao(voxels, axis, forward, right, bit_pos + air_dir, 0, 1))
    //             ) {
    //             bits_walking_right |= 1ULL << bit_pos;
    //             merged_right[bit_pos]++;
    //             merged_forward[rightxCS_P + bit_pos] = 0;
    //             continue;
    //         }

    //         bits_walking_right &= ~(1ULL << bit_pos);

    //         uint8_t mesh_left =  (uint8_t)(right - merged_right[bit_pos]);
    //         uint8_t mesh_right = (uint8_t)(right + 1);
    //         uint8_t mesh_front = (uint8_t)(forward - merged_forward[rightxCS_P + bit_pos]);
    //         uint8_t mesh_back =  (uint8_t)(forward + 1);
    //         uint8_t mesh_up =    (uint8_t)(bit_pos + (face % 2 == 0 ? 1 : 0));

    //         uint8_t ao_LB = 3, ao_RB = 3, ao_RF = 3, ao_LF = 3;
    //         if (bake_ao) {
    //             int c = bit_pos + air_dir;
    //             uint8_t ao_F = solid_check(voxels[get_axis_i(axis, right, forward - 1, c)]);
    //             uint8_t ao_B = solid_check(voxels[get_axis_i(axis, right, forward + 1, c)]);
    //             uint8_t ao_L = solid_check(voxels[get_axis_i(axis, right - 1, forward, c)]);
    //             uint8_t ao_R = solid_check(voxels[get_axis_i(axis, right + 1, forward, c)]);

    //             uint8_t ao_LFC = !ao_L && !ao_F && solid_check(voxels[get_axis_i(axis, right - 1, forward - 1, c)]);
    //             uint8_t ao_LBC = !ao_L && !ao_B && solid_check(voxels[get_axis_i(axis, right - 1, forward + 1, c)]);
    //             uint8_t ao_RFC = !ao_R && !ao_F && solid_check(voxels[get_axis_i(axis, right + 1, forward - 1, c)]);
    //             uint8_t ao_RBC = !ao_R && !ao_B && solid_check(voxels[get_axis_i(axis, right + 1, forward + 1, c)]);

    //             ao_LB = vertexAO(ao_L, ao_B, ao_LBC);
    //             ao_RB = vertexAO(ao_R, ao_B, ao_RBC);
    //             ao_RF = vertexAO(ao_R, ao_F, ao_RFC);
    //             ao_LF = vertexAO(ao_L, ao_F, ao_LFC);
    //         }

    //         merged_forward[rightxCS_P + bit_pos] = 0;
    //         merged_right[bit_pos] = 0;

    //         uint32_t v1, v2, v3, v4;
    //         if (face == 0) { //Z+
    //             // printf("Face 0:\n");
    //             v1 = get_vertex(mesh_back, mesh_left, mesh_up, type, face, ao_LB);
    //             v2 = get_vertex(mesh_back, mesh_right, mesh_up, type, face, ao_RB);
    //             v3 = get_vertex(mesh_front, mesh_right, mesh_up, type, face, ao_RF);
    //             v4 = get_vertex(mesh_front, mesh_left, mesh_up, type, face, ao_LF);
    //         }
    //         else if (face == 1) { //Z-
    //             // printf("Face 1:\n");
    //             v1 = get_vertex(mesh_back, mesh_left, mesh_up, type, face, ao_LB);
    //             v2 = get_vertex(mesh_front, mesh_left, mesh_up, type, face, ao_LF);
    //             v3 = get_vertex(mesh_front, mesh_right, mesh_up, type, face, ao_RF);
    //             v4 = get_vertex(mesh_back, mesh_right, mesh_up, type, face, ao_RB);
    //         }
    //         else if (face == 2) { //Y+
    //             // printf("Face 2:\n");
    //             v1 = get_vertex(mesh_left, mesh_up, mesh_back, type, face, ao_LB);
    //             v2 = get_vertex(mesh_right, mesh_up, mesh_back, type, face, ao_RB);
    //             v3 = get_vertex(mesh_right, mesh_up, mesh_front, type, face, ao_RF);
    //             v4 = get_vertex(mesh_left, mesh_up, mesh_front, type, face, ao_LF);
    //         }
    //         else if (face == 3) { //Y-
    //             // printf("Face 3:\n");
    //             v1 = get_vertex(mesh_left, mesh_up, mesh_back, type, face, ao_LB);
    //             v2 = get_vertex(mesh_left, mesh_up, mesh_front, type, face, ao_LF);
    //             v3 = get_vertex(mesh_right, mesh_up, mesh_front, type, face, ao_RF);
    //             v4 = get_vertex(mesh_right, mesh_up, mesh_back, type, face, ao_RB);
    //         }
    //         else if (face == 4) {//X+
    //             // printf("Face 4:\n");
    //             v1 = get_vertex(mesh_up, mesh_back, mesh_left, type, face, ao_LB);
    //             v2 = get_vertex(mesh_up, mesh_back, mesh_right, type, face, ao_RB);
    //             v3 = get_vertex(mesh_up, mesh_front, mesh_right, type, face, ao_RF);
    //             v4 = get_vertex(mesh_up, mesh_front, mesh_left, type, face, ao_LF);
    //         }
    //         else if (face == 5) {//X-
    //             // printf("Face 5:\n");
    //             v1 = get_vertex(mesh_up, mesh_back, mesh_left, type, face, ao_LB);
    //             v2 = get_vertex(mesh_up, mesh_front, mesh_left, type, face, ao_LF);
    //             v3 = get_vertex(mesh_up, mesh_front, mesh_right, type, face, ao_RF);
    //             v4 = get_vertex(mesh_up, mesh_back, mesh_right, type, face, ao_RB);
    //         }

    //         insert_quad(chunkData->vertices, v1, v2, v3, v4, ao_LB + ao_RF > ao_RB + ao_LF, vertexI, chunkData->maxVertices);
    //         }
    //     }
    //     }
    // }

    // chunkData->vertexCount = vertexI + 1;

    // };

    void init_vertex_pull_buffers(chunk_data* chunkData){
        #ifdef SERVER_BUILD
            return;
        #endif
        // printf("NEED TO HANDLE VERTEX PULL BUFFER SETUP IN VULKAN LAYER!\n");
    // bgfx::VertexLayout layout;
    //         layout.begin()
    //             .add(bgfx::Attrib::Position, 1, bgfx::AttribType::Float, false)
    //             .end();
    // uint32_t vertices[6] = {
    //             get_vertex(0, 0, 0, 1, 0, 0),
    //             get_vertex(1, 0, 0, 1, 0, 0),
    //             get_vertex(1, 1, 0, 1, 0, 0),
    //             get_vertex(0, 0, 0, 1, 0, 0),
    //             get_vertex(1, 1, 0, 1, 0, 0),
    //             get_vertex(0, 1, 0, 1, 0, 0),
    //         };

    //         uint32_t count = 0;
    //         for(int i = 0; i < FACE_MAX; i++) {
    //         //the vertex puller will determine which indices to use based on if the quad is flipped for AO or not
    //             chunkData->faceIndices[count + 0] = ((i * 6) + 0u);
    //             chunkData->faceIndices[count + 1] = ((i * 6) + 1u);
    //             chunkData->faceIndices[count + 2] = ((i * 6) + 2u);
    //             chunkData->faceIndices[count + 3] = ((i * 6) + 3u);
    //             chunkData->faceIndices[count + 4] = ((i * 6) + 4u);
    //             chunkData->faceIndices[count + 5] = ((i * 6) + 5u);
    //             count += 6;
    //         }

            
    //         const void* data = vertices;
    //         uint32_t sizeInBytes = static_cast<uint32_t>(1 * sizeof(uint32_t));

    
    //         if(bgfx::isValid(chunkData->vertexPulled_ibh)) {
    //             bgfx::destroy(chunkData->vertexPulled_ibh);
    //         }
    //         if(bgfx::isValid(chunkData->vertexPulled_vbh)) {
    //             bgfx::destroy(chunkData->vertexPulled_vbh);
    //         }
    //             // Define vertices for two triangles forming a quad
    //         // Using a 2x2x1 quad in the XY plane, centered at (0,0,0)
    //         // Assuming type = 1, norm = 0 (up), ao = 0 for simplicity
        
    //         const bgfx::Memory* mem = bgfx::copy(data, sizeInBytes);
    //         chunkData->vertexPulled_vbh = bgfx::createDynamicVertexBuffer(
    //             mem,
    //             layout,
    //             BGFX_BUFFER_ALLOW_RESIZE
    //         );

    // const bgfx::Memory* indexMem = bgfx::copy(chunkData->faceIndices, (uint32_t)count * 6 * sizeof(uint32_t));
    //         chunkData->vertexPulled_ibh = bgfx::createIndexBuffer(
    //             indexMem,
    //             BGFX_BUFFER_INDEX32  // Use this for 32-bit indices
    //         );
    }

 

        
    // //////////////////////////////////////////////////// DUMMY VERTEX PULLER START ////////////////////////////////////////////////////

    // void pop_triangle_buffers(chunk_data* chunkData){
    //     bgfx::VertexLayout layout;
    //     layout.begin()
    //         .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
    //         .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
    //         .end();

    //     // Create dynamic vertex buffer with initial size
    //     uint32_t initialVertexCount = 100000;  // Adjust as needed
    //     chunkData->tri_dvbh = bgfx::createDynamicVertexBuffer(
    //         initialVertexCount,
    //         layout,
    //         BGFX_BUFFER_ALLOW_RESIZE
    //     );

    //     // Create dynamic index buffer with initial size
    //     uint32_t initialIndexCount = 100000;  // Adjust as needed
    //     chunkData->tri_dibh = bgfx::createDynamicIndexBuffer(
    //         initialIndexCount,
    //         BGFX_BUFFER_ALLOW_RESIZE | BGFX_BUFFER_INDEX32
    //     );
    //     uint32_t initialLineIndexCount = 100000;  // Adjust as needed
    //     chunkData->tri_line_dibh = bgfx::createDynamicIndexBuffer(
    //         initialLineIndexCount,
    //         BGFX_BUFFER_ALLOW_RESIZE | BGFX_BUFFER_INDEX32
    //     );

    // }

    // void update_tri_buffers(chunk_data *chunkData){
        
    //     uint32_t num_quads = (uint32_t)(chunkData->pos_color_vertexCount /4);
    //     // Define vertices
    //     uint32_t current_indices_quads = (uint32_t)(chunkData->tri_indexCount / 6);
    //     uint32_t current_line_indices_quads = (uint32_t)(chunkData->tri_line_indexCount / 12);
        
    //     for(uint32_t i = current_indices_quads; i < num_quads; i++) {
    //         if(chunkData->flipped_quads[i]){//flipped
    //                 chunkData->tri_indices[chunkData->tri_indexCount + 0] = ((i << 2) | 0u);
    //                 chunkData->tri_indices[chunkData->tri_indexCount + 1] = ((i << 2) | 2u);
    //                 chunkData->tri_indices[chunkData->tri_indexCount + 2] = ((i << 2) | 1u);
    //                 chunkData->tri_indices[chunkData->tri_indexCount + 3] = ((i << 2) | 1u);
    //                 chunkData->tri_indices[chunkData->tri_indexCount + 4] = ((i << 2) | 2u);
    //                 chunkData->tri_indices[chunkData->tri_indexCount + 5] = ((i << 2) | 3u);

    //                 chunkData->tri_line_indices[chunkData->tri_line_indexCount + 0] = ((i << 2) | 0u);
    //                 chunkData->tri_line_indices[chunkData->tri_line_indexCount + 1] = ((i << 2) | 2u);
    //                 chunkData->tri_line_indices[chunkData->tri_line_indexCount + 2] = ((i << 2) | 2u);
    //                 chunkData->tri_line_indices[chunkData->tri_line_indexCount + 3] = ((i << 2) | 1u);
    //                 chunkData->tri_line_indices[chunkData->tri_line_indexCount + 4] = ((i << 2) | 1u);
    //                 chunkData->tri_line_indices[chunkData->tri_line_indexCount + 5] = ((i << 2) | 0u);

    //                 chunkData->tri_line_indices[chunkData->tri_line_indexCount + 6] = ((i << 2) | 1u);
    //                 chunkData->tri_line_indices[chunkData->tri_line_indexCount + 7] = ((i << 2) | 2u);
    //                 chunkData->tri_line_indices[chunkData->tri_line_indexCount + 8] = ((i << 2) | 2u);
    //                 chunkData->tri_line_indices[chunkData->tri_line_indexCount + 9] = ((i << 2) | 3u);
    //                 chunkData->tri_line_indices[chunkData->tri_line_indexCount + 10] = ((i << 2) | 3u);
    //                 chunkData->tri_line_indices[chunkData->tri_line_indexCount + 11] = ((i << 2) | 1u);
    //         }
    //         else{//not flipped
    //                 chunkData->tri_indices[chunkData->tri_indexCount + 0] = ((i << 2) | 0u);
    //                 chunkData->tri_indices[chunkData->tri_indexCount + 1] = ((i << 2) | 3u);
    //                 chunkData->tri_indices[chunkData->tri_indexCount + 2] = ((i << 2) | 1u);
    //                 chunkData->tri_indices[chunkData->tri_indexCount + 3] = ((i << 2) | 0u);
    //                 chunkData->tri_indices[chunkData->tri_indexCount + 4] = ((i << 2) | 2u);
    //                 chunkData->tri_indices[chunkData->tri_indexCount + 5] = ((i << 2) | 3u);

    //                 chunkData->tri_line_indices[chunkData->tri_line_indexCount + 0] = ((i << 2) | 0u);
    //                 chunkData->tri_line_indices[chunkData->tri_line_indexCount + 1] = ((i << 2) | 3u);
    //                 chunkData->tri_line_indices[chunkData->tri_line_indexCount + 2] = ((i << 2) | 3u);
    //                 chunkData->tri_line_indices[chunkData->tri_line_indexCount + 3] = ((i << 2) | 1u);
    //                 chunkData->tri_line_indices[chunkData->tri_line_indexCount + 4] = ((i << 2) | 1u);
    //                 chunkData->tri_line_indices[chunkData->tri_line_indexCount + 5] = ((i << 2) | 0u);

    //                 chunkData->tri_line_indices[chunkData->tri_line_indexCount + 6] = ((i << 2) | 0u);
    //                 chunkData->tri_line_indices[chunkData->tri_line_indexCount + 7] = ((i << 2) | 2u);
    //                 chunkData->tri_line_indices[chunkData->tri_line_indexCount + 8] = ((i << 2) | 2u);
    //                 chunkData->tri_line_indices[chunkData->tri_line_indexCount + 9] = ((i << 2) | 3u);
    //                 chunkData->tri_line_indices[chunkData->tri_line_indexCount + 10] = ((i << 2) | 3u);
    //                 chunkData->tri_line_indices[chunkData->tri_line_indexCount + 11] = ((i << 2) | 1u);
    //         }

    //         chunkData->tri_indexCount += 6;
    //         chunkData->tri_line_indexCount += 12;
    //     }

    // if (chunkData->pos_color_vertexCount > 0) {
    //         uint32_t vertexCount = static_cast<uint32_t>(chunkData->pos_color_vertexCount);
    //         const bgfx::Memory* vertexMem = bgfx::copy(chunkData->pos_color_vertices, vertexCount * sizeof(Vertex));
            
    //         bgfx::update(chunkData->tri_dvbh, 0, vertexMem);
    //     }

    //     if (chunkData->tri_indexCount > 0) {
    //         uint32_t indexCount = static_cast<uint32_t>(chunkData->tri_indexCount);
    //         const bgfx::Memory* indexMem = bgfx::copy(chunkData->tri_indices, indexCount * sizeof(uint32_t));
            
    //         bgfx::update(chunkData->tri_dibh, 0, indexMem);
    //     }
        
    //     if (chunkData->tri_line_indexCount > 0) {
    //         uint32_t lineIndexCount = static_cast<uint32_t>(chunkData->tri_line_indexCount);
    //         const bgfx::Memory* indexMem = bgfx::copy(chunkData->tri_line_indices, lineIndexCount * sizeof(uint32_t));
            
    //         bgfx::update(chunkData->tri_line_dibh, 0, indexMem);
    //     }
    //         // Update the index buffer if needed
    //     // bgfx::update(tri_dibh, 0, bgfx::copy(tri_indices.data(), tri_indices.size() * sizeof(uint32_t)));
    //     // bgfx::update(tri_dvbh, 0, bgfx::copy(pos_color_vertices.data(), pos_color_vertices.size() * sizeof(PosColorVertex)));

    // }



    vertexAOstruct print_vertexao_pulled_vertices_correct_faces(FaceData facedata, int vertexID){
        //lets do this for face 0 which we want to change to Z+
        ivec3 iVertexPos = ivec3_create(facedata.faceInfo1 & 0x3F, (facedata.faceInfo1 >> 6) & 0x3F, (facedata.faceInfo1 >> 12) & 0x3F);
        int w = ((facedata.faceInfo1 >> 18) & 0x3F);
        int h = ((facedata.faceInfo1 >> 24) & 0x3F);

        bool flipped = ((facedata.faceInfo1 >> 30) & 0x1);

        uint8_t packedAO = facedata.faceInfo2 & 0xFF;
        int face = ((facedata.faceInfo2 >> 29) & 0x07);
        printf("face: %d, flipped: %d\n", face, flipped);
        uint32_t wDir = face >> 2;
        uint32_t hDir = 1 + ((face >> 1) & 1) + ((face >> 2) & 1);

        int wMod = vertexID >> 1;
        int hMod = vertexID & 1;

        iVertexPos.e[wDir] += (w * wMod * flipLookup[face]);
        iVertexPos.e[hDir] += (h * hMod);

        vec3 vertexPos = vec3_create(iVertexPos);
        vertexPos.e[wDir] += (float)0.0007 * flipLookup[face] * (wMod * 2 - 1);
        vertexPos.e[hDir] += (float)0.0007 * (hMod * 2 - 1);
        vertexAOstruct vertexao = {};
        vertexao.pos = vertexPos;
        uint32_t ao = (packedAO >> (vertexID * 2)) & 0x03;

        float clampedAO = (float)fclamp(float(ao) / 3.0, 0.5, 1.0);

        vertexao.ao = clampedAO;
        
        return vertexao;
    }


    // void convert_binary_mesh_faces_to_test_quads(chunk_data* chunkData){
    //     chunkData->pos_color_vertexCount = 0;
    //     chunkData->quadCount = 0;
    //     chunkData->tri_indexCount = 0;
    //     //we want to extract the vertices and faces
    //     // for (const auto& packed_vertex : *(chunkData->vertices)) {
    //     for (size_t i = 0; i < chunkData->faceCounts[0]; i++) {
    //         const auto& quad = (chunkData->faces[0])[i];
    //         //unpacking 4 at a time because 
        


    //         vertexAOstruct vert0, vert1, vert2, vert3;
    //         vert0 = print_vertexao_pulled_vertices_correct_faces(quad, 0);
    //         vert1 = print_vertexao_pulled_vertices_correct_faces(quad, 1); //need to swap the order to get draw order correct
    //         vert2 = print_vertexao_pulled_vertices_correct_faces(quad, 2);
    //         vert3 = print_vertexao_pulled_vertices_correct_faces(quad, 3);
    //         printf("Vertex %d: (%f, %f, %f), ao: %f\n", 0, vert0.pos.x, vert0.pos.y, vert0.pos.z, vert0.ao);
    //         printf("Vertex %d: (%f, %f, %f), ao: %f\n", 1, vert1.pos.x, vert1.pos.y, vert1.pos.z, vert1.ao);
    //         printf("Vertex %d: (%f, %f, %f), ao: %f\n", 2, vert2.pos.x, vert2.pos.y, vert2.pos.z, vert2.ao);
    //         printf("Vertex %d: (%f, %f, %f), ao: %f\n", 3, vert3.pos.x, vert3.pos.y, vert3.pos.z, vert3.ao);

    //         bool flipped  = (quad.faceInfo1 >> 30) & 0x01;
    //         uint32_t face = (quad.faceInfo2 >> 16) & 0x07;
    //         bool faceNotZP = face & 7;
    //         bool faceNotZPorXP = (face & 0x3) != 0;
    //         // printf("curr face is not Z?: %d face: %d\n", faceNotZPorXP, face);
            
    //         if(faceNotZPorXP){ //we want to change the order of flipped Y faces
    //             flipped = !flipped;
    //         }

    //         chunkData->flipped_quads[chunkData->quadCount] = (flipped);

    //         vec3 baseColor = vec3_create(1.0f, 1.0f, 1.0f);//white
    //         // if(face == 4 || face == 5)baseColor = vec3_create(0.9f, 1.0f, 0.9f);//light green
        
            
    //         uint32_t v0color = getColor(baseColor * vert0.ao);
    //         uint32_t v1color = getColor(baseColor * vert1.ao);
    //         uint32_t v2color = getColor(baseColor * vert2.ao);
    //         uint32_t v3color = getColor(baseColor * vert3.ao);
    //         Vertex vertex0 = {};
    //         vertex0.position.x = vert0.pos.x;
    //         vertex0.position.y = vert0.pos.y;
    //         vertex0.position.z = vert0.pos.z;
    //         vertex0.color = v0color;

    //         Vertex vertex1 = {};
    //         vertex1.position.x = vert1.pos.x;
    //         vertex1.position.y = vert1.pos.y;
    //         vertex1.position.z = vert1.pos.z;
    //         vertex1.color = v1color;

    //         Vertex vertex2 = {};
    //         vertex2.position.x = vert2.pos.x;
    //         vertex2.position.y = vert2.pos.y;
    //         vertex2.position.z = vert2.pos.z;
    //         vertex2.color = v2color;

    //         Vertex vertex3 = {};
    //         vertex3.position.x = vert3.pos.x;
    //         vertex3.position.y = vert3.pos.y;
    //         vertex3.position.z = vert3.pos.z;
    //         vertex3.color = v3color;

    //         chunkData->pos_color_vertices[(chunkData->quadCount * 4) + 0] = vertex0;
    //         chunkData->pos_color_vertices[(chunkData->quadCount * 4) + 1] = vertex1;
    //         chunkData->pos_color_vertices[(chunkData->quadCount * 4) + 2] = vertex2;
    //         chunkData->pos_color_vertices[(chunkData->quadCount * 4) + 3] = vertex3;
    //         chunkData->quadCount++;
    //         chunkData->pos_color_vertexCount+= 4;
    //     }

    //     update_tri_buffers(chunkData);
    // }

    // //////////////////////////////////////////////////// DUMMY VERTEX PULLER END ////////////////////////////////////////////////////






// void create_packed_quad_test_buffers(chunk_data* chunkData){
//     bgfx::VertexLayout layout;
//     //4 bytes per vertex
//     layout.begin()
//         .add(bgfx::Attrib::Position, 1, bgfx::AttribType::Float, false)
//         .end();

//     if(chunkData->vertexCount > 0){
//         const void* data = chunkData->vertices;
//         uint32_t sizeInBytes = static_cast<uint32_t>(chunkData->vertexCount * sizeof(uint32_t));
//         const bgfx::Memory* vertexMem = bgfx::copy(data, sizeInBytes);
//         // Create dynamic vertex buffer
        
//         chunkData->packed_quadTest_dvbh = bgfx::createDynamicVertexBuffer(
//             vertexMem,
//             layout,
//             BGFX_BUFFER_ALLOW_RESIZE
//         );

//     }

    

// }




// void update_packed_quad_test_buffers(chunk_data* chunkData) {
//     bgfx::VertexLayout layout;
//     //4 bytes per vertex
//     layout.begin()
//         .add(bgfx::Attrib::Position, 1, bgfx::AttribType::Float, false)
//         .end();
//     // Check if the buffer handle is valid
//     if (!bgfx::isValid(chunkData->packed_quadTest_dvbh)) {
//         // If the buffer doesn't exist, create it
//         create_packed_quad_test_buffers(chunkData);
//         // return;
//     }


//     // Get the pointer to the vector's data and calculate the size in bytes
//     const void* data = chunkData->vertices;

//     uint32_t sizeInBytes = static_cast<uint32_t>(chunkData->vertexCount * sizeof(uint32_t));
//     if(chunkData->vertexCount > 0){
//         // Check if we need to resize the buffer
//         if (sizeInBytes > bgfx::getAvailTransientVertexBuffer((uint32_t)chunkData->vertexCount,layout)) {

            
//             // Resize the buffer
//             bgfx::destroy(chunkData->packed_quadTest_dvbh);
//             const bgfx::Memory* vertexMem = bgfx::copy(data, sizeInBytes);
//             chunkData->packed_quadTest_dvbh = bgfx::createDynamicVertexBuffer(
//                 vertexMem,
//             layout,
//                 BGFX_BUFFER_ALLOW_RESIZE
//             );
//         } else {
//             // Update the existing buffer
//             bgfx::update(chunkData->packed_quadTest_dvbh, 0, bgfx::copy(data, sizeInBytes));
//         }
//     }

// }



