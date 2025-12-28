#pragma once

// void camera_key_inputs(player_input* currInput, fpt_vec3 target, fpt_vec3 up, fpt& speed, fpt_vec3& pos_in_chunk, ivec3& chunk_coords, bool* inNewChunk = nullptr, bool toroidal_space_enabled = false);
// void RotateCameraEntity(fpt_quat& entityRotation, fpt_vec3& target, fpt_vec3& up, fpt& angleH, fpt& angleV, int16_t deltaX, int16_t deltaY);

   
struct CameraComp{
    vec3 m_rotation;
    vec3 m_target;
    vec3 m_up;
    // vec3 m_position = vec3_create(0.0f);
    // vec3 m_positionInChunk = vec3_create(0.0f);
    ivec3 chunk_coords;

    fpt_vec3 fptRotation;
    fpt_vec3 fptTarget;
    fpt_vec3 fptUp;

    fpt_quat entityRotation;

    fpt_vec3 freeTarget;
    fpt_vec3 freeUp;
    fpt_vec3 freePos;

    fpt_vec3 tempTarget;
    fpt_vec3 tempUp;
    fpt_vec3 tempPos;
    fpt_vec3 temppos_in_chunk;
    ivec3 temp_chunk_coords;
    fpt_quat tempRot;
    fpt tempAngleH;
    fpt tempAngleV;

    fpt third_person_offset;//distance from entity in third person mode

    // fpt_vec3 fptPos;
    fpt_vec3 fptLocalPos;
    fpt_vec3 pos_in_chunk;
    fpt fptSpeed;

    float nearPlane = 0.1f; // clipping plane
    float farPlane = 150.0f; // clipping plane
    float fovDegrees = 90.0f;

    float m_speed = 1.0f;
    bool inNewChunk;
    bool needsUpdate;

    float m_AngleH;
    float m_AngleV;

    fpt fptAngleH;
    fpt fptAngleV;

    //passed in from the gamestate, which points to the game memory location
    u32* windowWidth;
    u32* windowHeight;
  
    bool freeMode;
    bool toroidal_space_enabled;
    bool is_main_camera;
    vec2 mousePos;
    vec2 m_lastMouseDelta;

    vec3 frustumAABBMin;
    vec3 frustumAABBMax;
    
    fpt_vec3 fptFrustumAABBMin;
    fpt_vec3 fptFrustumAABBMax;

    mat4 viewMatrix;
    mat4 projectionMatrix;

    mat4 viewProjMatrix;    
    
    mat4 invViewMatrix;
    mat4 invProjMatrix;

    mat4 invViewProjMatrix;


    mat4 tempViewMatrix;
    mat4 tempProjectionMatrix;

    vec3 rayWorld;
};

  

        void RotateCameraEntity(fpt_quat& entityRotation, fpt_vec3& target, fpt_vec3& up, fpt& angleH, fpt& angleV, int16_t deltaX, int16_t deltaY);
        void init_camera_comp(game_state* GameState, CameraComp& camera, fpt_vec3 pos = fpt_vec3_create(0));

        void SetPosition(CameraComp& camera, float x, float y, float z);
        void SetRotation(CameraComp& camera, float x, float y, float z);
        void SetProjectionMatrix(CameraComp& camera);

        bool OnKeyboardEvent(player_input* prevInput, player_input* currInput, CameraComp& camera);

        void WindowResize(CameraComp& camera);
        void OnMouseEvent(CameraComp& camera, uint16_t mouseX, uint16_t mouseY, int16_t deltaX, int16_t deltaY);
        void OnMouseClickEvent(CameraComp& camera, int button);
        void SetMousePosition(CameraComp& camera, float x, float y);
        void OnMouseHeld(CameraComp& camera);
        void OnMouseRelease(CameraComp& camera);
        void OnRender(CameraComp& camera);
        void Init(game_state* GameState, CameraComp& camera);
        void setFOV(CameraComp& camera, float newFov);
        void setNearPlane(CameraComp& camera, float newNearPlane);
        void setFarPlane(CameraComp& camera, float newFarPlane);


        void RebaseLocalCameraPosition(CameraComp& camera);
        vec3 GetPointInFront(CameraComp& camera, float distance = 1.0f);
        vec3 CastMouseRay(CameraComp& camera, int x, int y);

        void UpdateMatrix(CameraComp& camera);
        void UpdateAngle(CameraComp& camera);
        void Translate(CameraComp& camera, vec3 newPosition);
        void Translate(CameraComp& camera, float x, float y, float z);
        void Rotate(CameraComp& camera, float x, float y, float z);


    
