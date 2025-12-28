// //*

#pragma once

// #include "labour/core/memory.h"
// #include "labour/core/inputs.h"


    void findAndEnqueueChunks(game_state* GameState);

    void recalculateViewFrustum(game_state* GameState);
    
    void drag_entity(game_state* GameState, fpt_vec3 rayDir, fpt_vec3 rayOrigin, ivec3 raychunk_coords);


    void chunk_camera_updated(game_state* GameState);
    void chunk_start(game_state* GameState);
    void chunk_update(game_state* GameState);

    void chunk_processInput(game_state* GameState, player_input& lastInput, player_input& currInput, uint32_t playerEntityID = NULL_ENTITY);
    void chunk_entityDestroyed(game_state* GameState, uint32_t entityID);
    void chunk_reload(game_state* GameState);
    void chunk_cleanup(game_state* GameState);
