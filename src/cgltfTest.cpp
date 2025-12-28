#include "cgltfTest.h"

#define CGLTF_IMPLEMENTATION
#include "3rdParty/cgltf.h"

//this causes errors if used in the same file with regular cgltf?
// #define CGLTF_WRITE_IMPLEMENTATION
// #include "labour/3rdParty/cgltf_write.h"


    void setup_model_bounds(model* curr_model){
        curr_model->min.x = min(curr_model->min.x, curr_model->min.z);
        curr_model->min.z = min(curr_model->min.x, curr_model->min.z);
        curr_model->max.x = max(curr_model->max.x, curr_model->max.z);
        curr_model->max.z = max(curr_model->max.x, curr_model->max.z);
        curr_model->center =  (curr_model->min + curr_model->max) * 0.5f;
        curr_model->extents = (curr_model->max - curr_model->min) * 0.5f;

    }

    static inline entity_bone_types string_to_entity_bone_types(char* given){
        entity_bone_types type = {};
            if     (handmade_strcmp(given, "head_bone"              )) type = humanoid_bone_head;
            else if(handmade_strcmp(given, "torso_bone"             )) type = humanoid_bone_torso;
            else if(handmade_strcmp(given, "right_upper_arm_bone"   )) type = humanoid_bone_right_upper_arm;
            else if(handmade_strcmp(given, "right_lower_arm_bone"   )) type = humanoid_bone_right_lower_arm;
            else if(handmade_strcmp(given, "left_upper_arm_bone"    )) type = humanoid_bone_left_upper_arm;
            else if(handmade_strcmp(given, "left_lower_arm_bone"    )) type = humanoid_bone_left_lower_arm;
            else if(handmade_strcmp(given, "right_upper_leg_bone"   )) type = humanoid_bone_right_upper_leg;
            else if(handmade_strcmp(given, "right_lower_leg_bone"   )) type = humanoid_bone_right_lower_leg;
            else if(handmade_strcmp(given, "left_upper_leg_bone"    )) type = humanoid_bone_left_upper_leg;
            else if(handmade_strcmp(given, "left_lower_leg_bone"    )) type = humanoid_bone_left_lower_leg;
            return type;
    };

    animation_type determine_type(char* given){
        animation_type type = animation_type::anim_none;

        if     (handmade_strcmp(given, "idle"               ))  type =  animation_type::anim_idle;
        else if(handmade_strcmp(given, "walk"               ))  type =  animation_type::anim_walk;
        else if(handmade_strcmp(given, "run"                ))  type =  animation_type::anim_run;
        else if(handmade_strcmp(given, "jump"               ))  type =  animation_type::anim_jump;
        else if(handmade_strcmp(given, "airborn"            ))  type =  animation_type::anim_airborn;
        else if(handmade_strcmp(given, "land"               ))  type =  animation_type::anim_land;
        else if(handmade_strcmp(given, "attack"             ))  type =  animation_type::anim_attack;
        else if(handmade_strcmp(given, "cast"               ))  type =  animation_type::anim_cast;
        else if(handmade_strcmp(given, "left_punch"         ))  type =  animation_type::anim_left_punch;
        else if(handmade_strcmp(given, "right_punch"        ))  type =  animation_type::anim_right_punch;
        else if(handmade_strcmp(given, "left_light_weapon"  ))  type =  animation_type::anim_left_light_weapon;
        else if(handmade_strcmp(given, "right_light_weapon" ))  type =  animation_type::anim_right_light_weapon;
        else if(handmade_strcmp(given, "left_kick"          ))  type =  animation_type::anim_left_kick;
        else if(handmade_strcmp(given, "right_kick"         ))  type =  animation_type::anim_right_kick;
        else if(handmade_strcmp(given, "left_block"         ))  type =  animation_type::anim_left_block;
        else if(handmade_strcmp(given, "right_block"        ))  type =  animation_type::anim_right_block;
        else if(handmade_strcmp(given, "pull_bow"           ))  type =  animation_type::anim_pull_bow;
        else if(handmade_strcmp(given, "release_bow"        ))  type =  animation_type::anim_release_bow;
        else if(handmade_strcmp(given, "dodge_roll"         ))  type =  animation_type::anim_dodge_roll;

        return type;
    }

    void clear_equipment_sockets(model* curr_model){
        curr_model->left_fore_arm_socket    = 0;
        curr_model->right_fore_arm_socket   = 0;
        curr_model->left_upper_arm_socket   = 0;
        curr_model->right_upper_arm_socket  = 0;
        curr_model->left_thigh_socket       = 0;
        curr_model->right_thigh_socket      = 0;
        curr_model->left_shin_socket        = 0;
        curr_model->right_shin_socket       = 0;
        curr_model->head_socket             = 0;
        curr_model->torso_socket            = 0;
    }

    //checks if the node has a key name (head/torso/right_fore_arm to attach to a socket for equipment)
    void bind_node_to_equipment_socket(char* node_name, int nodeID, model* curr_model){
        if     (handmade_strcmp(node_name, "left_fore_arm"   ))  curr_model->left_fore_arm_socket    = nodeID;
        else if(handmade_strcmp(node_name, "right_fore_arm"  ))  curr_model->right_fore_arm_socket   = nodeID;
        else if(handmade_strcmp(node_name, "left_upper_arm"  ))  curr_model->left_upper_arm_socket   = nodeID;
        else if(handmade_strcmp(node_name, "right_upper_arm" ))  curr_model->right_upper_arm_socket  = nodeID;
        else if(handmade_strcmp(node_name, "left_thigh"      ))  curr_model->left_thigh_socket       = nodeID;
        else if(handmade_strcmp(node_name, "right_thigh"     ))  curr_model->right_thigh_socket      = nodeID;
        else if(handmade_strcmp(node_name, "left_shin"       ))  curr_model->left_shin_socket        = nodeID;
        else if(handmade_strcmp(node_name, "right_shin"      ))  curr_model->right_shin_socket       = nodeID;
        else if(handmade_strcmp(node_name, "head"            ))  curr_model->head_socket             = nodeID;
        else if(handmade_strcmp(node_name, "torso"           ))  curr_model->torso_socket            = nodeID;
    }


    void extract_nodes(gltf_data* gltfData, cgltf_data* data, cgltf_node* node, model* curr_model, vec3 cumulative_translation, vec3 mesh_color = {0.5, 0.5, 0.5}/*  uint32_t mesh_color = 0xFFAAAAAA */, int depth = 0, int parent = NULL_ENTITY_MESH){
        size_t vertex_offset = 0;
        size_t index_offset = 0;
        int vertex_count = 0;
        int index_count = 0;
        bool vertices_acquired = false;
        uint8_t* position_data = nullptr;
        uint8_t* index_data = nullptr;

        vec3 desired_scale = vec3_create(2);

        char indent[64]; // Max indentation of 64 spaces (adjust if needed)
        memset(indent, ' ', depth * 4);
        indent[depth * 4] = '\0'; // Null-terminate the string



        MODEL_DEBUG_PRINTF("%snode name : %s\n" ,indent, node->name);
        MODEL_DEBUG_PRINTF("%schildren        : %zd\n",indent, node->children_count);
        

        //setup node info
        int nodeID = gltfData->node_count++;
        if(curr_model)curr_model->total_nodes++;
        if(nodeID >= MAX_ENTITY_NODES){
            MODEL_DEBUG_PRINTF("ERROR TOO MANY NODES!!!!\n");
            MODEL_DEBUG_PRINTF("ERROR TOO MANY NODES!!!!\n");
            MODEL_DEBUG_PRINTF("ERROR TOO MANY NODES!!!!\n");
            return;
        }
        entity_node& new_node = gltfData->nodes[nodeID];
        new_node.nodeID = nodeID;

        //checks if the node has a key name (head/torso/right_fore_arm to attach to a socket for equipment)
        bind_node_to_equipment_socket(node->name, nodeID, curr_model);


        new_node.parent = parent;
        new_node.rotation = quat_identity();
        if(parent != NULL_ENTITY_MESH){
            entity_node& parent_node = gltfData->nodes[parent];
            if(parent_node.child_count >= MAX_ENTITY_CHILDREN){
                MODEL_DEBUG_PRINTF("ERROR TOO MANY CHILDREN ON NODE: %d, child count: %d\n", parent, parent_node.child_count);
                return;
            }
            parent_node.children[parent_node.child_count++] = nodeID;
        }
        else{//assign new root node
            gltfData->root_nodes[gltfData->root_node_count++] = nodeID;
            if(curr_model)curr_model->root_node = nodeID;
        }
        strcpy(new_node.name, node->name);
        
        entity_bone_types boneType = string_to_entity_bone_types(new_node.name);

        for(int i = 0; i < data->animations_count; i++){
            cgltf_animation& animation = data->animations[i];
            for (int j = 0; j < animation.channels_count; j++)
            {
                cgltf_animation_channel* channel = animation.channels + j;
                cgltf_node* animation_node = channel->target_node;

                if(node == animation_node){
                    cgltf_animation_sampler* sampler = channel->sampler;
                    cgltf_accessor* input  = sampler->input;
                    cgltf_accessor* output = sampler->output;
                    cgltf_buffer_view* input_buffer_view  = input ->buffer_view;
                    cgltf_buffer_view* output_buffer_view = output->buffer_view;
                    
                    MODEL_DEBUG_PRINTF("ANIMATION NODE MATCH! NAME: %s\n", animation.name);
                    MODEL_DEBUG_PRINTF("input  buffer view offset: %zd, size: %zd, stride: %zd\n", input_buffer_view ->offset, input_buffer_view ->size, input_buffer_view ->stride);
                    MODEL_DEBUG_PRINTF("output buffer view offset: %zd, size: %zd, stride: %zd\n", output_buffer_view->offset, output_buffer_view->size, output_buffer_view->stride);
                    
                    MODEL_DEBUG_PRINTF("channel: %d sampler interpolation type: %d, animation path type: %d, node name: %s\n", j, sampler->interpolation, channel->target_path, node->name);
                    MODEL_DEBUG_PRINTF("    input accessor : name: %s, component_type: %d, type: %d, offset: %zd, count: %zd, stride: %2zd, has_min: %d, has_max: %d max: %10.5f %10.5f %10.5f %10.5f\n", input ->name, input ->component_type, input ->type, input ->offset, input ->count, input ->stride, input ->has_min, input ->has_max, input ->max[0],input ->max[1], input ->max[2], input ->max[3]);
                    MODEL_DEBUG_PRINTF("    output accessor: name: %s, component_type: %d, type: %d, offset: %zd, count: %zd, stride: %2zd, has_min: %d, has_max: %d max: %10.5f %10.5f %10.5f %10.5f\n", output->name, output->component_type, output->type, output->offset, output->count, output->stride, output->has_min, output->has_max, output->max[0],output->max[1], output->max[2], output->max[3]);
                    uint8_t* input_data  = (uint8_t*)  input_buffer_view->buffer->data + input_buffer_view ->offset  + input->offset;
                    uint8_t* output_data = (uint8_t*) output_buffer_view->buffer->data + output_buffer_view->offset + output->offset;
                
                    if(new_node.has_animation){
                        //dont do anything, dont increment animation node
                    }
                    else{
                        new_node.has_animation = true;
                        new_node.animated_nodeID = gltfData->animated_node_count++;

                    }
                    animated_node* anim_node = &gltfData->animated_nodes[new_node.animated_nodeID];
                    anim_node->bone_type = boneType;
                    float* keyframe_times = nullptr;
                    animation_type type = determine_type(animation.name);
                    if(type == animation_type::anim_none)continue;

                    if(input ->max[0] > curr_model->animations[type].max_time)curr_model->animations[type].max_time = input->max[0]; //stores the maximum time for every animation

                    if(input->count != output->count)MODEL_DEBUG_PRINTF("ERROR, COUNTS DONT EQUAL EACHOTHER??? INPUT: %zd OUTPUT: %zd\n", input->count, output->count);
                    animation_data* anim_data = &anim_node->animations[type];
                    if(output->type == cgltf_type::cgltf_type_vec3){
                        anim_data->num_position_keyframes = output->count;
                        if(anim_data->num_position_keyframes >= MAX_KEYFRAMES)MODEL_DEBUG_PRINTF("TOO MANY POSITION KEYFRAMES!!: %d\n", anim_data->num_position_keyframes);
                        assert(!anim_data->has_translation && "OVERWRITING TRANSLATION ANIMATION? ERROR!");
                        keyframe_times = anim_data->position_keyframe_times;
                    
                    }
                    else if(output->type == cgltf_type::cgltf_type_vec4){
                        anim_data->num_rotation_keyframes = output->count;
                        if(anim_data->num_rotation_keyframes >= MAX_KEYFRAMES)MODEL_DEBUG_PRINTF("TOO MANY ROTATION KEYFRAMES!!: %d\n", anim_data->num_rotation_keyframes);
                        assert(!anim_data->has_rotation && "OVERWRITING ROTATION ANIMATION? ERROR!");
                        keyframe_times = anim_data->rotation_keyframe_times;
                    
                    }

                    if(anim_data->max_keyframes < output->count)anim_data->max_keyframes = output->count; 
                    
                    anim_data->type = type;
                    handmade_strcpy(anim_data->name, animation.name);
                    MODEL_DEBUG_PRINTF("assigning anim %s to bone: %s, anim_node: %d, anim_data slot: %d\n", animation.name, node->name, new_node.animated_nodeID, type);
                    if(input ->component_type == 6){ //floats
                        if(input ->type == 1){//scalar
                            MODEL_DEBUG_PRINTF("input: keyframes: ");
                            for (int k = 0; k < input->count; k++)
                            {
                                size_t byte_offset = (k * input->stride);
                                float scalar = (*(float*)(input_data + byte_offset));
                                MODEL_DEBUG_PRINTF("%3.3f ", scalar);
                                keyframe_times[k] = scalar;
                            }
                            MODEL_DEBUG_PRINTF("\n");
                        }
                        else if(input ->type == 3){//vec3
                            MODEL_DEBUG_PRINTF("input: keyframes: ");
                            for (int k = 0; k < input->count; k++)
                            {
                                size_t byte_offset = (k * input->stride);
                                vec3 vec = (*(vec3*)(input_data + byte_offset));
                                MODEL_DEBUG_PRINTF("%3.3f %3.3f %3.3f ", vec.x, vec.y, vec.z);
                            }
                            MODEL_DEBUG_PRINTF("\n");
                        }
                        else if(input ->type == 4){//vec4
                            MODEL_DEBUG_PRINTF("input: keyframes: ");
                            for (int k = 0; k < input->count; k++)
                            {
                                size_t byte_offset = (k * input->stride);
                                vec4 quat = (*(vec4*)(input_data + byte_offset));
                                MODEL_DEBUG_PRINTF("%3.3f %3.3f %3.3f %3.3f ", quat.x, quat.y, quat.z, quat.w);
                            }
                            MODEL_DEBUG_PRINTF("\n");
                        }

                    }

                    if(output->component_type == 6){ //floats
                        if(output->type == 1){//scalar
                            MODEL_DEBUG_PRINTF("output: keyframes: ");
                            for (int k = 0; k < output->count; k++)
                            {
                                size_t byte_offset = (k * output->stride); // 2 bytes per index (1 uint16_t, 2 bytes each)
                                float scalar = (*(float*)(output_data + byte_offset));
                                MODEL_DEBUG_PRINTF("%3.3f ", scalar);
                            }
                            MODEL_DEBUG_PRINTF("\n");
                        }
                        else if(output->type == 3){//vec3
                            MODEL_DEBUG_PRINTF("output: vec3s: ");
                            for (int k = 0; k < output->count; k++)
                            {
                                size_t byte_offset = (k * output->stride);
                                vec3 vec = desired_scale * (*(vec3*)(output_data + byte_offset));
                                MODEL_DEBUG_PRINTF("%3.3f %3.3f %3.3f ", vec.x, vec.y, vec.z);
                                anim_data->keyframe_positions[k] = vec;
                            }
                            anim_data->has_translation = true;

                            MODEL_DEBUG_PRINTF("\n");
                        }
                        else if(output->type == 4){//vec4
                            MODEL_DEBUG_PRINTF("output: vec4s: ");
                            for (int k = 0; k < output->count; k++)
                            {
                                size_t byte_offset = (k * output->stride);
                                quat newquat = quat_identity();
                                newquat.x = *(float*)(output_data + (byte_offset));
                                newquat.y = *(float*)(output_data + (byte_offset +   sizeof(float)));
                                newquat.z = *(float*)(output_data + (byte_offset + 2*sizeof(float)));
                                newquat.w = *(float*)(output_data + (byte_offset + 3*sizeof(float)));
                                MODEL_DEBUG_PRINTF("%3.3f %3.3f %3.3f %3.3f ", newquat.x, newquat.y, newquat.z, newquat.w);
                                anim_data->keyframe_rotations[k] = newquat;
                            
                            }
                            anim_data->has_rotation = true;

                            MODEL_DEBUG_PRINTF("\n");
                        }

                        anim_data->valid_animation = true;

                    }




                }




            }
        }


        if(node->has_translation){
            MODEL_DEBUG_PRINTF("%stranslation     : %3.3f %3.3f %3.3f \n",   indent, node->translation[0], node->translation[1], node->translation[2]); 
            new_node.translation.x = node->translation[0] * desired_scale.x; 
            new_node.translation.y = node->translation[1] * desired_scale.y; 
            new_node.translation.z = node->translation[2] * desired_scale.z; 
        }  
        new_node.base_translation = new_node.translation;
        
        cumulative_translation += new_node.translation;

        if(node->has_rotation)   {
            MODEL_DEBUG_PRINTF("%srotation        : %3.3f %3.3f %3.3f %3.3f \n",indent, node->rotation[0], node->rotation[1], node->rotation[2], node->rotation[3]); 
            new_node.rotation.x = node->rotation[0]; 
            new_node.rotation.y = node->rotation[1]; 
            new_node.rotation.z = node->rotation[2]; 
            new_node.rotation.w = node->rotation[3]; 
        
        }  
        new_node.base_rotation = (new_node.rotation); // * angleAxis(radians(180.0f), vec3_create(0,1,0)); /tried to flip it around, works but now we face the wrong direction
        
        if(node->has_scale)      {
            MODEL_DEBUG_PRINTF("%sscale           : %3.3f %3.3f %3.3f \n", indent,node->scale[0], node->scale[1], node->scale[2]); 
        }  
        if(node->has_matrix){       
            MODEL_DEBUG_PRINTF("%smatrix: \n", indent);
            MODEL_DEBUG_PRINTF("%s%3.3f %3.3f %3.3f %3.3f\n",indent, node->matrix[0 ], node->matrix[1 ], node->matrix[2 ], node->matrix[3 ]);
            MODEL_DEBUG_PRINTF("%s%3.3f %3.3f %3.3f %3.3f\n",indent, node->matrix[4 ], node->matrix[5 ], node->matrix[6 ], node->matrix[7 ]);
            MODEL_DEBUG_PRINTF("%s%3.3f %3.3f %3.3f %3.3f\n",indent, node->matrix[8 ], node->matrix[9 ], node->matrix[10], node->matrix[11]);
            MODEL_DEBUG_PRINTF("%s%3.3f %3.3f %3.3f %3.3f\n",indent, node->matrix[12], node->matrix[13], node->matrix[14], node->matrix[15]);
        }




        //read the mesh
        if(node->mesh){
            cgltf_mesh& mesh = *node->mesh;
            new_node.has_mesh = true;
            new_node.draw = true;
            
            MODEL_DEBUG_PRINTF("mesh name: %s\n", mesh.name);
            MODEL_DEBUG_PRINTF("mesh primitives  count: %zd\n", mesh.primitives_count);
            if(mesh.primitives_count > 1){
                //need to once again refactor the entity mesh system to handle more than one primitive :^)
                assert(!"NOT EQUIPPED TO HANDLE MORE THAN ONE PRIMITIVE PER MESH!!");
            }

            gltfData->primitive_count = mesh.primitives_count;

            for(int j = 0; j < mesh.primitives_count; j++){

                cgltf_primitive& primitive = mesh.primitives[j];
                // MODEL_DEBUG_PRINTF("primitive type                      : %d\n", primitive.type);
                cgltf_accessor& accessor = *primitive.indices;
                // MODEL_DEBUG_PRINTF("PRIMITIVE INDICES:\n");
                // MODEL_DEBUG_PRINTF("primitive accessor component type   : %d\n", accessor.component_type);
                // MODEL_DEBUG_PRINTF("primitive accessor normalized       : %d\n", accessor.normalized);
                // MODEL_DEBUG_PRINTF("primitive accessor type             : %d\n", accessor.type);
                // MODEL_DEBUG_PRINTF("primitive accessor count            : %zd\n", accessor.count);
                // MODEL_DEBUG_PRINTF("primitive accessor offset           : %zd\n", accessor.offset);
                // MODEL_DEBUG_PRINTF("primitive accessor stride           : %zd\n", accessor.stride);
                // MODEL_DEBUG_PRINTF("min:\n"); 
                // MODEL_DEBUG_PRINTF("%3.3f %3.3f %3.3f %3.3f\n", accessor.min[0 ],accessor.min[1 ],accessor.min[2 ],accessor.min[3 ]);
                // MODEL_DEBUG_PRINTF("max:\n"); 
                // MODEL_DEBUG_PRINTF("%3.3f %3.3f %3.3f %3.3f\n", accessor.max[0 ],accessor.max[1 ],accessor.max[2 ],accessor.max[3 ]);
    
                // MODEL_DEBUG_PRINTF("INDICES BUFFER VIEW:\n");
                cgltf_buffer_view& buffer_view = *accessor.buffer_view;
                // MODEL_DEBUG_PRINTF("buffer view name: %s\n", buffer_view.name);
                // MODEL_DEBUG_PRINTF("buffer view name: %s\n",cgltf_buffer* buffer;
                // MODEL_DEBUG_PRINTF("buffer view offset  : %zd\n",buffer_view.offset);
                // MODEL_DEBUG_PRINTF("buffer view size    : %zd\n",buffer_view.size);
                // MODEL_DEBUG_PRINTF("buffer view stride  : %zd\n",buffer_view.stride); /* 0 == automatically determined by accessor */
                // MODEL_DEBUG_PRINTF("buffer view type    : %d\n",buffer_view.type);
                // MODEL_DEBUG_PRINTF("buffer view name: %s\n",void* data; /* overrides buffer->data if present, f
                // if(mesh.primitives_count == 1){
                    //accessor.component_type will be 4 for uint16_t, i immagine it could be a uint32_t for more larger sets of indices
                    if(buffer_view.type == 1){//scalar indices
                        index_offset = buffer_view.offset;
                        index_count = accessor.count;
                        index_data = (uint8_t*)buffer_view.buffer->data + buffer_view.offset + accessor.offset;
                        // MODEL_DEBUG_PRINTF("scalar indices for primitive: %d\n", j);
                        gltfData->index_offset[gltfData->processed_meshes] = gltfData->total_index_count;
                        gltfData->index_count[gltfData->processed_meshes] = index_count;
                        new_node.index_offset = gltfData->total_index_count;
                        new_node.index_count = index_count;
                        for (int l = 0; l < index_count; l++) {
                            // Calculate the byte offset for the j-th index
                            size_t byte_offset = (l * 2); // 2 bytes per index (1 uint16_t, 2 bytes each)
                            uint32_t index = ((uint32_t)*(uint16_t*)(index_data + byte_offset)) + gltfData->vertex_count;
                            // MODEL_DEBUG_PRINTF("index %d: %d\n", gltfData->total_index_count, index);
                            //store the index data
                            gltfData->indices[gltfData->total_index_count++] = index;
                        }

                    }
                // }
                // else{
                    // assert(!"MORE THAN ONE PRIMITIVE IN THE MESH?");
                // }
                // MODEL_DEBUG_PRINTF("primitive ATTRIBUTES count: %zd\n", primitive.attributes_count);
                for(int k = 0; k < primitive.attributes_count; k++){
                    cgltf_attribute& attribute = primitive.attributes[k];
                    MODEL_DEBUG_PRINTF("attribute name                  : %s \n", attribute.name);
                    MODEL_DEBUG_PRINTF("attribute type                  : %d\n", attribute.type); //type 1 is the vertex positions
                    MODEL_DEBUG_PRINTF("attribute index                 : %d\n", attribute.index);

                    cgltf_accessor& accessor = *attribute.data;
                    MODEL_DEBUG_PRINTF("component type                  : %d\n", accessor.component_type);
                    MODEL_DEBUG_PRINTF("normalized                      : %d\n", accessor.normalized);
                    MODEL_DEBUG_PRINTF("type                            : %d\n", accessor.type);
                    MODEL_DEBUG_PRINTF("count                           : %zd\n", accessor.count);
                    MODEL_DEBUG_PRINTF("offset                          : %zd\n", accessor.offset);
                    MODEL_DEBUG_PRINTF("stride                          : %zd\n", accessor.stride);
                    MODEL_DEBUG_PRINTF("min:\n"); 
                    MODEL_DEBUG_PRINTF("%3.3f %3.3f %3.3f %3.3f\n", accessor.min[0 ],accessor.min[1 ],accessor.min[2 ],accessor.min[3 ]);
                    MODEL_DEBUG_PRINTF("max:\n"); 
                    MODEL_DEBUG_PRINTF("%3.3f %3.3f %3.3f %3.3f\n", accessor.max[0 ],accessor.max[1 ],accessor.max[2 ],accessor.max[3 ]);
                    
                    cgltf_buffer_view* attribute_buffer_view = accessor.buffer_view;
                    // MODEL_DEBUG_PRINTF("buffer view name: %s\n", attribute_buffer_view->name);
                    // MODEL_DEBUG_PRINTF("buffer view name: %s\n",cgltf_buffer* buffer;
                    // MODEL_DEBUG_PRINTF("buffer view offset  : %zd\n",attribute_buffer_view->offset);
                    // MODEL_DEBUG_PRINTF("buffer view size    : %zd\n",attribute_buffer_view->size);
                    // MODEL_DEBUG_PRINTF("buffer view stride  : %zd\n",attribute_buffer_view->stride); /* 0 == automatically determined by accessor */
                    // MODEL_DEBUG_PRINTF("buffer view type    : %d\n", attribute_buffer_view->type);

                    //cache the vertex offset
                    if(attribute.type == 1){
                        vertex_offset = accessor.offset;
                        vertex_count = accessor.count;
                        gltfData->primitive_vertex_count = vertex_count;
                        // MODEL_DEBUG_PRINTF("accessor vertex count: %d\n", vertex_count);
                        position_data = (uint8_t*)attribute_buffer_view->buffer->data + attribute_buffer_view->offset + accessor.offset;
                        new_node.vertex_offset = gltfData->vertex_count;
                        new_node.min = vec3_create( 100);
                        new_node.max = vec3_create(-100);
                        for (int l = 0; l < vertex_count; l++) {

                            // Calculate the byte offset for the j-th vertex
                            size_t byte_offset = l * 12; // 12 bytes per vertex (3 floats, 4 bytes each)
                            // Access the vertex data as floats
                            float x = *(float*)(position_data + byte_offset);
                            float y = *(float*)(position_data + byte_offset + 4);
                            float z = *(float*)(position_data + byte_offset + 8);
                            // Print the vertex position
                            // MODEL_DEBUG_PRINTF("vert %d: %3.3f %3.3f %3.3f\n", gltfData->vertex_count, x, y, z);
                            //this will store it 3 times probably. need to fix eventually
                            //store the vertex data
                            gltfData->test_vertices[gltfData->vertex_count]  .color =  mesh_color;
                            // gltfData->test_vertices[gltfData->vertex_count]  .color =  {0.5, 0.5, 0.5};
                            vec3 vert_pos = vec3_create(x, y, z) * desired_scale;
                            gltfData->test_vertices[gltfData->vertex_count++].pos = vert_pos; 
                            new_node.min    = vec3_min(new_node.min, vert_pos);
                            new_node.max    = vec3_max(new_node.max, vert_pos);
                            curr_model->min = vec3_min(curr_model->min, vert_pos + cumulative_translation);
                            curr_model->max = vec3_max(curr_model->max, vert_pos + cumulative_translation);
                        }
                        curr_model->original_min = curr_model->min;
                        curr_model->original_max = curr_model->max;




                        int fuck_the_debugger = 0;
                    }
                    //component type of 6 is a float
                    //stride is 12, 12 bytes apart

                    gltfData->processed_meshes++;
                }

            }
        }
        
          





        depth++;
        for(int i = 0; i < node->children_count; i++){
            cgltf_node* child_node = node->children[i];
            extract_nodes(gltfData, data, child_node, curr_model, cumulative_translation, mesh_color,  depth, nodeID);

        }
        //this is the same as above
        // if(!node.mesh)continue;
        // cgltf_mesh& mesh = *node.mesh;
        // MODEL_DEBUG_PRINTF("mesh name: %s\n", mesh.name);
        // MODEL_DEBUG_PRINTF("mesh primitives  count: %zd\n", mesh.primitives_count);

        // for(int j = 0; j < mesh.primitives_count; j++){
        //     cgltf_primitive& primitive = mesh.primitives[j];
        //     MODEL_DEBUG_PRINTF("primitive type                      : %d\n", primitive.type);
        //     cgltf_accessor& accessor = *primitive.indices;
        //     MODEL_DEBUG_PRINTF("PRIMITIVE INDICES:\n");
        //     MODEL_DEBUG_PRINTF("primitive accessor component type   : %d\n", accessor.component_type);
        //     MODEL_DEBUG_PRINTF("primitive accessor normalized       : %d\n", accessor.normalized);
        //     MODEL_DEBUG_PRINTF("primitive accessor type             : %d\n", accessor.type);
        //     MODEL_DEBUG_PRINTF("primitive accessor count            : %zd\n", accessor.count);
        //     MODEL_DEBUG_PRINTF("primitive accessor offset           : %zd\n", accessor.offset);
        //     MODEL_DEBUG_PRINTF("primitive accessor stride           : %zd\n", accessor.stride);
        //     MODEL_DEBUG_PRINTF("min:\n"); 
        //     MODEL_DEBUG_PRINTF("%3.3f %3.3f %3.3f %3.3f\n", accessor.min[0 ],accessor.min[1 ],accessor.min[2 ],accessor.min[3 ]);
        //     MODEL_DEBUG_PRINTF("max:\n"); 
        //     MODEL_DEBUG_PRINTF("%3.3f %3.3f %3.3f %3.3f\n", accessor.max[0 ],accessor.max[1 ],accessor.max[2 ],accessor.max[3 ]);

        //     MODEL_DEBUG_PRINTF("INDICES BUFFER VIEW:\n");
        //     cgltf_buffer_view& buffer_view = *accessor.buffer_view;
        //     MODEL_DEBUG_PRINTF("buffer view name: %s\n", buffer_view.name);
        //     // MODEL_DEBUG_PRINTF("buffer view name: %s\n",cgltf_buffer* buffer;
        //     MODEL_DEBUG_PRINTF("buffer view offset  : %zd\n",buffer_view.offset);
        //     MODEL_DEBUG_PRINTF("buffer view size    : %zd\n",buffer_view.size);
        //     MODEL_DEBUG_PRINTF("buffer view stride  : %zd\n",buffer_view.stride); /* 0 == automatically determined by accessor */
        //     MODEL_DEBUG_PRINTF("buffer view type    : %d\n",buffer_view.type);
        //     // MODEL_DEBUG_PRINTF("buffer view name: %s\n",void* data; /* overrides buffer->data if present, f
        //     if(mesh.primitives_count == 1){
        //         //accessor.component_type will be 4 for uint16_t, i immagine it could be a uint32_t for more larger sets of indices
        //         if(buffer_view.type == 1){//scalar indices
        //             index_offset = buffer_view.offset;
        //             index_count = accessor.count;
        //         }
        //     }
        //     else{
        //         assert(!"MORE THAN ONE PRIMITIVE IN THE MESH?");
        //     }
        //     MODEL_DEBUG_PRINTF("primitive ATTRIBUTES count: %zd\n", primitive.attributes_count);
        //     for(int j = 0; j < primitive.attributes_count; j++){
        //         cgltf_attribute& attribute = primitive.attributes[j];
        //         MODEL_DEBUG_PRINTF("attribute name                  : %s \n", attribute.name);
        //         MODEL_DEBUG_PRINTF("attribute type                  : %d\n", attribute.type); //type 1 is the vertex positions
        //         MODEL_DEBUG_PRINTF("attribute index                 : %d\n", attribute.index);

        //         cgltf_accessor& accessor = *attribute.data;
        //         MODEL_DEBUG_PRINTF("component type                  : %d\n", accessor.component_type);
        //         MODEL_DEBUG_PRINTF("normalized                      : %d\n", accessor.normalized);
        //         MODEL_DEBUG_PRINTF("type                            : %d\n", accessor.type);
        //         MODEL_DEBUG_PRINTF("count                           : %zd\n", accessor.count);
        //         MODEL_DEBUG_PRINTF("offset                          : %zd\n", accessor.offset);
        //         MODEL_DEBUG_PRINTF("stride                          : %zd\n", accessor.stride);
        //         MODEL_DEBUG_PRINTF("min:\n"); 
        //         MODEL_DEBUG_PRINTF("%3.3f %3.3f %3.3f %3.3f\n", accessor.min[0 ],accessor.min[1 ],accessor.min[2 ],accessor.min[3 ]);
        //         MODEL_DEBUG_PRINTF("max:\n"); 
        //         MODEL_DEBUG_PRINTF("%3.3f %3.3f %3.3f %3.3f\n", accessor.max[0 ],accessor.max[1 ],accessor.max[2 ],accessor.max[3 ]);
    
        //         //cache the vertex offset
        //         if(attribute.type == 1){
        //             vertex_offset = accessor.offset;
        //             vertex_count = accessor.count;
        //         }
        //         //component type of 6 is a float
        //         //stride is 12, 12 bytes apart
    
        //     }

        // }
    }

    void extract_mesh_data(gltf_data* gltfData, cgltf_data* data, model* curr_model = nullptr, vec3 mesh_color = {0.5, 0.5, 0.5}/* uint32_t mesh_color = 0xFFAAAAAA */){
        gltfData->processed_meshes = 0;
        MODEL_DEBUG_PRINTF("EXTRACT MESH DATA HERE!\n");

        MODEL_DEBUG_PRINTF("mesh count: %zd\n", data->meshes_count);
        size_t vertex_offset = 0;
        size_t index_offset = 0;
        int vertex_count = {0};
        int index_count[32] = {0};
        bool vertices_acquired = false;
        
        cgltf_buffer_view* position_buffer_view = nullptr;
        cgltf_buffer_view* index_buffer_view = nullptr;
        uint8_t* position_data = nullptr;
        uint8_t* index_data[32] = {nullptr};
        
        if(data->meshes_count > 1){
            // assert(!"CODE CANT HANDLE MORE THAN 1 MESH!");
        }
        gltfData->mesh_count = data->meshes_count;

        for(int i = 0; i < data->meshes_count; i++){
            
        }
        MODEL_DEBUG_PRINTF("mesh accessors   count: %zd\n", data->accessors_count);
        // for(int i = 0; i < data->accessors_count; i++){
        //     cgltf_accessor& accessor = data->accessors[i];
        //     MODEL_DEBUG_PRINTF("accessor component type: %d\n", accessor.component_type);
        //     MODEL_DEBUG_PRINTF("accessor normalized: %d\n", accessor.normalized);
        //     MODEL_DEBUG_PRINTF("accessor type: %d\n", accessor.type);
        //     MODEL_DEBUG_PRINTF("accessor offset: %zd\n", accessor.offset);
        //     MODEL_DEBUG_PRINTF("accessor stride: %zd\n", accessor.stride);
        //     MODEL_DEBUG_PRINTF("min:\n"); 
        //     MODEL_DEBUG_PRINTF("%3.3f %3.3f %3.3f %3.3f\n", accessor.min[0 ],accessor.min[1 ],accessor.min[2 ],accessor.min[3 ]);
        //     // MODEL_DEBUG_PRINTF("%3.3f %3.3f %3.3f %3.3f\n", accessor.min[4 ],accessor.min[5 ],accessor.min[6 ],accessor.min[7 ]);
        //     // MODEL_DEBUG_PRINTF("%3.3f %3.3f %3.3f %3.3f\n", accessor.min[8 ],accessor.min[9 ],accessor.min[10],accessor.min[11]);
        //     // MODEL_DEBUG_PRINTF("%3.3f %3.3f %3.3f %3.3f\n", accessor.min[12],accessor.min[13],accessor.min[14],accessor.min[15]);
        //     MODEL_DEBUG_PRINTF("max:\n"); 
        //     MODEL_DEBUG_PRINTF("%3.3f %3.3f %3.3f %3.3f\n", accessor.max[0 ],accessor.max[1 ],accessor.max[2 ],accessor.max[3 ]);
        //     // MODEL_DEBUG_PRINTF("%3.3f %3.3f %3.3f %3.3f\n", accessor.max[4 ],accessor.max[5 ],accessor.max[6 ],accessor.max[7 ]);
        //     // MODEL_DEBUG_PRINTF("%3.3f %3.3f %3.3f %3.3f\n", accessor.max[8 ],accessor.max[9 ],accessor.max[10],accessor.max[11]);
        //     // MODEL_DEBUG_PRINTF("%3.3f %3.3f %3.3f %3.3f\n", accessor.max[12],accessor.max[13],accessor.max[14],accessor.max[15]);


        // }
        MODEL_DEBUG_PRINTF("mesh buffers count: %zd\n", data->buffers_count);

        MODEL_DEBUG_PRINTF("mesh buffer views count : %zd\n", data->buffer_views_count);
        for(int i = 0; i < data->buffers_count; i++){
            cgltf_buffer_view& buffer_view = data->buffer_views[i];
        }


        
        MODEL_DEBUG_PRINTF("mesh animations  count: %zd\n", data->animations_count);
        for(int i = 0; i < data->animations_count; i++){
            if(curr_model)curr_model->animated = true;
            cgltf_animation& animation = data->animations[i];
            MODEL_DEBUG_PRINTF("animation name          : %s\n", animation.name);
            MODEL_DEBUG_PRINTF("animation sampler count : %zd\n", animation.samplers_count);
            // for (int j = 0; j < animation.samplers_count; j++)
            // {
            //     cgltf_animation_sampler* sampler = animation.samplers + j;
            //     cgltf_accessor* input  = sampler->input;
            //     cgltf_accessor* output = sampler->output;
            //     MODEL_DEBUG_PRINTF("sampler: %d, interpolation type: %d\n", j, sampler->interpolation);
            // }
            
            MODEL_DEBUG_PRINTF("animation channel count : %zd\n", animation.channels_count);
            for (int j = 0; j < animation.channels_count; j++)
            {
                cgltf_animation_channel* channel = animation.channels + j;
                cgltf_node* node = channel->target_node;

                cgltf_animation_sampler* sampler = channel->sampler;
                cgltf_accessor* input  = sampler->input;
                cgltf_accessor* output = sampler->output;
                MODEL_DEBUG_PRINTF("channel: %d sampler interpolation type: %d, animation path type: %d, node name: %s\n", j, sampler->interpolation, channel->target_path, node->name);
                MODEL_DEBUG_PRINTF("    input accessor : name: %s, component_type: %d, type: %d, offset: %zd, count: %zd, stride: %2zd, has_min: %d, has_max: %d\n", input ->name, input ->component_type, input ->type, input ->offset, input ->count, input ->stride, input ->has_min, input ->has_max);
                MODEL_DEBUG_PRINTF("    output accessor: name: %s, component_type: %d, type: %d, offset: %zd, count: %zd, stride: %2zd, has_min: %d, has_max: %d\n", output->name, output->component_type, output->type, output->offset, output->count, output->stride, output->has_min, output->has_max);

            }


            // MODEL_DEBUG_PRINTF("animation name          : %s\n", animation.name);
        }

        MODEL_DEBUG_PRINTF("mesh nodes count        : %zd\n", data->nodes_count);
        for(int i = 0; i < data->nodes_count; i++){
            cgltf_node& node = data->nodes[i];
            if(node.parent)continue;//skip all nodes with parents, they will be processed in the child loop
            MODEL_DEBUG_PRINTF("node name: %s\n", node.name);
            MODEL_DEBUG_PRINTF("children : %zd\n", node.children_count);

            vec3 cumulative_translation = vec3_create(0);
            for(int j = 0; j < node.children_count; j++){
                cgltf_node* child_node = node.children[j];

                extract_nodes(gltfData, data, child_node, curr_model, cumulative_translation, mesh_color);
            }

        }

        MODEL_DEBUG_PRINTF("noot node count: %d\n", gltfData->root_node_count);
        MODEL_DEBUG_PRINTF("printing out all mesh/entity/node data:\n");
        for (int i = 0; i < gltfData->node_count; i++)
        {
            entity_node* current_node = &gltfData->nodes[i];
            
            MODEL_DEBUG_PRINTF("nodeID: %d\n", i);
            MODEL_DEBUG_PRINTF("name:           %s\n", current_node->name);
            MODEL_DEBUG_PRINTF("parentID:       %d\n", current_node->parent);
            MODEL_DEBUG_PRINTF("index offset:   %d\n", current_node->index_offset);
            MODEL_DEBUG_PRINTF("index count:    %d\n", current_node->index_count);
            MODEL_DEBUG_PRINTF("vertex offset:  %d\n", current_node->vertex_offset);
            MODEL_DEBUG_PRINTF("has mesh:       %d\n", current_node->has_mesh);
            MODEL_DEBUG_PRINTF("translation: %3.3f %3.3f %3.3f \n"     , current_node->translation.x,current_node->translation.y,current_node->translation.z);
            MODEL_DEBUG_PRINTF("rotation   : %3.3f %3.3f %3.3f %3.3f\n", current_node->rotation.w,current_node->rotation.x,current_node->rotation.y,current_node->rotation.z);
            MODEL_DEBUG_PRINTF("children: ");
            for (size_t j = 0; j < current_node->child_count; j++)
            {
                MODEL_DEBUG_PRINTF("%d ", gltfData->nodes[current_node->children[j]].nodeID);
            }
            MODEL_DEBUG_PRINTF("\n");
            MODEL_DEBUG_PRINTF("\n");

        }


    }

    void load_gltf_models(gltf_data* gltfData, char* basePath){

        //create an empty root node at slot 0 for the enumerator
        gltfData->root_node_count++;
        gltfData->node_count++;
        
        model* null_model = gltfData->models + model_type::model_none; 
        null_model->type = model_type::model_none;
        null_model->root_node = 0;
        handmade_strcpy(null_model->name, "NULL MODEL");
        gltfData->model_count++;

                

        MODEL_DEBUG_PRINTF("cgltf test\n");

        char gltfPath[256];
        size_t offset = handmade_strcpy(gltfPath, basePath); //strncpy adds null terminator
        gltfPath[offset ] = '\0';
        // strcat(gltfPath, "gltf/riggedFigure/RiggedFigure.gltf");
        // strcat(gltfPath, "gltf/entity_test.gltf");
        // strcat(gltfPath, "gltf/test_binary/humanoid_large.gltf");
        strcat(gltfPath, "assets/gltf/test_binary/cubeGuy.gltf");

        cgltf_options options {0};
        cgltf_data* data = NULL;
            // Parse the GLTF file
        cgltf_result result = cgltf_parse_file(&options, gltfPath, &data);
        if (result != cgltf_result_success) {
            MODEL_DEBUG_PRINTF("Failed to parse GLTF file: %s\n", gltfPath);
            return;
        }

        // Load buffers (including .bin files)
        result = cgltf_load_buffers(&options, data, gltfPath);
        if (result != cgltf_result_success) {
            MODEL_DEBUG_PRINTF("Failed to load buffers for GLTF file: %s\n", gltfPath);
            cgltf_free(data);
            return;
        
        }

        MODEL_DEBUG_PRINTF("read mesh success!\n");


        //extracts mesh data for the single player mesh being imported at the moment, our enum for this maps to 1
        //slots the model in the array at the enumerator's value
        model* humanoid_model = gltfData->models + model_type::model_humanoid;
        clear_equipment_sockets(humanoid_model);
        humanoid_model->type = model_type::model_humanoid;
        humanoid_model->min = vec3_create(100);
        humanoid_model->max = vec3_create(-100);
        handmade_strcpy(humanoid_model->name, "HUMANOID MODEL");
        extract_mesh_data(gltfData, data, humanoid_model);
        cgltf_free(data);
        setup_model_bounds(humanoid_model);

        MODEL_DEBUG_PRINTF("humanoid model root node: %d, total nodes; %d, min: %10.5f %10.5f %10.5f max: %10.5f %10.5f %10.5f, center: %10.5f %10.5f %10.5f\n",
                humanoid_model->root_node, humanoid_model->total_nodes, 
                (humanoid_model->min.x),    (humanoid_model->min.y),    (humanoid_model->min.z), 
                (humanoid_model->max.x),    (humanoid_model->max.y),    (humanoid_model->max.z),  
                (humanoid_model->center.x), (humanoid_model->center.y), (humanoid_model->center.z));  
        //make the x and z axes equal, keep the y axis the same, just to account for entity rotation
        gltfData->model_count++;


        //IMPORT SWORD
        offset = handmade_strcpy(gltfPath, basePath); //strncpy adds null terminator
        gltfPath[offset] = '\0';
        handmade_strcpy(gltfPath + offset, "assets/gltf/test_binary/sword.gltf");
        result = cgltf_parse_file(&options, gltfPath, &data);
        if (result != cgltf_result_success) {
            MODEL_DEBUG_PRINTF("Failed to parse GLTF file: %s\n", gltfPath);
            return;
        }
        // Load buffers (including .bin files)
        result = cgltf_load_buffers(&options, data, gltfPath);
        if (result != cgltf_result_success) {
            MODEL_DEBUG_PRINTF("Failed to load buffers for GLTF file: %s\n", gltfPath);
            cgltf_free(data);
            return;
        
        }
        MODEL_DEBUG_PRINTF("read SWORD mesh success!\n");

        //extracts mesh data for the single player mesh being imported at the moment, our enum for this maps to 1
        //slots the model in the array at the enumerator's value
        model* sword_model = gltfData->models + model_type::model_sword;
        sword_model->type = model_type::model_sword;
        sword_model->min = vec3_create(100);
        sword_model->max = vec3_create(-100);
        handmade_strcpy(sword_model->name, "SWORD MODEL");
        extract_mesh_data(gltfData, data, sword_model, {.6, .6, 1}/* 0xFFFFAAAA */);
        cgltf_free(data);
        setup_model_bounds(sword_model);

        //IMPORT SHIELD
        offset = handmade_strcpy(gltfPath, basePath); //strncpy adds null terminator
        gltfPath[offset] = '\0';
        handmade_strcpy(gltfPath + offset, "assets/gltf/test_binary/shield.gltf");
        result = cgltf_parse_file(&options, gltfPath, &data);
        if (result != cgltf_result_success) {
            MODEL_DEBUG_PRINTF("Failed to parse GLTF file: %s\n", gltfPath);
            return;
        }
        // Load buffers (including .bin files)
        result = cgltf_load_buffers(&options, data, gltfPath);
        if (result != cgltf_result_success) {
            MODEL_DEBUG_PRINTF("Failed to load buffers for GLTF file: %s\n", gltfPath);
            cgltf_free(data);
            return;
        
        }
        MODEL_DEBUG_PRINTF("read SHIELD mesh success!\n");

        //extracts mesh data for the single player mesh being imported at the moment, our enum for this maps to 1
        //slots the model in the array at the enumerator's value
        model* shield_model = gltfData->models + model_type::model_shield;
        shield_model->type = model_type::model_shield;
        shield_model->min = vec3_create(100);
        shield_model->max = vec3_create(-100);
        handmade_strcpy(shield_model->name, "SHIELD MODEL");
        extract_mesh_data(gltfData, data, shield_model, {1, .6, .6}/* 0xFFAAAAFF */);
        cgltf_free(data);
        setup_model_bounds(shield_model);
            
        
      
        // gltf_data* gltfData = gltfData;

        // gltfData->debug_current_animation_type = animation_type::anim_idle;
        gltfData->debug_current_animation_type = animation_type::anim_walk;


        int fuck_the_debugger = 0;
    }


