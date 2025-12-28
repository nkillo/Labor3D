
#include "entity.h"
        // Helper function to safely get an entity's name
    const char* getEntityName(EntityComponent& ec, uint16_t entityID) {
        if (entityID >= MAX_ENTITIES) return "Invalid Entity";
        return &ec.names[entityID * MAX_NAME];
    }

    uint32_t get_action_tick(uint32_t tick_rate, float action_time){
        return (uint32_t)(tick_rate * action_time);        
    }

    entity_state_action create_entity_action(entity_action_type type, uint32_t tick, bool optional){
        entity_state_action action = {};
        action.type = type;
        action.tick = tick;
        action.optional = optional;
        
        return action;
    }

    void hardcode_entity_actions(game_state* GameState){
        EntityComponent& ec = *GameState->entityComponent;
        uint32_t tick_rate = GameState->fixed_tick_rate;

        entity_body_types bodyType = entityBody_humanoid;
        uint32_t current_action = 0;

        //trim the end state time a bit so that the animation doesn't loop and jitter for one frame

        //humanoid entity left punch
        ec.entity_state_actions[bodyType][entity_combat_state_left_punch][current_action++] = create_entity_action(entity_action_type::action_type_lock_interact,     0.00f * tick_rate, false);
        ec.entity_state_actions[bodyType][entity_combat_state_left_punch][current_action++] = create_entity_action(entity_action_type::action_type_hitbox,          .42f * tick_rate, false);
        ec.entity_state_actions[bodyType][entity_combat_state_left_punch][current_action++] = create_entity_action(entity_action_type::action_type_unlock_all, .50f * tick_rate, false);
        ec.entity_state_actions[bodyType][entity_combat_state_left_punch][current_action++] = create_entity_action(entity_action_type::action_type_end_state,      0.9f * tick_rate, false);

        //humanoid entity right punch
        current_action = 0;
        ec.entity_state_actions[bodyType][entity_combat_state_right_punch][current_action++] = create_entity_action(entity_action_type::action_type_lock_interact,     0.00f * tick_rate, false);
        ec.entity_state_actions[bodyType][entity_combat_state_right_punch][current_action++] = create_entity_action(entity_action_type::action_type_hitbox,          .42f * tick_rate, false);
        ec.entity_state_actions[bodyType][entity_combat_state_right_punch][current_action++] = create_entity_action(entity_action_type::action_type_unlock_all, .50f * tick_rate, false);
        ec.entity_state_actions[bodyType][entity_combat_state_right_punch][current_action++] = create_entity_action(entity_action_type::action_type_end_state,      0.9f * tick_rate, false);


        
        entity_state_action action = {};
        action.type = entity_action_type::action_type_velocity;
        action.tick = 0.3f * tick_rate;
        action.optional = false;
        action.velocity = {0, 25 << 16, 0};
        current_action = 0;
        // ec.entity_state_actions[bodyType][entity_movement_state_jump][current_action++] = create_entity_action(entity_action_type::action_type_lock_movement,     0.00f * tick_rate, false);
        ec.entity_state_actions[bodyType][entity_movement_state_jump][current_action++] = action;
        ec.entity_state_actions[bodyType][entity_movement_state_jump][current_action++] = create_entity_action(entity_action_type::action_type_end_state,      0.37f * tick_rate, false);

        current_action = 0;
        ec.entity_state_actions[bodyType][entity_movement_state_land][current_action++] = create_entity_action(entity_action_type::action_type_lock_movement,     0.00f * tick_rate, false);
        ec.entity_state_actions[bodyType][entity_movement_state_land][current_action++] = create_entity_action(entity_action_type::action_type_end_state,      0.25f * tick_rate, false);

    }

    //experimental, unused since we decided on dynamic weights
    // //called on startup to populate the table. The weights are for movement animations. To get interaction weight we do 1.0f - returnedValue
    void hardcode_animation_weights(game_state* GameState){
        EntityComponent& ec = *GameState->entityComponent;
        //                  [movement][interaction]
        ec.animationWeights[0][anim_left_punch] = 0.1f;
        ec.animationWeights[1][anim_left_punch] = 0.9f;
        ec.animationWeights[0][anim_right_punch] = 0.1f;
        ec.animationWeights[1][anim_right_punch] = 0.9f;

        //everything else is zero for now, any combat animation will override any movement animation until we have a better idea
    }



/////////////////////////////////////////////////// PLAYER /////////////////////////////////////////////////////////

    void addPlayerComp(EntityComponent& ec, uint16_t entityID, uint8_t testInfo){

        PlayerComp playerComp = {};
        playerComp.test = testInfo;

        
        playerComp.angleH = 5898199; //i dunno its what the camera starts to in fpt
        playerComp.angleV = 0;
        playerComp.brushSize = FPT_ONE;

        ADD_ENTITY_COMPONENT(Player, entityID, &playerComp, MAX_PLAYERS, MAX_ENTITIES, NULL_PLAYER);

    }

    void removePlayerComp(EntityComponent& ec, uint16_t entityID) {
        REMOVE_ENTITY_COMPONENT(Player, entityID, MAX_ENTITIES, NULL_PLAYER);
    }


    void addCameraComp(game_state* GameState, EntityComponent& ec, uint16_t entityID, fpt_vec3 pos){
        CameraComp cameraComp = {};

        init_camera_comp(GameState, cameraComp, pos);
        
        ADD_ENTITY_COMPONENT(Camera, entityID, &cameraComp, MAX_PLAYERS, MAX_ENTITIES, NULL_PLAYER);

    }

    void removeCameraComp(EntityComponent& ec, uint16_t entityID) {
        REMOVE_ENTITY_COMPONENT(Camera, entityID, MAX_ENTITIES, NULL_PLAYER);
    }


//////////////////////////////////////////////////// AABB //////////////////////////////////////////////////////

    void addAabbComp(EntityComponent& ec, uint16_t entityID, AabbComp* aabbComp) {
        AabbComp givenComp = {};
        if(aabbComp){
            givenComp = *aabbComp;
        }else{
       
        }
        givenComp.flags |= aabb_dirty;

        ADD_ENTITY_COMPONENT(Aabb, entityID, &givenComp, MAX_ENTITIES, MAX_ENTITIES, NULL_ENTITY);

    }

    void removeAabbComp(EntityComponent& ec, uint16_t entityID) {
        REMOVE_ENTITY_COMPONENT(Aabb, entityID, MAX_ENTITIES, NULL_ENTITY);
    }




//////////////////////////////////////////////////// OBB //////////////////////////////////////////////////////

    void addObbComp(EntityComponent& ec, uint16_t entityID, ObbComp* givenComp) {
        ADD_ENTITY_COMPONENT(Obb, entityID, givenComp, MAX_ENTITIES, MAX_ENTITIES, NULL_ENTITY);
    }

    void removeObbComp(EntityComponent& ec, uint16_t entityID) {
        REMOVE_ENTITY_COMPONENT(Obb, entityID, MAX_ENTITIES, NULL_ENTITY);
    }




    //////////////////////////////////////////////////// TRANSFORM //////////////////////////////////////////////////////

    void addTransComp(EntityComponent& ec, uint16_t entityID, TransComp* transComp, fpt_vec3 pos) {
        TransComp givenComp = {};
        if(transComp){
            givenComp = *transComp;
            givenComp.flags |= trans_dirty;
        }else{
            givenComp.rotation = fpt_quat_create(0,0,0,FPT_ONE);
            givenComp.pos_in_chunk = fpt_vec3_create(0);
            givenComp.chunk_coords = {};
            givenComp.scale = fpt_vec3_create(FPT_ONE);
            givenComp.forward   = fpt_vec3_create(0,0,FPT_ONE);
            givenComp.up        = fpt_vec3_create(0,FPT_ONE,0);
            givenComp.right     = fpt_vec3_create(FPT_ONE,0,0);
            givenComp.flags |= trans_dirty;
            givenComp.speed = FPT_QUARTER;
        }

        ADD_ENTITY_COMPONENT(Trans, entityID, &givenComp, MAX_ENTITIES, MAX_ENTITIES, NULL_ENTITY);

    }

    void removeTransComp(EntityComponent& ec, uint16_t entityID) {
        REMOVE_ENTITY_COMPONENT(Trans, entityID, MAX_ENTITIES, NULL_ENTITY);
    }

    //////////////////////////////////////////////////// MODEL //////////////////////////////////////////////////////

    void addModelComp(EntityComponent& ec, uint16_t entityID, ModelComp* givenComp){
        ADD_ENTITY_COMPONENT(Model, entityID, givenComp, MAX_ENTITIES, MAX_ENTITIES, NULL_ENTITY);
    }

    void removeModelComp(EntityComponent& ec, uint16_t entityID){
        REMOVE_ENTITY_COMPONENT(Model, entityID, MAX_ENTITIES, NULL_ENTITY);

    }


    //////////////////////////////////////////////////// MESH //////////////////////////////////////////////////////


    void addMeshComp(EntityComponent& ec, uint16_t entityID, MeshComp* givenComp) {
        ADD_ENTITY_COMPONENT(Mesh, entityID, givenComp, MAX_ENTITIES, MAX_ENTITIES, NULL_ENTITY);
    }

 
    void removeMeshComp(EntityComponent& ec, uint16_t entityID) {
        REMOVE_ENTITY_COMPONENT(Mesh, entityID, MAX_ENTITIES, NULL_ENTITY);
    }

    //////////////////////////////////////////////////// HEALTH //////////////////////////////////////////////////////


    void addHealthComp(EntityComponent& ec, uint16_t entityID, HealthComp* givenComp) {
        ADD_ENTITY_COMPONENT(Health, entityID, givenComp, MAX_ENTITIES, MAX_ENTITIES, NULL_ENTITY);
    }

    void removeHealthComp(EntityComponent& ec, uint16_t entityID) {
        REMOVE_ENTITY_COMPONENT(Health, entityID, MAX_ENTITIES, NULL_ENTITY);
    }




    //////////////////////////////////////////////////// PHYSICS //////////////////////////////////////////////////////


    void addPhysicsComp(EntityComponent& ec, uint16_t entityID, PhysicsComp* givenComp) {
        PhysicsComp physicsComp = {0};
        if(givenComp){
            //could instead do
            physicsComp = *givenComp;
        }
        else{
            physicsComp.mass = FPT_ONE;
            physicsComp.inverse_mass = FPT_ONE;
            physicsComp.damping = FPT_EIGHTH;
            physicsComp.velocity = fpt_vec3_create(0, 0, 0);
            fpt_quat orientation = fpt_quat_create(0, 0, 0, FPT_ONE);
            fpt_flatmat34 transform_matrix = fpt_flatmat34_create();
            physicsComp.transform_matrix = transform_matrix;
            physicsComp.orientation = orientation;
            physicsComp.linear_damping  = FPT_QUARTER;
            physicsComp.angular_damping = FPT_EIGHTH;
            physicsComp.half_size = fpt_vec3_create(FPT_HALF);//half size/scale/intertia tensor will be a standard cube
            fpt_flatmat3 temp_mat = {0};
            fpt_flatmat3_set_block_inertia_tensor(temp_mat, physicsComp.half_size, physicsComp.mass);
            fpt_flatmat3_set_inverse(physicsComp.inverse_inertia_tensor, temp_mat);
        }

        ADD_ENTITY_COMPONENT(Physics, entityID, &physicsComp, MAX_ENTITIES, MAX_ENTITIES, NULL_ENTITY);

    }

    void removePhysicsComp(EntityComponent& ec, uint16_t entityID) {
        REMOVE_ENTITY_COMPONENT(Physics, entityID, MAX_ENTITIES, NULL_ENTITY);
    }



    //////////////////////////////////////////////////// STATE //////////////////////////////////////////////////////


    
    void addStateComp(EntityComponent& ec, uint16_t entityID, StateComp* givenComp){
        ADD_ENTITY_COMPONENT(State, entityID, givenComp, MAX_ENTITIES, MAX_ENTITIES, NULL_ENTITY);
    }
    void removeStateComp(EntityComponent& ec, uint16_t entityID){
        REMOVE_ENTITY_COMPONENT(State, entityID, MAX_ENTITIES, NULL_ENTITY);
    }


        //////////////////////////////////////////////////// EQUIPMENT //////////////////////////////////////////////////////

        void addInventoryComp(EntityComponent& ec, uint16_t entityID, InventoryComp* givenComp){
            ADD_ENTITY_COMPONENT(Inventory, entityID, givenComp, MAX_ENTITIES, MAX_ENTITIES, NULL_ENTITY);
        }
        void removeInventoryComp(EntityComponent& ec, uint16_t entityID){
            REMOVE_ENTITY_COMPONENT(Inventory, entityID, MAX_ENTITIES, NULL_ENTITY);
        }
        
        
    
            
        //////////////////////////////////////////////////// BODY //////////////////////////////////////////////////////
    
        void addBodyComp(EntityComponent& ec, uint16_t entityID, BodyComp* givenComp){
            ADD_ENTITY_COMPONENT(Body, entityID, givenComp, MAX_ENTITIES, MAX_ENTITIES, NULL_ENTITY);
        }
        
        void removeBodyComp(EntityComponent& ec, uint16_t entityID){
            REMOVE_ENTITY_COMPONENT(Body, entityID, MAX_ENTITIES, NULL_ENTITY);
        }
    

        
    //////////////////////////////////////////////////// DATA //////////////////////////////////////////////////////

    
    void addDataComp(EntityComponent& ec, uint16_t entityID, DataComp* givenComp){
        DataComp dataComp = {};
        if(givenComp){
            //could instead do
            dataComp = *givenComp;
        }

        ADD_ENTITY_COMPONENT(Data, entityID, &dataComp, MAX_ENTITIES, MAX_ENTITIES, NULL_ENTITY);
    }
    
    void removeDataComp(EntityComponent& ec, uint16_t entityID){
        REMOVE_ENTITY_COMPONENT(Data, entityID, MAX_ENTITIES, NULL_ENTITY);
    }


    //////////////////////////////////////////////////// ENTITY CREATION/DESTRUCTION/INIT //////////////////////////////////////////////////////



    bool isEntityActive(EntityComponent& ec, uint16_t entityID) {
        if (entityID >= MAX_ENTITIES) return false;
        if (ec.activeEntities[entityID]) return true; //if true the entity is active
        return false;
    }

    uint16_t createEntity(EntityComponent& ec, const char* name) {
        if (ec.entityCount >= MAX_ENTITIES) return NULL_ENTITY;
        uint16_t entityID = ec.entityIDQueue[ec.entityCount];

        if (ec.activeEntities[entityID]) {
            printf("Entity already exists: %s\n", getEntityName(ec, entityID));
            return entityID; //used to return NULL_ENTITY, but whats the harm in returning a valid ID if it already exists?
        }

        ec.entityIDQueue[ec.entityCount] = NULL_ENTITY; // Mark as empty

        ec.activeEntities[entityID] = true;

        // Calculate the starting index for this entity's name in the names array
        uint16_t nameIndex = entityID * MAX_NAME;
        
        // Copy the name safely
        strncpy(&ec.names[nameIndex], name, MAX_NAME - 1);
        ec.names[nameIndex + MAX_NAME - 1] = '\0'; // Ensure null-termination
    


        ec.entityCount++;
        return entityID;
    }

    void destroyEntity(EntityComponent& ec, uint16_t entityID) {
        if (entityID >= MAX_ENTITIES) return;

        if (ec.activeEntities[entityID]) {
            // printf("destroying entity: %s\n", getEntityName(ec, entityID));
            printf("destroying entityID: %d\n",  entityID);
            ec.activeEntities[entityID] = false;
        }
        else{
            printf("Entity already destroyed: %s\n", getEntityName(ec, entityID));
            return;
        }

        if (ec.entityToDataMap      [entityID]  != NULL_ENTITY) removeDataComp      (ec, entityID);
        if (ec.entityToTransMap     [entityID]  != NULL_ENTITY) removeTransComp     (ec, entityID);
        if (ec.entityToModelMap     [entityID]  != NULL_ENTITY) removeModelComp     (ec, entityID);
        if (ec.entityToMeshMap      [entityID]  != NULL_ENTITY) removeMeshComp      (ec, entityID);
        if (ec.entityToHealthMap    [entityID]  != NULL_ENTITY) removeHealthComp    (ec, entityID);
        if (ec.entityToPhysicsMap   [entityID]  != NULL_ENTITY) removePhysicsComp   (ec, entityID);
        if (ec.entityToAabbMap      [entityID]  != NULL_ENTITY) removeAabbComp      (ec, entityID);
        if (ec.entityToObbMap       [entityID]  != NULL_ENTITY) removeObbComp       (ec, entityID);
        if (ec.entityToStateMap     [entityID]  != NULL_ENTITY) removeStateComp     (ec, entityID);
        if (ec.entityToInventoryMap [entityID]  != NULL_ENTITY) removeInventoryComp (ec, entityID);
        if (ec.entityToBodyMap      [entityID]  != NULL_ENTITY) removeBodyComp      (ec, entityID);
        if (ec.entityToPlayerMap    [entityID]  != NULL_PLAYER) removePlayerComp    (ec, entityID);
        if (ec.entityToCameraMap    [entityID]  != NULL_PLAYER) removeCameraComp    (ec, entityID);


        ec.versionIDs[entityID]++;

        ec.entityCount--;
        ec.entityIDQueue[ec.entityCount] = entityID;



    }

    void ecs_init(game_state* GameState, EntityComponent& ec) {
        for (uint16_t i = 0; i < MAX_ENTITIES; i++) {
            ec.entityIDQueue[i]         = i;
            ec.entityToTransMap[i]      = NULL_ENTITY;
            ec.entityToMeshMap[i]       = NULL_ENTITY;
            ec.entityToModelMap[i]      = NULL_ENTITY;
            ec.entityToHealthMap[i]     = NULL_ENTITY;
            ec.entityToPhysicsMap[i]    = NULL_ENTITY;
            ec.entityToAabbMap[i]       = NULL_ENTITY;
            ec.entityToObbMap[i]        = NULL_ENTITY;
            ec.entityToStateMap[i]      = NULL_ENTITY;
            ec.entityToBodyMap[i]       = NULL_ENTITY;
            ec.entityToInventoryMap[i]  = NULL_ENTITY;
            ec.entityToDataMap[i]       = NULL_ENTITY;

            ec.entityToPlayerMap[i]     = NULL_PLAYER;
            ec.entityToCameraMap[i]     = NULL_PLAYER;
            ec.liveEntities[i]          = false;
            ec.activeEntities[i]        = false;
            ec.MeshComps[i].meshIndex   = NULL_MESH;
            ec.body_parts[i] = {};
            ec.inventory_items[i] = {};
            ec.versionIDs[i] = 0;


            //specific component clearing logic
            ec.DataComps[i].entity_interaction_lookup_chunkID = NULL_CHUNK;
            ec.DataComps[i].entity_interaction_lookup_index = NULL_ENTITY;
        }
        for(uint8_t i = 0; i < MAX_PLAYERS; i++){
            ec.PlayerToEntityMap[i] = NULL_PLAYER;
            ec.CameraToEntityMap[i] = NULL_PLAYER;
        }
        ec.TransCount           = 0;
        ec.MeshCount            = 0;
        ec.ModelCount           = 0;
        ec.HealthCount          = 0;
        ec.PhysicsCount         = 0;
        ec.StateCount           = 0;
        ec.ObbCount             = 0;
        ec.AabbCount            = 0;
        ec.InventoryCount       = 0;
        ec.BodyCount            = 0;
        ec.PlayerCount          = 0;
        ec.CameraCount          = 0;
        
        ec.entityCount          = 0;
        
        //seperate arrays that the entities index into from their components
        ec.inventory_item_count = 1; //0th slot means empty, no item goes there. like a sentinel value
        ec.body_part_count      = 0;

        // ec.enlargeAABBAmount = fpt_add(FPT_ONE, FPT_HALF);
        ec.enlargeAABBAmount = FPT_ONE;
        // ec.enlargeAABBAmountLast = fpt_add(FPT_ONE, FPT_HALF);
        ec.enlargeAABBAmountLast = FPT_ONE;
        ec.enlargeAABB = true;

        hardcode_entity_actions(GameState);
        hardcode_animation_weights(GameState);

    }



