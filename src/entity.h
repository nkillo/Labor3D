#pragma once

#include "constants.h"
#include "fptvec.h"
#include "camera.h"

using entityID = uint16_t;

/*
NEW COMPONENT CHECKLIST
    1. create struct
    
    2. determine max size (default should be MAX_ENTITIES but it can be whatever you want below that)
    
    3. in the entityComponent struct:
        - add the array of components
        - add the map so the entity can know which component slot it has
        - add the count for the components (starts at 0)

    3. create add/remove function, remove implements pop and swap, mimic other functions as a guideline
    
    4. DEFAULT INIT THE ARRAYS TO NULL_ENTITY OR NULL_PLAYER or some null value that matches your total array size
*/



struct capsule_collider{
    fpt_vec3 start_point;
    fpt_vec3 end_point;
    fpt radius;
};

struct tool_data{
    tool_types type;
    int damage;
    int enchantment;
    capsule_collider collider;
    int spell_index; //just an idea, an index into a list of active spells, just another entity array, could modify this item, same as the gem_index
    int gem_index; 
};
struct equipment_data{
    equipment_types type;
    int damage;
    int enchantment;
    capsule_collider collider;
    int spell_index; //just an idea, an index into a list of active spells, just another entity array, could modify this item, same as the gem_index
    int gem_index; 
};
struct trinket_data{
    trinket_types type;
    int placeholder;
};
struct inventory_data{
    int placeholder;
};

struct item_data{
    item_types type;
    model_type item_model;

    int quantity;

    union{
        tool_data       tool;
        equipment_data  equipment;    
        trinket_data    trinket;    
        inventory_data  item;
    };
};

struct inventory_item{
    item_data data;
    vec3 position;
    quat rotation;
};

enum entity_body_types{
    entityBody_humanoid = 0,
    entityBody_item,
    entityBody_count,
};

enum entity_types{
    entity_none,

    entity_actor,
    entity_hitbox,
    entity_projectile,
    entity_item,//pickup
    entity_environment,//doors, interactible/destructable environment stuff
};

enum body_part_type{
    body_part_none,

    body_part_head,
    body_part_torso,
    body_part_left_fore_arm,
    body_part_right_fore_arm,
    body_part_left_upper_arm,
    body_part_right_upper_arm,
    body_part_left_thigh,
    body_part_right_thigh,
    body_part_left_shin,
    body_part_right_shin,

};

struct body_part{
    body_part_type type;
    int mesh;
    vec3 position;
    quat rotation;
};

struct DataComp{
    entity_types type;

    union{
        uint16_t item_index;
    };

    uint32_t entity_interaction_lookup_chunkID;
    uint8_t  entity_interaction_lookup_index;
    
};

struct BodyComp{
    int body_part_index;//index into the larger array of body parts
    int body_part_count;
    int active_body_part_count;
};


struct InventoryComp{//just lookups into an array of equipment shared between all entities'
    bool left_item_equipped;
    bool right_item_equipped;
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
    //should we compress all the different attachment point into an array of indices?
    //int* equipment_sockets; //dunno yet   
    int* trinket_sockets;  //extendable/extra places to store stuff, not setup yet


    u32 equipment[10];
    u32 trinkets[32];
    u32 items[32];
    u32 left_hand[5];
    u32 right_hand[5];

    u32 hotslot; //when the UI is interacting with an item, this is where that item index goes for the duration of that interaction

    u32 trinket_count;
    u32 item_count;
    u32 equipment_count;

    //index into the inventoryComp array, not the item array
    u32 current_left_hand;
    u32 current_right_hand;
    u32 left_hand_count;
    u32 right_hand_count;
};

struct AabbComp{
    fpt_vec3 min;
    fpt_vec3 max;
    fpt_vec3 offset; //relative to current entity position. cumulative offset from the center of the enlarged box. becomes the box movement delta when we moved too far
    fpt_vec3 scale;
    fpt_vec3 minDelta;
    fpt_vec3 maxDelta;
    fpt_vec3 posDelta;
    uint8_t flags;

    uint16_t primitiveCount;
};

struct ObbComp{
    fpt_quat rotation;
    fpt_vec3 position; //relative to current chunk
    fpt_vec3 scale;
    fpt_vec3 extents;
    fpt_vec3 localCenter;
    fpt_vec3 aabbMin;
    fpt_vec3 aabbMax;
};

//how do we work with entity body parts?
// #define VOLUME_COUNT 10
// struct bodyPartComp{
//     fpt_vec3 volumeMin[VOLUME_COUNT];
//     fpt_vec3 volumeMax[VOLUME_COUNT];
// }

struct TransComp{
    fpt_quat rotation;
    // fpt_vec3 position;
    fpt_vec3 pos_in_chunk;
    
    fpt_vec3 desired_movement;
    fpt_vec3 collide_movement;
    fpt_vec3 move_delta;
    ivec3 chunk_coords;
    ivec3 chunk_coords_delta;
    fpt_vec3 forward;
    fpt_vec3 up;
    fpt_vec3 right;

    fpt_vec3 scale;
    uint16_t owningChunkID;//need to see if chunkIDs can change without being deleted, need to make this perfect
    bool inNewChunk;
    uint8_t flags;
    fpt speed;

    // quat debug_rotation;
    // vec3 debug_pos_in_chunk;
    // vec3 debug_pos_in_chunk_delta;
    // vec3 debug_forward;
    // vec3 debug_up;
    // vec3 debug_right;
    // vec3 debug_scale;
    // vec3 debug_moveDelta;
    // float debug_speed;
};

//animated model
struct ModelComp{
    model_type type;
};

// simple mesh, not animated, single 3d object
struct MeshComp {
    uint16_t meshIndex;
};

struct HealthComp {
    uint16_t health;
};

struct PlayerComp{
    uint8_t test;
    uint8_t playerIndex;
    fpt angleH;
    fpt angleV;

    fpt_vec3 rayDir;

    fpt_vec3 brushPos;
    ivec3 brushChunkCoords;
    fpt brushSize;

};

struct PhysicsComp{
    fpt_vec3 velocity;
    fpt_vec3 rotation; //angular velocity

    fpt_vec3 acceleration;
    fpt_vec3 last_frame_acceleration;
    fpt_vec3 angular_acceleration;
    fpt_vec3 force;
    fpt_vec3 force_accumulator;
    fpt_vec3 torque_accumulator;

    fpt_vec3 half_size;

    fpt mass;
    fpt inverse_mass;
    fpt damping;
    
    fpt linear_damping;
    fpt angular_damping;
    
    fpt_quat orientation;

    fpt_flatmat34 transform_matrix;
    fpt_flatmat3 inverse_inertia_tensor;
    fpt_flatmat3 inverse_inertia_tensor_world;

    fpt kinetic_energy;

    fpt radius; //WIP sphere stuff

    bool is_awake;



    //debug
    // vec3 debug_velocity;
    // vec3 debug_rotation; //angular velocity
    // vec3 debug_acceleration;
    // vec3 debug_last_frame_acceleration;
    // vec3 debug_angular_acceleration;
    // vec3 debug_force;
    // vec3 debug_force_accumulator;
    // vec3 debug_torque_accumulator;
    // vec3 debug_half_size;
    // float debug_mass;
    // float debug_inverse_mass;
    // float debug_damping;
    // float debug_linear_damping;
    // float debug_angular_damping;
    // quat debug_orientation;
    // mat4 debug_transform_matrix;
    // mat3  debug_inverse_inertia_tensor;
    // mat3  debug_inverse_inertia_tensor_world;
    // float debug_kinetic_energy;
    // float debug_radius; //WIP sphere stuff

};



enum entity_movement_state{
    entity_movement_none = 0,
    entity_movement_idle,
    entity_movement_walk,
    entity_movement_run,
    entity_movement_jump,
    entity_movement_airborn,
    entity_movement_land,
    entity_movement_roll,


};

enum entity_combat_state{
    entity_combat_none = 0,
    entity_combat_idle,
    entity_combat_left_punch, 
    entity_combat_right_punch,
    entity_combat_left_kick,  
    entity_combat_right_kick, 
    entity_combat_left_weapon, 
    entity_combat_right_weapon,
    entity_combat_left_block, 
    entity_combat_right_block,
};

enum entity_interact_state{
    entity_interact_none = 0,
    entity_interact_idle,
};

enum entity_state_type{
    entity_state_none = 0,

    entity_idle_state_default,//placeholder

    entity_movement_state_idle,
    entity_movement_state_walk,
    entity_movement_state_jump,
    entity_movement_state_airborn,
    entity_movement_state_land,
    entity_movement_state_roll,

    entity_combat_state_left_punch, 
    entity_combat_state_right_punch,
    entity_combat_state_left_kick,  
    entity_combat_state_right_kick, 
    entity_combat_state_left_weapon, 
    entity_combat_state_right_weapon,
    entity_combat_state_left_block, 
    entity_combat_state_right_block,

    entity_interact_state_idle,

    entity_state_count,

};

enum entity_action_type{
    action_type_none,

    action_type_hitbox,
    action_type_lock_interact,

    action_type_movement,
    action_type_velocity,

    action_type_cast_projectile,
    action_type_lock_movement,
    action_type_unlock_movement,
    action_type_unlock_interact,
    action_type_unlock_all,
    action_type_end_state,
};

struct entity_state_action{
    entity_action_type type;
    uint32_t tick;
    bool optional;
    union{
        uint16_t hitbox_index; //index into array of hitboxes if we wanted to store a specific hitbox for this tick in the entity state
        uint16_t movement_index; //index into array of movements
        fpt_vec3 velocity;
        // AABB hitbox_bounds;//we could just store the hitbox directly but that would bloat memory more, same as the movement
        // fpt_vec3 movement;

    };
};

struct entity_animation_track{
    float anim_time;
    int anim_segment;
    float segment_length;
    animation_type anim_type;
    bool looped;
    float weight;
    float blendIn;
    float blendOut;
    float blend;
    bool looping;
};
struct entity_action_data{
    uint32_t tick;//ticks since start of state
    uint32_t prev_action_index;
    uint32_t curr_action_index;
};

struct entity_state_machine{
    
    entity_state_type cur;
    entity_state_type prev;
    entity_animation_track curTrack;
    entity_animation_track preTrack;
    entity_action_data action;
    bool locked;
    bool looped;
    bool looping;
    float blendstep;
    float blend;
    float blendTarget;
    float interBlend;//among distinct states
    float intraBlend;//within prev/cur track
};

//introspection parse test
introspect(category: "entity state")struct StateComp{
    // entity_state_type previous;
    // entity_state_type current;
    // bool is_locked;
    // bool movement_is_locked;
    entity_body_types body_type;//index into an arary of actions for different entity types and their states

    entity_state_machine smMove;
    entity_state_machine smAir;
    entity_state_machine smAct;



    // uint32_t tick;//ticks since start of state
    // uint32_t prev_action_index;
    // uint32_t curr_action_index;
    
    // int action_index;//index into an arary of actions for different entity types and their states

    bool debug_print_hitbox_positions;
    // float anim_time;
    // animation_type anim_type;
    // int anim_segment;
    // float segment_length;
    bool grounded;
    bool landed;
    float landingIntensity;

};



struct EntityComponent {
    uint16_t entityIDQueue[MAX_ENTITIES];

    uint16_t networkToEntityID[MAX_ENTITIES];
    uint16_t entityToNetworkID[MAX_ENTITIES];

    //identifiers
    char names[MAX_ENTITIES * MAX_NAME];
    uint8_t versionIDs[MAX_ENTITIES];
    
    TransComp TransComps[MAX_ENTITIES];

    ModelComp ModelComps[MAX_ENTITIES];
    MeshComp MeshComps[MAX_ENTITIES];
    HealthComp HealthComps[MAX_ENTITIES];

    AabbComp AabbComps[MAX_ENTITIES];
    ObbComp ObbComps[MAX_ENTITIES];

    PhysicsComp PhysicsComps[MAX_ENTITIES];
    StateComp StateComps[MAX_ENTITIES];

    InventoryComp InventoryComps[MAX_ENTITIES];
    BodyComp BodyComps[MAX_ENTITIES];

    DataComp   DataComps[MAX_ENTITIES];

    PlayerComp PlayerComps[MAX_PLAYERS];
    CameraComp CameraComps[MAX_PLAYERS];


    //maps work as follows:
    //the element/index in the map array is the entityID
    //the value in the map array is the index in the component array
    //you should ONLY index into the entityToComponentMap WITH ENTITYIDs
    uint16_t entityToTransMap[MAX_ENTITIES];
    uint16_t TransToEntityMap[MAX_ENTITIES];

    uint16_t entityToModelMap[MAX_ENTITIES];
    uint16_t ModelToEntityMap[MAX_ENTITIES];

    uint16_t entityToMeshMap[MAX_ENTITIES];
    uint16_t MeshToEntityMap[MAX_ENTITIES];

    uint16_t entityToHealthMap[MAX_ENTITIES];
    uint16_t HealthToEntityMap[MAX_ENTITIES];

    uint16_t entityToAabbMap[MAX_ENTITIES];
    uint16_t AabbToEntityMap[MAX_ENTITIES];

    uint16_t entityToObbMap[MAX_ENTITIES];
    uint16_t ObbToEntityMap[MAX_ENTITIES];

    uint16_t entityToPhysicsMap[MAX_ENTITIES];
    uint16_t PhysicsToEntityMap[MAX_ENTITIES];

    uint16_t StateToEntityMap[MAX_ENTITIES];
    uint16_t entityToStateMap[MAX_ENTITIES];

    uint16_t InventoryToEntityMap[MAX_ENTITIES];
    uint16_t entityToInventoryMap[MAX_ENTITIES];

    uint16_t BodyToEntityMap[MAX_ENTITIES];
    uint16_t entityToBodyMap[MAX_ENTITIES];

    uint16_t DataToEntityMap[MAX_ENTITIES];
    uint16_t entityToDataMap[MAX_ENTITIES];
    
    
    uint8_t  entityToPlayerMap[MAX_ENTITIES]; //uint8 since the size is limited
    uint16_t PlayerToEntityMap[MAX_PLAYERS];

    uint16_t entityToCameraMap[MAX_ENTITIES];
    uint16_t CameraToEntityMap[MAX_PLAYERS];
    
    bool liveEntities[MAX_ENTITIES];
    bool activeEntities[MAX_ENTITIES];


    //need to move this into another struct that we keep a pointer to from here, otherwise it will get copied for network rollback
    ////////////////////////////////////////// STATIC DATA /////////////////////////////////////////////////////
    //indexed into from different components
    inventory_item inventory_items[MAX_ENTITIES];
    int             inventory_item_count;
    body_part body_parts[MAX_ENTITIES];
    int             body_part_count;

    //for entities to look up actions that should happen at a certain time/tick in their current state
    //lookup happens like state.action_index + state.type
    entity_state_action entity_state_actions[entityBody_count][entity_state_count][MAX_STATE_ACTIONS];
    //                     movement    interact
    // float animationWeights[anim_count][anim_count];
    float animationWeights[2][anim_count];
    // AABB hitbox_aabbs[MAX_ENTITIES];//stores the hitbox data for any given entity state action
    fpt_vec3 action_movements[MAX_ENTITIES];//stores the movements for any given entity state action
    ////////////////////////////////////////// STATIC DATA /////////////////////////////////////////////////////

    
    uint16_t entityCount;

    uint16_t AabbCount;
    uint16_t ObbCount;

    uint16_t TransCount;
    uint16_t ModelCount;
    uint16_t MeshCount;
    uint16_t HealthCount;
    uint16_t PhysicsCount;
    uint16_t StateCount;
    uint16_t InventoryCount;
    uint16_t BodyCount;
    uint16_t DataCount;

    uint8_t  PlayerCount;
    uint8_t  CameraCount;

    fpt enlargeAABBAmount;
    fpt enlargeAABBAmountLast;
    bool enlargeAABB = true;
};



#define ADD_ENTITY_COMPONENT(compType, entityID, givenComp, COMP_COUNT, ENTITY_COUNT, NULL_VALUE)\
    if (ec.compType##Count >= COMP_COUNT || entityID >= ENTITY_COUNT){ \
        printf(#compType "Count too large, or entityID too large\n"); \
        return; \
    } \
    if (ec.entityTo##compType##Map[entityID] != NULL_VALUE){ \
        printf("EntityID %d already has " #compType " component at comp position: %d: %s\n", \
            entityID, ec.entityTo##compType##Map[entityID], getEntityName(ec, entityID)); \
        return; \
    } \
    compType##Comp comp = {}; \
    if(givenComp){ \
        comp = *givenComp; \
    } \
    ec.compType##Comps[ec.compType##Count] = comp; \
    ec.entityTo##compType##Map[entityID] = ec.compType##Count; \
    ec.compType##ToEntityMap[ec.compType##Count] = entityID; \
    ec.compType##Count++;



#define REMOVE_ENTITY_COMPONENT(compType, entityID, ENTITY_COUNT, NULL_VALUE)\
        if (entityID >= ENTITY_COUNT || ec.entityTo##compType##Map[entityID] == NULL_VALUE) return;\
        uint16_t lastIndex = ec.##compType##Count - 1;\
        if(lastIndex != ec.entityTo##compType##Map[entityID] && lastIndex != 0){\
            uint16_t removedIndex = ec.entityTo##compType##Map[entityID];\
            ec.##compType##Comps[removedIndex] = ec.##compType##Comps[lastIndex];\
            uint16_t lastEntityIndex = ec.##compType##ToEntityMap[lastIndex];\
            ec.entityTo##compType##Map[lastEntityIndex] = removedIndex;\
            ec.##compType##ToEntityMap[removedIndex] = ec.##compType##ToEntityMap[lastIndex];\
        }\
        ec.entityTo##compType##Map[entityID] = NULL_VALUE;\
        ec.##compType##ToEntityMap[lastIndex] = NULL_VALUE;\
        ec.##compType##Count--;


        // Helper function to safely get an entity's name
    const char* getEntityName(EntityComponent& ec, uint16_t entityID);



//////////////////////////////////////////////////// PLAYER ///////////////////////////////////
    void addPlayerComp(EntityComponent& ec, uint16_t entityID, uint8_t testInfo);
    void removePlayerComp(EntityComponent& ec, uint16_t entityID);

    void addCameraComp(game_state* GameState, EntityComponent& ec, uint16_t entityID, fpt_vec3 pos = fpt_vec3_create(0));
    void removeCameraComp(EntityComponent& ec, uint16_t entityID);


//////////////////////////////////////////////////// AABB //////////////////////////////////////////////////////

    void addAabbComp(EntityComponent& ec, uint16_t entityID, AabbComp* aabbComp = nullptr);
    void removeAabbComp(EntityComponent& ec, uint16_t entityID);

//////////////////////////////////////////////////// OBB //////////////////////////////////////////////////////

    void addObbComp(EntityComponent& ec, uint16_t entityID, ObbComp* givenComp = nullptr);
    void removeObbComp(EntityComponent& ec, uint16_t entityID);


    //////////////////////////////////////////////////// TRANSFORM //////////////////////////////////////////////////////

    void addTransComp(EntityComponent& ec, uint16_t entityID, TransComp* transComp = nullptr, fpt_vec3 pos = fpt_vec3_create(0,0,0));
    void removeTransComp(EntityComponent& ec, uint16_t entityID);

    //////////////////////////////////////////////////// MODEL //////////////////////////////////////////////////////


    void addModelComp(EntityComponent& ec, uint16_t entityID, ModelComp* givenComp = nullptr);
    void removeModelComp(EntityComponent& ec, uint16_t entityID);

    //////////////////////////////////////////////////// MESH //////////////////////////////////////////////////////


    void addMeshComp(EntityComponent& ec, uint16_t entityID, MeshComp* givenComp = nullptr);
    void removeMeshComp(EntityComponent& ec, uint16_t entityID);
    //////////////////////////////////////////////////// HEALTH //////////////////////////////////////////////////////


    void addHealthComp(EntityComponent& ec, uint16_t entityID, HealthComp* givenComp = nullptr);
    void removeHealthComp(EntityComponent& ec, uint16_t entityID);


    //////////////////////////////////////////////////// PHYSICS //////////////////////////////////////////////////////

    void addPhysicsComp(EntityComponent& ec, uint16_t entityID, PhysicsComp* givenComp = nullptr);
    void removePhysicsComp(EntityComponent& ec, uint16_t entityID);

    
    //////////////////////////////////////////////////// STATE //////////////////////////////////////////////////////

    void addStateComp(EntityComponent& ec, uint16_t entityID, StateComp* givenComp = nullptr);
    void removeStateComp(EntityComponent& ec, uint16_t entityID);


    //////////////////////////////////////////////////// EQUIPMENT //////////////////////////////////////////////////////

    void addInventoryComp(EntityComponent& ec, uint16_t entityID, InventoryComp* givenComp = nullptr);
    void removeInventoryComp(EntityComponent& ec, uint16_t entityID);
    
    

        
    //////////////////////////////////////////////////// BODY //////////////////////////////////////////////////////

    void addBodyComp(EntityComponent& ec, uint16_t entityID, BodyComp* givenComp = nullptr);
    void removeBodyComp(EntityComponent& ec, uint16_t entityID);

    //////////////////////////////////////////////////// DATA //////////////////////////////////////////////////////

    void addDataComp(EntityComponent& ec, uint16_t entityID, DataComp* givenComp = nullptr);
    void removeDataComp(EntityComponent& ec, uint16_t entityID);





    //////////////////////////////////////////////////// ENTITY CREATION/DESTRUCTION/INIT //////////////////////////////////////////////////////



    bool isEntityActive(EntityComponent& ec, uint16_t entityID);
    uint16_t createEntity(EntityComponent& ec, const char* name);

    void destroyEntity(EntityComponent& ec, uint16_t entityID);
    void init(game_state* GameState, EntityComponent& ec);


static inline void apply_desired_movement(TransComp& transComp, fpt_vec3& movement){

    fpt_quat new_rotation = rotate_in_direction_of_movement_y(movement, transComp.rotation);

    fpt_quat temp_rotation = transComp.rotation;
    transComp.rotation = fpt_nlerp_y_axis(transComp.rotation, new_rotation, FPT_QUARTER);
    if(temp_rotation != transComp.rotation)transComp.flags |= rot_dirty;

    transComp.forward = fpt_quat_rotate_vec3(transComp.rotation, fpt_vec3_create(0,0,FPT_ONE));

    //TEST FOR SMOOTHER MOVEMENT BASED ON ENTITY ROTATION
    // Compute alignment between forward direction and desired movement
    fpt_vec3 ignore_y_movement = fpt_vec3_create(movement.x, 0, movement.z);
    fpt_vec3 ignore_y_forward  = fpt_vec3_create(transComp.forward.x, 0, transComp.forward.z); 
    fpt alignment = fpt_vec3_dot(fpt_vec3_normalize(ignore_y_forward), fpt_vec3_normalize(ignore_y_movement));

    // Clamp alignment to [0, 1] if you don't want negative values
    alignment = fpt_max(0, alignment);

    // Scale the movement vector by the alignment factor
    movement.x = fpt_mul(movement.x, alignment);
    movement.z = fpt_mul(movement.z, alignment);
    transComp.desired_movement += movement;


    //debug check to make sure the rotation is valid
    quat rotation = fpt_to_flt_quat(transComp.rotation);
    float node_float = (rotation.x * rotation.x) + (rotation.y * rotation.y) + (rotation.z * rotation.z) + (rotation.w * rotation.w);
    if(fabs(1.0f - node_float) > 0.01f){
        printf("TRANS ROTATION INVALID! %3.3f %3.3f %3.3f %3.3f SQRD == %3.3f \n", rotation.x, rotation.y, rotation.z, rotation.w, node_float);

    }
    
    transComp.flags |= pos_dirty;
}

static inline void apply_entity_movement(TransComp& transComp, fpt_vec3 movement, bool toroidal_space_enabled = false){

    transComp.pos_in_chunk +=  movement;    

    ivec3 old_chunk_coords = transComp.chunk_coords;
    
    rebasePosition(transComp.pos_in_chunk, transComp.chunk_coords, &transComp.inNewChunk, toroidal_space_enabled);
    transComp.chunk_coords_delta.x = transComp.chunk_coords.x - old_chunk_coords.x;
    transComp.chunk_coords_delta.y = transComp.chunk_coords.y - old_chunk_coords.y;
    transComp.chunk_coords_delta.z = transComp.chunk_coords.z - old_chunk_coords.z;
    

    transComp.flags |= pos_dirty;

    //debug syncing
    // transComp.debug_pos_in_chunk = fpt_to_flt_vec3_create(transComp.pos_in_chunk);

}

static inline animation_type state_to_animation_type(entity_state_type& type){
    if(type ==        entity_movement_state_walk)       return anim_walk;
    else if(type ==   entity_movement_state_jump)       return anim_jump;
    else if(type ==   entity_movement_state_airborn)    return anim_airborn;
    else if(type ==   entity_movement_state_land)       return anim_land;
    else if(type ==   entity_movement_state_idle)       return anim_idle;
    
    else if(type ==   entity_combat_state_left_punch)   return anim_left_punch;
    else if(type ==   entity_combat_state_right_punch)  return anim_right_punch;
    else if(type ==   entity_interact_state_idle)       return anim_idle;
    return anim_idle;
}


static inline void get_blend_weights(EntityComponent* ec, entity_animation_track* track){
    track->blendIn = ec->animationWeights[0][track->anim_type];
    track->blendOut = ec->animationWeights[1][track->anim_type];
}

// static inline void transition_entity_state(StateComp& stateComp, entity_state_type new_state, bool is_locked = false){
//     if(stateComp.is_locked)printf("TRANSITIONING FROM LOCKED STATE!\n");
//     stateComp.previous = stateComp.current;
//     stateComp.current = new_state;
//     stateComp.is_locked = is_locked;
    
//     stateComp.prev_action_index = 0;
//     stateComp.curr_action_index = 0;
//     stateComp.anim_time = 0;
//     stateComp.tick = 0;

// }


static inline void transition_entity_state(StateComp& stateComp, entity_state_machine* sm, entity_state_type new_state, bool is_locked = false){
    // if(stateComp.is_interact_locked)printf("TRANSITIONING INTERACTION FROM LOCKED STATE!\n");
    sm->prev = sm->cur;
    sm->cur = new_state;
    sm->locked = is_locked;
    
    sm->action.prev_action_index = 0;
    sm->action.curr_action_index = 0;
    sm->curTrack.anim_time = 0;
    sm->curTrack.looped = false;
    sm->action.tick = 0;


}


// static inline void transition_entity_movement_state(StateComp& stateComp, entity_state_type new_state, bool is_locked = false){
    // if(stateComp.is_movement_locked)printf("TRANSITIONING MOVEMENT FROM LOCKED STATE!\n");
    // stateComp.prevMovement = stateComp.curMovement;
    // stateComp.curMovement = new_state;
    // stateComp.is_movement_locked = is_locked;
    // 
    // stateComp.moveAction.prev_action_index = 0;
    // stateComp.moveAction.curr_action_index = 0;
    // stateComp.cur_move_track.anim_time = 0;
    // stateComp.cur_move_track.looped = false;
    // stateComp.moveAction.tick = 0;
    // trying to blend between different states better, need per bone blending
// }

static inline const char* get_entity_state_name(entity_state_type& type){
    switch(type){
        case entity_state_none:         {
            return("type: NONE");
            break;
        }
        case entity_idle_state_default: {
            return("type: IDLE");
            break;
        }
        case entity_movement_state_walk:    return("type: MOVEMENT: WALK"); break;
        case entity_movement_state_jump:    return("type: MOVEMENT: JUMP"); break;
        case entity_movement_state_airborn: return("type: MOVEMENT: AIRBORN"); break;
        case entity_movement_state_land:    return("type: MOVEMENT: LAND"); break;
        case entity_movement_state_roll:    return("type: MOVEMENT: ROLL"); break;

        case entity_combat_state_left_punch:     return("type: COMBAT: LEFT PUNCH"); break;
        case entity_combat_state_right_punch:    return("type: COMBAT: RIGHT PUNCH"); break;
        case entity_combat_state_left_block:     return("type: COMBAT: LEFT BLOCK"); break;
        case entity_combat_state_right_block:    return("type: COMBAT: RIGHT BLOCK"); break;
    }
    return("NONE");
}


// static inline void resolve_entity_collision(TransComp& transComp, fpt_vec3& resolution, bool toroidal_space_enabled = false){
//     //how would this interact with the aabb.offset when we have enlarged aabbs?

//     fpt_vec3 move_delta = transComp.pos_in_chunk;

//     transComp.pos_in_chunk +=  movement;    

//     ivec3 old_chunk_coords = transComp.chunk_coords;
    
//     rebasePosition(transComp.pos_in_chunk, transComp.chunk_coords, &transComp.inNewChunk, toroidal_space_enabled);
//     transComp.chunk_coords_delta = (transComp.chunk_coords - old_chunk_coords);
    
//     transComp.moveDelta +=  ((transComp.pos_in_chunk - move_delta) + (flt_to_fpt_vec3(transComp.chunk_coords_delta) * FPT_CHUNK_SIZE)) ; 
//     transComp.pos_in_chunk_delta += movement;

//     transComp.flags |= pos_dirty;

//     //debug syncing
//     // transComp.debug_pos_in_chunk = fpt_to_flt_vec3_create(transComp.pos_in_chunk);

// }