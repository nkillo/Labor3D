#include "camera.h"
// #include "WorldTransform.h"
// #include "labour/collision/chunkManager.h"







static inline void debug_rebasePosition(vec3& pos_in_chunk){

    pos_in_chunk.x = debug_adjustToChunkRange(pos_in_chunk.x);
    pos_in_chunk.y = debug_adjustToChunkRange(pos_in_chunk.y);
    pos_in_chunk.z = debug_adjustToChunkRange(pos_in_chunk.z);
  }


static inline void rebasePosition(fpt_vec3& pos_in_chunk, ivec3& chunk_coords, bool* inNewChunk = nullptr,  bool toroidal_space_enabled = false){
    // ivec3 newchunk_coords = calculateFPTChunkCoordinates(pos_in_chunk);
    // if(newchunk_coords != chunk_coords){
        // printf("old chunk coords: %d %d %d", chunk_coords.x,       chunk_coords.y,      chunk_coords.z);
        // printf("new chunk coords: %d %d %d", newchunk_coords.x,    newchunk_coords.y,   newchunk_coords.z);
    // }
        // Adjust positions to be within the -31 to 31 range


    fpt_vec3 temp_pos = pos_in_chunk;
    // if(fpt_abs(temp_pos.x) > (FPT_CHUNK_SIZE + FPT_HALF_CHUNK_SIZE)){
    //     printf("X POS IN CHUNK TOO LARGE: %d", temp_pos.x);
    // }
    // if(fpt_abs(temp_pos.y) > (FPT_CHUNK_SIZE + FPT_HALF_CHUNK_SIZE)){
    //     printf("Y POS IN CHUNK TOO LARGE: %d", temp_pos.y);
    // }
    // if(fpt_abs(temp_pos.z) > (FPT_CHUNK_SIZE + FPT_HALF_CHUNK_SIZE)){
    //     printf("Z POS IN CHUNK TOO LARGE: %d", temp_pos.z);
    // }

    // Adjust positions to be within the -31 to 31 range
    pos_in_chunk.x = adjustToChunkRange(pos_in_chunk.x);
    pos_in_chunk.y = adjustToChunkRange(pos_in_chunk.y);
    pos_in_chunk.z = adjustToChunkRange(pos_in_chunk.z);
    
    if(temp_pos != pos_in_chunk){
        // if(toroidal_space_enabled)return;//TOROIDAL SPACE TEST
        // inNewChunk = true;
        // ivec3 newchunk_coords = vec3_create(0);
        // if      (temp_pos.x > pos_in_chunk.x)newchunk_coords.x =  1; //old pos is greater than, +1
        // else if (temp_pos.x < pos_in_chunk.x)newchunk_coords.x = -1;
        // if      (temp_pos.y > pos_in_chunk.y)newchunk_coords.y =  1;
        // else if (temp_pos.y < pos_in_chunk.y)newchunk_coords.y = -1;
        // if      (temp_pos.z > pos_in_chunk.z)newchunk_coords.z =  1;
        // else if (temp_pos.z < pos_in_chunk.z)newchunk_coords.z = -1;
        // chunk_coords += newchunk_coords;
        if (toroidal_space_enabled) return; // TOROIDAL SPACE TEST

        if(inNewChunk) *inNewChunk = true;

        // Calculate the number of chunks traversed in each direction
        ivec3 chunkDelta = {};

        if (temp_pos.x != pos_in_chunk.x) {
            fpt tempStep1 = fpt_div(pos_in_chunk.x, FPT_CHUNK_SIZE);
            fpt tempStep2 = fpt_floor(tempStep1);
            fpt tempPos         = fpt2i(fpt_floor(fpt_div(temp_pos.x, FPT_CHUNK_SIZE)));
            fpt tempPosInChunk  = fpt2i(fpt_floor(fpt_div(pos_in_chunk.x, FPT_CHUNK_SIZE)));
            chunkDelta.x = fpt2i(fpt_floor(fpt_div(temp_pos.x, FPT_CHUNK_SIZE))) - 
                           fpt2i(fpt_floor(fpt_div(pos_in_chunk.x, FPT_CHUNK_SIZE)));
        }
        if (temp_pos.y != pos_in_chunk.y) {
            chunkDelta.y = fpt2i(fpt_floor(fpt_div(temp_pos.y, FPT_CHUNK_SIZE))) - 
                           fpt2i(fpt_floor(fpt_div(pos_in_chunk.y, FPT_CHUNK_SIZE)));
        }
        if (temp_pos.z != pos_in_chunk.z) {
            chunkDelta.z = fpt2i(fpt_floor(fpt_div(temp_pos.z, FPT_CHUNK_SIZE))) - 
                           fpt2i(fpt_floor(fpt_div(pos_in_chunk.z, FPT_CHUNK_SIZE)));
        }

        // Update the chunk coordinates
        chunk_coords.x += chunkDelta.x;
        chunk_coords.y += chunkDelta.y;
        chunk_coords.z += chunkDelta.z;

        // Debugging output
        // printf("Chunk Delta: (%d, %d, %d)\n", chunkDelta.x, chunkDelta.y, chunkDelta.z);

    }


    vec3 postModPos = fpt_to_flt_vec3(pos_in_chunk);
    printf("post mod position : %f %f %f\n", postModPos.x, postModPos.y, postModPos.z);

  }



void entity_update_view_angles(fpt& angleH, fpt& angleV, int16_t deltaX, int16_t deltaY){
    angleH = fpt_add(angleH, fpt_div(i2fpt(-deltaX), 1310720)); //20.0f to fpt
    angleV = fpt_add(angleV, fpt_div(i2fpt(deltaY), 3276800)); //50.0f to fpt
    angleV = fpt_clamp(angleV, -5242880, 5242880);//5242880 = 80 degrees

}


void entity_camera_key_inputs(player_input* currInput, fpt_vec3 target, fpt_vec3 up, fpt& speed, fpt_vec3& pos_in_chunk, ivec3& chunk_coords, bool* inNewChunk, bool toroidal_space_enabled){
    fpt_vec3 translation = fpt_vec3_create(0, 0, 0);
    if (currInput->bits.forward) {
    translation = translation + fpt_vec3_normalize(target);
    }
    if (currInput->bits.back) {
    translation = translation - fpt_vec3_normalize(target);
    }
    if (currInput->bits.left) {
    fpt_vec3 fptLeft = fpt_vec3_normalize(fpt_vec3_cross(up, target));
    translation = translation + fptLeft;
    }
    if (currInput->bits.right) {
    fpt_vec3 fptRight = fpt_vec3_normalize(fpt_vec3_cross(target, up));
    translation = translation + fptRight;
    }
    if (currInput->bits.up) {
    translation = translation + fpt_vec3_create(0, FPT_ONE, 0);
    }
    if (currInput->bits.down) {
    translation = translation - fpt_vec3_create(0, FPT_ONE, 0);
    }
    if (!(translation.x == 0 && translation.y == 0 && translation.z == 0)) {
    translation = fpt_vec3_normalize(translation);
    translation = fpt_vec3_mul_scalar(translation, speed);
    pos_in_chunk = fpt_vec3_add(pos_in_chunk, translation);
    //   vec3 testPos = fpt_to_flt_vec3_create(pos_in_chunk);
    pos_in_chunk = fpt_vec3_add(pos_in_chunk, translation);
    // printf("camPos: %f %f %f\n", fpt2fl(pos_in_chunk.x),fpt2fl(pos_in_chunk.y),fpt2fl(pos_in_chunk.z));
    rebasePosition(pos_in_chunk, chunk_coords, inNewChunk, toroidal_space_enabled);
    }
    if(currInput->function.equals){ //equals is also plus
        // camera.m_speed += 0.01f;
        // camera.m_speed += 0.00999450683f;
        speed += 655;
        printf("Speed changed to: %f\n", fpt2fl(speed));
    }
    if(currInput->function.minus){
            // camera.m_speed -= 0.00999450683f;
        speed -= 655;
        if(speed <  655){
            // camera.m_speed = 0.00999450683f;
            speed = 655;
        }
        printf("Speed changed to: %f\n", fpt2fl(speed));
    }

}

void RotateCameraEntity(fpt_quat& entityRotation, fpt_vec3& target, fpt_vec3& up, fpt& angleH, fpt& angleV, int16_t deltaX, int16_t deltaY){
    
    entity_update_view_angles(angleH, angleV, deltaX, deltaY);

    fpt_vec3 fptYAxis = fpt_vec3_create(0, FPT_ONE, 0);
    fpt_vec3 fptView = fpt_vec3_create(FPT_ONE, 0, 0);
    fpt_quat fptQuatH = fpt_quat_from_axis_angle(fptYAxis, fpt_radians(angleH));
    
    fptView = fpt_quat_rotate_vec3(fptQuatH, fptView);
    fptView = fpt_vec3_normalize(fptView);
    fpt_vec3 fptU = fpt_vec3_cross(fptYAxis, fptView);
    fptU = fpt_vec3_normalize(fptU);
    
    fpt_quat fptQuatV = fpt_quat_from_axis_angle(fptU, fpt_radians(angleV));
    fptView = fpt_quat_rotate_vec3(fptQuatV, fptView);

    target = fptView;
    target = fpt_vec3_normalize(target);
    up = fpt_vec3_cross(target, fptU);
    up = fpt_vec3_normalize(up);

    entityRotation = fpt_quat_LookAtRH(target, up);

}


void init_camera_comp(game_state* GameState, CameraComp& camera, fpt_vec3 pos){

    camera.m_rotation = {};
    camera.fptSpeed = 65536;
    camera.nearPlane = 0.1f; // clipping plane
    camera.farPlane = 150.0f; // clipping plane
    camera.fovDegrees = 90.0f;

    camera.needsUpdate = true;
    camera.inNewChunk = true;
    
    camera.toroidal_space_enabled = false;
    camera.frustumAABBMin = {};
    camera.frustumAABBMax = {};

    camera.fptFrustumAABBMin = fpt_vec3_create(0,0,0);
    camera.fptFrustumAABBMax = fpt_vec3_create(0,0,0);

    camera.viewMatrix = mat4_identity();
    camera.projectionMatrix = mat4_identity();
    
    camera.pos_in_chunk = pos;
    
    camera.third_person_offset = FPT_MAX_CAMERA_ZOOM;
     
    rebasePosition(camera.pos_in_chunk, camera.chunk_coords, &camera.inNewChunk);

    camera.m_target = vec3_normalize(vec3_create(0.0f, 0.0f, -1.0f));
    
    camera.fptTarget = fpt_vec3_create(0, 0, -65536); //-1 * 2^16
    camera.fptUp = fpt_vec3_create(0, 65536, 0); //1 * 2^16
    camera.fptRotation = fpt_vec3_create(0, 0, 0);
    
    camera.freeTarget = fpt_vec3_create(0, -65536, 0);
    camera.freeUp = fpt_vec3_create(0, 0, -65536);

    // m_target = normalize(vec3_create(0.0f, 0.0f, 0.0f));
    camera.m_up =       vec3_create(0.0f, 1.0f, 0.0f);
    
    camera.m_speed = 1.0f;
    camera.windowWidth  = GameState->window_width;
    camera.windowHeight = GameState->window_height;



    Init(GameState, camera);

    UpdateMatrix(camera);
    SetProjectionMatrix(camera);

    camera.temppos_in_chunk = camera.pos_in_chunk;
    camera.temp_chunk_coords = camera.chunk_coords;
}




void SetProjectionMatrix(CameraComp& camera){
    #ifdef SERVER_BUILD
        return;
    #endif
    float aspectRatio = (float)*camera.windowWidth / (float)*camera.windowHeight;

    float fov = camera.fovDegrees * 0.01745329f; // convert 67 degrees to radians //one degree in radians is 0.01745329

    // printf("aspect ratio: %f, width: %u, height: %u\n", aspectRatio, *camera.windowWidth, *camera.windowHeight);
    mat4_perspective(&camera.projectionMatrix, fov, aspectRatio, camera.nearPlane, camera.farPlane);
    camera.viewProjMatrix = mat4_mul(camera.projectionMatrix, camera.viewMatrix);

    //calculate inverse
    // mat4_inverse(camera.projectionMatrix.m, camera.invProjMatrix.m);
    // mat4_inverse(camera.viewProjMatrix.m, camera.invViewProjMatrix.m);
    stable_mat4_inverse(&camera.projectionMatrix, &camera.invProjMatrix);
    stable_mat4_inverse(&camera.viewProjMatrix,   &camera.invViewProjMatrix);

    
    // test_mat4_inverse(&camera.invProjMatrix, &camera.projectionMatrix);
    // test_mat4_inverse(&camera.invViewProjMatrix, &camera.viewProjMatrix);


    if(camera.freeMode)return;


}



bool OnKeyboardEvent(player_input* prevInput, player_input* currInput, CameraComp& camera) {
    //calling translate updates the matrix
    bool camera_dirty = false;
    if((currInput->buttons || currInput->mouseButtons || currInput->functionKeys)){
        //CHANGE BACK ONCE WE ADD IN ENTITY
#if DEBUG_CAMERA
        if(!camera.freeMode){
#else
        if(camera.freeMode){
#endif
            entity_camera_key_inputs(currInput, camera.fptTarget, camera.fptUp, camera.fptSpeed, camera.pos_in_chunk, camera.chunk_coords, &camera.inNewChunk, camera.toroidal_space_enabled);
            UpdateMatrix(camera);
            
            camera_dirty = true;
            
        }else{
            // empty_event event;
            // add_camera_position_updated_event(camera.disp, event);
            // add_camera_updated_event(camera.disp, event);

        }

    }


    
    if(WAS_PRESSED((*currInput), flags.freeCam, InputTypes::input_key_Q )){
        camera.freeMode = !camera.freeMode;
        if(camera.freeMode){
            camera.freeMode = true;
            camera.tempTarget = camera.fptTarget;
            camera.tempUp = camera.fptUp;
            // camera.tempPos = camera.fptPos;
            camera.temppos_in_chunk = camera.pos_in_chunk;
            camera.temp_chunk_coords = camera.chunk_coords;
            camera.tempAngleH = camera.fptAngleH;
            camera.tempAngleV = camera.fptAngleV;
            // camera.tempRot = camera.entityRotation;
            
            camera.tempViewMatrix = camera.viewMatrix;
            camera.tempProjectionMatrix = camera.projectionMatrix;
            camera_dirty = true;
            
            // camera.fptTarget = camera.freeTarget;
            // camera.fptUp = camera.freeUp;
            // camera.fptPos = camera.freePos;
            // camera.pos_in_chunk = camera.freePos;
            // camera.fptAngleH = i2fpt(90);
            // camera.fptAngleV = i2fpt(90);
            // // RebaseLocalCameraPosition(camera);
            // rebasePosition(camera.fptPos, camera.pos_in_chunk, camera.chunk_coords, camera.inNewChunk);

            // UpdateMatrix(camera);
            // SetProjectionMatrix(camera);

        }

        else{//set camera position back to what it was before going to free mode
            camera.freeMode = false;
            camera.fptTarget = camera.tempTarget;
            camera.fptUp = camera.tempUp;
            // camera.fptPos = camera.tempPos;
            camera.pos_in_chunk = camera.temppos_in_chunk;
            camera.chunk_coords = camera.temp_chunk_coords;

            camera.fptAngleH = camera.tempAngleH;
            camera.fptAngleV = camera.tempAngleV;
            // camera.entityRotation = camera.tempRot;
            rebasePosition(camera.pos_in_chunk, camera.chunk_coords, &camera.inNewChunk, camera.toroidal_space_enabled);
            UpdateMatrix(camera);
            SetProjectionMatrix(camera);
            camera_dirty = true;

        }
    }
    return camera_dirty;
}




void Rotate(CameraComp& camera, float x, float y, float z){
    camera.m_rotation += vec3_create(x, y, z);

    // Reset rotation values if they exceed 360 degrees
    camera.m_rotation.x = (camera.m_rotation.x >= 360.0f) ? 0.0f : camera.m_rotation.x;
    camera.m_rotation.y = (camera.m_rotation.y >= 360.0f) ? 0.0f : camera.m_rotation.y;
    camera.m_rotation.z = (camera.m_rotation.z >= 360.0f) ? 0.0f : camera.m_rotation.z;

    UpdateMatrix(camera);

}

void setFOV(CameraComp& camera, float newFov){
    camera.fovDegrees = newFov;
    SetProjectionMatrix(camera);
}

void setNearPlane(CameraComp& camera, float newNearPlane){
    camera.nearPlane = newNearPlane;
    SetProjectionMatrix(camera);
}

void setFarPlane(CameraComp& camera, float newFarPlane){
    camera.farPlane = newFarPlane;
    SetProjectionMatrix(camera);
}




void UpdateMatrix(CameraComp& camera) {
    #ifdef SERVER_BUILD
        return;
    #endif
    // vec3 targetPoint = m_position + m_target; // Target as a point
    
    //lookAt expects a target point, not a target vector
    // printf("target point: %f, %f, %f\n", targetPoint.x, targetPoint.y, targetPoint.z);
    // viewMatrix = lookAt(m_position, targetPoint, m_up);
    //for camera relative rendering, we don't use the targetPoint, we just use the m_target direction vector
    // viewMatrix = lookAt(vec3_create(0.0f, 0.0f, 0.0f), targetPoint, m_up);
    // camera.viewMatrix = lookAt(vec3_create(0.0f, 0.0f, 0.0f), camera.m_target, camera.m_up);
    vec3 target = fpt_to_flt_vec3(camera.fptTarget);
    vec3 up = fpt_to_flt_vec3(camera.fptUp);
    mat4_lookAt(&camera.viewMatrix, vec3_create(0.0f, 0.0f, 0.0f), target, up);

    camera.viewProjMatrix = mat4_mul(camera.projectionMatrix, camera.viewMatrix);

    //calculate inverse
    // mat4_inverse(camera.viewMatrix.m, camera.invViewMatrix.m);
    // mat4_inverse(camera.viewProjMatrix.m, camera.invViewProjMatrix.m);
    stable_mat4_inverse(&camera.viewMatrix,     &camera.invViewMatrix);
    stable_mat4_inverse(&camera.viewProjMatrix, &camera.invViewProjMatrix);
    
    // test_mat4_inverse(&camera.invViewMatrix, &camera.viewMatrix);
    // test_mat4_inverse(&camera.invViewProjMatrix, &camera.viewProjMatrix);



    // quat glmRotation = quatLookAt(normalize(target), up);

    //old entity rotation code        
    // if(!camera.freeMode)camera.entityRotation = fpt_quat_LookAtRH(camera.fptTarget, camera.fptUp);


    // glmRotation = fpt_quat_LookAtRH(camera.fptTarget, camera.fptUp);

    // camera.fptTotalRotation = glm_to_fpt_quat(glmRotation);

    if(camera.freeMode)return;

    camera.temppos_in_chunk = camera.pos_in_chunk;
    camera.temp_chunk_coords = camera.chunk_coords;
    // empty_event event;
    // add_view_matrix_updated_event(camera.disp, event);
    // add_camera_updated_event(camera.disp, event);

}

void UpdateAngle(CameraComp& camera){

    char buf[16*4];
    int offset = 16;

    fpt_vec3 fptYAxis = fpt_vec3_create(0, 65536, 0);
    fpt_vec3 fptView = fpt_vec3_create(65536, 0, 0);
    fpt_quat fptQuatH = fpt_quat_from_axis_angle(fptYAxis, fpt_radians(camera.fptAngleH));
    
    // fpt_quat_str(fptQuatH, buf, offset);
    // printf("fptQuatH: %s %s %s %s\n", buf + (offset * 0),buf + (offset * 1), buf + (offset * 2),buf + (offset * 3));



    fptView = fpt_quat_rotate_vec3(fptQuatH, fptView);
    
    // fpt_vec3_str(fptView, buf, offset);
    // printf("fptView: %s %s %s\n", buf + (offset * 0),buf + (offset * 1), buf + (offset * 2));


    fptView = fpt_vec3_normalize(fptView);
    // fpt_vec3_str(fptView, buf, offset);
    // printf("fptView normalized: %s %s %s\n", buf + (offset * 0),buf + (offset * 1), buf + (offset * 2));


    fpt_vec3 fptU = fpt_vec3_cross(fptYAxis, fptView);
    // fpt_vec3_str(fptU, buf, offset);
    // printf("fptU cross: %s %s %s\n", buf + (offset * 0),buf + (offset * 1), buf + (offset * 2));


    fptU = fpt_vec3_normalize(fptU);

    // fpt_vec3_str(fptU, buf, offset);
    // printf("fptU normalized: %s %s %s\n", buf + (offset * 0),buf + (offset * 1), buf + (offset * 2));



    fpt_quat fptQuatV = fpt_quat_from_axis_angle(fptU, fpt_radians(camera.fptAngleV));
    // fpt_quat_str(fptQuatV, buf, offset);
    // printf("fptQuatV: %s %s %s %s\n", buf + (offset * 0),buf + (offset * 1), buf + (offset * 2),buf + (offset * 3));


    fptView = fpt_quat_rotate_vec3(fptQuatV, fptView);

    // fpt_vec3_str(fptView, buf, offset);
    // printf("fptView rotated: %s %s %s\n", buf + (offset * 0),buf + (offset * 1), buf + (offset * 2));

    camera.fptTarget = fptView;
    // fpt_vec3_str(camera.fptTarget, buf, offset);
    // printf("fptTarget: %s %s %s\n", buf + (offset * 0),buf + (offset * 1), buf + (offset * 2));

    camera.fptTarget = fpt_vec3_normalize(camera.fptTarget);
    // fpt_vec3_str(camera.fptTarget, buf, offset);
    // printf("fptTarget normalized: %s %s %s\n", buf + (offset * 0),buf + (offset * 1), buf + (offset * 2));

    camera.fptUp = fpt_vec3_cross(camera.fptTarget, fptU);
    // fpt_vec3_str(camera.fptUp, buf, offset);
    // printf("fptTarget cross: %s %s %s\n", buf + (offset * 0),buf + (offset * 1), buf + (offset * 2));

    camera.fptUp = fpt_vec3_normalize(camera.fptUp);
    // fpt_vec3_str(camera.fptUp, buf, offset);
    // printf("fptTarget normalized: %s %s %s\n", buf + (offset * 0),buf + (offset * 1), buf + (offset * 2));

    // vec3 Yaxis = vec3_create(0.0f, 1.0f, 0.0f);
    
    // Rotate the view vector by the horizontal angle around the vertical axis
    // vec3 View = vec3_create(1.0f, 0.0f, 0.0f);

    // Create a quaternion for the horizontal angle rotation around the Y axis
    //printf("m_AngleH: %f\n", m_AngleH);
    // float radians = radians(camera.m_AngleH);
    // quat quaternionH = angleAxis(radians, Yaxis);
    // printf("quatH: %f %f %f %f\n", quaternionH.x,quaternionH.y,quaternionH.z,quaternionH.w);

    // Apply the quaternion rotation to the view vector
    // View = quaternionH * View; // Operator overloading applies the rotation
    // printf("View: %f %f %f\n", View.x,View.y,View.z);

    // View = normalize(View);
    //printf("normalized View: %f, %f, %f\n", View.x, View.y, View.z);
    // printf("View normalized: %f %f %f\n", View.x,View.y,View.z);

    // Rotate the view vector by the vertical angle around the horizontal axis
    // vec3 U = cross(Yaxis,View); //is this the correct order?
    // printf("U: %f %f %f\n", U.x,U.y,U.z);
    
    // U = normalize(U);
    // printf("U normalized: %f %f %f\n", U.x,U.y,U.z);

    // Create a quaternion for the vertical angle rotation around the horizontal axis (U)
    //printf("m_AngleV: %f\n", m_AngleV);

    // quat quaternionV = angleAxis(radians(camera.m_AngleV), U);
    // printf("quatV: %f %f %f %f\n", quaternionV.x,quaternionV.y,quaternionV.z,quaternionV.w);

    // Apply the quaternion rotation to the view vector
    // View = quaternionV * View; // Operator overloading applies the rotation
    // printf("View: %f %f %f\n", View.x,View.y,View.z);
    

    // camera.m_target = View;
    // printf("View: %f %f %f\n", View.x,View.y,View.z);


    // camera.m_target = normalize(camera.m_target);
    // printf("camera.m_target normalized: %f %f %f\n", camera.m_target.x,camera.m_target.y,camera.m_target.z);

    // camera.m_up = cross(camera.m_target, U);
    // printf("camera.m_up cross: %f %f %f\n", camera.m_up.x,camera.m_up.y,camera.m_up.z);

    // camera.m_up = normalize(camera.m_up);
    // printf("camera.m_up normalized: %f %f %f\n", camera.m_up.x,camera.m_up.y,camera.m_up.z);

    UpdateMatrix(camera);

}

void Init(game_state* GameState, CameraComp& camera){
    vec3 HTarget = vec3_create(camera.m_target.x, 0.0, camera.m_target.z);
    HTarget = vec3_normalize(HTarget);
    float Angle = degrees(asin(fabs(HTarget.z)));

    fpt_vec3 fptHTarget = fpt_vec3_create(camera.fptTarget.x, 0, camera.fptTarget.z);
    fptHTarget = fpt_vec3_normalize(fptHTarget);
    fpt fptAngle = fpt_degrees(GameState->platform_fpt_asin(fpt_abs(fptHTarget.z)));

    if (HTarget.z >= 0.0f)
    {
        if (HTarget.x >= 0.0f)
        {
            camera.m_AngleH = 360.0f - Angle;
        }
        else
        {
            camera.m_AngleH = 180.0f + Angle;
        }
    }
    else
    {
        if (HTarget.x >= 0.0f)
        {
            camera.m_AngleH = Angle;
        }
        else
        {
            camera.m_AngleH = 180.0f - Angle;
        }
    }

    if (fptHTarget.z >= 0)
    {
        if (fptHTarget.x >= 0)
        {
            camera.fptAngleH = fpt_sub(23592960, fptAngle); //360 - angle
            
        }
        else
        {
            camera.fptAngleH = fpt_add(11796480, fptAngle); //180 + angle
        }
    }
    else
    {
        if (fptHTarget.x >= 0)
        {
            camera.fptAngleH = fptAngle;
        }
        else
        {
            camera.fptAngleH = fpt_sub(11796480, fptAngle); //180 - angle
        }
    }

    camera.m_AngleV = -degrees(asin(camera.m_target.y));
    camera.fptAngleV = -fpt_degrees(GameState->platform_fpt_asin(camera.fptTarget.y));

    camera.mousePos.x  = (float)*camera.windowWidth / 2;
    camera.mousePos.y  = (float)*camera.windowHeight / 2;
    //updateAngle updates the matrix
    UpdateAngle(camera);

}
void SetMousePosition(CameraComp& camera, float x, float y){
    camera.mousePos.x = x;
    camera.mousePos.y = y;
}


void WindowResize(CameraComp& camera){
    UpdateAngle(camera);
    SetProjectionMatrix(camera);
}

void OnMouseEvent(CameraComp& camera, uint16_t mouseX, uint16_t mouseY, int16_t deltaX, int16_t deltaY){
    //printf("mouse is down, offsets: (%d, %d)\n",x, y);

    //printf("Delta: (%d, %d)\n", DeltaX, DeltaY);
    // camera.m_mousePos.x = mouseX;
    // camera.m_mousePos.y = mouseY;

    // camera.m_AngleH += (float)-deltaX / 20.0f;
    // camera.m_AngleV += (float)deltaY / 50.0f;

    entity_update_view_angles(camera.fptAngleH, camera.fptAngleV, deltaX, deltaY);
    UpdateAngle(camera);

    SetProjectionMatrix(camera);

}


//TODO fix ray casting
// vec3 CastMouseRay(CameraComp& camera, int x, int y){
//     float rayX = (2.0f * x) / camera.m_windowWidth - 1.0f;
//     float rayY = 1.0f - (2.0f * y) / camera.m_windowHeight;
//     float rayZ = 1.0f;
//     vec3 mouseRay = vec3_create(rayX, rayY, rayZ);
//     vec4 rayClip = vec4_create(mouseRay.x, mouseRay.y, -1.0f, 1.0f);
//     vec4 rayEye = mat4_invert(camera.projectionMatrix) * rayClip;
//     rayEye = vec4(rayEye.x, rayEye.y, -1.0f, 0.0f); //unproject xy part, manually set zw to forwards and not a point
//     mat4 tempMat = 
//     return camera.rayWorld = vec3_normalize(mat4_invert(camera.viewMatrix) * rayEye);
// }


vec3 GetPointInFront(CameraComp& camera, float distance) {
    // m_target is already normalized, so we can use it directly
    return vec3_create(0.0f) + (camera.m_target * distance);
}


