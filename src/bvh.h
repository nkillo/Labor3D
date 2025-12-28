#pragma once
#include "chunkManager.h"
#include "ray.h"


    fpt get_Total_Best_Cost(bvh_tree& tree);
    fpt get_Average_Best_Cost(bvh_tree& tree);
    int get_Total_Node_Pairings(bvh_tree& tree);

    int get_Total_Internal_Nodes(bvh_tree& tree);
    int get_Total_Leaf_Nodes(bvh_tree& tree);

    int get_Total_Allocated_Nodes(bvh_tree& tree);

    AABB Union(AABB a, AABB b);

    bool isLeaf(bvh_tree& tree, uint16_t node);

    void printTree(bvh_tree& tree, uint16_t node, int depth = 0);

    void updateTreeToFile(bvh_tree& tree);

    fpt SurfaceArea(AABB& box);

    int CountPrimitivesInNode(bvh_tree& tree, uint16_t node, int recursion_count = 0);

    fpt CalculateSAHCost(bvh_tree& tree, uint16_t currentNode, uint16_t insertedNode, fpt surfaceAreaInsertedNode);

    fpt ComputeCostInternalNodes(bvh_tree& tree);

    uint16_t AllocateLeafNode(bvh_tree& tree, uint32_t entityID, const AABB& box);

    uint16_t AllocateInternalNode(bvh_tree& tree);

    fpt CalculateCombinedSurfaceArea(bvh_tree& tree, uint16_t node);

    void UpdateBoundingBoxes(bvh_tree& tree, uint16_t node);

    void PerformLeftRotation(bvh_tree& tree, uint16_t node);
    void PerformRightRotation(bvh_tree& tree, uint16_t node);
    void PerformLeftRightRotation(bvh_tree& tree, uint16_t node);
    void PerformRightLeftRotation(bvh_tree& tree, uint16_t node);

    fpt CalculateCombinedSurfaceAreaAfterLeftRotation(bvh_tree& tree, uint16_t node);
    fpt CalculateCombinedSurfaceAreaAfterRightRotation(bvh_tree& tree, uint16_t node);
    fpt CalculateCombinedSurfaceAreaAfterLeftRightRotation(bvh_tree& tree, uint16_t node);
    fpt CalculateCombinedSurfaceAreaAfterRightLeftRotation(bvh_tree& tree, uint16_t node);

    bool Rotate(bvh_tree& tree, uint16_t node);

    uint16_t PickBest(bvh_tree& tree, uint16_t insertedNode, uint16_t oldSibling = NULL_NODE);

    void InsertLeaf(bvh_tree& tree, uint32_t entityID, AABB box, int primitiveCount, uint16_t bestPick = NULL_NODE);

    uint16_t createNode(bvh_tree& tree, uint32_t entityID = NULL_NODE);
    void destroyNode(bvh_tree& tree, uint16_t nodeID, uint32_t entityID = NULL_NODE);
    
    void reset_bvh(chunk_data* chunkData, uint32_t chunkID);
    void reset(game_state* GameState);
    int ReinsertLeaf(bvh_tree& tree, uint32_t entityID, AABB box, int primitiveCount);

    bool removeLeafNode(bvh_tree& tree, uint32_t entityID);


    uint16_t rayTraverseNode(game_state* GameState, bvh_tree& tree, uint16_t node, fpt_vec3 cameraPosition, fpt_vec3 relativePosition, fpt_vec3 ray, fpt& closestDistance, fpt_vec3& collisionPosition);
    
    int start_broad_phase_collision_check(game_state* GameState, bvh_tree& tree, uint32_t chunkID = NULL_CHUNK);

    int broad_phase_collision_check(game_state* GameState, bvh_tree& tree, uint16_t node, uint16_t other_node, int depth);

    int broad_phase_collision_entity_voxel(game_state *GameState, bvh_tree& tree, uint16_t node, CoarseBrickmap64* coarse = nullptr);

   // Set entity bit in specific chunk
    // void setEntityInChunk(bvh_tree& tree, uint16_t chunkID, uint32_t entityID);

    // // Test if entity exists in chunk
    // bool isEntityInChunk(bvh_tree& tree, uint16_t chunkID, uint32_t entityID);

    // // Clear entity bit in chunk
    // void removeEntityFromChunk(bvh_tree& tree, uint16_t chunkID, uint32_t entityID);

