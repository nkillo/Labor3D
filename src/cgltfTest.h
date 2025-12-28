#pragma once
#include "constants.h"

#define MODEL_DEBUGS 0

#if MODEL_DEBUGS
    #define MODEL_DEBUG_PRINTF(fmt, ...)            printf(fmt, ##__VA_ARGS__)
    #define MODEL_DEBUG_SPDLOG_INFO(fmt, ...)       spdlog::info(fmt, ##__VA_ARGS__)
    #define MODEL_DEBUG_PRINT_FPT_VEC3(string, v)   print_fpt_vec3(string, v)
#else
    #define MODEL_DEBUG_PRINTF(fmt, ...)            ((void)0)
    #define MODEL_DEBUG_SPDLOG_INFO(fmt, ...)       ((void)0)
    #define MODEL_DEBUG_PRINT_FPT_VEC3(string, v)   ((void)0)
#endif


#define NULL_ENTITY_MESH -1
#define MAX_ENTITY_NODES 1024
#define MAX_ENTITY_CHILDREN 16
#define MAX_KEYFRAMES 32
#define MAX_ANIMATIONS 32
#define MAX_MODELS 32


static inline const char* get_anim_type_name(animation_type type){
    switch (type){
      
        case anim_idle:                 return "anim_idle";
        case anim_walk:                 return "anim_walk";
        case anim_run:                  return "anim_run";
        case anim_jump:                 return "anim_jump";
        case anim_airborn:              return "anim_airborn";
        case anim_land:                 return "anim_land";
        case anim_attack:               return "anim_attack";
        case anim_cast:                 return "anim_cast";
        case anim_left_punch:           return "anim_left_punch";
        case anim_right_punch:          return "anim_right_punch";
        case anim_left_light_weapon:    return "anim_left_light_weapon";
        case anim_right_light_weapon:   return "anim_right_light_weapon";
        case anim_left_kick:            return "anim_left_kick";
        case anim_right_kick:           return "anim_right_kick";
        case anim_left_block:           return "anim_left_block";
        case anim_right_block:          return "anim_right_block";
        case anim_pull_bow:             return "anim_pull_bow";
        case anim_release_bow:          return "anim_release_bow";
        case anim_dodge_roll:           return "anim_dodge_roll";
    }
    return nullptr;
}

static inline const char* get_model_type_name(model_type type){
    switch (type){
      
        case model_none:            return "model_none";
        case model_humanoid:        return "model_humanoid";
        case model_sword:           return "model_sword";
        case model_shield:          return "model_shield";
    }
    return nullptr;
}

struct entity_node{
    vec3 translation;
    quat rotation;


    vec3 base_translation;
    quat base_rotation;


    vec3 relative_translation; //relative to base mesh orientation/position
    quat relative_rotation;

    vec3 min;
    vec3 max;

    vec3 relative_translation_min;
    vec3 relative_translation_max;

    int index_offset;
    int index_count;
    int vertex_offset;

    bool mesh_initialized;
    bool draw;
    bool has_mesh;

    int parent;
    int children[MAX_ENTITY_CHILDREN];
    char name[32];
    int child_count;
    int nodeID;

    bool has_animation;
    int animated_nodeID;
};

struct animation_data{
    float position_keyframe_times[MAX_KEYFRAMES];
    float rotation_keyframe_times[MAX_KEYFRAMES];
    quat keyframe_rotations[MAX_KEYFRAMES];
    vec3 keyframe_positions[MAX_KEYFRAMES];
    int position_movement_type; //linear/smooth/steps
    int rotation_movement_type; //linear/smooth/steps

    char    name[32];
    animation_type type;
    bool    valid_animation;
    float   animation_time;
    bool    has_translation;
    bool    has_rotation;
    bool    has_action;
    int     num_position_keyframes;
    int     num_rotation_keyframes;
    int     max_keyframes;//stores the max keyframes from any one animation
};

struct animated_node{
    int entity_nodeID;
    entity_bone_types bone_type;
    animation_data animations[MAX_ANIMATIONS];
    int num_animations;
};

// struct equipment_sockets{
//     int left_hand_socket_node;
//     int right_hand_socket_node;
//     int left_arm_socket_node;
//     int right_arm_socket_node;
//     int left_leg_socket_node;
//     int right_leg_socket_node;
//     int head_socket_node;
//     int torso_socket_node;
// };

enum keyframe_action_type{
    keyframe_action_type_none,

    keyframe_action_type_create_hitbox,
    keyframe_action_type_cast_projectile,
    keyframe_action_type_lock_movement,
    keyframe_action_type_unlock_movement,

    keyframe_action_type_dash,//?

};

struct keyframe_action{
    keyframe_action_type type;
    int keyframe;
    float keyframe_time;

};

struct model_animation{
    animation_type type;
    float max_time;

    int keyframe_actions_index;//index into global array of keyframe actions
    int keyframe_action_count;
};

struct model{
    model_type type;
    int root_node;
    vec3 min;
    vec3 max;
    vec3 center;
    vec3 extents;

    vec3 original_min;//for storing the untampered bounds for aabb drawing/collision pre processing 
    vec3 original_max;

    vec3 preprocessed_min;//to store positions during attack animations for printing out
    vec3 preprocessed_max;
    vec3 preprocessed_pos;

    bool animated;
    char name[32];
    int total_nodes;

    //these will work as lookups to update where to draw equipment to the entity
    // equipment_sockets sockets;
    model_animation animations[MAX_ANIMATIONS];
    //THESE POINT TO NODE IDs
    int left_fore_arm_socket;
    int right_fore_arm_socket;
    int left_upper_arm_socket;
    int right_upper_arm_socket;
    int left_thigh_socket;
    int right_thigh_socket;
    int left_shin_socket;
    int right_shin_socket;
    int head_socket;
    int torso_socket;
};

struct gltf_data{
    Vertex test_vertices[200000];
    uint32_t vertex_count;
    uint32_t indices[100000];
    uint32_t total_index_count;

    // //bgfx::VertexBufferHandle vbh;
    // //bgfx::IndexBufferHandle ibh;
    fpt_vec3 min;
    fpt_vec3 max;
    fpt_vec3 extents;
    bool mesh_initialized;
    int processed_meshes;

    //multiple primitives test
    bool                        primitives_to_draw          [32];
    bool                        meshes_to_draw              [32];
    int                         vertex_offset               ;
    int                         index_offset                [32];
    int                         index_count                 [32];
    int                         primitive_vertex_count      ;
    int                         primitive_index_count       ;
    //bgfx::VertexBufferHandle    primitive_vbh               ;
    //bgfx::IndexBufferHandle     primitive_ibh               ;
    fpt_vec3                    primitive_min               [32];
    fpt_vec3                    primitive_max               [32];
    fpt_vec3                    primitive_extents           [32];
    int                         primitive_count                 ;

    vec3                   transform                   [32];
    model                       models                     [MAX_MODELS];
    int                         model_count                ;
    
    animated_node               animated_nodes             [MAX_ENTITY_NODES];
    int                         animated_node_count        ;
    entity_node                 nodes                      [MAX_ENTITY_NODES];
    int                         root_nodes                 [MAX_ENTITY_NODES];
    int                         node_counts                [MAX_ENTITY_NODES];
    int                         node_count                 ;
    int                         root_node_count            ;
    int                         mesh_count                 ;
    
    bool                        pause_animation            ;
    bool                        step_animation             ; //for animation debugging
    
    animation_type debug_current_animation_type;
    bool debug_walk_animation;
    bool debug_jump_animation;

};


    void load_gltf_models(gltf_data* gltfData, char* basePath);

    static void dumpMem(const void* data, size_t size) {
        const unsigned char* bytes = static_cast<const unsigned char*>(data);
        for (size_t i = 0; i < size; i++) {
            printf("%02x ", bytes[i]);
            if ((i + 1) % 16 == 0) printf("\n");
        }
        printf("\n");
    }

    static void init_cgltf_buffer(gltf_data* cgltfData){

        //bgfx::VertexLayout pcvLayout; //short for position color vertex layout from PosColorVertex struct in cube.h
        // pcvLayout
        //     .begin()
        //     .add(//bgfx::Attrib::Position, 3, //bgfx::AttribType::Float)
        //     .add(//bgfx::Attrib::Color0, 4, //bgfx::AttribType::Uint8, true)
        //     .end();

            printf("initializing meshes, primitive count: %d\n", cgltfData->primitive_count);
            // cgltfData->primitive_vbh = BGFX_INVALID_HANDLE;
            // cgltfData->primitive_ibh = BGFX_INVALID_HANDLE;
            // cgltfData->primitive_vbh = //bgfx::createVertexBuffer(//bgfx::makeRef(cgltfData->test_vertices,    (uint32_t)((cgltfData->vertex_count  )  * sizeof(Vertex))), pcvLayout);
            // cgltfData->primitive_ibh = //bgfx::createIndexBuffer (//bgfx::makeRef(cgltfData->indices ,    (uint32_t)((cgltfData->total_index_count  )  * sizeof(uint32_t))), BGFX_BUFFER_INDEX32);

            for(int i = 0; i < cgltfData->mesh_count; i++){
                cgltfData->meshes_to_draw[i] = true;

            //     printf("primitive:      %d\n", i);
            //     printf("vertex offset:  %d\n", cgltfData->vertex_offset);
            //     printf("index offset:   %d\n", cgltfData->index_offset[i]);
            //     printf("vertex count:   %d\n", cgltfData->primitive_vertex_count);
            //     printf("index count:    %d\n", cgltfData->primitive_index_count[i]);

            }

        // cgltfData->vbh = BGFX_INVALID_HANDLE;
        // cgltfData->ibh = BGFX_INVALID_HANDLE;
        // printf("vertex 2: %f, %f, %f\n", vertices[2].position.x, vertices[2].position.y, vertices[2].position.z);
        // cgltfData->vbh = //bgfx::createVertexBuffer(//bgfx::makeRef(cgltfData->test_vertices, (uint32_t)(cgltfData->vertex_count * sizeof(Vertex))), pcvLayout);
        // cgltfData->ibh = //bgfx::createIndexBuffer (//bgfx::makeRef(cgltfData->indices,  (uint32_t)(cgltfData->index_count * sizeof(uint16_t))));

        // printf("VBH valid: %d, IBH valid: %d\n", 
        //     //bgfx::isValid(cgltfData->vbh), 
        //     //bgfx::isValid(cgltfData->ibh));




        //this seems to work, loading the cgltf mesh is failing though
        // static Vertex testCubeVertices[8] =       
        // {      
        //     {{-0.5f, -0.5f,  0.5f}, 0x00000000}, // Vertex 0
        //     {{ 0.5f, -0.5f,  0.5f}, 0x00000000}, // 1
        //     {{ 0.5f,  0.5f,  0.5f}, 0x00000000}, // 2
        //     {{-0.5f,  0.5f,  0.5f}, 0x00000000}, // 3
        //     {{-0.5f, -0.5f, -0.5f}, 0x00000000}, // 4
        //     {{ 0.5f, -0.5f, -0.5f}, 0x00000000}, // 5
        //     {{ 0.5f,  0.5f, -0.5f}, 0x00000000}, // 6
        //     {{-0.5f,  0.5f, -0.5f}, 0x00000000}  // 7
        // };
        // static Vertex testCubeVertices[3] =       
        // {      
        //     {{0.0f, 0.0f, 0.0f}, 0x00000000}, // Vertex 0
        //     {{10.0f, 0.0f, 0.0f}, 0x00000000}, // 1
        //     {{0.0f, 10.0f, 0.0f}, 0x00000000}, // 2
        // };
        // static uint16_t testCubeIndices[]=
        // {
        //     0, 1, 2, 0, 2, 3,
        //     1, 5, 6, 1, 6, 2,
        //     5, 4, 7, 5, 7, 6,
        //     4, 0, 3, 4, 3, 7,
        //     3, 2, 6, 3, 6, 7,
        //     4, 5, 1, 4, 1, 0,
        // };
    //     static uint16_t testCubeIndices[]=
    //     {
    //         0, 1, 2, 
    //     };
    //     assert(testCubeVertices[0].position == cgltfData->test_vertices[0].position && "vertex 0 mismatch");
    //     assert(testCubeVertices[1].position == cgltfData->test_vertices[1].position && "vertex 1 mismatch");
    //     assert(testCubeVertices[2].position == cgltfData->test_vertices[2].position && "vertex 2 mismatch");
        
    //     assert(testCubeIndices[0] == cgltfData->indices[0] && "index 0 mismatch");
    //     assert(testCubeIndices[1] == cgltfData->indices[1] && "index 1 mismatch");
    //     assert(testCubeIndices[2] == cgltfData->indices[2] && "index 2 mismatch");

    //     printf("Vertex 0: Hardcoded (%f, %f, %f) vs CGLTF (%f, %f, %f)\n", 
    //    testCubeVertices[0].position.x, testCubeVertices[0].position.y, testCubeVertices[0].position.z,
    //    cgltfData->test_vertices[0].position.x, cgltfData->test_vertices[0].position.y, cgltfData->test_vertices[0].position.z);

    //    cgltfData->vbh = //bgfx::createVertexBuffer(//bgfx::makeRef(cgltfData->test_vertices, (uint32_t)(cgltfData->vertex_count * sizeof(Vertex))), pcvLayout);
    //    cgltfData->ibh = //bgfx::createIndexBuffer (//bgfx::makeRef(cgltfData->indices,  (uint32_t)(cgltfData->index_count * sizeof(uint32_t))), BGFX_BUFFER_INDEX32);

        // cgltfData->vbh = //bgfx::createVertexBuffer(//bgfx::makeRef(testCubeVertices, (uint32_t)(3 * sizeof(Vertex))), pcvLayout);
        // cgltfData->ibh = //bgfx::createIndexBuffer (//bgfx::makeRef(testCubeIndices,  (uint32_t)(3 * sizeof(uint16_t))));

        // dumpMem(testCubeVertices, 3 * sizeof(Vertex));
        // dumpMem(cgltfData->test_vertices, 3 * sizeof(Vertex));

        //     printf("indices sanity check:\n");
        //     printf("%d\n", cgltfData->indices[0]);
        //     printf("%d\n", cgltfData->indices[1]);
        //     printf("%d\n", cgltfData->indices[2]);
        // dumpMem(testCubeIndices, 3 * sizeof(   uint16_t));
        // dumpMem(cgltfData->indices, 3 * sizeof(uint32_t));

        cgltfData->mesh_initialized = true;
    
    }
    static void destroy_cgltf_buffer(gltf_data* cgltfData){
        if(cgltfData->mesh_initialized)return;
        // if(//bgfx::isValid(cgltfData->primitive_vbh)) //bgfx::destroy(cgltfData->primitive_vbh);
        // if(//bgfx::isValid(cgltfData->primitive_ibh)) //bgfx::destroy(cgltfData->primitive_ibh);

        for(int i = 0; i < cgltfData->primitive_count; i++){
            cgltfData->meshes_to_draw[i] = false;
        }
        cgltfData->mesh_initialized = false;

    }
    
  