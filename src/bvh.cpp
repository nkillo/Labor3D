#include "bvh.h"
#include "boxIntersect.h"
#include "voxelHelpers.h"

// // #include "labour/config/configStructs.h"

    
    constexpr int rotation_Threshold = 5;
    constexpr int update_Primitives_Threshold = 5;
    constexpr int inherited_Cost_Max_Depth = 10;
    constexpr fpt average_Best_Cost_Threshold = i2fpt(70); //determined to be about 70, adjust as needed


//     float get_Total_Best_Cost(bvh_tree& treetree){return tree.total_Pick_Best_Cost;}
//     float get_average_Best_Cost(bvh_tree& treetree){return tree.average_Best_Cost;}

//     int get_total_Node_Pairings(bvh_tree& treetree){return tree.total_Node_Pairings;}

//     int get_total_Internal_Nodes(bvh_tree& treetree) { return tree.total_Internal_Nodes; }
//     int get_total_Leaf_Nodes(bvh_tree& treetree) { return tree.total_Leaf_Nodes; }

//     int get_total_Allocated_Nodes(bvh_tree& treetree) {return tree.total_Allocated_Nodes; }


//     void setTotalAllocatedNodes(bvh_tree& treetree, int value) {
//         tree.total_Allocated_Nodes = value;
//     }
//     // In a .cpp file
//     // PoolAllocator<BVHNode>& GetNodePoolAllocator() {
//     //     static PoolAllocator<BVHNode> nodePoolAllocator(1000 * sizeof(BVHNode), sizeof(BVHNode), true);
//     //     return nodePoolAllocator;
//     // }

    void print_aabb_values(AABB a){
        printf("min: %6.2f %6.2f %6.2f", fpt2fl(a.min.x),fpt2fl(a.min.y),fpt2fl(a.min.z));
        printf("min: %6.2f %6.2f %6.2f", fpt2fl(a.max.x),fpt2fl(a.max.y),fpt2fl(a.max.z));
        printf("area:%6.2f", fpt2fl(SurfaceArea(a)));

    }

    void print_node_aabb_values(bvh_tree& tree, uint16_t node){
        AABB a = tree.box[node];
        printf("n%d min: %6.2f %6.2f %6.2f", node, fpt2fl(a.min.x),fpt2fl(a.min.y),fpt2fl(a.min.z));
        printf("n%d min: %6.2f %6.2f %6.2f", node, fpt2fl(a.max.x),fpt2fl(a.max.y),fpt2fl(a.max.z));
        printf("n%d area:%6.2f", node, fpt2fl(SurfaceArea(a)));
    }


    AABB Union(AABB a, AABB b){ //would probably benefit from SIMD
        AABB result;
        result.min = fpt_vec3_min(a.min, b.min);
        result.max = fpt_vec3_max(a.max, b.max);
        return result;
    }

    bool isLeaf(bvh_tree& tree, uint16_t node){
        return tree.left[node] == NULL_NODE && tree.right[node] == NULL_NODE;
    }

    //*
    void printTree(bvh_tree& tree, uint16_t node, int depth) {
        bool isLeafBool = false;
        if(node == NULL_NODE) return;
        if (isLeaf(tree, node)) {
            isLeafBool = true;
        }
        char label[128]; // Increased size to accommodate indentation
        //determine if left or right node
        const char* orientation = "";
        uint16_t parent_node = tree.parent[node];
        if ( parent_node != NULL_NODE) {
            if (node ==  tree.left[parent_node]) {
                orientation = "Left";
            } else if (node == tree.right[parent_node]) {
                orientation = "Right";
            }
        }

        //determine label

        if (isLeafBool) {
            sprintf(label, "%*s%s Child n%d, Entity: %u", depth * 5, "", orientation, node, tree.nodeToEntityID[node]);  // party for leaves
        } else {
            char entityStr[50]; // Buffer for the entity string
            snprintf(entityStr, sizeof(entityStr), "%u", tree.nodeToEntityID[node]); // Assuming it's an unsigned integer

            sprintf(label, "%*s%s %s n%d%s%s", depth * 5, "", orientation,  tree.parent[node] ? "Internal" : "Root", node, tree.nodeToEntityID[node] != NULL_ENTITY ? ", Entity: " : "", tree.nodeToEntityID[node] != NULL_ENTITY ? entityStr : "");

        }
        fpt area = SurfaceArea(tree.box[node]);
        
        printf("%s", label);
        printf(" area: %f\n", fpt2fl(area));
        if(!isLeafBool){
            printTree(tree, tree.left[node], depth + 1);
            printTree(tree, tree.right[node], depth + 1);
        }
    }
    //*/

//     void printTreeToFile(uint16_t node, std::ofstream& file, int depth) {
//         if (node == NULL_NODE) return;
        
//         // Still using your charmingly oversized char array
//         char label[4096];
//         const char* orientation = "";

//         if (node tree.parent[]) {
//             orientation = (node == node tree.parent[] tree.left[]) ? "Left" : "Right";
//         }

//         if (isLeaf(tree, node)) {
//             sprintf(label, "%*s%s Child n%d, Entity: %zu", depth * 5, "", orientation, node->id, static_cast<size_t>(*node->entity));
//         } else {
//             char entityStr[50]; // Buffer for the entity string
//             snprintf(entityStr, sizeof(entityStr), "%zu", static_cast<size_t>(*node->entity)); // Assuming it's an unsigned integer

//             sprintf(label, "%*s%s %s n%d%s%s", depth * 5, "", orientation, node tree.parent[] ? "Internal" : "Root", node->id, *node->entity != entt::null ? ", Entity: " : "", *node->entity != entt::null ? entityStr : "");

//         }

//         file << label << std::endl;

//         if (!isLeaf(tree, node)) {
//             printTreeToFile(node tree.left[], file, depth + 1);
//             printTreeToFile(node tree.right[], file, depth + 1);
//         }
//     }

// void updateTreeToFile(bvh_tree& treetree){
//         std::ofstream file("tree_structure.txt", std::ios::trunc); // open in truncate mode to overwrite
//         printTreeToFile(tree.rootNode, file);
//         file.close();

//     }
    fpt SurfaceArea(AABB& box){
        fpt_vec3 d = box.max - box.min;
        //float result = 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
        //printf("SurfaceArea of box: {}", result);
        //return result;
        return fpt_mul(FPT_TWO , fpt_add(fpt_mul(d.x , d.y),fpt_add(fpt_mul(d.y , d.z) , fpt_mul(d.z , d.x))));
    }

    //this can stack overflow on us, need to figure out more debugs to wrap it in
    int CountPrimitivesInNode(bvh_tree& tree, uint16_t node, int recursion_count) {
        if (node >= NULL_NODE)return 0;//node isnt active
        if (isLeaf(tree, node)) {
            // leaf nodes will only contain one entity/primitive(AABB)
            return  tree.primitiveCount[node];
        } else {
            // Recursively count primitives in the children
            int count = 0;
            
            recursion_count++;
            assert(recursion_count < 5000 && "MAX RECURSION EXCEEDED?? ERROR");
            
            count += CountPrimitivesInNode(tree, tree.left [node] , recursion_count);
            count += CountPrimitivesInNode(tree, tree.right[node], recursion_count);
            //printf("Count primitives in node: n{}, count: {}", node->id, count);
             tree.primitiveCount[node] = count;

            return count;
        }
    }

    fpt CalculateSAHCost(bvh_tree& tree, uint16_t currentNode, uint16_t insertedNode, fpt surfaceAreaInsertedNode) {
        if(surfaceAreaInsertedNode <= FPT_ONE){//preventing divide by 0 or large numbers from tiny number division
            return FPT_ONE;
        }
        //printf("[BVH] CalculateSAHCost between currentNode: n{}, insertedNode: n{}", currentNode->id, insertedNode->id);
        // Constants (These should be chosen based on your specific requirements and profiling)
        const fpt C_trav = FPT_ONE; // Cost of traversing a node
        const fpt C_int  = FPT_ONE;  // Cost of intersecting a primitive

        // Calculate combined bounding box of the currentNode and insertedNode
        AABB combinedBox = Union( tree.box[currentNode],  tree.box[insertedNode]);
        
        ///*TAG*/printf("current node : {} BOX", currentNode);
        ///*TAG*/printf("MIN: {} {} {}", fpt2fl(tree.box[currentNode].min.x),fpt2fl(tree.box[currentNode].min.y),fpt2fl(tree.box[currentNode].min.z));
        ///*TAG*/printf("MAX: {} {} {}", fpt2fl(tree.box[currentNode].max.x),fpt2fl(tree.box[currentNode].max.y),fpt2fl(tree.box[currentNode].max.z));
        
        ///*TAG*/printf("inserted node: {} BOX", insertedNode);
        ///*TAG*/printf("MIN: {} {} {}", fpt2fl(tree.box[insertedNode].min.x),fpt2fl(tree.box[insertedNode].min.y),fpt2fl(tree.box[insertedNode].min.z));
        ///*TAG*/printf("MAX: {} {} {}", fpt2fl(tree.box[insertedNode].max.x),fpt2fl(tree.box[insertedNode].max.y),fpt2fl(tree.box[insertedNode].max.z));

        ///*TAG*/printf("COMBINED BOX", insertedNode);
        ///*TAG*/printf("MIN: {} {} {}", fpt2fl(combinedBox.min.x),fpt2fl(combinedBox.min.y),fpt2fl(combinedBox.min.z));
        ///*TAG*/printf("MAX: {} {} {}", fpt2fl(combinedBox.max.x),fpt2fl(combinedBox.max.y),fpt2fl(combinedBox.max.z));


        fpt combinedSurfaceArea = SurfaceArea(combinedBox);

        // Estimate the number of primitives in currentNode and insertedNode
        //store entity count in node for easy access
        //used to countPrimitives every time, now we count them after a certain number of insertions/deletions
        // int N_current =   tree.primitiveCount[currentNode];//CountPrimitivesInNode(currentNode); 
        // int N_inserted =  tree.primitiveCount[insertedNode];
        // if(N_current == 0)N_current = CountPrimitivesInNode(tree, currentNode);

        //printf("N_current: {}, N_inserted: {}", N_current, N_inserted);
        fpt surfaceAreaCurrentNode = SurfaceArea( tree.box[currentNode]);
        ///*TAG*/fpt old_directCost = fpt_add(C_trav , fpt_add(fpt_mul(fpt_div(surfaceAreaCurrentNode , combinedSurfaceArea) , fpt_mul(N_current , C_int)) , fpt_mul(fpt_div(surfaceAreaInsertedNode , combinedSurfaceArea) , fpt_mul(N_inserted , C_int))));
        fpt directCost     = fpt_add(C_trav , fpt_add(fpt_mul(fpt_div(surfaceAreaCurrentNode , combinedSurfaceArea) , C_int) , fpt_mul(fpt_div(surfaceAreaInsertedNode , combinedSurfaceArea) , C_int)));
        ///*TAG*/printf("old direct cost: {}", fpt2fl(old_directCost));
        ///*TAG*/printf("new direct cost: {}", fpt2fl(directCost));
        //printf("directCost = {} + (({} / {}) * {} * {}) + (({} / {}) * {} * {}) = {}", C_trav, surfaceAreaCurrentNode, combinedSurfaceArea, N_current, C_int, surfaceAreaInsertedNode, combinedSurfaceArea, N_inserted, C_int, directCost);
        
        // Initialize inherited cost
        fpt inheritedCost = 0;

        // Calculate the inherited cost
        uint16_t parentPtr=  tree.parent[currentNode];
        int depth = 0;
        while (parentPtr != NULL_NODE) {
            AABB oldParentBox =  tree.box[parentPtr];
            AABB newParentBox = Union(oldParentBox, combinedBox);
            inheritedCost = fpt_add(inheritedCost ,fpt_sub(SurfaceArea(newParentBox) , SurfaceArea(oldParentBox)));

            // Set up for next iteration
            combinedBox = newParentBox;
            parentPtr =  tree.parent[parentPtr];
            depth++;
            if (depth > inherited_Cost_Max_Depth) {
                //printf("inheritedCost exceeded max depth of {}, breaking", inherited_Cost_Max_Depth);
                break;
            }
        }

        fpt totalCost = fpt_add(directCost , inheritedCost);
        ///*TAG*/printf("Total SAH Cost of potentialSibling n{} and insertion n{}: {}", currentNode,insertedNode, fpt2fl(totalCost));
        return totalCost;
    }


    // float ComputeCostInternalNodes(bvh_tree& treetree) {
    //     // Look ma, no hands! (i.e., no manual stack management)
    //     std::function<float(uint16_t)> computeCost = [&](uint16_t node) -> float {
    //         if (!node || isLeaf(tree, node)) return 0.0f; // Simplified null and leaf checks
    //         return SurfaceArea(node tree.box[]) + computeCost(node tree.left[]) + computeCost(node tree.right[]);
    //     };

    //     return computeCost(tree.rootNode); // Kick off the recursion
    // }


    uint16_t AllocateLeafNode(bvh_tree& tree, uint32_t entityID, const AABB& box) {
        uint16_t leafID = createNode(tree, entityID);
        tree.entityToNodeID[entityID] = leafID;
        tree.nodeToEntityID[leafID] = entityID;

        if(leafID == NULL_NODE) return leafID;

        tree.box[leafID] = box;
        tree.left[leafID] = NULL_NODE;
        tree.right[leafID] = NULL_NODE;
        tree.parent[leafID] = NULL_NODE;
        tree.primitiveCount[leafID] = 0;

        tree.total_Leaf_Nodes++;
        return leafID;
    }


    uint16_t AllocateInternalNode(bvh_tree& tree) {
        uint16_t nodeID = createNode(tree);
        if(nodeID == NULL_NODE) return nodeID;



        tree.left[nodeID] = NULL_NODE;
        tree.right[nodeID] = NULL_NODE;
        tree.parent[nodeID] = NULL_NODE;
        tree.primitiveCount[nodeID] = 0;

        tree.box[nodeID] = {};
        tree.total_Internal_Nodes++;

        return nodeID;
  
    }

    fpt CalculateCombinedSurfaceArea(bvh_tree& tree, uint16_t nodeID) {
        if (nodeID == NULL_NODE) return 0.0f;

        fpt totalArea = SurfaceArea(tree.box[nodeID]);
        //if (node tree.left[]) totaFlArea += SurfaceArea(node tree.left[] tree.box[]);
        //if (node tree.right[]) totalArea += SurfaceArea(node tree.right[] tree.box[]);

        return totalArea;

    }

    void UpdateBoundingBoxes(bvh_tree& tree, uint16_t node) {
        // Traverse up the tree, updating bounding boxes
        while (node != NULL_NODE &&  tree.left[node] != NULL_NODE &&  tree.right[node] != NULL_NODE) {
            //printf("Updating Bounding Boxes for node: {}", node->id);
            //printf("node tree.left[] tree.box[].min:  {}, {}, {},   MAX: {}, {}, {} ", node tree.left[] tree.box[].min.x, node tree.left[] tree.box[].min.y, node tree.left[] tree.box[].min.z, node tree.left[] tree.box[].max.x, node tree.left[] tree.box[].max.y, node tree.left[] tree.box[].max.z);
            //printf("node tree.right[] tree.box[].min: {}, {}, {},   MAX: {}, {}, {} ", node tree.right[] tree.box[].min.x, node tree.right[] tree.box[].min.y, node tree.right[] tree.box[].min.z, node tree.right[] tree.box[].max.x, node tree.right[] tree.box[].max.y, node tree.right[] tree.box[].max.z);
            tree.box[node] = Union(tree.box[tree.left[node]], tree.box[tree.right[node]]);
            //printf("UPDATEDnode tree.box[].min: {}, {}, {},   MAX: {}, {}, {} ", node tree.box[].min.x, node tree.box[].min.y, node tree.box[].min.z,node tree.box[].max.x, node tree.box[].max.y, node tree.box[].max.z);
            if(node == tree.parent[node]){printf("UPDATE BOUNDING BOXES INFINITE LOOP BREAKOUT");printf("UPDATE BOUNDING BOXES INFINITE LOOP BREAKOUT");printf("UPDATE BOUNDING BOXES INFINITE LOOP BREAKOUT"); return;}
            node = tree.parent[node];
        }
    }


    void PerformLeftRotation(bvh_tree& tree, uint16_t node) {
        //printf("[BVH ROTATE] Performing left rotation on node: n{}", node->id);
        uint16_t rightChild =  tree.right[node];

        //bind node to rightChild's left child
        tree.right[node] =  tree.left[rightChild];
        tree.parent[tree.left[rightChild]] = node;

        tree.parent[rightChild] =  tree.parent[node];
        if (tree.parent[node] == NULL_NODE) {
            // node was root
            tree.rootNode = rightChild;
        }
        else if (node == tree.left[tree.parent[node]]) {
            tree.left[tree.parent[node]] = rightChild;
        }
        else {
            tree.right[tree.parent[node]] = rightChild;
        }

        tree.left[rightChild] = node;
        tree.parent[node] = rightChild;

        // Update bounding boxes
        UpdateBoundingBoxes(tree, node);
        UpdateBoundingBoxes(tree, rightChild);
    }

    void PerformRightRotation(bvh_tree& tree, uint16_t node) {
        //printf("[BVH ROTATE] Performing right rotation on node: n{}", node->id);
        uint16_t leftChild =  tree.left[node];

         tree.left[node] =  tree.right[leftChild];
        tree.parent[tree.right[leftChild]] = node;

         tree.parent[leftChild] =  tree.parent[node];
        if (tree.parent[node] == NULL_NODE) {
            // node was root
            tree.rootNode = leftChild;
        }
        else if (node == tree.right[tree.parent[node]]) {
              tree.right[tree.parent[node]] = leftChild;
        }
        else {
              tree.left[tree.parent[node]] = leftChild;
        }

         tree.right[leftChild] = node;
         tree.parent[node] = leftChild;

        // Update bounding boxes
        UpdateBoundingBoxes(tree, node);
        UpdateBoundingBoxes(tree, leftChild);
    }

    void PerformLeftRightRotation(bvh_tree& tree, uint16_t node) {
        PerformLeftRotation(tree, tree.left[node]);
        PerformRightRotation(tree, node);
    }

    void PerformRightLeftRotation(bvh_tree& tree, uint16_t node) {
        PerformRightRotation(tree, tree.right[node]);
        PerformLeftRotation(tree, node);     
    }

    fpt CalculateCombinedSurfaceAreaAfterLeftRotation(bvh_tree& tree, uint16_t node) {
        ///*TAG*/printf("calculate left rotation:");
        // Assume left rotation is performed
        uint16_t rightChild =  tree.right[node];
        uint16_t rightLeftChild =  tree.left[rightChild];
        uint16_t rightRightChild =  tree.right[rightChild];
        AABB null_aabb = {0};
        // Calculate the new combined area
        AABB newLeftChildBox = Union( (tree.left[node] < NULL_NODE) ?   tree.box[tree.left[node]] : null_aabb, (rightLeftChild < NULL_NODE) ?  tree.box[rightLeftChild] : null_aabb);
        ///*TAG*/printf("new left child box:");
        ///*TAG*/print_aabb_values(newLeftChildBox);

        AABB newParentBox = Union(newLeftChildBox, (rightRightChild < NULL_NODE) ?  tree.box[rightRightChild] : null_aabb);
        ///*TAG*/printf("new parent box:");
        ///*TAG*/print_aabb_values(newParentBox);

        return SurfaceArea(newParentBox);// + SurfaceArea(newLeftChildBox);

    }

    fpt CalculateCombinedSurfaceAreaAfterRightRotation(bvh_tree& tree, uint16_t node) {
        ///*TAG*/printf("calculate right rotation:");
        uint16_t leftChild =  tree.left[node];
        uint16_t leftLeftChild =  tree.left[leftChild];
        uint16_t leftRightChild =  tree.right[leftChild];
        AABB null_aabb = {0};

        AABB newRightChildBox = Union( (tree.right[node] < NULL_NODE) ?   tree.box[tree.right[node]] : null_aabb, (leftRightChild < NULL_NODE) ?  tree.box[leftRightChild] : null_aabb);
        ///*TAG*/printf("new right child box:");
        ///*TAG*/print_aabb_values(newRightChildBox);

        AABB newParentBox = Union(newRightChildBox, (leftLeftChild < NULL_NODE) ?  tree.box[leftLeftChild] : null_aabb);
        ///*TAG*/printf("new parent box:");
        ///*TAG*/print_aabb_values(newParentBox);

        return SurfaceArea(newParentBox);// + SurfaceArea(newRightChildBox);

    }

    fpt CalculateCombinedSurfaceAreaAfterLeftRightRotation(bvh_tree& tree, uint16_t node) {
        ///*TAG*/printf("calculate left right rotation:");

        uint16_t LL =   tree.left[tree.left[node]];
        uint16_t LR =   tree.right[tree.left[node]];
        uint16_t LRL =  tree.left[LR];
        uint16_t LRR =  tree.right[LR];
        AABB null_aabb = {0};

        AABB newLeftBox = Union((LL < NULL_NODE) ?  tree.box[LL] : null_aabb, (LRL < NULL_NODE) ?  tree.box[LRL] : null_aabb);
        ///*TAG*/printf("new left box:");
        ///*TAG*/print_aabb_values(newLeftBox);
        
        AABB newRightBox = Union( (tree.right[node] < NULL_NODE) ?   tree.box[tree.right[node]] : null_aabb, (LRR < NULL_NODE) ?  tree.box[LRR] : null_aabb);
        ///*TAG*/printf("new right box:");
        ///*TAG*/print_aabb_values(newRightBox);
        
        AABB newParentBox = Union(newLeftBox, newRightBox);
        ///*TAG*/printf("new parent box:");
        ///*TAG*/print_aabb_values(newParentBox);

        return SurfaceArea(newParentBox);// + SurfaceArea(newLeftBox) + SurfaceArea(newRightBox);

    }

    fpt CalculateCombinedSurfaceAreaAfterRightLeftRotation(bvh_tree& tree, uint16_t node) {
        ///*TAG*/printf("calculate right left rotation:");
        uint16_t RL =   tree.left[tree.right[node]];
        uint16_t RR =   tree.right[tree.right[node]];
        uint16_t RLL =  tree.left[RL];
        uint16_t RLR =  tree.right[RL];
        AABB null_aabb = {0};

        AABB newLeftBox = Union( (tree.left[node] < NULL_NODE) ?   tree.box[tree.left[node]] : null_aabb, (RLL < NULL_NODE) ?  tree.box[RLL] : null_aabb);
        ///*TAG*/printf("new left box:");
        ///*TAG*/print_aabb_values(newLeftBox);

        AABB newRightBox = Union((RR < NULL_NODE) ?  tree.box[RR] : null_aabb, (RLR < NULL_NODE) ?  tree.box[RLR] : null_aabb);
        ///*TAG*/printf("new right box:");
        ///*TAG*/print_aabb_values(newRightBox);

        AABB newParentBox = Union(newLeftBox, newRightBox);
        ///*TAG*/printf("new parent box:");
        ///*TAG*/print_aabb_values(newParentBox);

        return SurfaceArea(newParentBox);// + SurfaceArea(newLeftBox) + SurfaceArea(newRightBox);
    }

    bool Rotate(bvh_tree& tree, uint16_t node) {
        bool rotated = false;
        ///*TAG*/printTree(tree, node);
        // if(tree.flags.bvh_Debug_Log){
        //     //printf("ROTATING, current tree: ");
        //     printTree(tree.rootNode);
        //     updateTreeToFile(tree);
        // }
        //if (!node || isLeaf(tree, node)) return;
        //printf("ROTATE() node tree.box[].min: {}, {}, {},   MAX: {}, {}, {} ", node tree.box[].min.x, node tree.box[].min.y, node tree.box[].min.z, node tree.box[].max.x, node tree.box[].max.y, node tree.box[].max.z);

        //float currentArea = CalculateCombinedSurfaceArea(node);
        fpt bestArea = SurfaceArea( tree.box[node]);//currentArea;
        // std::function<void(chunk_data&, uint16_t)> bestRotation = NULL_NODE;
        bool right = false;
        bool rightleft = false;
        bool left = false;
        bool leftright = false;
        bool bestrotation = false;

        
        float debugBestArea = fpt2fl(bestArea);
        ///*TAG*/printf("[BVH ROTATE] bestArea: {} node: {}", debugBestArea, node);
        ///*TAG*/print_node_aabb_values(tree, node);

        // Check each rotation case
        //can rotate left
        if ( tree.right[node] < NULL_NODE && !isLeaf(tree, tree.right[node])){
            ///*TAG*/printf("BVH CAN ROTATE LEFT");
            fpt leftRotationArea = CalculateCombinedSurfaceAreaAfterLeftRotation(tree, node);
            ///*TAG*/printf("left rotation area: {}", fpt2fl(leftRotationArea));
            if (leftRotationArea < bestArea) {
                bestArea = leftRotationArea;
                // bestRotation = PerformLeftRotation;
                right = false;
                rightleft = false;
                left = true;
                leftright = false;
                bestrotation = true;

            }

            //can rotate right-left
            if (  tree.left[tree.right[node]]< NULL_NODE && !isLeaf(tree, tree.left[tree.right[node]])) {
                ///*TAG*/printf("BVH CAN ROTATE RIGHT-LEFT");

                fpt rightLeftRotationArea = CalculateCombinedSurfaceAreaAfterRightLeftRotation(tree, node);
                ///*TAG*/printf("right left rotation area: {}", fpt2fl(rightLeftRotationArea));

                if (rightLeftRotationArea < bestArea) {
                    bestArea = rightLeftRotationArea;
                    // bestRotation = PerformRightLeftRotation;
                    right = false;
                    rightleft = true;
                    left = false;
                    leftright = false;
                    bestrotation = true;


                }
            }
        }


        //can rotate right
        if ( tree.left[node]< NULL_NODE && !isLeaf(tree, tree.left[node])) {
            ///*TAG*/printf("BVH CAN ROTATE RIGHT");

            fpt rightRotationArea = CalculateCombinedSurfaceAreaAfterRightRotation(tree, node);
            ///*TAG*/printf("right rotation area: {}", fpt2fl(rightRotationArea));
            if (rightRotationArea < bestArea) {
                bestArea = rightRotationArea;
                // bestRotation = PerformRightRotation;
                right = true;
                rightleft = false;
                left = false;
                leftright = false;
                bestrotation = true;

            }

            //can rotate left-right
            if (  tree.right[tree.left[node]]< NULL_NODE && !isLeaf(tree, tree.right[tree.left[node]])){
                ///*TAG*/printf("BVH CAN ROTATE LEFT-RIGHT");
                fpt leftRightRotationArea = CalculateCombinedSurfaceAreaAfterLeftRightRotation(tree, node);
                ///*TAG*/printf("left right rotation area: {}", fpt2fl(leftRightRotationArea));
                if (leftRightRotationArea < bestArea) {
                    bestArea = leftRightRotationArea;
                    // bestRotation = PerformLeftRightRotation;
                    right = false;
                    rightleft = false;
                    left = false;
                    leftright = true;
                    bestrotation = true;

                }
            }
        }

        if (bestrotation) {
            //printf("[BVH ROTATE] bestArea predicted: {}", bestArea);
            if(right){PerformRightRotation(tree, node);}
            else if(rightleft){PerformRightLeftRotation(tree, node);}
            else if(left){PerformLeftRotation(tree, node);}
            else if(leftright){PerformLeftRightRotation(tree, node);}
            // bestRotation(tree, node);
            rotated = true;
        }

        //printf("JUST BEFORE UPDATEBOUNDINGBOXES() node tree.box[].min: {}, {}, {},   MAX: {}, {}, {} ", node tree.box[].min.x, node tree.box[].min.y, node tree.box[].min.z, node tree.box[].max.x, node tree.box[].max.y, node tree.box[].max.z);
        //printf("node {}", node->id);
        UpdateBoundingBoxes(tree, node);
        return true;

    }


    
//     // TODO: limit queue size, to 10 maybe
//     //  find first parent that encapsulates the node, scan its children
//     //  use a first fit threshold/value to find the first node that fits to save time
//     //  tree rebuilding every few minutes to correct aggressive optimizations
    constexpr int  MAX_QUEUE_SIZE = 1024;

    uint16_t PickBest(bvh_tree& tree, uint16_t insertedNode, uint16_t oldSibling) {
    // static auto compare = [](const std::pair<float, uint16_t>& a, const std::pair<float, uint16_t>& b) {
    //     return a.first > b.first; // We want the node with the smallest cost
    // };
        // fpt costQueue[MAX_QUEUE_SIZE];
        uint16_t nodeQueue[MAX_QUEUE_SIZE];
        int queueSize = 0;

        // fpt bestCost = FPT_MAX;
        fpt bestCost = 655360000; // 10000 << 16
        
        uint16_t bestSibling = NULL_NODE;
        //printf("[BVH] PickBest for node: n{}", insertedNode->id);
        if (tree.nodeCount == 2) {
            //printf("[BVH] PickBest: tree.nodeCount == 2");
            return tree.rootNode;
        }
        fpt insertedNodeSurfaceArea = SurfaceArea(tree.box[insertedNode]);
        //passing in oldSibling to try to reduce bounds flickering (when choosing a new node, the costs are close enough to keep changing every move)
        if(oldSibling <  NULL_NODE){bestCost = CalculateSAHCost(tree, oldSibling, insertedNode, insertedNodeSurfaceArea); /* printf("evaluating oldSibling n{} cost: {}", oldSibling->id, bestCost); */ bestSibling = oldSibling;}
        if(bestCost > 655360000){
            printf("BEST COST TOO LARGE!\n");
        }

        // Starting with a subset of nodes, e.g., immediate children of the root
        //immediate parent of root subset
        //uint16_t initialSubset[2] = {tree.rootNode tree.left[],  tree.rootNode tree.right[] };
        uint16_t initialSubset[2];

        if (oldSibling < NULL_NODE &&  tree.parent[oldSibling] < NULL_NODE) {
            initialSubset[0] =   tree.left[tree.parent[oldSibling]];
            initialSubset[1] =   tree.right[tree.parent[oldSibling]];
        } else {
            initialSubset[0] =  tree.left[tree.rootNode];
            initialSubset[1] =  tree.right[tree.rootNode];
        }
        //parent of oldSibling subset

        //printf("tree rootnode: n{}", tree.rootNode->id);
        // Add initial nodes to the queue
        //dont push to node if cost is higher than bestCost
        for (uint16_t node : initialSubset) {
            if (node >= NULL_NODE){ /* printf("[PICK BEST] an initial subset node is null"); */ continue;}
            if (node != oldSibling && node != insertedNode) {
                ///*TAG*/printf("Calulating SAH cost for insertedNode n{} and Node: n{}",insertedNode, node);
                fpt cost = CalculateSAHCost(tree, node, insertedNode, insertedNodeSurfaceArea);
                if(cost < bestCost){
                    ///*TAG*/printf("cost: {} < bestCost: {}", fpt2fl(cost), fpt2fl(bestCost));
                    nodeQueue[queueSize] = node;
                    // costQueue[queueSize] = cost;
                    queueSize++;
                }
            }
        }

        while (queueSize > 0) {
            queueSize--;
            uint16_t currentNode = nodeQueue[queueSize];
            // auto [currentCost, currentNode] = queue.top();
            // queue.pop();

            if (currentNode == insertedNode) continue;

            if(isLeaf(tree, currentNode)){
                fpt currentCost = CalculateSAHCost(tree, currentNode, insertedNode, insertedNodeSurfaceArea);

                //can only insert next to another leaf node
                //float newCost = CalculateSAHCost(currentNode, insertedNode);
                //printf("comparing currentCost: {}, bestCost: {}", currentCost, bestCost);
                if (currentCost < bestCost || (tree.flags.enable_First_Fit && currentCost <= average_Best_Cost_Threshold)) {
                    bestCost = currentCost;
                    if(bestCost > 655360000){
                        printf("BEST COST TOO LARGE!\n");
                    }
            
                    bestSibling = currentNode;

                    ///*TAG*/printf("NEW bestCost: {}, bestSibling: n{}", fpt2fl(bestCost), bestSibling);
                    if(tree.flags.enable_First_Fit || bestCost < tree.average_Best_Cost){//optimize by checking if its better than the average cost and returning then too?
                        //printf("FIRST FIT ENABLED, BREAKING EARLY");
                        break;
                    }

                }

            }
   

            //check against insertedNode more efficiently
            // Push children
            if ( tree.left[currentNode]  &&  tree.left[currentNode]  != insertedNode &&  tree.left[currentNode]  != oldSibling){
                    nodeQueue[queueSize] = tree.left[currentNode];
                    // costQueue[queueSize] = CalculateSAHCost(tree, tree.left[currentNode], insertedNode, insertedNodeSurfaceArea);
                    queueSize++;
            }
            if ( tree.right[currentNode] &&  tree.right[currentNode] != insertedNode &&  tree.right[currentNode] != oldSibling){
                    nodeQueue[queueSize] = tree.right[currentNode];
                    // costQueue[queueSize] = CalculateSAHCost(tree,  tree.right[currentNode], insertedNode,insertedNodeSurfaceArea);
                    queueSize++;
            }
        }
        if(bestCost > 655360000){
            printf("BEST COST TOO LARGE!\n");
        }

        //check for total cost overflow
        if (tree.total_Pick_Best_Cost > 655360000) {
            //printf("tree.total_Pick_Best_Cost overflow, RESETTING");
            tree.total_Pick_Best_Cost = 0;
            tree.total_Node_Pairings = 0;

        } else if(bestSibling < NULL_NODE){
            // Safe to do the operation
            tree.total_Pick_Best_Cost = fpt_add(tree.total_Pick_Best_Cost, bestCost);
            tree.total_Node_Pairings+= FPT_ONE;  

        }

        //printf("bestCost:  \033[31m{}\033[0m", bestCost);
        
        if(tree.total_Node_Pairings <= 0){
            tree.average_Best_Cost = FPT_MAX;    
        }
        else{
            tree.average_Best_Cost = fpt_div(tree.total_Pick_Best_Cost , tree.total_Node_Pairings);
        }
        ///*TAG*/printf("average best cost: {}", fpt2fl(tree.average_Best_Cost));

        ///*TAG*/printf("Pickbest returning nodeIndex: n{}", bestSibling ? bestSibling : -1);
        // //printf("Pickbest returning nodeIndex ENTITY: {}", bestSibling ? static_cast<size_t>(bestSibling->entity) : static_cast<size_t>(entt::null));
        return bestSibling;
    }



    void InsertLeaf(bvh_tree& tree, uint32_t entityID, AABB box, int primitiveCount, uint16_t bestPick)
    {
        ///*TAG*/printTree(tree, tree.rootNode);
        // if(tree.flags.bvh_Debug_Log){
        //     //printf("INSERTING, current tree: ");
        //     printTree(tree.rootNode);
        //     updateTreeToFile(tree);
        // }

        uint16_t leafNode = tree.entityToNodeID[entityID]; 

        if(leafNode == NULL_NODE){
            // setEntityInChunk(tree, entityID);
            printf("[BVH TREE] Inserting new entity into tree\n");
            leafNode = AllocateLeafNode(tree, entityID, box);
            if(leafNode == NULL_NODE){printf("InsertLeaf() too many nodes, can't allocate more, returning\n"); return;}
        }
        //update node's AABB
        tree.box[leafNode] = box;
        tree.primitiveCount[leafNode] = primitiveCount;
        //printf("[BVH TREE] Inserting leaf index: n{}", leafNode->id);
        uint16_t sibling = NULL_NODE;
        if (tree.nodeCount == 1)//only either one or 2 nodes in whole tree
        {
                printf("Root node: %u\n", leafNode);
                tree.rootNode = leafNode;
                return;
        }
        
        //stage 1: find best sibling for new leaf
        if(bestPick == NULL_NODE){
            //printf("PickBest returned NULL_NODE, finding best sibling");
            sibling = PickBest(tree, leafNode);
        }
        else{
            //printf("PickBest returned bestPick, using that");
            sibling = bestPick;
        }
        if (sibling == NULL_NODE) {
            printf("PickBest returned NULL_NODE, returning...\n");
            return;
        }

        // Stage 2: create a new parent
        uint16_t oldParent = tree.parent[sibling];
        uint16_t newParent;
        if(tree.parent[leafNode] != NULL_NODE){
            //printf("recycle old parent node");
            newParent =  tree.parent[leafNode];
        }
        else{
            newParent = AllocateInternalNode(tree);
            if(newParent == NULL_NODE){printf("InsertLeaf() too many nodes, can't allocate more, returning\n"); return;}
        }
        
        tree.parent[newParent] = oldParent;
        tree.box[newParent] = Union(box, tree.box[sibling]);
        if (oldParent != NULL_NODE)
        {
            // The sibling was not the root
            if (tree.left[oldParent] == sibling)
            {
                tree.left[oldParent] = newParent;
            }
            else
            {
                tree.right[oldParent] = newParent;
            }

            tree.left[newParent] = sibling;
            tree.right[newParent] = leafNode;
            tree.parent[sibling] = newParent;
            tree.parent[leafNode] = newParent;
        }
        else
        {
            // The sibling was the root
            tree.left[newParent] = sibling;
            tree.right[newParent] = leafNode;
            tree.parent[sibling] = newParent;
            tree.parent[leafNode] = newParent;
            tree.rootNode = newParent;
        }


        // Stage 3: walk back up the tree refitting AABBs
        tree.insertionCount++;
        tree.insertions_to_rotation++;
        ///*TAG*/printf("insertion count: {}", tree.insertionCount);
        uint16_t node =  tree.parent[leafNode];
        if(tree.insertions_to_rotation > rotation_Threshold){
            tree.insertions_to_rotation = 0; //reset the insertion count
            ///*TAG*/printf("INSERTION COUNT REACHED, ROTATING, recent insertion count: {}, threshold: {}", tree.insertions_to_rotation, rotation_Threshold);
            //*
            while (node != NULL_NODE)
            {
                //rotate also refits AABBs
                //node tree.box[] = Union(node tree.left[] tree.box[], node tree.right[] tree.box[]);
                //printf("box MIN:       {}, {}, {},   MAX: {}, {}, {} ", box.min.x, box.min.y, box.min.z, box.max.x, box.max.y, box.max.z);
                //printf("node tree.box[].min: {}, {}, {},   MAX: {}, {}, {} ", node tree.box[].min.x, node tree.box[].min.y, node tree.box[].min.z, node tree.box[].max.x, node tree.box[].max.y, node tree.box[].max.z);
                ///*TAG*/printf("rebalancing tree, current node: n{}", node);
                //skips node if its a leaf or if both of its children are leaf nodes
                bool node_has_valid_left_child  =  tree.left[node]  < NULL_NODE;
                bool node_left_child_is_leaf = true;
                if(node_has_valid_left_child) node_left_child_is_leaf = isLeaf(tree, tree.left[node]);

                bool node_has_valid_right_child =  tree.right[node] < NULL_NODE;
                bool node_right_child_is_leaf = true;
                if(node_has_valid_right_child) node_right_child_is_leaf = isLeaf(tree, tree.right[node]);
                

                if ((isLeaf(tree, node)) || (node_right_child_is_leaf && node_left_child_is_leaf)){
                    node =  tree.parent[node];
                    continue;
                }

                if(Rotate(tree, node)){
                    node =  tree.parent[node];
                }
                //UpdateBoundingBoxes(node);
                
                if(node != NULL_NODE) node =  tree.parent[node];
            }
            //*/
            // CountPrimitivesInNode(tree, tree.rootNode);
            // tree.insertionCount = 0;
        }
        else{
            UpdateBoundingBoxes(tree, node);
        }


        // if (tree.flags.bvh_Debug_Log)updateTreeToFile(tree);
        

    }
    void reset_bvh(chunk_data* chunkData, uint32_t chunkID){
        bvh_tree& tree = chunkData->bvhTrees[chunkID];
        for(int i = 0; i < MAX_BVHNODES; i++)
        {
            tree.left               [i]     = NULL_NODE;
            tree.right              [i]     = NULL_NODE;
            tree.parent             [i]     = NULL_NODE;
            tree.primitiveCount     [i]     = NULL_NODE;
            tree.entityToNodeID     [i]     = NULL_NODE;
            tree.nodesToDraw        [i]     = NULL_NODE;
            tree.nodeToDrawPos      [i]     = NULL_NODE;
            tree.colliding_node_ids [i]     = NULL_NODE;
            tree.nodeToEntityID     [i]     = NULL_ENTITY;
            tree.activeNodes        [i]     = false;
            tree.nodeIDQueue        [i]     = i;
        }
        tree.rootNode = NULL_NODE;
        tree.nodeCount = 0;
        tree.colliding_node_count = 0;
        tree.entity_collision_count = 0;
        tree.entity_voxel_collision_count = 0;
        tree.insertions_to_rotation = 0;
        tree.insertionCount = 0;
        tree.removalCount = 0;
        tree.is_dirty = false;
    }



    uint16_t createNode(bvh_tree& tree, uint32_t entityID){
        if (tree.nodeCount >= MAX_BVHNODES) return NULL_NODE;
        
        if (entityID != NULL_NODE && tree.entityToNodeID[entityID] != NULL_NODE){
            uint16_t nodeID = tree.entityToNodeID[entityID];
            printf("entityID %u already has node %u\n", entityID, nodeID);
            return nodeID;
        }
        uint16_t nodeID = tree.nodeIDQueue[tree.nodeCount];

        tree.nodesToDraw[tree.nodeCount] = nodeID;

        if (tree.activeNodes[nodeID]) {
            printf("createNode() NodeID: %u already exists\n", nodeID);
            return nodeID;
        }
        // tree.nodeIDQueue[tree.nodeCount] = NULL_NODE; // dont nullify, we can probly use the index to render
        
        tree.nodeToDrawPos[nodeID] = tree.nodeCount;

        tree.activeNodes[nodeID] = true;

        tree.nodeCount++;
        tree.total_Allocated_Nodes++;

        return nodeID;
    }

    void destroyNode(bvh_tree& tree, uint16_t nodeID, uint32_t entityID){
      if (nodeID >= MAX_BVHNODES) return;
      
        if(entityID != NULL_NODE){
            tree.entityToNodeID[entityID] = NULL_NODE;
            tree.nodeToEntityID[nodeID] = NULL_ENTITY;
        }

        if (tree.activeNodes[nodeID]) {
            printf("destroying nodeID: %u\n",  nodeID);
            tree.activeNodes[nodeID] = false;
        }
        else{
            printf("NodeID %u already destroyed\n", nodeID);
            return;
        }
        tree.nodeCount--;
        
        uint16_t posToRemove = tree.nodeToDrawPos[nodeID];
         // Skip swap if it's the last element
        if (posToRemove < tree.nodeCount) {
            // Get the last node
            uint16_t lastNodeID = tree.nodesToDraw[tree.nodeCount];
            
            // Move last node to the removed position
            tree.nodesToDraw[posToRemove] = lastNodeID;
            // Update the position map for the moved node
            tree.nodeToDrawPos[lastNodeID] = posToRemove;
        }
        
        tree.total_Allocated_Nodes--;
        tree.removalCount--;

        tree.nodeIDQueue[tree.nodeCount] = nodeID;


    }


    int ReinsertLeaf(bvh_tree& tree, uint32_t entityID, AABB box, int primitiveCount) {
        // if(tree.flags.bvh_Debug_Log){
        //     // printf("REINSERTING, current tree: ");
        //     if(tree.rootNode == NULL_NODE)//printf("Root node is null");
        //     printTree(tree.rootNode);
        //     updateTreeToFile(tree);
        // }
        uint16_t bestPick = NULL_NODE;
        if (tree.entityToNodeID[entityID] == NULL_NODE) {
            // Entity is not in the tree
            printf("Entity {} is not in tree, inserting", entityID);
        }
        else 
        {
            uint16_t leafNodeID = tree.entityToNodeID[entityID];
            tree.box[leafNodeID] = box;
            tree.primitiveCount[leafNodeID] = primitiveCount;
            if (leafNodeID == NULL_NODE)
            {
                printf("ERROR: leaf index is null");
                return -1;
            }
            if (leafNodeID == tree.rootNode) {
                // Handle root removal
                ///*TAG*/printf("reinsert root node");
                //just return, this is the only node in the tree
                return 0;
            } else {
                uint16_t parentID = tree.parent[leafNodeID];
                if (parentID == NULL_NODE)
                {
                    if(tree.rootNode == NULL_NODE){
                        printf("ERROR: node inserted into null chunk?");
                    }
                    printf("ERROR: parent index is null chunk {} {} {},PUT BREAKPOINT HERE"  , tree.chunk_coords.x,tree.chunk_coords.y,tree.chunk_coords.z); 
                    printf("ERROR: parent index is null chunk {} {} {}, returning"           , tree.chunk_coords.x,tree.chunk_coords.y,tree.chunk_coords.z); 
                    return -1;
                }
                // printf("leafNodeID pointer: %p, parent pointer: %p\n", static_cast<void*>(leafNodeID), static_cast<void*>(parent));
                uint16_t grandParentID = tree.parent[parentID];

                uint16_t siblingID = (tree.left[parentID] == leafNodeID) ? tree.right[parentID] : tree.left[parentID];
                bestPick = PickBest(tree, leafNodeID, siblingID);
                if(siblingID != bestPick){
                    //check if grandParent is null
                    if (grandParentID < NULL_NODE) {
                        // Connect sibling to grandParent
                        if (tree.left[grandParentID] == parentID) {
                            tree.left[grandParentID] = siblingID;
                        }
                        else {
                            tree.right[grandParentID] = siblingID;
                        }
                        tree.parent[siblingID] = grandParentID;
                    }
                    else {
                        // parent is root, just insert again
                        //printf("no grandparent, parent is root");
                        tree.rootNode = siblingID;
                        tree.parent[siblingID] = NULL_NODE;
                    }
            }
                else{
                    //printf("sibling is best, dont remove");
                    UpdateBoundingBoxes(tree, tree.parent[leafNodeID]);
                    return 0;
                }
            }
        }
        InsertLeaf(tree, entityID, box, primitiveCount, bestPick);
        ///*TAG*/printf("POST INSERTION TREE PRINT:");
        ///*TAG*/printTree(tree, tree.rootNode);
        // tree.flags.imGuiReDraw = true;
        return 0;
    }


    bool removeLeafNode(bvh_tree& tree, uint32_t entityID) {
        uint16_t leafNode = tree.entityToNodeID[entityID] ;

        if(leafNode == NULL_NODE){
            // Entity is not in the tree
            printf("Entity is not in tree's leafNode map, cant remove, returning\n");
            return false;
        }
        if (leafNode == tree.rootNode) {
            // Handle root removal
            printf("removing root node from chunk bvhTree\n");
            destroyNode(tree, leafNode, entityID);
            tree.rootNode = NULL_NODE;
        } else {
            uint16_t parent = tree.parent[leafNode];
            uint16_t grandParent = tree.parent[parent];
            uint16_t sibling = (tree.left[parent] == leafNode) ?
                                tree.right[parent] : tree.left[parent];

            if (grandParent != NULL_NODE) {
                // Connect sibling to grandParent
                if ( tree.left[grandParent] == parent) {
                     tree.left[grandParent] = sibling;
                } else {
                     tree.right[grandParent] = sibling;
                }
                 tree.parent[sibling] = grandParent;
                // Update bounding boxes up the tree
                UpdateBoundingBoxes(tree, grandParent);
            } else {
                // Sibling becomes the new root
                tree.rootNode = sibling;
                tree.parent[sibling] = NULL_NODE;
            }
            // Delete parent and leaf node (if managing dynamic memory)
            //i think this is unnecessary, since we are deleting the parent immediately afterward anyway
            // UpdateBoundingBoxes(parent);

            tree.parent[parent] = NULL_NODE;
            tree.left[parent] = NULL_NODE;
            tree.right[parent] = NULL_NODE;

            destroyNode(tree,parent);
            tree.total_Internal_Nodes--;
            
            parent = NULL_NODE;

            tree.parent[leafNode] = NULL_NODE;
            tree.left[leafNode] = NULL_NODE;
            tree.right[leafNode] = NULL_NODE;


            destroyNode(tree, leafNode, entityID);
            leafNode = NULL_NODE;

        }
        // tree.entityToNodeMap.erase(entity);
        tree.total_Leaf_Nodes--;
        if(tree.removalCount >= update_Primitives_Threshold){
            // CountPrimitivesInNode(tree, tree.rootNode);
            tree.removalCount = 0;
        }
        // removeEntityFromChunk(tree, entityID);
        
        return true;

    }


    uint16_t rayTraverseNode(game_state* GameState, bvh_tree& tree, uint16_t node, fpt_vec3 cameraPosition, fpt_vec3 relativePosition, fpt_vec3 ray, fpt& closestDistance, fpt_vec3& collisionPosition) {
            uint16_t closestEntity = NULL_ENTITY;
            //*
            // Check intersection with current node
            // /*TAG*/vec3 floatMin = fpt_to_flt_vec3(tree.box[node].min);
            // /*TAG*/vec3 floatMax = fpt_to_flt_vec3(tree.box[node].max);
            // /*TAG*/printf("node {} min: {} {} {}",node, floatMin.x, floatMin.y, floatMin.z);
            // /*TAG*/printf("node {} max: {} {} {}",node, floatMax.x, floatMax.y, floatMax.z);

            if (!HitBoundingBox(tree.box[node].min + relativePosition, tree.box[node].max + relativePosition, 
                                    cameraPosition, ray, 
                                    collisionPosition)) {
                return closestEntity;
            }
            // /*TAG*/printf("ray intersects node: {}, is leaf node: {}, left: {}, right: {}", node, isLeaf(tree, node), tree.left[node], tree.right[node]);
            // Check entities in this node
            if(isLeaf(tree, node)){

                // printf("checking intersection with entity: {}", tree.nodeToEntityID[node]);
                uint32_t obbIndex       = GameState->entityComponent->entityToObbMap       [tree.nodeToEntityID[node]];
                uint32_t transIndex     = GameState->entityComponent->entityToTransMap     [tree.nodeToEntityID[node]];
                if(obbIndex < NULL_ENTITY && transIndex < NULL_ENTITY){
                    TransComp& trans = GameState->entityComponent->TransComps[transIndex];
                    ObbComp& obb = GameState->entityComponent->ObbComps[obbIndex];
                    //bool intersects = false;
                    // print_fpt_vec3("camera position: ", cameraPosition);
                    // print_fpt_vec3("transPosinChunk: ", trans.pos_in_chunk);
                    if (RayIntersectsOBB(cameraPosition, ray, obb, trans.pos_in_chunk + relativePosition, collisionPosition)) {
                        // float distance = length(collisionPosition - cameraPosition);
                        //relative position is the chunk offset from the ray origin
                        collisionPosition += (relativePosition + trans.pos_in_chunk);
                        fpt distance = fpt_vec3_length(collisionPosition - cameraPosition);
                        float debugDistance = fpt2fl(distance);
                        float debugClosest = fpt2fl(closestDistance);
                        // /*TAG*/printf("distance: {}, closestDistance: {}", debugDistance, debugClosest);
                        if (distance < closestDistance) {
                            closestDistance = distance;
                            closestEntity = tree.nodeToEntityID[node];
                            // printf("closest entity: {}", static_cast<size_t>(closestEntity));
                        }
                            //intersects = true;
                            ////printf("ray intersects entity: {}", entity);
                    }
                        ////printf("entity: {}, intersects?: {}", entity, intersects);

                }
             

            }
            else{//not node leaf, check children
                if(tree.left[node] != NULL_NODE){
                    auto hitEntity = rayTraverseNode(GameState, tree, tree.left[node], cameraPosition, relativePosition, ray, closestDistance, collisionPosition);
                    if (hitEntity != NULL_ENTITY) {
                        closestEntity = hitEntity;
                    }
                }
                if(tree.right[node] != NULL_NODE){
                    auto hitEntity = rayTraverseNode(GameState, tree, tree.right[node], cameraPosition, relativePosition, ray, closestDistance, collisionPosition);
                    if (hitEntity != NULL_ENTITY) {
                        closestEntity = hitEntity;
                    }
                }

                
            }
        return closestEntity;
    }

    int start_broad_phase_collision_check(game_state* GameState, bvh_tree& tree, uint32_t chunkID){
        if(chunkID == NULL_CHUNK)return 0;
        ///*TAG*/printf("START BROAD PHASE COLLISION CHECK FOR CHUNK {} {} {}", tree.chunk_coords.x,tree.chunk_coords.y,tree.chunk_coords.z);
        if(tree.rootNode >= NULL_NODE){
            return 0;
        }
        memset(tree.node_children_checked, 0, sizeof(uint8_t) * MAX_BVHNODES);
        tree.colliding_node_count = 0;

        tree. entity_collision_count = 0;
        tree. entity_voxel_collision_count = 0;

        tree.total_collision_checks = 0;
        uint16_t left  = tree.left [tree.rootNode];
        uint16_t right = tree.right[tree.rootNode];
        int depth = 0;

        broad_phase_collision_check(GameState, tree, left, right, depth);
        // /*TAG*/printf("total nodes: {}, total collision checks: {}, colliding nodes: {}", tree.nodeCount, tree.total_collision_checks, tree.colliding_node_count);

        //entity vs voxel broad phase
        chunk_data* chunkData = GameState->chunkData;
        
        //TODO: do we need to check if the brickmap is valid? should probably just assert
        CoarseBrickmap64* coarse = &chunkData->coarse_brickmaps[chunkID];
        ivec3 coords = chunkData->coords[chunkID];
        // printf("-----------------------BROAD PHASE VOXEL COLLISION STEP, chunk ID: {}, coords {} {} {}-----------------------\n", chunkID, coords.x, coords.y, coords.z);

        broad_phase_collision_entity_voxel(GameState, tree, tree.rootNode, coarse);
        // /*TAG*/printf("total entity v voxel collisions: {}", tree.entity_voxel_collision_count);



        return 0;
    }

    int broad_phase_collision_check(game_state* GameState, bvh_tree& tree, uint16_t node, uint16_t other_node, int depth)
    {  
        //its assumed we pass the immediate children of the root node into the function

        //what do I do if one of the nodes is null?
        if(node >= NULL_NODE || other_node >= NULL_NODE || node == other_node){
            return 0;
        }

        //recursively scan from rootnode down

        //NULL_NODE is a value like 4096, under which any node is valid, over which it is an invalid value. just more thorough than direct == check



        AABB& a = tree.box[node];
        AABB& b = tree.box[other_node];

        fpt sa_a = SurfaceArea(a);
        fpt sa_b = SurfaceArea(b);

        //check if the 2 nodes intersect
        bool intersects = aabbIntersectTest(a.min, a.max, b.min, b.max);

        tree.total_collision_checks++;
        depth++;

        bool node_is_leaf       = isLeaf(tree, node);
        bool other_node_is_leaf = isLeaf(tree, other_node);
        if (!node_is_leaf && !tree.node_children_checked[node]) {
                tree.node_children_checked[node] = 1;
                broad_phase_collision_check(GameState, tree, tree.left[node], tree.right[node], depth);
        }
        if (!other_node_is_leaf && !tree.node_children_checked[other_node]) {
                tree.node_children_checked[other_node] = 1;
                broad_phase_collision_check(GameState, tree, tree.left[other_node], tree.right[other_node], depth);
        }
        if(!intersects){
            ///*TAG*/printf("NO COLLISION n{} vs n{}", node, other_node);
            return 0;
        }


        ///*TAG*/printf("CHECK n{} vs n{}", node, other_node);
        
        if(node_is_leaf && other_node_is_leaf){
            //need to refine to add collision PAIRs, at the moment we just want to color all colliding nodes red
            tree.colliding_node_ids[tree.colliding_node_count++] = node;
            tree.colliding_node_ids[tree.colliding_node_count++] = other_node;
                
            entity_v_entity_collision newCollision = {0};
            newCollision.entityID = tree.nodeToEntityID[node];
            newCollision.other_entityID = tree.nodeToEntityID[other_node];
            tree.entity_collisions[tree.entity_collision_count++] = newCollision;
            if(tree.entity_collision_count > MAX_ENTITIES){
                printf("TOO MANY ENTITY V ENTITY COLLISIONS, COUNT: {}", tree.entity_collision_count);
            }
            ///*TAG*/printf("NODES {} AND {} ARE INTERSECTING!", node, other_node);
            return 0;

        }
        
        ///*TAG*/if(!node_is_leaf)       printf("n{} is branching", node);
        ///*TAG*/if(!other_node_is_leaf) printf("n{} is branching", other_node);
        

        //determine which node to descend into
        //if either is a leaf, descend the other, if both are branches, use the largest size
        if(other_node_is_leaf || (!node_is_leaf && sa_a > sa_b)){
            ///*TAG*/printf("recursing into left/right nodes for       n{}", node);
            //recurse into self
            broad_phase_collision_check(GameState, tree, tree.left [node], other_node, depth);

            //do other side too
            broad_phase_collision_check(GameState, tree, tree.right[node], other_node, depth);
            // Check for collisions between the children of the node

        }else{
            ///*TAG*/printf("recursing into left/right nodes for OTHER n{}", other_node);
            //recurse into other node
            broad_phase_collision_check(GameState, tree, node, tree.left [other_node], depth);


            broad_phase_collision_check(GameState, tree, node, tree.right[other_node], depth);

            // Check for collisions between the children of the other node

        }



        
        return 0;
    }

    int broad_phase_collision_entity_voxel(game_state *GameState, bvh_tree& tree, uint16_t node, CoarseBrickmap64* coarse){
        //what do I do if one of the nodes is null?
        if(node >= NULL_NODE){
            return 0;
        }

        AABB& a = tree.box[node];

        //get coarse grid coordinates of the node's aabb
        // ivec3 chunk_coords = chunkData->coords[chunkID];

        // fpt_vec3 bmPos =  fpt_brickmap_local_chunk_position(bmCoords, bmScale, bmHalfScale);
        fpt_vec3 bmPos =  fpt_vec3_create(0);

        // printf("chunkID: %d, brickmapID %d\n", chunkID, brickmapID);
        // printf("bvh min: %10.5f %10.5f %10.5f\n", fpt2fl(a.min.x),fpt2fl(a.min.y),fpt2fl(a.min.z));
        // printf("bvh max: %10.5f %10.5f %10.5f\n", fpt2fl(a.max.x),fpt2fl(a.max.y),fpt2fl(a.max.z));

        ivec3 voxCoordsMin = fpt_calculateAABBVoxelCoordinates(a.min, -FPT_HUNDREDTH);
        ivec3 voxCoordsMax = fpt_calculateAABBVoxelCoordinates(a.max,  FPT_HUNDREDTH);
        
        // ivec3 voxCoordsMin = fpt_calculatePaddedVoxelCoordinates(a.min, -FPT_HUNDREDTH);
        // ivec3 voxCoordsMax = fpt_calculatePaddedVoxelCoordinates(a.max,  FPT_HUNDREDTH);


        ivec3 coarseCoordsMin = voxCoordsMin / 8;
        ivec3 coarseCoordsMax = voxCoordsMax / 8;

        // printf("vox coords min: %d %d %d, coarse coords min: %d %d %d\n", voxCoordsMin.x,voxCoordsMin.y,voxCoordsMin.z, coarseCoordsMin.z,coarseCoordsMin.y,coarseCoordsMin.z);
        // printf("vox coords max: %d %d %d, coarse coords max: %d %d %d\n", voxCoordsMax.x,voxCoordsMax.y,voxCoordsMax.z, coarseCoordsMax.z,coarseCoordsMax.y,coarseCoordsMax.z);
        //convert raw voxel coordinates to coarse grid coordinates

        
        bool potential_collision = false;
        bool is_leaf = isLeaf(tree, node);
        for(int x = coarseCoordsMin.x; x <= coarseCoordsMax.x; x++){
            for(int y = coarseCoordsMin.y; y <= coarseCoordsMax.y; y++){
                for(int z = coarseCoordsMin.z; z <= coarseCoordsMax.z; z++){
                    uint32_t coarse_index = x + (y * 8) + (z * 64);
                    if(coarse->active_count[coarse_index]){ //coarse grid contains voxels, potential collision!
                        potential_collision = true;
                        if(is_leaf){
                            tree.colliding_node_ids[tree.colliding_node_count++] = node;
                            entity_v_voxel_collision newCollision = {0};
                            newCollision.entityID = tree.nodeToEntityID[node];
                            newCollision.coarse_grid_id = coarse_index;
                            tree.entity_voxel_collisions[tree.entity_voxel_collision_count++] = newCollision;
                            // printf("COLLISION BETWEEN entityID: %d, coarse grid id: %d, coards grid coords: %d %d %d\n", newCollision.entityID, coarse_index, x,y,z);

                            if(tree.entity_voxel_collision_count > MAX_ENTITIES){
                                printf("TOO MANY ENTITY V VOXEL COLLISIONS, COUNT: {}", tree.entity_voxel_collision_count);
                            }
                        }
                    }
                }
            }
        }
        if(potential_collision){
            if(!is_leaf){
                //recurse into children
                broad_phase_collision_entity_voxel(GameState, tree, tree.left[node], coarse);
                broad_phase_collision_entity_voxel(GameState, tree, tree.right[node], coarse);
            }
        }


        return 0;
    }



//     void clearNode(uint16_t node){
//         node tree.left[] = NULL_NODE;
//         node tree.right[] = NULL_NODE;
//         node tree.parent[] = NULL_NODE;
//         // node->entity = NULL_NODE;//leaf nodes will contain only one entity max
//         node tree.primitiveCount[] = 0;
//         node = NULL_NODE;
//     }


   // Set entity bit in specific chunk
    // void setEntityInChunk(bvh_tree& tree, uint16_t chunkID, uint32_t entityID) {
    //     // Find start of chunk's words: chunkId * NUM_WORDS
    //     // Then find specific word: entityId / 32
    //     uint16_t wordIndex = (chunkId * NUM_WORDS) + (entityId / BITS_PER_WORD);
    //     uint16_t bitIndex = entityId % BITS_PER_WORD;
    //     tree.chunkToEntityBits[wordIndex] |= (1u << bitIndex);
    // }

    // // Test if entity exists in chunk
    // bool isEntityInChunk(bvh_tree& tree, uint16_t chunkID, uint32_t entityID) {
    //     uint16_t wordIndex = (chunkId * NUM_WORDS) + (entityId / BITS_PER_WORD);
    //     uint16_t bitIndex = entityId % BITS_PER_WORD;
    //     return (tree.chunkToEntityBits[wordIndex] & (1u << bitIndex)) != 0;
    // }

    // // Clear entity bit in chunk
    // void removeEntityFromChunk(bvh_tree& tree, uint16_t chunkID, uint32_t entityID) {
    //     uint16_t wordIndex = (chunkId * NUM_WORDS) + (entityId / BITS_PER_WORD);
    //     uint16_t bitIndex = entityId % BITS_PER_WORD;
    //     tree.chunkToEntityBits[wordIndex] &= ~(1u << bitIndex);
    // }
