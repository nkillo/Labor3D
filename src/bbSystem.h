#pragma once
#include "constants.h"
#include "entity.h"
#include "boxIntersect.h"
#include "cgltfTest.h"

    //      used to be (ObbComp, MeshComp(for min and extents) , transComp)
    void recalculateOBB(ObbComp& ObbComp, TransComp& transComp, fpt_vec3& min, fpt_vec3& extents) {
            // Scale the extents
            fpt_vec3 scaledExtents = extents * transComp.scale;
            
            // Scale the local center
            fpt_vec3 localCenter = (min + extents) * transComp.scale;

            // Then, apply rotation to the local center to find the world center of the OBB.
            // fpt_vec3 worldCenter = transComp.position + (fpt_quat_to_mat3(transComp.rotation) * localCenter);
            
            //don't know what our requirements are until we import meshes and bones
            fpt_vec3 localChunkCenter = (fpt_quat_to_mat3(transComp.rotation) * localCenter);

            // Now set the OBB's properties.
            // ObbComp.position =      worldCenter;
            ObbComp.position =      localChunkCenter;
            ObbComp.extents =       scaledExtents;
            ObbComp.localCenter =   localCenter;    //commented out for testing transComp scaling in scene.h start()
            ObbComp.scale =         scaledExtents * FPT_TWO; // Scale is the full size, not half-extents.
            ObbComp.rotation =      transComp.rotation;

        }


        void recalculateAABB(AabbComp& AabbComp, ObbComp& ObbComp, TransComp& transComp, bool enlargeAABB = true, fpt enlargeAABBAmount = 98304) { //1.5 * 2^16
   



               // Store old bounds for delta calculation
            // glm::vec3 oldMin = fpt_to_glm_vec3(AabbComp.min);
            // glm::vec3 oldMax = fpt_to_glm_vec3(AabbComp.max);

            // glm::vec3 center = fpt_to_glm_vec3(ObbComp.aabbMin + ObbComp.aabbMax) * 0.5f;
            // glm::vec3 extents = fpt_to_glm_vec3(ObbComp.aabbMax - ObbComp.aabbMin) * 0.5f;

            // glm::vec3 aabbScale = extents * 2.0f;
            // printf("recalculateAABB() scale: %f %f %f\n", aabbScale.x,aabbScale.y,aabbScale.z);
            // if(enlargeAABB){
            //     extents *= fpt2fl(enlargeAABBAmount);
            // }
            // AabbComp.min =          glm_to_fpt_vec3(center - extents);
            // AabbComp.max =          glm_to_fpt_vec3(center + extents);
            // AabbComp.position =     (AabbComp.min + AabbComp.max) * FPT_HALF;
            // AabbComp.scale =        glm_to_fpt_vec3(extents * 2.0f);


            // // Calculate deltas (positive means expanding, negative means contracting)
            // AabbComp.minDelta = AabbComp.min - glm_to_fpt_vec3(oldMin);
            // AabbComp.maxDelta = AabbComp.max - glm_to_fpt_vec3(oldMax);
            // aabbScale = fpt_to_glm_vec3(AabbComp.scale);
            // printf("recalculateAABB() scale: %f %f %f\n", aabbScale.x,aabbScale.y,aabbScale.z);



            // Store old bounds for delta calculation
            fpt_vec3 oldMin = AabbComp.min ;
            fpt_vec3 oldMax = AabbComp.max ;

            fpt_vec3 center = ((ObbComp.aabbMin + ObbComp.aabbMax)) *  FPT_HALF;
            fpt_vec3 extents = (ObbComp.aabbMax - ObbComp.aabbMin) * FPT_HALF;
            
            if(enlargeAABB){
                extents *= enlargeAABBAmount;
            }
            AabbComp.min = center - extents;
            AabbComp.max = center + extents;
            AabbComp.scale = (extents) * FPT_TWO;
            // glm::vec3 aabbScale = fpt_to_glm_vec3(AabbComp.scale);
            // printf("recalculateAABB() scale: %f %f %f\n", aabbScale.x,aabbScale.y,aabbScale.z);

            //we will need these if we ever want to track the old bounds of the aabb if it ever changes
            //we used to allow changing the aabb, which is why we tracked its previous size like this
            //CHANGING THIS COULD CHANGE HOW WE REMOVE THE ENTITY FROM OLD CHUNKS in the chunkSystem.cpp
            //AabbComp.minDelta = AabbComp.min - oldMin;
            //AabbComp.maxDelta = AabbComp.max - oldMax;

            AabbComp.posDelta = AabbComp.offset;


            // glm::vec3 debugOldMin       = fpt_to_glm_vec3(oldMin      );
            // glm::vec3 debugOldMax       = fpt_to_glm_vec3(oldMax      );
            // glm::vec3 debugCenter       = fpt_to_glm_vec3(center      );
            // glm::vec3 debugExtents      = fpt_to_glm_vec3(extents     );
            // glm::vec3 debugaabbMin      = fpt_to_glm_vec3(AabbComp.min     );
            // glm::vec3 debugaabbMax      = fpt_to_glm_vec3(AabbComp.max     );
            // glm::vec3 debugaabbOffset   = fpt_to_glm_vec3(AabbComp.offset  );
            // glm::vec3 debugaabbScale    = fpt_to_glm_vec3(AabbComp.scale   );
            // glm::vec3 debugaabbMinDelta = fpt_to_glm_vec3(AabbComp.minDelta);
            // glm::vec3 debugaabbMaxDelta = fpt_to_glm_vec3(AabbComp.maxDelta);
            // glm::vec3 debugaabbPosDelta = fpt_to_glm_vec3(AabbComp.posDelta);

            // spdlog::info("debugOldMin      : {:6.2f} {:6.2f} {:6.2f}", debugOldMin      .x, debugOldMin      .y, debugOldMin      .z);
            // spdlog::info("debugOldMax      : {:6.2f} {:6.2f} {:6.2f}", debugOldMax      .x, debugOldMax      .y, debugOldMax      .z);
            // spdlog::info("debugCenter      : {:6.2f} {:6.2f} {:6.2f}", debugCenter      .x, debugCenter      .y, debugCenter      .z);
            // spdlog::info("debugExtents     : {:6.2f} {:6.2f} {:6.2f}", debugExtents     .x, debugExtents     .y, debugExtents     .z);
            // spdlog::info("debugaabbMin     : {:6.2f} {:6.2f} {:6.2f}", debugaabbMin     .x, debugaabbMin     .y, debugaabbMin     .z);
            // spdlog::info("debugaabbMax     : {:6.2f} {:6.2f} {:6.2f}", debugaabbMax     .x, debugaabbMax     .y, debugaabbMax     .z);
            // spdlog::info("debugaabbOffset  : {:6.2f} {:6.2f} {:6.2f}", debugaabbOffset  .x, debugaabbOffset  .y, debugaabbOffset  .z);
            // spdlog::info("debugaabbScale   : {:6.2f} {:6.2f} {:6.2f}", debugaabbScale   .x, debugaabbScale   .y, debugaabbScale   .z);
            // spdlog::info("debugaabbMinDelta: {:6.2f} {:6.2f} {:6.2f}", debugaabbMinDelta.x, debugaabbMinDelta.y, debugaabbMinDelta.z);
            // spdlog::info("debugaabbMaxDelta: {:6.2f} {:6.2f} {:6.2f}", debugaabbMaxDelta.x, debugaabbMaxDelta.y, debugaabbMaxDelta.z);
            // spdlog::info("debugaabbPosDelta: {:6.2f} {:6.2f} {:6.2f}", debugaabbPosDelta.x, debugaabbPosDelta.y, debugaabbPosDelta.z);


            AabbComp.offset = fpt_vec3_create(0);


    
        }


        // Helper function to create an AABB that fully encompasses an OBB
        void createEncompassingAABB(TransComp& transComp, ObbComp& ObbComp) {
            fpt_vec3 obbCorners[8];
            
            // Define the 8 corners of the OBB in local space
            fpt_vec3 localCorners[8] = {
                fpt_vec3_create(-ObbComp.extents.x, -ObbComp.extents.y, -ObbComp.extents.z),
                fpt_vec3_create(-ObbComp.extents.x, -ObbComp.extents.y, ObbComp.extents.z),
                fpt_vec3_create(-ObbComp.extents.x, ObbComp.extents.y, -ObbComp.extents.z),
                fpt_vec3_create(-ObbComp.extents.x, ObbComp.extents.y, ObbComp.extents.z),
                fpt_vec3_create(ObbComp.extents.x, -ObbComp.extents.y, -ObbComp.extents.z),
                fpt_vec3_create(ObbComp.extents.x, -ObbComp.extents.y, ObbComp.extents.z),
                fpt_vec3_create(ObbComp.extents.x, ObbComp.extents.y, -ObbComp.extents.z),
                fpt_vec3_create(ObbComp.extents.x, ObbComp.extents.y, ObbComp.extents.z)
            };

            fpt_mat4 fptRotationMatrix = fpt_quat_to_mat4(ObbComp.rotation);
            // fpt_mat4 fptTranslationMatrix = fpt_translate(transComp.pos_in_chunk);  // dont want to put in world space
            fpt_mat4 fptTranslationMatrix = fpt_translate(fpt_vec3_create(0));// Create a translation matrix
            fpt_mat4 fptTransformMatrix = fpt_mul_mat4(fptTranslationMatrix, fptRotationMatrix);  // Combine the translation and rotation into a single matrix
            
            // glm::mat4 rotationMatrix = fpt_to_glm_mat4(fptRotationMatrix);//glm::mat4_cast(fpt_to_glm_quat(ObbComp.rotation));
            // glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), fpt_to_glm_vec3(ObbComp.position)); // Create a translation matrix
            // glm::mat4 transformMatrix = fpt_to_glm_mat4(fptTransformMatrix);//translationMatrix * rotationMatrix; // Combine the translation and rotation into a single matrix

            for (int i = 0; i < 8; ++i) {
                // glm::vec3 temp = glm::vec3(transformMatrix * glm::vec4(fpt_to_glm_vec3(localCorners[i]), 1.0f));
                // obbCorners[i] = glm_to_fpt_vec3(temp);
                obbCorners[i] = fpt_vec3_create(fptTransformMatrix * fpt_vec4_create(localCorners[i], FPT_ONE));

            }

            // Create the encompassing AABB
            ObbComp.aabbMin = obbCorners[0];
            ObbComp.aabbMax = obbCorners[0];
            for (const auto& corner : obbCorners) {
                // ObbComp.aabbMin = fpt_vec3_min((ObbComp.aabbMin), corner);
                // ObbComp.aabbMax = fpt_vec3_max((ObbComp.aabbMax), corner);

                ObbComp.aabbMin = fpt_vec3_min(ObbComp.aabbMin, corner);
                ObbComp.aabbMax = fpt_vec3_max(ObbComp.aabbMax, corner);
            }
        }


        void bb_start(game_state* GameState){

            //use defaults for potentially missing entity data
            ObbComp defaultObbComp = {};                
            MeshComp defaultMeshComp = {};           
            ModelComp defaultModelComp = {};           
            TransComp defaultTransComp = {};     

            EntityComponent& ec = *GameState->entityComponent;
            //all entities we want to work with will have an aabb component, start with that
            for(int i = 0; i < ec.AabbCount; i++){
                uint32_t entityID = ec.AabbToEntityMap[i];
                AabbComp& aabbComp = ec.AabbComps[i];

                //just access all the necessary data, its all initialized anyway, the entityID will be valid
                uint32_t obbIndex = ec.entityToObbMap[entityID];
                uint32_t meshIndex = ec.entityToMeshMap[entityID];
                uint32_t modelIndex = ec.entityToModelMap[entityID];
                uint32_t transIndex = ec.entityToTransMap[entityID];
                
                ObbComp& ObbComp = obbIndex != NULL_ENTITY ? ec.ObbComps[obbIndex] : defaultObbComp;
                MeshComp& MeshComp = meshIndex != NULL_ENTITY ? ec.MeshComps[meshIndex] : defaultMeshComp;
                ModelComp& ModelComp = modelIndex != NULL_ENTITY ? ec.ModelComps[modelIndex] : defaultModelComp;
                TransComp& transComp = transIndex != NULL_ENTITY ? ec.TransComps[transIndex] : defaultTransComp;
                // Mesh& mesh = GameState->meshData->meshes[MeshComp.meshIndex];

                

                fpt_vec3 min = fpt_vec3_create(-FPT_ONE);
                fpt_vec3 extents = fpt_vec3_create(FPT_ONE);
                //determine the bounds for the obb/aabb based off the mesh or model the entity uses
                if(modelIndex != NULL_ENTITY){
                    model*  entity_model = GameState->gltfData->models + ModelComp.type;
                    min     = flt_to_fpt_vec3(entity_model->min);
                    extents = flt_to_fpt_vec3(entity_model->extents);

                // }else if(meshIndex != NULL_ENTITY){
                //     min     = mesh.min;
                //     extents = mesh.extents;
                // }
                }else{//hardcoded cube extents because I'm too lazy to organize, preprocess, and pipeline mesh data through
                    min = fpt_vec3_create(FPT_HALF);
                    extents = fpt_vec3_create(FPT_ONE);
                }
                recalculateOBB(ObbComp, transComp, min, extents);
                createEncompassingAABB(transComp, ObbComp);


                recalculateAABB(aabbComp, ObbComp, transComp, ec.enlargeAABB, ec.enlargeAABBAmount);

                aabbComp.flags |= aabb_dirty;
            }
        }





        void bb_update(game_state* GameState){
            fpt enlargeAABBAmount = 98304;

                     //use defaults for potentially missing entity data
            ObbComp defaultObbComp = {};                
            MeshComp defaultMeshComp = {};           
            TransComp defaultTransComp = {};     

            EntityComponent& ec = *GameState->entityComponent;
            //all entities we want to work with will have an aabb component, start with that
            for(int i = 0; i < ec.AabbCount; i++){
                


                uint32_t entityID = ec.AabbToEntityMap[i];
                uint32_t transIndex = ec.entityToTransMap[entityID];
                assert(transIndex != NULL_ENTITY && "bbSystem update() transIndex is NULL");

                TransComp& transComp = transIndex != NULL_ENTITY ? ec.TransComps[transIndex] : defaultTransComp;

                if((transComp.flags & trans_dirty) != 0){
                // if((ec.transFlags[transIndex] & 0x07) != 0){
                    // printf("entityID %d transComp flag is dirty, tick: %u\n", entityID, GameState->tick);
                    AabbComp& aabbComp = ec.AabbComps[i];

                    //just access all the necessary data, its all initialized anyway, the entityID will be valid
                    uint32_t obbIndex = ec.entityToObbMap[entityID];
                    // uint32_t meshIndex = ec.entityToMeshMap[entityID];
                    
                    ObbComp& ObbComp = obbIndex != NULL_ENTITY ? ec.ObbComps[obbIndex] : defaultObbComp;
                    // MeshComp& MeshComp = meshIndex != NULL_ENTITY ? ec.MeshComps[meshIndex] : defaultMeshComp;
                    // Mesh& mesh = GameState->meshData->meshes[MeshComp.meshIndex];





                    if((transComp.flags & scale_dirty) != 0){
                        //stop recalculating the obb after creation, we may not keep it once we experiment with capsules
                        // recalculateOBB(ObbComp, mesh, transComp);

                        //this would expand/shrink the aabb which we need to stay consistent for voxel collision checking, so we never resize it
                        // createEncompassingAABB(transComp, ObbComp);
                    }
                    if((transComp.flags & pos_dirty) != 0){
                        //if any of these flags are dirty we process them in the chunkSystem for the bvh to know what happened 
                    }
                    if((transComp.flags & rot_dirty) != 0){
                        // recalculateOBB(ObbComp, mesh, transComp);

                        // createEncompassingAABB(transComp, ObbComp);            

                        transComp.flags &= ~rot_dirty;
                    }
                    //
                    transComp.flags &= ~trans_dirty; //set to false in chunkSystem update
                    
                    // fpt_vec3 aabbRelMin = AabbComp.min          + transComp.pos_in_chunk;
                    // fpt_vec3 aabbRelMax = AabbComp.max          + transComp.pos_in_chunk;
                    
                    fpt_vec3 obbaabbRelMin = ObbComp.aabbMin    ;//+ AabbComp.offset;
                    fpt_vec3 obbaabbRelMax = ObbComp.aabbMax    ;//+ AabbComp.offset;
                    // print_fpt_vec3("aabb offset/pos in chunk delta: ", AabbComp.offset);

                    //JUST TO GET THE UPDATE TO THE CHUNK SYSTEM TO CHECK FOR COLLISIONS
                    aabbComp.flags |= aabb_dirty;

                    // print_fpt_vec3("AabbComp min : ",AabbComp.min );
                    // print_fpt_vec3("AabbComp max : ",AabbComp.max );
                    // print_fpt_vec3("obbAbbRel min: ",obbaabbRelMin); 
                    // print_fpt_vec3("obbAbbRel max: ",obbaabbRelMax);
                    if(aabbOutsideOrIntersectWallTest(aabbComp.min, aabbComp.max, obbaabbRelMin, obbaabbRelMax)){


                        recalculateAABB(aabbComp, ObbComp, transComp, ec.enlargeAABB, ec.enlargeAABBAmount);
                        aabbComp.flags |= aabb_dirty;
                        // printf("aabbFlag %d set to dirty\n", i);
                        // aabbComp.primitiveCount = mesh.primitiveCount;
                        ec.enlargeAABBAmountLast = enlargeAABBAmount;
                    }
                    
                    //}
                    //ObbComp.flag &= ~obbIsDirtyFlag;
                }

                //AABB logic

                
                //auto update the AABB if the slider has been adjusted
                //TODO:
                //UNCOMMENT WHEN WE ADD BACK IN ADJUSTABLE/ENLARGED AABBs
                // if(ec.enlargeAABB && ec.enlargeAABBAmountLast != enlargeAABBAmount){


                //     AabbComp& AabbComp = ec.AabbComps[i];

                //     //just access all the necessary data, its all initialized anyway, the entityID will be valid
                //     uint32_t obbIndex = ec.entityToObbMap[entityID];
                //     uint32_t meshIndex = ec.entityToMeshMap[entityID];
                    
                //     ObbComp& ObbComp = obbIndex != NULL_ENTITY ? ec.ObbComps[obbIndex] : defaultObbComp;
                //     MeshComp& MeshComp = meshIndex != NULL_ENTITY ? ec.MeshComps[meshIndex] : defaultMeshComp;
                //     Mesh& mesh = GameState->meshData->meshes[MeshComp.meshIndex];


                //     recalculateAABB(AabbComp, ObbComp, transComp, ec.enlargeAABB, ec.enlargeAABBAmount);

                //     ec.aabbFlags[i] |= aabb_dirty;
                //     AabbComp.primitiveCount = mesh.primitiveCount;
                //     ec.enlargeAABBAmountLast = enlargeAABBAmount;
                // }



            }
        }




