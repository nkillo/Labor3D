#include "ui.h"
#include "entity.h"


const char* element_type_to_str(ui_element_types type){
    switch (type){
        // case window_type_none :     {return "NONE";}break;
        case ui_element_type_none:                      {return "NONE";}
        case ui_element_type_bar:                       {return "BAR";}
        case ui_element_type_bar_graph:                 {return "BAR GRAPH";}
        case ui_element_type_bounds:                    {return "BOUNDS";}
        case ui_element_type_resize:                    {return "RESIZE";}
        case ui_element_type_drag:                      {return "DRAG";}
        case ui_element_type_checkbox:                  {return "CHECKBOX";}
        case ui_element_type_slider_float:              {return "SLIDER FLOAT";}
        case ui_element_type_slider_int:                {return "SLIDER INT";}
        case ui_element_type_button:                    {return "BUTTON";}
        case ui_element_type_window_base:               {return "WINDOW BASE";}
        case ui_element_type_editable_text:             {return "EDIT TEXT";}
        case ui_element_type_right_book_page:           {return "RIGHT BOOK PAGE";}
        case ui_element_type_right_arrow:               {return "LEFT ARROW";}
        case ui_element_type_left_arrow:                {return "RIGHT ARROW";}
        case ui_element_type_inventory_slot:            {return "INV SLOT";}
        case ui_element_type_horizontal_scrollbar:      {return "HORIZONTAL SCROLLBAR";}
        case ui_element_type_vertical_scrollbar:        {return "VERTICAL SCROLLBAR";}
        case ui_element_type_text:                      {return "TEXT";}
    }
    return "NONE";
}


//in case memory is hot reloaded, we need to clear all the char* labels, invalidating the entire table
void clearUIHashTable(ui_data* uiData){
    memset(uiData->uiHash, 0, sizeof(ui_hash_entry) * MAX_UI_HASH_SLOTS * MAX_UI_HASH_BUCKETS);
    memset(uiData->uiHashBucketCount, 0, sizeof(u8) * MAX_UI_HASH_SLOTS);
}


ui_hash_entry* UIHashElement(ui_data* uiData, char* label, ui_element* element, u32* parentHash = NULL){
    u32 hash = hash_str(label) & (MAX_UI_HASH_SLOTS - 1);
    element->hash = hash;
    //TODO: (nate) may want to incorporate parent hashes later
    // if(parentHash){
    //     ui_element* parent = element->parent;
    //     while(parent){
    //         hash ^= parent->hash;
    //         parent = parent->parent;

    //     }
    // }
    // hash ^= (*parentHash);
    
    u8* bucketCount = uiData->uiHashBucketCount + hash;
    bool found = false;
    ui_hash_entry* entry = 0;

    for(u8 bucket = 0; bucket < MAX_UI_HASH_BUCKETS; bucket++){
        entry = uiData->uiHash[hash] + bucket;
        if(!entry->element){
            Assert((*bucketCount) < MAX_UI_HASH_BUCKETS && "HASH SLOT OUT OF BUCKET MEMORY!");
            (*bucketCount)++;
            entry->element = element;
            entry->prevType = element->type;
            entry->label = label;
            entry->versionID = uiData->curVersionID;
            found = true;
            break;
        }else{//handle collision with populated entry
            if(!handmade_strcmp(entry->label, label)){
                //go to next bucket
                continue;
            }else{
                entry->element = element;
                if(entry->prevType != element->type){//basic check for future type checking in case we eventually need it
                    printf("existing hashed element: %s type mismatch? oldType: %s, newType: %s\n", label, element_type_to_str(entry->prevType), element_type_to_str(element->type));
                } 
                entry->versionID = uiData->curVersionID;
                found = true;
                break;//entry matches us
            }
        }
    }
    
    if(!found){
        printf("UI HASH SLOT %d OUT OF SPACE!\n", hash);
        Assert(!"OUT OF BUCKET SPACE!");
    }
    Assert(entry);
    return entry;
}

ui_hash_entry* UIFindHashElement(ui_data* uiData, char* label, ui_element* element, u32* parentHash = NULL){
    u32 hash = hash_str(label) & (MAX_UI_HASH_SLOTS - 1);
    //TODO: (nate) may want to incorporate parent hashes later
    if(parentHash)hash ^= (*parentHash);

    u8* bucketCount = uiData->uiHashBucketCount + hash;
    ui_hash_entry* entry = 0;

    for(u8 bucket = 0; bucket < (*bucketCount); bucket++){
        entry = uiData->uiHash[hash] + bucket;
        if(!entry->element){//if we hit an empty bucket, this hash hasnt yet been populated
            entry = 0;
            break;
        }else{//handle collision with populated entry
            if(!handmade_strcmp(entry->label, label)){
                //go to next bucket
                continue;
            }else{
                //dont yet know how we will use the versionID
                // if(entry->versionID != uiData->curVersionID)entry = 0;
                break;//entry matches us
            }
        }
    }
    
    //can return null entry if not found
    return entry;
}



const char* window_type_to_str(ui_window_types type){
    switch (type){
        case window_type_chat             :{return "CHAT ";}break;
        case window_type_inventory_hotbar :{return "HOTBAR ";}break;
        case window_type_inventory        :{return "INVENTORY ";}break;
        case window_type_inventory_other  :{return "OTHER INV" ;}break;
        case window_type_inventory_storage:{return "STORAGE ";}break;
        case window_type_book_text        :{return "BOOK TEXT ";}break;
        case window_type_book_cover       :{return "BOOK COVER ";}break;
        case window_type_stats            :{return "STATS ";}break;
        case window_type_feedback         :{return "FEEDBACK ";}break;
        case window_type_debug            :{return "DEBUG ";}break;
        case window_type_count            :{return "COUNT ";}break;
    }
    return "NONE";
}



const char* item_type_to_str(ui_item_types type){
    switch (type){
        // case window_type_none :     {return "NONE";}break;
     
        case ui_item_type_none:{return "NONE";}
        case ui_item_type_sword:{return "SWORD";}
        case ui_item_type_shield:{return "SHIELD";}
        case ui_item_type_helm:{return "HELMET";}
    }
    return "NONE";
}


const char* element_inventory_type_to_str(ui_element_inventory_types type){
    switch (type){

        case ui_element_inventory_type_none:            {return "NONE";}
        case ui_element_inventory_type_item_slot:       {return "ITEM";}
        case ui_element_inventory_type_trinket_slot:    {return "TRINKET";}
        case ui_element_inventory_type_equipment_slot:  {return "EQUIPMENT";}
        case ui_element_inventory_type_hotbar_slot:     {return "HOTBAR";}
    }
    return "NONE";
}

const char* item_to_str(item_data& item){
    switch (item.type){


        
            case item_type_none          :{return "NONE";}
            case item_type_tool          :{
                switch(item.tool.type){
                    case tool_type_none:    {return "TOOL: NONE";}
                    case tool_type_sword:   {return "TOOL: SWORD";}
                    case tool_type_shield:  {return "TOOL: SHIELD";}
                    case tool_type_bow:     {return "TOOL: BOW";}
                    case tool_type_book:    {return "TOOL: BOOK";}
                    case tool_type_unique:  {return "TOOL: UNIQUE";}
                }
            }
            case item_type_equipment     :{
                switch(item.equipment.type){
                    case equipment_type_none:       {return "EQUIP: NONE";}
                    case equipment_type_helmet:     {return "EQUIP: HELMET";}
                    case equipment_type_torso:      {return "EQUIP: TORSO";}
                    case equipment_type_upper_arm:  {return "EQUIP: UPPER ARM";}
                    case equipment_type_upper_leg:  {return "EQUIP: UPPER LEG";}
                    case equipment_type_lower_arm:  {return "EQUIP: LOWER ARM";}
                    case equipment_type_lower_leg:  {return "EQUIP: LOWER LEG";}
                }
            }    
            case item_type_trinket       :{
                switch(item.trinket.type){
                    case trinket_type_none:         {return "TRINKET: NONE";}
                    case trinket_type_amulet:       {return "TRINKET: AMULET";}
                    case trinket_type_ring:         {return "TRINKET: RING";}
                    case trinket_type_bracelet:     {return "TRINKET: BRACELET";}
                    case trinket_type_artifact:     {return "TRINKET: ARTIFACT";}
                }
            }    
            case item_type_voxel         :{
                return "VOXEL";
                // switch(item.item.type){

                // }
            }
            case item_type_consumable    :{
                return "CONSUMABLE";
                // switch(item.item.type){
// 
                // }
            }            
        
    }
    return "NONE";
}



    void append_feedback_text_element(ui_data* uiData, const char* text){
        ui_window& feedback = uiData->windows[ui_window_types::window_type_feedback];

        if(feedback.data.feedback.write_slot > (MAX_FEEDBACK_ELEMENTS - 1))feedback.data.feedback.write_slot = 0;

        char* dest = feedback.data.feedback.text_labels[feedback.data.feedback.write_slot];
        int len = handmade_strcpy(dest, text);
        dest[len] = 0;
        ui_element* element = feedback.data.feedback.elements + feedback.data.feedback.write_slot++;
        if(feedback.data.feedback.element_count < MAX_FEEDBACK_ELEMENTS)feedback.data.feedback.element_count++;
        element->type = ui_element_types::ui_element_type_text;
        element->data.text.label = dest;

        //calculate new scrollbar size
        int element_count = feedback.data.feedback.element_count;
        float combined_element_height = element_count * MIN_WINDOW_DIMENSIONS;
        ui_element* scrollbar = &feedback.vertical_scrollbar;
        scrollbar->data.slider_float.visible_range = feedback.base.height / combined_element_height;

        if(scrollbar->data.slider_float.visible_range > 1.0f) scrollbar->data.slider_float.visible_range = 1.0f;
        scrollbar->data.slider_float.current_value = 1.0f;

        scrollbar->data.slider_float.max = combined_element_height >= feedback.base.height ? combined_element_height : feedback.base.height; 

    }

    static inline void transition_ui_state(ui_data* uiData, ui_state_types new_state){
        if(uiData->current_state != new_state){
            uiData->previous_state = uiData->current_state;
            uiData->current_state = new_state;
        }

        
    }

    
    const char* ui_state_to_str(ui_state_types state){
        switch (state){
    
            case ui_state_none              :   {return "NONE";}
            case ui_state_resize_window     :   {return "RESIZE WINDOW";}
            case ui_state_drag_window       :   {return "DRAG WINDOW";}
            case ui_state_drag_item         :   {return "DRAG ITEM";}
            case ui_state_drag_slider       :   {return "DRAG SLIDER";}
            case ui_state_chat_input        :   {return "CHAT INPUT";}
            case ui_state_text_editor_input :   {return "TEXT EDITOR INPUT";}
        }
        return "NONE";
        
    }


uint32_t hash_string(const char* key){
    uint32_t hash = 2166136261u;
    for(int i = 0; key[i]; i++){
        hash ^= key[i];
        hash *= 16777619;
    }
    return (hash & LABEL_TABLE_ENTRIES - 1);
}   


void flush_table(ui_label_table* table){
    memset(table, 0, sizeof(ui_label_table));
}



ui_element_data push_table(ui_label_table* table, char* key, ui_element_data data, ui_element_types type){
    ui_element_data temp_data = {};

    if(key == NULL){
        printf("push_table() given key is NULL!\n");
        return temp_data;
    }
    uint32_t hash = hash_string(key);
    uint32_t bucket = 0;
    

    for(bucket = 0; bucket < LABEL_TABLE_BUCKETS; bucket++){
        if(!table->entries[hash][bucket].name){//found unoccupied bucket
            //if its unoccupied, set the label at that slot in memory and assign it
            handmade_strcpy(table->labels[hash][bucket], key);
            table->entries[hash][bucket].name = table->labels[hash][bucket];
            table->entries[hash][bucket].data = data;
            table->entries[hash][bucket].type = type;
            table->entry_count[hash]++;
            table->total_entry_count++;
            // printf("successfully added key %s at hash %u, entry count at hash: %d, total_entry_count: %d\n", key, hash, table->entry_count[hash], table->total_entry_count);
            return data;
        }else if(handmade_strcmp(table->entries[hash][bucket].name, key)){
            // printf("KEY %s ALREADY EXISTS AT HASH %d IN SCOPE! ERROR! REASSIGNING!\n", key, hash);
            // return 0;
            //allow redefinition
            temp_data = table->entries[hash][bucket].data; //pulls out old information before we reset it, useful for getting back if the element was clicked
            table->entries[hash][bucket].data = data;
            // printf("key %s ALREADY EXISTS at hash %u, entry count at hash: %d, total_entry_count: %d\n", key, hash, table->entry_count[hash], table->total_entry_count);
            return temp_data;
        }
    }

    printf("ERROR! table AT HASH %d FULL!!\n", hash);
    return temp_data;
}

void drag_item(game_state* GameState, ui_data* uiData){
    uint32_t entityID = GameState->localPlayerEntityIDs[0];
    u32 invIndex = GameState->entityComponent->entityToInventoryMap[entityID];
    InventoryComp& invComp = GameState->entityComponent->InventoryComps[invIndex != NULL_ENTITY ? invIndex : 2046];//give it some kind of valid value just in case 
    
    u32 stateIndex = GameState->entityComponent->entityToInventoryMap[entityID];
    StateComp& state = GameState->entityComponent->StateComps[stateIndex];
    //prevent item manipulation during certain entity states
    switch(state.smAct.cur){
        case entity_state_none:{}break;
        case entity_idle_state_default:  {}break;
        case entity_movement_state_walk: {}break;
        case entity_movement_state_jump: {}break;
        case entity_movement_state_roll: {}break;
        case entity_combat_state_left_punch:   //falls through
        case entity_combat_state_right_punch:  
        case entity_combat_state_left_kick :   
        case entity_combat_state_right_kick:   
        case entity_combat_state_left_weapon:  
        case entity_combat_state_right_weapon: 
        case entity_combat_state_left_block:   
        case entity_combat_state_right_block:  {
            append_feedback_text_element(uiData, "CANT CLICK INVENTORY WHEN FIGHTING!");
            return;
        }break;
        case entity_interact_state_idle: {}break;
        default:{}break;
    }
    
    
    printf("CLICKING ON ENTITY ITEM: %d\n", uiData->hovered_itemID);
    transition_ui_state(uiData, ui_state_types::ui_state_drag_item);
    uiData->dragged_itemID = uiData->hovered_itemID;
    uiData->dragged_item_original_slot = uiData->hovered_item_slot;
    *uiData->dragged_item_original_slot = 0;//empty the slot
    uiData->hovered_itemID = 0;
    uiData->dragged_item_scale_x = uiData->selected_element->width;
    uiData->dragged_item_scale_y = uiData->selected_element->height;
    //drag the item instead
    printf("SELECTING NEW DRAGGING ITEM ORIGINAL SLOT\n");
}

int get_table(ui_label_table* table, char* key, ui_element_data& data, ui_element_types type){
    if(key == NULL){
        printf("get_table() given key is NULL!\n");
        return 0;
    }
    uint32_t hash = hash_string(key);
    uint32_t bucket = 0;
    

    for(bucket = 0; bucket < LABEL_TABLE_BUCKETS; bucket++){
        if(!table->entries[hash][bucket].name){//found unoccupied bucket
            //if its unoccupied, set the label at that slot in memory and assign it
            handmade_strcpy(table->labels[hash][bucket], key);
            table->entries[hash][bucket].name = table->labels[hash][bucket];
            table->entries[hash][bucket].data = data;
            table->entries[hash][bucket].type = type;
            table->entry_count[hash]++;
            table->total_entry_count++;
            // printf("successfully added key %s at hash %u, entry count at hash: %d, total_entry_count: %d\n", key, hash, table->entry_count[hash], table->total_entry_count);
            return 1;
        }else if(handmade_strcmp(table->entries[hash][bucket].name, key)){
            data = table->entries[hash][bucket].data;
            switch(table->entries[hash][bucket].type){
                case ui_element_types::ui_element_type_checkbox:{
                    table->entries[hash][bucket].data.checkbox.clicked = false;
                }
                case ui_element_types::ui_element_type_button:{
                    table->entries[hash][bucket].data.button.clicked = false;

                }
            }
            return 1;
        }
    }

    printf("ERROR! COULDN'T FIND KEY %s at HASH %d!!\n",key, hash);
    return 0;
}

    void init_slider(ui_element* bar){
        bar->data.slider_float.current_value = 0.0f;
        bar->data.slider_float.min = 1.0f;
        bar->data.slider_float.max = 1.0f;
        bar->data.slider_float.visible_range = 1.0f;
    }



    void adjust_slider(ui_element* bar, float dimension, float amount){
        float vis_range = bar->data.slider_float.visible_range;
        if(vis_range == 1.0f)return;

        // printf("vis_range: %f\n", vis_range);
        // printf("dimension: %f\n", dimension);
        // printf("amount: %f\n", amount);
        float scrollDiff = dimension - (dimension * vis_range);
        // printf("scrollDiff: %f\n", scrollDiff);

        //normalized
        float normalized_movement = amount  / scrollDiff;
        float new_val = bar->data.slider_float.current_value + (normalized_movement);
        // printf("normalized_movement: %f\n", normalized_movement);
        // printf("new_val: %f\n", new_val);
        // printf("current value:  %f\n", bar->data.slider_float.current_value);
        
        bar->data.slider_float.current_value += normalized_movement;
        if(new_val > 1.0f)bar->data.slider_float.current_value = 1.0f;
        else if(new_val < 0.0f)bar->data.slider_float.current_value = 0.0f;

        // printf("final value: : %f\n", bar->data.slider_float.current_value);
        
    } 

    
    void adjust_debug_slider(ui_element* bar, float dimension, float amount){
        float vis_range = bar->data.debug_slider_float.visible_range;
        if(vis_range == 1.0f)return;

        float scrollDiff = dimension - (dimension * vis_range);

        //normalized
        // float normalized_movement = (amount + (bar->data.debug_slider_float.max - bar->data.debug_slider_float.min))  / scrollDiff;
        float normalized_movement = amount  / scrollDiff;
        // float new_val = (*bar->data.debug_slider_float.current_value) + (normalized_movement);
        float new_val = (bar->data.debug_slider_float.norm_value) + (normalized_movement);

        (bar->data.debug_slider_float.norm_value) += normalized_movement;
        if(new_val > 1.0f)bar->data.debug_slider_float.norm_value = 1.0f;
        else if(new_val < 0.0f)bar->data.debug_slider_float.norm_value = 0.0f;
        (*bar->data.debug_slider_float.current_value) = ((bar->data.debug_slider_float.norm_value) * (bar->data.debug_slider_float.max - bar->data.debug_slider_float.min)) + bar->data.debug_slider_float.min;  
        if(bar->data.debug_slider_float.adjusted)(*bar->data.debug_slider_float.adjusted) = true;
    } 

    
    void scroll_debug_slider(ui_element* bar, float dimension, float amount){
        float vis_range = bar->data.debug_slider_float.visible_range;
        if(vis_range == 1.0f)return;

        // float scrollDiff = dimension - (dimension * vis_range);
        float scrollDiff = 2000.0f;//same scrolling for every debug slider element

        //normalized
        // float normalized_movement = (amount + (bar->data.debug_slider_float.max - bar->data.debug_slider_float.min))  / scrollDiff;
        float normalized_movement = amount  / scrollDiff;
        // float new_val = (*bar->data.debug_slider_float.current_value) + (normalized_movement);
        float new_val = (bar->data.debug_slider_float.norm_value) + (normalized_movement);

        (bar->data.debug_slider_float.norm_value) += normalized_movement;
        if(new_val > 1.0f)bar->data.debug_slider_float.norm_value = 1.0f;
        else if(new_val < 0.0f)bar->data.debug_slider_float.norm_value = 0.0f;
        (*bar->data.debug_slider_float.current_value) = ((bar->data.debug_slider_float.norm_value) * (bar->data.debug_slider_float.max - bar->data.debug_slider_float.min)) + bar->data.debug_slider_float.min;  
        if(bar->data.debug_slider_float.adjusted)(*bar->data.debug_slider_float.adjusted) = true;
    } 



    void enable_element(ui_element& element){
        element.active = true;
    }


    void disable_element(ui_element& element){
        element.active = false;
    }


    bool intersectElement(game_state* GameState, vec2 mousePos, ui_element& element, float rootx = 0.0f, float rooty = 0.0f){
        ui_data* uiData = GameState->uiData;
        
        float minx = (element.posx + rootx) - element.width  * 0.5f;
        float miny = (element.posy + rooty) - element.height * 0.5f;
        float maxx = (element.posx + rootx) + element.width  * 0.5f;
        float maxy = (element.posy + rooty) + element.height * 0.5f;

        element.hovered = false;
        element.clicked = false;

        if(mousePos.x > minx && mousePos.x < maxx &&
           mousePos.y > miny && mousePos.y < maxy){
            element.hovered = true;
            return true;
        }

        return false;
    }
    
    bool intersect_horizontal_slider_bounds(game_state* GameState, ui_data* uiData,  vec2 mousePos, ui_element* slider, float rootx = 0.0f, float rooty = 0.0f){
        float vis_range = slider->data.slider_float.visible_range;
        float curval = slider->data.slider_float.current_value;
        float scrollMin = (slider->minx); //minx
        float scrollMax = (slider->maxx); //maxx
        float scrollDiff = scrollMax - scrollMin - (slider->width * vis_range); //the range from 0 to 1 we can slide in //width


        float scrollbar_posX    = rootx + (scrollMin + (slider->width * vis_range * 0.5f)) + (scrollDiff * curval);// * (0.25f));
        float scrollbar_posY    = rooty + slider->posy;
        float scrollbar_scaleX  = slider->scale.x * vis_range;
        float scrollbar_scaleY  = slider->scale.y;

        float minx = scrollbar_posX - (scrollbar_scaleX * 0.5f);
        float maxx = scrollbar_posX + (scrollbar_scaleX * 0.5f);
        float miny = scrollbar_posY - (scrollbar_scaleY * 0.5f);
        float maxy = scrollbar_posY + (scrollbar_scaleY * 0.5f);
        if(mousePos.x > minx && mousePos.x < maxx &&
            mousePos.y > miny && mousePos.y < maxy){
            uiData->mouse_intersects_slider = true;
            return true;
        }
        return false;
    }


    bool intersect_horizontal_debug_slider_bounds(game_state* GameState, ui_data* uiData,  vec2 mousePos, ui_element* slider, float rootx = 0.0f, float rooty = 0.0f){
        float vis_range = slider->data.debug_slider_float.visible_range;
        // float* curval = slider->data.debug_slider_float.current_value;
        float normval = slider->data.debug_slider_float.norm_value;
        float scrollMin = (slider->minx); //minx
        float scrollMax = (slider->maxx); //maxx
        float scrollDiff = scrollMax - scrollMin - (slider->width * vis_range); //the range from 0 to 1 we can slide in //width

        float scrollbar_posX    = rootx + (scrollMin + (slider->width * vis_range * 0.5f)) + (scrollDiff * (normval));// * (0.25f));
        float scrollbar_posY    = rooty + slider->posy;
        float scrollbar_scaleX  = slider->scale.x * vis_range;
        float scrollbar_scaleY  = slider->scale.y;

        float minx = scrollbar_posX - (scrollbar_scaleX * 0.5f);
        float maxx = scrollbar_posX + (scrollbar_scaleX * 0.5f);
        float miny = scrollbar_posY - (scrollbar_scaleY * 0.5f);
        float maxy = scrollbar_posY + (scrollbar_scaleY * 0.5f);
        if(mousePos.x > minx && mousePos.x < maxx &&
            mousePos.y > miny && mousePos.y < maxy){
            uiData->mouse_intersects_slider = true;
            return true;
        }
        return false;
    }
    
    bool intersect_vertical_debug_slider_bounds(game_state* GameState, ui_data* uiData,  vec2 mousePos, ui_element* slider, float rootx = 0.0f, float rooty = 0.0f){
        float vis_range = slider->data.debug_slider_float.visible_range;
        // float* curval = slider->data.debug_slider_float.current_value;
        float normval = slider->data.debug_slider_float.norm_value;
        float scrollMin = (slider->miny); //minx
        float scrollMax = (slider->maxy); //maxx
        float scrollDiff = scrollMax - scrollMin - (slider->height * vis_range); //the range from 0 to 1 we can slide in //width

        float scrollbar_posX    = rootx + slider->posx;
        float scrollbar_posY    = rooty + (scrollMin + (slider->height * vis_range * 0.5f)) + (scrollDiff * (1.0f - normval)) + slider->posy;// * (0.25f));
        float scrollbar_scaleX  = slider->scale.x;
        float scrollbar_scaleY  = slider->scale.y * vis_range;

        float minx = scrollbar_posX - (scrollbar_scaleX * 0.5f);
        float maxx = scrollbar_posX + (scrollbar_scaleX * 0.5f);
        float miny = scrollbar_posY - (scrollbar_scaleY * 0.5f);
        float maxy = scrollbar_posY + (scrollbar_scaleY * 0.5f);
        if(mousePos.x > minx && mousePos.x < maxx &&
            mousePos.y > miny && mousePos.y < maxy){
            uiData->mouse_intersects_slider = true;
            return true;
        }
        return false;
    }

    bool intersect_vertical_slider_bounds(game_state* GameState, ui_data* uiData, vec2 mousePos, ui_element* slider, float rootx = 0.0f, float rooty = 0.0f){
        float vis_range = slider->data.slider_float.visible_range;
        float curval = slider->data.slider_float.current_value;
        float scrollMin = (slider->miny); //minx
        float scrollMax = (slider->maxy); //maxx
        float scrollDiff = scrollMax - scrollMin - (slider->height * vis_range); //the range from 0 to 1 we can slide in //width


        float scrollbar_posX    = rootx + slider->posx;
        float scrollbar_posY    = rooty + (scrollMin + (slider->height * vis_range * 0.5f)) + (scrollDiff * curval);// * (0.25f));
        float scrollbar_scaleX  = slider->scale.x;
        float scrollbar_scaleY  = slider->scale.y * vis_range;

        float minx = scrollbar_posX - (scrollbar_scaleX * 0.5f);
        float maxx = scrollbar_posX + (scrollbar_scaleX * 0.5f);
        float miny = scrollbar_posY - (scrollbar_scaleY * 0.5f);
        float maxy = scrollbar_posY + (scrollbar_scaleY * 0.5f);
        if(mousePos.x > minx && mousePos.x < maxx &&
           mousePos.y > miny && mousePos.y < maxy){
            uiData->mouse_intersects_slider = true;
            return true;
        }
        return false;
    }


    bool intersectBounds(game_state* GameState, vec2 mousePos, f32 minx, f32 miny, f32 maxx, f32 maxy){
        if(mousePos.x > minx && mousePos.x < maxx &&
           mousePos.y > miny && mousePos.y < maxy){
            return true;
        }
        return false;
    }

    ui_element* select_window_ui(game_state* GameState, ui_window& window, ui_element& sub_element, vec2 mousePos){
        ui_data* uiData = GameState->uiData;

        float rootx = window.base.posx - (window.base.width  * 0.5f);
        float rooty = window.base.posy - (window.base.height * 0.5f);

        if(intersectElement(GameState, mousePos, sub_element, rootx, rooty)){
            window.selected_bounds = &sub_element;
            window.selected_bounds_rootx = rootx;
            window.selected_bounds_rooty = rooty;

            switch(sub_element.type){
                case ui_element_type_vertical_scrollbar:{
                    ui_element* bar = window.selected_bounds;
                    if(intersect_vertical_slider_bounds(GameState, uiData, mousePos,  bar, rootx, rooty)){
                        window.selected_element = window.selected_bounds;
                        uiData->selected_element = &sub_element;
    
                    }
                }break;
                case ui_element_type_horizontal_scrollbar:{
                    ui_element* bar = window.selected_bounds;
                    if(intersect_horizontal_slider_bounds(GameState, uiData, mousePos,  bar, rootx, rooty)){
                        window.selected_element = window.selected_bounds;
                        uiData->selected_element = &sub_element;
                        
    
                    }
                }break;
                case ui_element_type_editable_text:{
                    window.selected_element = &sub_element;
                    window.selected_element_rootx = rootx;
                    window.selected_element_rooty = rooty;
                    uiData->selected_element =  &sub_element;
                    return &sub_element;
                }break;
                case ui_element_type_bar:{}break;
                default:{
                    window.selected_element = &sub_element;
                    uiData->selected_element = &sub_element;
                }
                
            }
          
        
        }
        return nullptr;
    }
    
    ui_element* select_inventory_ui(game_state* GameState, ui_window& inv, ui_element& sub_element, ui_element* slot_array, int start, int count, vec2 mousePos, u32* entity_slots){
        ui_data* uiData = GameState->uiData;
        
        
        float rootx = inv.base.posx - (inv.base.width  * 0.5f);
        float rooty = inv.base.posy - (inv.base.height * 0.5f);

        if(inv.docked){
            rootx = 0.0f;
            rooty = 0.0f;
        }



        if(intersectElement(GameState, mousePos, sub_element, rootx, rooty)){
            inv.selected_bounds = &sub_element;
            inv.selected_bounds_rootx = rootx;
            inv.selected_bounds_rooty = rooty;
            rootx +=  (sub_element.posx - (sub_element.width * 0.5f));
            rooty +=  (sub_element.posy - (sub_element.height * 0.5f));
            for(int i = start; i < count; i++){
                if(intersectElement(GameState, mousePos, slot_array[i], rootx, rooty)){
                    inv.selected_element = slot_array + i;
                    // if(uiData->selected_element && (uiData->selected_element->type != ui_element_types::ui_element_type_inventory_slot) && (uiData->selected_element->data.inventory_slot.item_type != ui_item_types::ui_item_type_none)){
                        // printf("SELECTING INVENTORY ITEM FROM %s SLOT: %d\n",element_inventory_type_to_str(slot_array[i].data.inventory_slot.inventory_type), i);
                        // printf("ITEM TYPE %s\n",item_type_to_str(slot_array[i].data.inventory_slot.item_type));
                        uiData->selected_element = slot_array + i;
                //   }
                // if(uiData->selected_element)printf("selected element type: %s\n", element_type_to_str(uiData->selected_element->type));

                    inv.selected_element_rootx = rootx;
                    inv.selected_element_rooty = rooty;

                    char temp_label[64];
                    int len = 0;
                    len += handmade_strcpy(temp_label + len, element_type_to_str(slot_array[i].type));
                    temp_label[len] = ' ';
                    len++;
                    if(slot_array[i].type == ui_element_types::ui_element_type_inventory_slot){
                        uiData->hovered_item_slot = entity_slots + i;
                        uiData->hovered_item_slot_type = slot_array[i].data.inventory_slot.inventory_type;

                        len += handmade_strcpy(temp_label + len, element_inventory_type_to_str(slot_array[i].data.inventory_slot.inventory_type));
                        temp_label[len] = ' ';
                        len++;

                        if(entity_slots && entity_slots[i]){
                            uiData->hovered_itemID = entity_slots[i];
                            
                            item_data& item = GameState->entityComponent->inventory_items[entity_slots[i]].data;
                            
                            

                            len += handmade_strcpy(temp_label + len, item_to_str(item));
                            temp_label[len] = ' ';
                            len++;
                        }
    
                    }
                    len += int_to_string(i, temp_label + len, 64);

                    uiData->information_panel_size += handmade_strcpy(uiData->information_panel_text + uiData->information_panel_size, temp_label);
                    // printf("information panel char count: %d\n", uiData->information_panel_size);

                    return slot_array + i;


                }
            }
        }
        return nullptr;
        
    }

    void resizeElement(game_state* GameState, ui_data* uiData, ui_element& element, int windowWidth, int windowHeight){



        element.posx = (float)windowWidth * element.normPosX;
        element.posy = (float)windowHeight * element.normPosY;
        element.windowScale = windowWidth * (element.baseScale);

        element.scalex = element.windowScale * element.ratio;
        element.scaley = element.windowScale;

        element.minx = (-.5f * element.width)  + element.posx;
        element.miny = (-.5f * element.height) + element.posy;
        element.maxx = ( .5f * element.width)  + element.posx;
        element.maxy = ( .5f * element.height) + element.posy;

        element.scale.x = element.maxx - element.minx;
        element.scale.y = element.maxy - element.miny;

    }

    void initElement(game_state* GameState, ui_data* uiData, ui_element& element, int windowWidth, int windowHeight, float normX = 0, float normY = 0, float ratio = 0, float scale = 0,
        vec4 color = {.75f, .5f, .5f, 1.0f}, float width = 100.0f, float height = 100.0f, bool subwindow = false){

        element.baseScale = scale;
        element.ratio = ratio;
        
        element.parentWidth = windowWidth;
        element.parentHeight = windowHeight;

        element.color = color;
        
        element.uvCoords.x = 0.0f;
        element.uvCoords.y = 0.0f;
        element.uvCoords.z = 1.0f;
        element.uvCoords.w = 1.0f;

        element.pixelWidth =  1.0f;
        element.pixelHeight = 1.0f;


        //guard against initing a window offscreen

        element.windowScale = windowWidth * (element.baseScale);

        element.scalex = element.windowScale * element.ratio;
        element.scaley = element.windowScale;

        if(!subwindow){

            // if(normX <= 0.05f || normX >= 0.95f){
            //     printf("WINDOW normX too small! moving to center!\n");
            //     normX = 0.5f;
            // }
            // if(normY <= 0.05f || normY >= 0.95f)
            // {
            //     printf("WINDOW normY too small! moving to center!\n");
            //     normY = 0.5f;
            // }
            
            // if(     width  < MIN_WINDOW_DIMENSIONS){
            //     printf("WINDOW width too small! resizing to %d!\n", MIN_WINDOW_DIMENSIONS);
            //     width  = MIN_WINDOW_DIMENSIONS;
            // }     
            // else if(width  > windowWidth          ){
            //     width  = windowWidth * 0.5f;
            //     printf("WINDOW width too large! resizing to %f!\n", width);
            // }     
            
                
            if(     height < MIN_WINDOW_DIMENSIONS){
                // printf("WINDOW height too small! resizing to %d!\n", MIN_WINDOW_DIMENSIONS);
                height = MIN_WINDOW_DIMENSIONS;
            }     
                
            else if(height > windowHeight         ){
                height = windowHeight* 0.5f;
                // printf("WINDOW height too large! resizing to %f!\n", height);
                
            }     
        }


        float posx = (float)windowWidth  * normX;
        float posy = (float)windowHeight * normY;

        float minx = (-.5f * width)  + posx;
        float maxx = ( .5f * width)  + posx;
        float miny = (-.5f * height) + posy;
        float maxy = ( .5f * height) + posy;
        //check to make sure the intitial position isnt off the screen (within reason)
        if(!subwindow){
            if     ((minx < 0 )||
            (maxx > windowWidth)|| 
            (miny < 0)|| 
            (maxy > windowHeight)){

                // printf("WINDOW OFF SCREEN! minx: %f maxx: %f miny: %f maxy: %f, windowWidth: %f, windowHeight: %f!\n",minx, maxx, miny, maxy, (float)windowWidth, (float)windowHeight);
                if(minx < 0)minx = 0.0f;
                else if(maxx > windowWidth)maxx = windowWidth - 1.0f;
                if(miny < 0)miny = 0.0f;
                else if(maxy > windowHeight)maxy = windowHeight - 1.0f;

                width = maxx - minx;
                height = maxy - miny;

                posx = width  * .5f;
                posy = height * .5f;

                normX = posx / windowWidth;
                normY = posy / windowHeight;

                //second check, fail safe window teleports to center of screen
                if(((minx < 0 )||
                (maxx > windowWidth)|| 
                (miny < 0)|| 
                (maxy > windowHeight))){

                    posx = windowWidth  * .5f;
                    posy = windowHeight * .5f;
                    width = 200.0f;
                    height = 200.0f;
                    minx = (-.5f * width)  + posx;
                    maxx = ( .5f * width)  + posx;
                    miny = (-.5f * height) + posy;
                    maxy = ( .5f * height) + posy;
                    normX = 0.5f;
                    normY = 0.5f;
                    // printf("RESET VALUES! minx: %f maxx: %f miny: %f maxy: %f!\n",minx, maxx, miny, maxy);
                }
             


    }
        }

      
        element.width  = width;
        element.height = height;
        element.scale.x = width;
        element.scale.y = height;
        element.normPosX = normX;
        element.normPosY = normY;
        element.posx = posx;
        element.posy = posy;



        element.hovered = false;
        element.clicked = false;

        resizeElement(GameState, uiData, element, windowWidth, windowHeight);
    }

    void ui_resize_window(game_state* GameState, ui_window& window, float normPosX = 0.0f, float normPosY = 0.0f, float width = 0.0f, float height = 0.0f){

    }

    
    void dock_to_window(game_state* GameState, ui_window& src, ui_window& dest){
        src.docked = true;
        src.docked_to = &dest;
        
        printf("docking %s to %s\n", window_type_to_str(src.type), window_type_to_str(dest.type));

        //traverse the chain of docked children
        ui_window* link = src.docked_child;
        ui_window* parent_link = &src;
        printf("NEW CHAIN:  %s ->", window_type_to_str(dest.type));

        printf("children of %s ->", window_type_to_str(src.type));
        while(link){
            printf(" %s ->", window_type_to_str(link->type));
            link->docked_to = &dest;
            parent_link = link; 
            link = link->docked_child;

        }
        printf(" NULL\n");
        link = dest.docked_child;
        if(parent_link) parent_link->docked_child = link;

        dest.docked_child = &src;
        
        int debugger = 0;
        
    }

    void undock_window(game_state* GameState, ui_window& src, ui_window& dest){
        src.docked = false;
        src.docked_to = nullptr;
        printf("UNdocking %s from %s\n", window_type_to_str(src.type), window_type_to_str(dest.type));

        //go through the chain of docked children, remove src and cross the gap
        ui_window* link = dest.docked_child;

        printf("children of dest %s ->", window_type_to_str(dest.type));

        ui_window* parent_link = &dest;
        while(link){
            if(link == &src){
                printf(" %s IS THE REMOVED NODE ->", window_type_to_str(link->type));
                parent_link->docked_child = link->docked_child;

            }else{
                parent_link = link;
                printf(" %s ->", window_type_to_str(link->type));

            }
            link = link->docked_child;


        }
        printf(" NULL\n");

        src.docked_child = nullptr;//give the next docked window in the chain to the parent/docked to window
    
        int debugger = 0;

    }


    void resize_feedback_window(game_state* GameState, ui_data* uiData, ui_window& window){
        ui_window_data::feedback_data& feedback = window.data.feedback;
    
    vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
        float G = 1.0f;
        float B = 1.0f;
        float scale  = 1.0f;
        float windowWidth  = window.base.width;
        float windowHeight = window.base.height; 
    
    
    
        //horizontal/vertical sliders
        float vertical_sliderWidth  = (MIN_WINDOW_DIMENSIONS);
        float vertical_sliderHeight = (windowHeight); 
        float normPosX = ((windowWidth) - (vertical_sliderWidth * 0.5f)) / windowWidth;
        float normPosY = ((vertical_sliderHeight * 0.5f)) / windowHeight;
        initElement(GameState, uiData, window.vertical_scrollbar, windowWidth, windowHeight, normPosX, normPosY, 1.0f, scale, color, vertical_sliderWidth, vertical_sliderHeight, true);
    
    
    
    }
    
    
    void resize_debug_window(game_state* GameState, ui_data* uiData, ui_window& window){
        
         window.vertical_scrollbar.data.slider_float.max = 0;
         window.horizontal_scrollbar.data.slider_float.max = 0;

        vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
        float scale  = 1.0f;
        int windowWidth  = window.base.width;
        int windowHeight = window.base.height;
        float width  = windowWidth;

        //horizontal/vertical sliders
        float vertical_sliderWidth  = (MIN_WINDOW_DIMENSIONS);
        float vertical_sliderHeight = (windowHeight); 
        float cumHeight = 0.0f;
        float largestWidth = 0.0f;

        for(int i = 0; i < window.data.debug.element_count; i++){
            ui_element* element = window.data.debug.elements + i;
            float normPosX = (((element->maxx - element->minx) * 0.5f)) / windowWidth;//want this window to hug the top left corner 
            float normPosY = ((element->maxy - element->miny * 0.5f)) / windowHeight;
            
            if(element->type != ui_element_type_bar){//dont count bar graph sub boxes as part of the window size
                normPosX = ((element->width * 0.5f)) / windowWidth;//want this window to hug the top left corner 
                normPosY = ((cumHeight)+(element->height * 0.5f)) / windowHeight;
                cumHeight += element->height;        
                if(element->width > largestWidth)largestWidth = element->width;

                initElement(GameState, uiData, *element, windowWidth, windowHeight, normPosX, normPosY, 1.0f, scale, element->color, element->width, element->height, true);
                
                
            }else{
                //the bar graph elements min/max x and y are already set correctly
                //so we should just be able to mouse over them/intersect test them
                //like everything else
                // normPosX = (element->minx + (element->maxx * 0.5f));//want this window to hug the top left corner 
                // float barWidth = element->maxx - element->minx;
                // float barHeight = element->maxy - element->miny;
                // initElement(GameState, uiData, *element, windowWidth, windowHeight, normPosX, normPosY, 1.0f, scale, element->color, barWidth, barHeight, true);

            }

        }
    

        window.vertical_scrollbar.data.slider_float.max = cumHeight;
        window.horizontal_scrollbar.data.slider_float.max = largestWidth;

        if(window.vertical_scrollbar.data.slider_float.max > window.base.height){
            width -= vertical_sliderWidth;
            window.vertical_scrollbar.data.slider_float.visible_range = window.base.height / window.vertical_scrollbar.data.slider_float.max;

        }else{
            window.vertical_scrollbar.data.slider_float.max = window.base.height;
            window.vertical_scrollbar.data.slider_float.visible_range = 1.0f;
        }
        //horizontal scrollbar
        if(window.horizontal_scrollbar.data.slider_float.max > window.base.width){
            window.horizontal_scrollbar.data.slider_float.visible_range = window.base.width / window.horizontal_scrollbar.data.slider_float.max;

        }else{
            window.horizontal_scrollbar.data.slider_float.max = window.base.width;
            window.horizontal_scrollbar.data.slider_float.visible_range = 1.0f;
        }


        //resize vertical scrollbar
        float normPosX = ((windowWidth) - (vertical_sliderWidth * 0.5f)) / windowWidth; 
        float normPosY = ((vertical_sliderHeight * 0.5f)) / windowHeight;
        initElement(GameState, uiData, window.vertical_scrollbar, windowWidth, windowHeight, normPosX, normPosY, 1.0f, scale, color, vertical_sliderWidth, vertical_sliderHeight, true);
    

        //resize horizontal scrollbar
        normPosX = ((windowWidth * 0.5f)) / windowWidth;
        normPosY = ((windowHeight) - (MIN_WINDOW_DIMENSIONS * 0.5f)) / windowHeight; 
        initElement(GameState, uiData, window.horizontal_scrollbar, windowWidth, windowHeight, normPosX, normPosY, 1.0f, scale, color, windowWidth, MIN_WINDOW_DIMENSIONS, true);
    
    }


    void resize_inventory_hotbar_window(game_state* GameState, ui_data* uiData, ui_window& window){
        ui_window_data::inventory_hotbar_data& inv = window.data.inventory_hotbar;        

        vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
        float G = 1.0f;
        float B = 1.0f;

        int windowWidth  = window.base.width;
        int windowHeight = window.base.height;
        float scale  = 1.0f;
        
        scale  = 1.0f;
        float width  = windowWidth;
        float height = windowHeight;

        float normPosX = ((width * 0.5f)) / windowWidth;//want this window to hug the top left corner 
        float normPosY = ((height * 0.5f)) / windowHeight;

        scale  = 1.0f;
        width  = windowWidth;
        height = windowHeight;

        normPosX = ((width * 0.5f)) / windowWidth;//want this window to hug the top left corner 
        normPosY = (windowHeight - (height * 0.5f)) / windowHeight;


        initElement(GameState, uiData, inv.hotbar_bounds, windowWidth, windowHeight, normPosX, normPosY, 1.0f, scale, color, width, height, true);

        width  = width / 10;
        scale  = 1.0f;
        // height = windowHeight * 0.15f;


        float hotbar_slot_width = width;
        float hotbar_slot_height = height;

        for(int i = 0; i < 10; i++){
            normPosX = (((((i+1)* width - (width * 0.5f)))) / windowWidth);//want this window to hug the top left corner 
            normPosY = ((height * .5f)) / windowHeight;
        
            initElement(GameState, uiData, inv.hotbar_slots[i], windowWidth, windowHeight, normPosX, normPosY, 1.0f, scale, color, width, height, true);

        }
    }


    void resize_inventory_window(game_state* GameState, ui_data* uiData, ui_window& window){
        ui_window_data::inventory_data& inv = window.data.inventory;        

        vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};

        int windowWidth  = window.base.width;
        int windowHeight = window.base.height;
        float scale  = 1.0f;
        
        scale  = 1.0f;
        float width  = windowWidth;
        float height = windowHeight * 0.15f; //make them squares

        float normPosX = ((width * 0.5f)) / windowWidth;//want this window to hug the top left corner 
        float normPosY = ((height * 0.5f)) / windowHeight;


        initElement(GameState, uiData, inv.equipment_slot_bounds, windowWidth, windowHeight, normPosX, normPosY, 1.0f, scale, color, width, height, true);

        scale  = 1.0f;
        width  = windowWidth / (float)entity_body_parts::entity_part_count;
        height = windowHeight * 0.15f;

        float equipment_slot_width = width;
        float equipment_slot_height = height;

        for(int i = 0; i < MAX_EQUIPMENT; i++){
            normPosX = (((((i+1)* width))) / windowWidth);//want this window to hug the top left corner 
            normPosY = ((height * .5f)) / windowHeight;
        
            initElement(GameState, uiData, inv.body_slots[i], windowWidth, windowHeight, normPosX, normPosY, 1.0f, scale, color, width, height, true);

        }


        scale  = 1.0f;
        width  = windowWidth;
        height = windowHeight * 0.15f;

        normPosX = ((width * 0.5f)) / windowWidth;//want this window to hug the top left corner 
        normPosY = (windowHeight - (height * 0.5f)) / windowHeight;

        initElement(GameState, uiData, inv.hotbar_bounds, windowWidth, windowHeight, normPosX, normPosY, 1.0f, scale, color, width, height, true);

        scale  = 1.0f;
        width  = width / 10;
        // height = windowHeight * 0.15f;


        float hotbar_slot_width = width;
        float hotbar_slot_height = height;

        for(int i = 0; i < 10; i++){
            normPosX = (((((i+1)* width - (width * 0.5f)))) / windowWidth);//want this window to hug the top left corner 
            normPosY = ((height * .5f)) / windowHeight;
        
            initElement(GameState, uiData, inv.hotbar_slots[i], windowWidth, windowHeight, normPosX, normPosY, 1.0f, scale, color, width, height, true);

        }

        windowWidth  = window.base.width;
        windowHeight = window.base.height;

        //render item portion
        scale  = 1.0f;
        width  = windowWidth;
        height = windowHeight * 0.5f;
        height = (windowHeight * 0.5f)  + hotbar_slot_height;

        normPosX = ((width * 0.5f)) / windowWidth;//want this window to hug the bottom left corner 
        normPosY = (windowHeight - (height * 0.5f)) / windowHeight;
        height = (windowHeight * 0.5f)  - hotbar_slot_height;

        //setup bounds for the item slots
        initElement(GameState, uiData, inv.inventory_slot_bounds, windowWidth, windowHeight, normPosX, normPosY, 1.0f, scale, color, width, height, true);


        scale  = 1.0f;
        width  = windowWidth / 7.99;//if we use 8 then there is a tiny X gap between the slots 
        height = height / (4);
        //setup individual item slots
        for(int i = 0; i < ITEM_SLOT_COUNT; i++){
            //i+1 because inventory slots start at 0
            int x_offset = (i & 7);
            normPosX = (((((x_offset) * width))   + (width * 0.5f))  / windowWidth);//want this window to hug the top left corner 

            int y_offset = (int)((i) / 8);
            normPosY = (   (y_offset) * (height)  + (height * 0.5f)) / windowHeight;
        
            initElement(GameState, uiData, inv.item_slots[i], windowWidth, windowHeight, normPosX, normPosY, 1.0f, scale, color, width, height, true);

        }


        //RESIZE TRINKET SLOTS
        float equipment_offset_height = windowHeight * 0.15f;

        scale  = 1.0f;
        width  = windowWidth;
        //stretch the height out to correctly center
        height = (windowHeight * 0.5f)  + equipment_slot_height;
        normPosX = ((width * 0.5f)) / windowWidth;//want this window to hug the top left corner 
        normPosY = ((height * 0.5f)) / windowHeight;
        
        //subtract from height to shift back into place relative to where we draw under
        height = (windowHeight * 0.5f) - equipment_slot_height;
        //setup bounds for the item slots
        initElement(GameState, uiData, inv.trinket_slot_bounds, windowWidth, windowHeight, normPosX, normPosY, 1.0f, scale, color, width, height, true);


        scale  = 1.0f;
        width  = windowWidth / 7.99;
        height = height / (4);
        //setup individual trinket slots
        for(int i = 0; i < ITEM_SLOT_COUNT; i++){
            //i+1 because trinket slots start at 0
            int x_offset = (i & 7);
            normPosX = (((((x_offset)* width)) + (width * 0.5f)) / windowWidth);//want this window to hug the top left corner 

            int y_offset = (int)((i) / 8);
            normPosY = ((y_offset) * (height) + (height * 0.5f)) / windowHeight;
        
            initElement(GameState, uiData, inv.trinket_slots[i], windowWidth, windowHeight, normPosX, normPosY, 1.0f, scale, color, width, height, true);

        }


        




        
    }


void resize_text_window(game_state* GameState, ui_data* uiData, ui_window& window){
    ui_window_data::book_text_data& book = window.data.book_text;
    
    vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};

    float scale  = 1.0f;
    float windowWidth  = window.base.width;
    float windowHeight = window.base.height; 
    float width  = windowWidth * 0.5f;
    float height = windowHeight; 

    //BORDERS BEGIN

    // top_border

    float vertical_border_width     = MIN_WINDOW_DIMENSIONS;
    float vertical_border_height    = windowHeight;
    float horizontal_border_width   = windowWidth;
    float horizontal_border_height  = MIN_WINDOW_DIMENSIONS;


    float normPosX = ((horizontal_border_width * 0.5f)) / windowWidth;//top left corner 
    float normPosY = ((horizontal_border_height * 0.5f)) / windowHeight;

    initElement(GameState, uiData, book.top_border, windowWidth, windowHeight, normPosX, normPosY, 1.0f, scale, color, horizontal_border_width, horizontal_border_height, true);

    
    

    normPosX = ((horizontal_border_width * 0.5f)) / windowWidth;//bottom left corner 
    normPosY = (windowHeight - (horizontal_border_height * 0.5f)) / windowHeight;

    initElement(GameState, uiData, book.bottom_border, windowWidth, windowHeight, normPosX, normPosY, 1.0f, scale, color, horizontal_border_width, horizontal_border_height, true);

    
    

    normPosX = ((vertical_border_width * 0.5f)) / windowWidth;//top left corner 
    normPosY = ((vertical_border_height * 0.5f)) / windowHeight;

    initElement(GameState, uiData, book.left_border, windowWidth, windowHeight, normPosX, normPosY, 1.0f, scale, color, vertical_border_width, vertical_border_height, true);




    normPosX = (windowWidth - (vertical_border_width * 0.5f)) / windowWidth;//top right corner 
    normPosY = ((vertical_border_height * 0.5f)) / windowHeight;

    initElement(GameState, uiData, book.right_border, windowWidth, windowHeight, normPosX, normPosY, 1.0f, scale, color, vertical_border_width, vertical_border_height, true);

    //BORDERS END


    float horizontal_sliderWidth  = (windowWidth * 0.5f)   - (vertical_border_width);
    float horizontal_sliderHeight = (MIN_WINDOW_DIMENSIONS); 

    //horizontal/vertical sliders
    float vertical_sliderWidth  = (MIN_WINDOW_DIMENSIONS);
    float vertical_sliderHeight = (windowHeight)         - (horizontal_border_height * 2) - horizontal_sliderHeight; 
    //position it immediately to the left of the right page
    normPosX = ((windowWidth * 0.5f) - (vertical_sliderWidth * 0.5f)) / windowWidth;//want this window to hug the top left corner 
    normPosY = (horizontal_border_height + (vertical_sliderHeight * 0.5f)) / windowHeight;
    initElement(GameState, uiData, book.left_vertical_scrollbar, windowWidth, windowHeight, normPosX, normPosY, 1.0f, scale, color, vertical_sliderWidth, vertical_sliderHeight, true);
    // data.book_text.left_vertical_scrollbar.type



    normPosX = (vertical_border_width + (horizontal_sliderWidth * 0.5f)) / windowWidth;//want this window to hug the top left corner 
    normPosY = (windowHeight - horizontal_border_height - (horizontal_sliderHeight * 0.5f)) / windowHeight;
    initElement(GameState, uiData, book.left_horizontal_scrollbar, windowWidth, windowHeight, normPosX, normPosY, 1.0f, scale, color, horizontal_sliderWidth, horizontal_sliderHeight, true);



    width  = (windowWidth * 0.5f)   - (vertical_border_width) - vertical_sliderWidth;
    height = (windowHeight)         - (horizontal_border_height * 2) - horizontal_sliderHeight; 
    
    normPosX = (vertical_border_width + (width * 0.5f)) / windowWidth;//want this window to hug the top left corner 
    normPosY = (horizontal_border_height + (height * 0.5f)) / windowHeight;

    initElement(GameState, uiData, book.left_page, windowWidth, windowHeight, normPosX, normPosY, 1.0f, scale, color, width, height, true);
    
    



    width  = (windowWidth * 0.5f)   - (vertical_border_width);
    height = (windowHeight)         - (horizontal_border_height * 2); 
    
    normPosX = (windowWidth - vertical_border_width - (width * 0.5f)) / windowWidth;
    normPosY = (horizontal_border_height + (height * 0.5f)) / windowHeight;
    initElement(GameState, uiData, book.right_page, windowWidth, windowHeight, normPosX, normPosY, 1.0f, scale, color, width, height, true);


}




    void resize_base_window(game_state* GameState, ui_window& window, float normPosX = 0.0f, float normPosY = 0.0f, float width = 0.0f, float height = 0.0f){
        ui_data* uiData = GameState->uiData;

    vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
        float G = 1.0f;
        float B = 1.0f;

        int windowWidth  = *GameState->window_width;
        int windowHeight = *GameState->window_height;
        float scale  = 1.0f;
        if(!width)  width  =    window.base.width;
        if(!height) height =    window.base.height;
        if(!normPosX)normPosX = window.base.normPosX;
        if(!normPosY)normPosY = window.base.normPosY;

        initElement(GameState, uiData, window.base, windowWidth, windowHeight, normPosX, normPosY, 1.0f, scale, color, width, height, window.docked);


        if(window.base.width < MIN_WINDOW_DIMENSIONS) window.base.width  = MIN_WINDOW_DIMENSIONS;
        if(window.base.height < MIN_WINDOW_DIMENSIONS)window.base.height = MIN_WINDOW_DIMENSIONS; 

        if(!window.docked){//dont bother resizing sub elements if the window is docked
            switch(window.type){
                case window_type_chat            :{}break;
                case window_type_inventory_hotbar:{resize_inventory_hotbar_window(GameState, uiData, window);}break;
                case window_type_inventory       :{resize_inventory_window(GameState, uiData, window);}break;
                case window_type_book_text       :{resize_text_window(GameState, uiData, window);}break;
                case window_type_book_cover      :{}break;
                case window_type_feedback        :{resize_feedback_window(GameState, uiData, window);}break;
                case window_type_stats           :{}break;
                case window_type_debug           :{resize_debug_window(GameState, uiData, window);}break;
            }
        }

        

        if(window.docked_child){
            float width  = 50.0f;
            float height = 50.0f;
            float normPosX = (window.base.maxx  + (width * 0.5f)) /  window.base.parentWidth ;//want this window to hug the bottom right corner 
            float normPosY = (window.base.miny  + (height * 0.5f)) / window.base.parentHeight;
            resize_base_window(GameState, *window.docked_child, normPosX, normPosY, width, height);
        }
        
    }


    void inventory_hotbar_window_hovered(game_state* GameState, ui_data* uiData, ui_window& window, player_input& currInput,  vec2 mouse_pos){
        u32 invIndex = GameState->entityComponent->entityToInventoryMap[GameState->localPlayerEntityIDs[0]];
        InventoryComp& invComp = GameState->entityComponent->InventoryComps[invIndex != NULL_ENTITY ? invIndex : 2046];//give it some kind of valid value just in case 
        
        ui_window_data::inventory_hotbar_data& hotbar = window.data.inventory_hotbar;
        select_inventory_ui(GameState, window, hotbar.hotbar_bounds,  hotbar.hotbar_slots , 0, MAX_ITEMS_PER_HAND, mouse_pos, invComp.left_hand);
        select_inventory_ui(GameState, window, hotbar.hotbar_bounds,  hotbar.hotbar_slots + MAX_ITEMS_PER_HAND, 0, MAX_ITEMS_PER_HAND, mouse_pos, invComp.right_hand);
          
    }

    void window_hovered(game_state* GameState, ui_data* uiData, ui_window& window, player_input& currInput, vec2 mouse_pos){
        uiData->selected_element = &window.base;
    }

    void inventory_window_hovered(game_state* GameState, ui_data* uiData, ui_window& window, player_input& currInput, vec2 mouse_pos){
        u32 invIndex = GameState->entityComponent->entityToInventoryMap[GameState->localPlayerEntityIDs[0]];
        InventoryComp& invComp = GameState->entityComponent->InventoryComps[invIndex != NULL_ENTITY ? invIndex : 2046];//give it some kind of valid value just in case 
        
        ui_window_data::inventory_data& inv = window.data.inventory;
        select_inventory_ui(GameState, window, inv.equipment_slot_bounds,  inv.body_slots , 0, MAX_EQUIPMENT, mouse_pos, invComp.equipment);
        if(!window.selected_element) {//item slots
            select_inventory_ui(GameState, window, inv.inventory_slot_bounds,  inv.item_slots , 0, ITEM_SLOT_COUNT, mouse_pos, invComp.items);
        }
        if(!window.selected_element) {//trinket slots
            select_inventory_ui(GameState, window, inv.trinket_slot_bounds,  inv.trinket_slots , 0, ITEM_SLOT_COUNT, mouse_pos, invComp.trinkets);
        }
        if(!window.selected_element) {//trinket slots
            select_inventory_ui(GameState, window, inv.hotbar_bounds,  inv.hotbar_slots , 0, MAX_ITEMS_PER_HAND, mouse_pos, invComp.left_hand);
            select_inventory_ui(GameState, window, inv.hotbar_bounds,  inv.hotbar_slots + MAX_ITEMS_PER_HAND, 0, MAX_ITEMS_PER_HAND, mouse_pos, invComp.right_hand);
        }
    }
    


    void book_text_window_hovered(game_state* GameState, ui_data* uiData, ui_window& window,player_input& currInput, vec2 mouse_pos){
        select_window_ui(GameState, window, window.data.book_text.left_page, mouse_pos);
        if(!window.selected_element) {
            select_window_ui(GameState, window,  window.data.book_text.right_page, mouse_pos);
        }
        if(!window.selected_element) {
            select_window_ui(GameState, window,  window.data.book_text.left_horizontal_scrollbar, mouse_pos);
        }
        if(!window.selected_element) {
            select_window_ui(GameState, window,  window.data.book_text.left_vertical_scrollbar, mouse_pos);
        }
        if(!window.selected_element)select_window_ui(GameState, window,  window.data.book_text.top_border, mouse_pos);
        if(!window.selected_element)select_window_ui(GameState, window,  window.data.book_text.bottom_border, mouse_pos);
        if(!window.selected_element)select_window_ui(GameState, window,  window.data.book_text.left_border, mouse_pos);
        if(!window.selected_element)select_window_ui(GameState, window,  window.data.book_text.right_border, mouse_pos);
    }

    void feedback_window_hovered(game_state* GameState, ui_data* uiData,        ui_window& window, player_input& currInput, vec2 mouse_pos){
        uiData->selected_element = &window.base;

        if(window.vertical_scrollbar.data.slider_float.max > window.base.height){
            select_window_ui(GameState, window, window.vertical_scrollbar, mouse_pos);
        }
    }

    
    void book_cover_window_hovered(game_state* GameState, ui_data* uiData,  ui_window& window, player_input& currInput,  vec2 mouse_pos){}
    void chat_window_hovered(game_state* GameState, ui_data* uiData,        ui_window& window, player_input& currInput,  vec2 mouse_pos){}
    void debug_window_hovered(game_state* GameState, ui_data* uiData,      ui_window& window, player_input& currInput,  vec2 mouse_pos){
        
        float rootx = window.base.posx - (window.base.width  * 0.5f);
        float rooty = window.base.posy - (window.base.height * 0.5f);
        uiData->selected_element = &window.base;

        ui_element* bar = &window.vertical_scrollbar;

        float scroll_offset = (bar->data.slider_float.max - window.base.height) * bar->data.slider_float.current_value;
        float min_visible_y = window.base.miny;
        float max_visible_y = window.base.maxy;
        
        bar = &window.horizontal_scrollbar;
        float scroll_offsetx = (bar->data.slider_float.max - window.base.width) * bar->data.slider_float.current_value;
        float min_visible_x = window.base.minx;
        float max_visible_x = window.base.maxx;
                        

        float windowRootx = rootx;
        float windowRooty = rooty;

        rooty -= scroll_offset;
        rootx -= scroll_offsetx;

        vec2 pos = {};
        for(int i = 0; i < window.data.debug.element_count; i++){
            ui_element* sub_element = window.data.debug.elements + i;
            
            //TODO: select actual bar graph sub elements, they need to be placed correctly
            //just so we dont highlight the incorrectly drawing graph elements...need to handle this better
            if(sub_element->type == ui_element_type_bar)continue;

            pos.x = sub_element->posx + rootx; pos.y = sub_element->posy + rooty;
            float halfHeight = sub_element->scale.y * 0.5;

            if (pos.y >= min_visible_y - halfHeight && pos.y <= max_visible_y + halfHeight) {


                if(intersectElement(GameState, mouse_pos, *sub_element, windowRootx, rooty)){
                    window.selected_bounds   = sub_element;
                    window.selected_element  = sub_element;
                    uiData->selected_element = sub_element;
                    // if(uiData->selected_element)printf("selected element type: %s\n", element_type_to_str(uiData->selected_element->type));
    
                    uiData->debug_element_selected = true;
                    uiData->debug_scrolly_offset = scroll_offset;
                    uiData->debug_scrollx_offset = scroll_offsetx;
                    uiData->selected_debug_element_memory = *sub_element;
                    uiData->selected_debug_element_window_base_memory = window.base;
                    // ui_element* selected_debug_element_parent;
                    uiData->debug_root = {rootx - scroll_offsetx, rooty - scroll_offset};
                    uiData->debug_scissor = {windowRootx, windowRooty, window.base.width, window.base.height};
                    // uiData->debug_scissor = {};

                    switch(sub_element->type){
                        case ui_element_type_bar_graph:{
                            vec2 scale = {};
                            vec2 pos = {};
                            float currOffsetX = rootx;
                            //loop over all the bar graphs sub element bars
                            for(int j = 0; j < sub_element->data.barGraph.elementCount; j++){
                                ui_element* bar = sub_element->data.barGraph.start + j;
                                scale.x = (bar->maxx - bar->minx);
                                scale.y = bar->maxy - bar->miny;
                                currOffsetX = bar->minx + rootx + (scale.x * 0.5f); 
                                
                                pos.x = currOffsetX ; pos.y = sub_element->maxy + rooty - (scale.y * 0.5);
                                
                                if((mouse_pos.x >= pos.x - (scale.x * 0.5)) && mouse_pos.x <= pos.x + (scale.x * 0.5)){
                                    // printf("selected bar graph bar %d!\n", i);
                                    uiData->selected_bar_element = bar;
                                    uiData->selected_bargraph_element = sub_element;
                                    uiData->selected_element = bar;
                                    uiData->previously_selected_element = bar;
                                    uiData->previously_selected_parent_element = sub_element;
                                    uiData->previously_selected_element_index = j;
                                    break;
                                }
                            }
                            // const char* bargraphToolTip = "BAR GRAPH!";

                            // uiData->tooltipText = bargraphToolTip;
                            
                        }break;
                        case ui_element_type_horizontal_slider_float:{
                            intersect_horizontal_debug_slider_bounds(GameState, uiData, mouse_pos, sub_element, rootx, rooty);
                        }break;
                        case ui_element_type_vertical_slider_float:{
                            intersect_vertical_debug_slider_bounds(GameState, uiData, mouse_pos, sub_element, rootx, rooty);
                        }break;
                        case ui_element_type_inline:{
                            uiData->inline_element_selected = false;
                            sub_element->data.inlineElement.selected_element = nullptr;
                            vec2 scale = {};
                            vec2 pos = {};
                            float currOffsetX = rootx;
                            float currOffsetY = rooty;
                            //loop over all the bar graphs sub element bars
                            for(int i = 0; i < sub_element->data.inlineElement.elementCount; i++){
                                ui_element* sub = sub_element->data.inlineElement.start + i;
                                scale.x = (sub->maxx - sub->minx);
                                scale.y = sub->maxy - sub->miny;
                                currOffsetX = sub->minx + rootx + (scale.x * 0.5f); 
                                currOffsetY = sub_element->miny + rooty + (scale.y * 0.5f) + sub->miny; 
                                
                                pos.x = currOffsetX ;
                                pos.y = currOffsetY;
                                if(((mouse_pos.x >= pos.x - (scale.x * 0.5)) && mouse_pos.x <= pos.x + (scale.x * 0.5)) &&
                                   ((mouse_pos.y >= pos.y - (scale.y * 0.5)) && mouse_pos.y <= pos.y + (scale.y * 0.5))){
                                    // printf("selected inline element %d!\n", i);
                                    switch(sub->type){
                                        case ui_element_type_horizontal_slider_float:{
                                            intersect_horizontal_debug_slider_bounds(GameState, uiData, mouse_pos, sub, rootx, rooty);
                                        }break;
                                        case ui_element_type_vertical_slider_float:{
                                            sub->posy = sub_element->miny;
                                            intersect_vertical_debug_slider_bounds(GameState, uiData, mouse_pos, sub, rootx, rooty);
                                        }break;
                                    }
                                    sub_element->data.inlineElement.selected_element = sub;
                                    uiData->selected_inline_element = &(sub_element->data.inlineElement.selected_element);
                                    uiData->selected_element = sub;
                                    uiData->selected_inline_element_hash = sub->hashEntry;
                                    uiData->inline_element_selected = true;
                                    uiData->selected_inline_element_parent_hash = sub->parentHash;
                                    // printf("selected parent element:%p\n",sub_element);
                                    ui_element* bar = &window.vertical_scrollbar;


                                    float scroll_offset = (bar->data.slider_float.max - window.base.height) * bar->data.slider_float.current_value;
                                    float min_visible_y = window.base.miny;
                                    float max_visible_y = window.base.maxy;
                                    
                                    uiData->inline_root = {window.base.posx - (window.base.width  * 0.5f) - scroll_offsetx, window.base.posy - (window.base.height  * 0.5f) - scroll_offset};
                                    uiData->inline_scissor = {windowRootx, windowRooty, window.base.width, window.base.height};
                                    break;
                                }
                            }
                        }break;
                    }

                }
            }

        }
        //if the scrollbars should be interactlable, check if the mouse intersects them
        if(window.vertical_scrollbar.data.slider_float.max > window.base.height){
            select_window_ui(GameState, window, window.vertical_scrollbar, mouse_pos);
        }
        if(window.horizontal_scrollbar.data.slider_float.max > window.base.width){
            select_window_ui(GameState, window, window.horizontal_scrollbar, mouse_pos);
        }
    }

    
    void ui_input_example(game_state* GameState, ui_data* uiData, player_input& currInput,  vec2 mouse_pos){
        ui_window* windows = uiData->windows;
        
        for(int i = 1; i < MAX_WINDOWS; i++){
        }
        for(int i = 1; i < MAX_WINDOWS; i++){
        }

    }

    void ui_mouse_released(game_state* GameState, ui_data* uiData, player_input& currInput,  vec2 mouse_pos){
        ui_window* windows = uiData->windows;
        
        if(uiData->selected_window)uiData->selected_window->base.clicked = false;

        

        uiData->selected_element = nullptr;
        // uiData->hovered_element = nullptr;
        uiData->dragging_item = nullptr;
        uiData->dragging_slider = nullptr;
        
        for(int i = 1; i < MAX_WINDOWS; i++){

            windows[i].base.clicked   = false;
            windows[i].resize_widget_selected = false;
        }


    }

    
    void drag_ui_element(game_state* GameState, ui_element& element, vec2 mouse_delta, ui_window* window){

        bool moved = false;
        bool docked = window && window->docked;
        if(docked || ((element.minx + mouse_delta.x) >= 0) &&
            ((element.maxx + mouse_delta.x) <= *GameState->window_width )){
            element.posx += mouse_delta.x;
            element.minx = element.posx - element.width  * 0.5f;
            element.maxx = element.posx + element.width  * 0.5f;
            element.normPosX = element.posx / element.parentWidth;

            moved = true;
        }else mouse_delta.x = 0.0f;
        
        if(docked ||((element.miny + mouse_delta.y) >= 0) &&
            ((element.maxy + mouse_delta.y) <= *GameState->window_height)){
            element.posy += mouse_delta.y;
            element.miny = element.posy - element.height * 0.5f;
            element.maxy = element.posy + element.height * 0.5f;
            element.normPosY = element.posy / element.parentHeight;
            moved = true;
        }else mouse_delta.y = 0.0f;//nullify the movement for any children

        //if a main window is being dragged, we also need to move its docked children if they exist
        if(moved && window && window->docked_child){
            drag_ui_element(GameState, window->docked_child->base, mouse_delta, window->docked_child);
        }
}

    void ui_mouse_moved(game_state* GameState, ui_data* uiData, player_input& currInput, vec2 mouse_pos, vec2 mouse_delta, bool clicked){
        ui_window* windows = uiData->windows;
        
        uiData->hovered_element = nullptr;
        uiData->hovered_window = nullptr;
        uiData->selected_element = nullptr;
        uiData->mouse_intersects_slider = false;

        //selected element type clearing
        uiData->inline_element_selected = false;
        uiData->debug_element_selected = false;

        uiData->selected_bar_element = nullptr;
        uiData->selected_bargraph_element = nullptr;
        //to track which if any inline elements has been selected recently, and to clear it when moving the mouse again
        if(uiData->selected_inline_element)(*uiData->selected_inline_element) = nullptr;
        
        for(int i = 1; i < MAX_WINDOWS; i++){
            
            windows[i].base.hovered   = false;

            windows[i].selected_element   = nullptr;

            windows[i].selected_bounds    = nullptr;
        }

        for(int i = 1; i < MAX_WINDOWS; i++){
            if(intersectElement(GameState,  mouse_pos, windows[i].base)  && windows[i].base.active && windows[i].interactable){
                windows[i].base.hovered = true;
                uiData->mouse_intersects_element = true;

                if(!uiData->hovered_window)uiData->hovered_window = windows + i;
                
                if(!uiData->hovered_element && !windows[i].docked){
                    //information panel logic
                    //if multiple elements are hovered, this information will update to the first one in the stack hovered
                    //doesnt display information for docked panels
                    vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
                    float G = 1.0f;
                    float B = 1.0f;
                    int windowWidth  = *GameState->window_width;
                    int windowHeight = *GameState->window_height;
                    float scale  = 1.0f;
                    float width  =    windows[i].base.width;//same width and height of the hovered panel
                    float height =    windows[i].base.height;
                    float normPosX = (windows[i].base.minx  - (width *  0.5f))  /  windows[i].base.parentWidth ;//want this window to hug the bottom right corner 
                    float normPosY = (windows[i].base.miny  + (height * 0.5f)) / windows[i].base.parentHeight;
                    initElement(GameState, uiData, uiData->information_panel, windowWidth, windowHeight, normPosX, normPosY, 1.0f, scale, color, width, height, true);
                    const char* window_label = window_type_to_str(windows[i].type);
                    int len = handmade_strcpy(uiData->information_panel_text, "PANEL INFO: \n"); 
                    handmade_strcpy(uiData->information_panel_text + len, window_label);
                    uiData->information_panel_size = handmade_strlen(uiData->information_panel_text);

                    uiData->hovered_element = &windows[i].base;
                
                }



                


                if(!windows[i].docked){
                    switch (windows[i].type){
                        case window_type_chat               :{window_hovered(GameState, uiData, windows[i], currInput, mouse_pos);}break;
                        case window_type_inventory          :{inventory_window_hovered(GameState, uiData, windows[i], currInput, mouse_pos);}break;
                        case window_type_inventory_hotbar   :{inventory_hotbar_window_hovered(GameState, uiData, windows[i], currInput, mouse_pos);}break;
                        case window_type_book_text          :{book_text_window_hovered(GameState, uiData, windows[i], currInput, mouse_pos);}break;
                        case window_type_book_cover         :{window_hovered(GameState, uiData, windows[i], currInput, mouse_pos);}break;
                        case window_type_stats              :{window_hovered(GameState, uiData, windows[i], currInput, mouse_pos);}break;
                        case window_type_feedback           :{feedback_window_hovered(GameState, uiData, windows[i], currInput, mouse_pos);}break;
                        case window_type_debug              :{
                            debug_window_hovered(GameState, uiData, windows[i], currInput, mouse_pos);
                        }break;
                    }

                }else{
                    //need to assign something as the select element, even if its docked, just so we can still drag/interact with it
                    uiData->selected_element = &windows[i].base;

                }
            }
        }

        
        // if(uiData->selected_element){
        //     uiData->tooltipTextLen = handmade_strcpy(uiData->tooltipText, element_type_to_str(uiData->selected_element->type));
        // }
          
            
            // uiData->inventory_window.base.normPosX * GameState->sharedState->width;
    }



    void ui_window_click(game_state* GameState, ui_data* uiData, player_input& currInput,  vec2 mouse_pos){
        ui_window* windows = uiData->windows;
        for(int i = 1; i < MAX_WINDOWS; i++){
            //clear all hovered buttons
            windows[i].base.hovered = false;

            windows[i].base.clicked = false;
        }
        
        for(int i = 1; i < MAX_WINDOWS; i++){
            if(intersectElement(GameState, mouse_pos, windows[i].base) && windows[i].base.active && windows[i].interactable){
                uiData->mouse_intersects_element = true;
                windows[i].base.clicked = true;
                // uiData->selected_element = &windows[i].base;
                uiData->selected_window =   windows + i;

                if(windows[i].docked){
                    //special docking logic to select that window

                }else{
                    f32 minx = (windows[i].base.posx) + (windows[i].base.width * 0.5f) - MIN_WINDOW_DIMENSIONS;
                    f32 miny = (windows[i].base.posy) + (windows[i].base.height* 0.5f) - MIN_WINDOW_DIMENSIONS;
                    f32 maxx = (windows[i].base.posx) + (windows[i].base.width * 0.5f);
                    f32 maxy = (windows[i].base.posy) + (windows[i].base.height* 0.5f);
                    if(intersectBounds(GameState, mouse_pos, minx, miny, maxx, maxy)){
                        windows[i].resize_widget_selected = true;
                        break;
                    }
    
                }
                // break;
            }

        }
    }

    //used exclusively to consume mouse input if a window was clicked by the rightmouse button
    void ui_mouse_window_intersect_test(game_state* GameState, ui_data* uiData, player_input& currInput,  vec2 mouse_pos){
        ui_window* windows = uiData->windows;
        
        for(int i = 1; i < MAX_WINDOWS; i++){
            if(intersectElement(GameState, mouse_pos, windows[i].base) && windows[i].base.active){
                uiData->mouse_intersects_element = true;
                break;
            }

        }
    }

    
    void ui_resizeElements(game_state* GameState){
        ui_data* uiData = GameState->uiData;

        ui_window* windows = uiData->windows;

        for(int i = 1; i < MAX_WINDOWS; i++){
            float width     = 0.0f;
            float height    = 0.0f;
            float normPosX  = 0.0f;
            float normPosY  = 0.0f;
            //default values are 0 so that the windows will use their existing data
            switch(windows[i].type){
                case window_type_chat             :{}break;
                case window_type_inventory_hotbar :{}break;
                case window_type_inventory        :{}break;
                case window_type_book_text        :{}break;
                case window_type_book_cover       :{}break;
                case window_type_stats            :{}break;
                case window_type_debug            :{}break;
                default: break;
            }
          
            resize_base_window(GameState, windows[i], normPosX, normPosY, width, height);
            
        }
        int windowWidth  = *GameState->window_width;
        int windowHeight = *GameState->window_height;

    
    }




    void ui_start(game_state* GameState){
        ui_data* uiData = GameState->uiData;

        flush_table(&uiData->label_table);

        ui_window* windows = uiData->windows;

        int windowWidth  = *GameState->window_width;
        int windowHeight = *GameState->window_height;
        
        uiData->last_frame_debug_window_count = 0;

    vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
        float G = 1.0f;
        float B = 1.0f;

        windows[window_type_chat                ].type = window_type_chat               ;
        windows[window_type_inventory_hotbar    ].type = window_type_inventory_hotbar   ;
        windows[window_type_inventory_other     ].type = window_type_inventory_other    ;
        windows[window_type_inventory_storage   ].type = window_type_inventory_storage  ;
        windows[window_type_inventory           ].type = window_type_inventory          ;
        windows[window_type_book_text           ].type = window_type_book_text          ;
        windows[window_type_book_cover          ].type = window_type_book_cover         ;
        windows[window_type_stats               ].type = window_type_stats              ;
        windows[window_type_feedback            ].type = window_type_feedback           ;
        windows[window_type_debug               ].type = window_type_debug              ;


        


        for(int i = 1; i < MAX_WINDOWS; i++){
            float width     = 0.0f;
            float height    = 0.0f;
            float normPosX  = 0.0f;
            float normPosY  = 0.0f;
            windows[i].interactable = true;//mostly used for debug windows/other windows I just want to display
            //default values are 0 so that the windows will use their existing data
            switch(windows[i].type){
                case window_type_chat      :{
                    width  = 50.0f;
                    height = 50.0f;
                    normPosX = 0.075f; 
                    normPosY = 0.8f;
                    windows[i].base.active = false;
                }break;
                case window_type_inventory_hotbar :{
                    width  = 400.0f;
                    height = 100.0f;
                    normPosX = 0.5f;
                    normPosY = (windowHeight - (height * 0.5f)) / windowHeight;
                    windows[i].base.active = false;
                }break;
                case window_type_feedback :{
                    width  = 400.0f;
                    height = 300.0f;
                    normPosX = 0.25f;
                    normPosY = (windowHeight - (height * 0.5f)) / windowHeight;
                    windows[i].base.active = false;
                }break;
                case window_type_inventory :{
                    width  = 300.0f;
                    height = 400.0f;
                    normPosX = (windowWidth  - (width * 0.5f)) / windowWidth;
                    normPosY = (windowHeight - (height * 0.5f)) / windowHeight;
                    windows[i].base.active = false;
                }break;
                case window_type_inventory_other      :{
                    width  = 50.0f;
                    height = 50.0f;
                    normPosX = 0.175f; 
                    normPosY = 0.8f;
                    windows[i].base.active = false;

                }break;
                case window_type_inventory_storage      :{
                    width  = 50.0f;
                    height = 50.0f;
                    normPosX = 0.275f; 
                    normPosY = 0.8f;
                    windows[i].base.active = false;

                }break;
                case window_type_book_text :{
                    width  = 1000.0f;
                    height = 600.0f;
                    normPosX = ((width * 0.5f)) / windowWidth;
                    // normPosY = (windowHeight - (height * 0.5f)) / windowHeight;
                    normPosY = ((height * 0.5f)) / windowHeight;
                    windows[i].base.active = false;
                }break;
                case window_type_book_cover:{
                    width  = 50.0f;
                    height = 50.0f;
                    normPosX = ((width * 0.5f)) / windowWidth;
                    normPosY = ((height * 0.5f)) / windowHeight;
                    windows[i].base.active = false;

                }break;
                case window_type_stats     :{
                    width  = 50.0f;
                    height = 50.0f;
                    normPosX = ((width * 0.5f)) / windowWidth;//want this window to hug the bottom right corner 
                    normPosY = ((windowHeight - (height * 0.5f)) / windowHeight);
                    windows[i].base.active = false;

                }break;
                case window_type_debug     :{
                    width  = 50.0f;
                    height = 50.0f;
                    normPosX = ((width * 0.5f)) / windowWidth;//want this window to hug the bottom right corner 
                    normPosY = ((windowHeight - (height * 0.5f)) / windowHeight);
                    windows[i].base.active = false;


                }break;
                default: {
                    width  = 50.0f;
                    height = 50.0f;
                    normPosX = 0.8f; 
                    normPosY = 0.8f;
                    windows[i].base.active = false;

                }break;
            }

            //init each window's built in vertical scrollbar
            ui_element* bar = &windows[i].vertical_scrollbar;
            bar->type = ui_element_types::ui_element_type_vertical_scrollbar;
            init_slider(bar);
            windows[i].vertical_scroll_height = &bar->height;//dimension to send to the scrollbar adjust function
          
            //horizontal scrollbar setup
            bar = &windows[i].horizontal_scrollbar;
            bar->type = ui_element_types::ui_element_type_horizontal_scrollbar;
            init_slider(bar);
            windows[i].horizontal_scroll_width = &bar->width;//dimension to send to the scrollbar adjust function
          

            resize_base_window(GameState, windows[i], normPosX, normPosY, width, height);
            windows[i].dockable = true;

            windows[i].base.type = ui_element_types::ui_element_type_window_base;
            
        }

        windows[window_type_inventory_hotbar    ].dockable = false   ;

        ui_window& hotbar = windows[ui_window_types::window_type_inventory_hotbar];
        ui_element* slots = hotbar.data.inventory_hotbar.hotbar_slots;    
        for(int i = 0; i < MAX_HOTBAR_SLOTS; i++){
            slots[i].type = ui_element_types::ui_element_type_inventory_slot;
            slots[i].data.inventory_slot.inventory_type = ui_element_inventory_types::ui_element_inventory_type_hotbar_slot;
            slots[i].data.inventory_slot.item_type = ui_item_types::ui_item_type_none;
        }

        //init inventory window slots
        ui_window& inv = windows[ui_window_types::window_type_inventory];
        // inv.data.inventory.item_slots[0].type = ui_element_types::ui_element_type_inventory_item_slot;
        //EQUIPMENT/BODY SLOTS
        slots = inv.data.inventory.body_slots;    
        for(int i = 0; i < MAX_EQUIPMENT; i++){
            slots[i].type = ui_element_types::ui_element_type_inventory_slot;
            slots[i].data.inventory_slot.inventory_type = ui_element_inventory_types::ui_element_inventory_type_equipment_slot;
            slots[i].data.inventory_slot.item_type = ui_item_types::ui_item_type_none;
        
        }

        //TRINKET SLOTS
        slots = inv.data.inventory.trinket_slots;    
        for(int i = 0; i < ITEM_SLOT_COUNT; i++){
            slots[i].type = ui_element_types::ui_element_type_inventory_slot;
            slots[i].data.inventory_slot.inventory_type = ui_element_inventory_types::ui_element_inventory_type_trinket_slot;
            slots[i].data.inventory_slot.item_type = ui_item_types::ui_item_type_none;

        }
        
        //ITEM SLOTS
        slots = inv.data.inventory.item_slots;    
        for(int i = 0; i < ITEM_SLOT_COUNT; i++){
            slots[i].type = ui_element_types::ui_element_type_inventory_slot;
            slots[i].data.inventory_slot.inventory_type = ui_element_inventory_types::ui_element_inventory_type_item_slot;
            slots[i].data.inventory_slot.item_type = ui_item_types::ui_item_type_none;
            
        }

        //HOTBAR SLOTS
        slots = inv.data.inventory.hotbar_slots;    
        for(int i = 0; i < MAX_HOTBAR_SLOTS; i++){
            slots[i].type = ui_element_types::ui_element_type_inventory_slot;
            slots[i].data.inventory_slot.inventory_type = ui_element_inventory_types::ui_element_inventory_type_hotbar_slot;
            slots[i].data.inventory_slot.item_type = ui_item_types::ui_item_type_none;
        }
        
        //this was a test to move around the inventory item
        // windows[ui_window_types::window_type_inventory].data.inventory.item_slots[0].data.inventory_slot.item_type = ui_item_types::ui_item_type_sword;
        


        //set book text window widget types
        
        ui_window& book = windows[ui_window_types::window_type_book_text];
    
        book.data.book_text.top_border.type = ui_element_types::ui_element_type_window_base;
        book.data.book_text.bottom_border.type = ui_element_types::ui_element_type_window_base;
        book.data.book_text.left_border.type = ui_element_types::ui_element_type_window_base;
        book.data.book_text.right_border.type = ui_element_types::ui_element_type_window_base;

        ui_element* bar = &windows[ui_window_types::window_type_book_text].data.book_text.left_vertical_scrollbar;
        bar->type = ui_element_types::ui_element_type_vertical_scrollbar;
        init_slider(bar);
        bar->data.slider_float.visible_range = 0.5f;

        windows[ui_window_types::window_type_book_text].vertical_scroll_height = &bar->height;//dimension to send to the scrollbar adjust function


        bar = &windows[ui_window_types::window_type_book_text].data.book_text.left_horizontal_scrollbar;
        bar->type = ui_element_types::ui_element_type_horizontal_scrollbar;
        init_slider(bar);
        bar->data.slider_float.visible_range = 0.5f;

        windows[ui_window_types::window_type_book_text].data.book_text.right_page.type = ui_element_types::ui_element_type_text;
        windows[ui_window_types::window_type_book_text].data.book_text.left_page.type = ui_element_types::ui_element_type_editable_text;
        
        //book text window text input init
        TextInputState* text_input      = &windows[ui_window_types::window_type_book_text      ].data.book_text.text_input;
        text_input->buffer              =  windows[ui_window_types::window_type_book_text      ].data.book_text.chars;
        text_input->cursor_positions    =  windows[ui_window_types::window_type_book_text      ].data.book_text.cursor_positions;
        text_input->buffer_size = 128;
        text_input->length = 0;
        text_input->cursor_pos = 0;
        text_input->cursor_num_positions = 0;
        text_input->gapStart = 0;
        text_input->gapEnd   = text_input->buffer_size - 1;
        text_input->newLines = windows[ui_window_types::window_type_book_text      ].data.book_text.newLines;
        text_input->lineCount = 0;
        text_input->history         =  windows[ui_window_types::window_type_book_text      ].data.book_text.text_input_history;
        text_input->historySize     =  windows[ui_window_types::window_type_book_text      ].data.book_text.text_input_history_size;
        text_input->historyIndex    =  windows[ui_window_types::window_type_book_text      ].data.book_text.text_input_history_index;
        text_input->historyMax      =  windows[ui_window_types::window_type_book_text      ].data.book_text.text_input_history_max;

        
        //chat window text input init
        TextInputState* chat_input      = &windows[window_type_chat      ].data.chat.text_input;
        chat_input->buffer              =  windows[window_type_chat      ].data.chat.input_buffer;
        chat_input->cursor_positions    =  windows[window_type_chat      ].data.chat.cursor_positions;
        chat_input->newLines            =  windows[window_type_chat      ].data.chat.newLines;
        chat_input->buffer_size = MAX_MSG_LEN;
        chat_input->length = 0;
        chat_input->cursor_pos = 0;
        chat_input->cursor_num_positions = 0;
        

        uiData->selected_element = nullptr;
        uiData->selected_window = nullptr;
        uiData->hovered_element = nullptr;
        uiData->dragging_item = nullptr;
        uiData->dragging_slider = nullptr;

    
    }
    
    void ui_update(game_state* GameState){
        ui_data* uiData = GameState->uiData;


    }   
    


    void enable_network_buttons(game_state* GameState){
        ui_data* uiData = GameState->uiData;
        // enable_element(uiData->listen_server_button.base);
        // enable_element(uiData->client_button.base);
    }

    void disable_network_buttons(game_state* GameState){
        ui_data* uiData = GameState->uiData;
        // disable_element(uiData->listen_server_button.base);
        // disable_element(uiData->client_button.base);
    }


    void ui_process_input(game_state* GameState, player_input& currInput){
        ui_data* uiData = GameState->uiData;
        
        uiData->mouse_intersects_element = false;

        if(WAS_PRESSED(currInput, function.keyTAB, InputTypes::input_key_tab)){
            GameState->uiData->display_UI_window = !GameState->uiData->display_UI_window ;

            if(GameState->uiData->display_UI_window){
                uiData->freezeTooltipWindow = false;
                //enable/disable windows
                uiData->windows[window_type_chat                ].base.active = false ;
                uiData->windows[window_type_inventory_hotbar    ].base.active = false ;
                uiData->windows[window_type_inventory_other     ].base.active = false;
                uiData->windows[window_type_inventory_storage   ].base.active = false;
                uiData->windows[window_type_inventory           ].base.active = false ;
                uiData->windows[window_type_book_text           ].base.active = false ;
                uiData->windows[window_type_book_cover          ].base.active = false;
                uiData->windows[window_type_stats               ].base.active = false;
                uiData->windows[window_type_feedback            ].base.active = false ;
                uiData->windows[window_type_debug               ].base.active = false;
                for(int i = 0; i < uiData->last_frame_debug_window_count; i++){
                    uiData->windows[ui_window_types::window_type_count + i].base.active = true;
                }
            }else{
                uiData->selected_window = nullptr;
                uiData->selected_element = nullptr;
                uiData->windows[window_type_chat                ].base.active = false   ;
                uiData->windows[window_type_inventory_hotbar    ].base.active = false    ;
                uiData->windows[window_type_inventory_other     ].base.active = false   ;
                uiData->windows[window_type_inventory_storage   ].base.active = false   ;
                uiData->windows[window_type_inventory           ].base.active = false   ;
                uiData->windows[window_type_book_text           ].base.active = false   ;
                uiData->windows[window_type_book_cover          ].base.active = false   ;
                uiData->windows[window_type_stats               ].base.active = false   ;
                uiData->windows[window_type_feedback            ].base.active = false   ;
                uiData->windows[window_type_debug               ].base.active = false   ;
                for(int i = 0; i < uiData->last_frame_debug_window_count; i++){
                    uiData->windows[ui_window_types::window_type_count + i].base.active = false;
                }
                uiData->inline_element_selected = false;//disable drawing selected elements

                
                switch (uiData->current_state){
                            
                    case ui_state_drag_item         :   {
                            // uiData->dragging_item->posy = mouse_pos.y;
                            // uiData->dragging_item->posx = mouse_pos.x;
                            // uiData->dragging_item->minx = mouse_pos.x + (uiData->dragging_item->width * 0.5f);
                            // uiData->dragging_item->maxx = mouse_pos.x + (uiData->dragging_item->width * 0.5f);
                            // uiData->dragging_item->miny = mouse_pos.y + (uiData->dragging_item->height * 0.5f);
                            // uiData->dragging_item->maxy = mouse_pos.y + (uiData->dragging_item->height * 0.5f);
                            // uiData->dragging_item->normPosY = mouse_pos.y / GameState->sharedState->height;
                            // uiData->dragging_item->normPosX = mouse_pos.x / GameState->sharedState->width;
                            printf("WINDOWS CLOSED WHILE DRAGGING ITEM!\n");
                    }break;
                  
                    case ui_state_chat_input        ://falls through to default
                    case ui_state_resize_window     :   
                    case ui_state_drag_window       :   
                    case ui_state_drag_slider       :  
                    case ui_state_text_editor_input :
                    case ui_state_none              :
                    default: {
                        transition_ui_state(uiData, ui_state_types::ui_state_none);

                    }break;
                }
            }
        }   
        

        if(WAS_PRESSED(currInput, function.keyESCAPE, InputTypes::input_key_escape)){
                    //this never seems to get triggered. when we press escape that just cancels text input, never propogates here
            switch (uiData->current_state){
  
                case ui_state_drag_item         :   {
                    if(uiData->dragged_item_original_slot){
                        *uiData->dragged_item_original_slot = uiData->dragged_itemID;
                        uiData->dragged_itemID = 0;
                        uiData->dragged_item_original_slot = nullptr;
                    }

                    transition_ui_state(uiData, ui_state_types::ui_state_none);
                }break;
                case ui_state_chat_input        : break;
                case ui_state_resize_window     : break;  
                case ui_state_drag_window       : break;  
                case ui_state_drag_slider       : break; 
                case ui_state_text_editor_input :{
                    transition_ui_state(uiData, ui_state_types::ui_state_none);
                };break;
                case ui_state_none              :
                default: {}break;
           }
        }

        //need some method of interacting with the hotbar even if we aren't displaying all the UI windows
        vec2 mouse_pos   = vec2_create(currInput.mouse_x, currInput.mouse_y);
        vec2 mouse_delta = vec2_create(currInput.mouse_dx, currInput.mouse_dy);
        
        {//data that needs to be reset every input update/update in general
            uiData->information_panel_size = 0;


        }



        //if mouse motion
        bool mouse_moved = currInput.mouse_dx || currInput.mouse_dy;
        // bool mouse_clicked = currInput.mouse.left && !prevInput.mouse.left || (currInput.mouse.sideFront && !prevInput.mouse.sideFront);
        bool mouse_clicked = WAS_PRESSED(currInput, mouse.left, InputTypes::input_mouse_left) || WAS_PRESSED(currInput, mouse.sideFront, InputTypes::input_mouse_sideFront);
        if(mouse_moved){

            ui_mouse_moved(GameState, uiData, currInput, mouse_pos, mouse_delta, mouse_clicked);

            switch (uiData->current_state){

                case ui_state_resize_window     :   {

                          
                    if(uiData->selected_window && uiData->selected_window->resize_widget_selected){//resize window
                        ui_element& element = uiData->selected_window->base;
                        ui_window& window = *uiData->selected_window;
                        window.fixed = false;//if window was fixed, its not any more

                        bool resized = false;
                        currInput.consumedMouse.delta = 1;

                        if(((element.width  + mouse_delta.x) > MIN_WINDOW_DIMENSIONS)){
                            float new_width = element.width  + mouse_delta.x;
                            float new_posx = element.posx   + (mouse_delta.x * 0.5f);
                            float new_minx = new_posx - new_width  * 0.5f;
                            float new_maxx = new_posx + new_width  * 0.5f;
                            //restrict window from expanding off screen
                            if(((new_minx) >= 0) &&
                               ((new_maxx) <= *GameState->window_width )){
                                element.width  = new_width;
                                element.posx   = new_posx;
                                element.minx = new_minx;
                                element.maxx = new_maxx;
                                element.normPosX =     new_posx / element.parentWidth;
                                element.scale.x = element.maxx - element.minx;
                                // printf("width: %f, posx: %f, minx: %f, maxx: %f, normPosX: %f, scale[0]: %f\n", element.width, element.posx, element.minx, element.maxx, element.normPosX, element.scale.x);
                                resized = true;
                            }

                        }

                        if(((element.height  + mouse_delta.y) > MIN_WINDOW_DIMENSIONS)){
                            float new_height = element.height  + mouse_delta.y;
                            float new_posy = element.posy   + (mouse_delta.y * 0.5f);
                            float new_miny = new_posy - new_height  * 0.5f;
                            float new_maxy = new_posy + new_height  * 0.5f;
                            //restrict window from expanding off screen
                            if(((new_miny) >= 0) &&
                               ((new_maxy) <= *GameState->window_height )){
        
                                element.height = new_height;
                                element.posy = new_posy;
                                element.miny = new_miny;
                                element.maxy = new_maxy;
                                element.normPosY =     new_posy / element.parentHeight;
                                element.scale.y = element.maxy - element.miny;
                                resized = true;
                                // printf("height: %f, posy: %f, miny: %f, maxy: %f, normPosY: %f, scale[1]: %f\n", element.height, element.posy, element.miny, element.maxy, element.normPosY, element.scale[1]);

                            }

                            //resize sub slots inside the window
                            resize_base_window(GameState, *uiData->selected_window);

                          
                        }
                    }else{//error, transition back to no state
                        transition_ui_state(uiData, ui_state_types::ui_state_none);

                    }
                    // }else if(uiData->selected_element && uiData->hovered_itemID && uiData->selected_element->type == ui_element_types::ui_element_type_inventory_slot){
                    //     uiData->selected_element->clicked = true;
                    //     drag_item(GameState, uiData);

                }break;
                case ui_state_drag_window       :   {
                    if(!uiData->selected_window){
                        printf("WINDOW IS NULL? DEBUG THIS!\n");
                        break;
                    };
                    ui_element& element = uiData->selected_window->base;
                    ui_window& window = *uiData->selected_window;
                    window.fixed = false;//if window was fixed, its not any more
                    currInput.consumedMouse.delta = 1;
                    if(window.docked && (fabs(mouse_delta.x) > 10 || fabs(mouse_delta.y) > 10)){
                        undock_window(GameState, window, *window.docked_to);
                        //return to original size, but maintain current position
                        window.original.posx     = window.base.posx; 
                        window.original.posy     = window.base.posy; 
                        window.original.normPosX = window.base.normPosX; 
                        window.original.normPosY = window.base.normPosY; 
                        window.base = window.original; 
                        window.base.active = true;
                        window.base.hovered = false;
                        window.base.widget_selected = false;
                        window.base.clicked = false;
                        resize_base_window(GameState, window);

                        // drag_ui_element(GameState, element, mouse_delta);

                    }else if(!window.docked){
                        drag_ui_element(GameState, window.base, mouse_delta, &window);

                    }
                }break;
                case ui_state_drag_item         :   {
                    currInput.consumedMouse.delta = 1;

                        // uiData->dragging_item->posy = mouse_pos.y;
                        // uiData->dragging_item->posx = mouse_pos.x;
                        // uiData->dragging_item->minx = mouse_pos.x + (uiData->dragging_item->width * 0.5f);
                        // uiData->dragging_item->maxx = mouse_pos.x + (uiData->dragging_item->width * 0.5f);
                        // uiData->dragging_item->miny = mouse_pos.y + (uiData->dragging_item->height * 0.5f);
                        // uiData->dragging_item->maxy = mouse_pos.y + (uiData->dragging_item->height * 0.5f);
                        // uiData->dragging_item->normPosY = mouse_pos.y / GameState->sharedState->height;
                        // uiData->dragging_item->normPosX = mouse_pos.x / GameState->sharedState->width;
                        printf("DRAGGING ITEM!\n");
                }break;
                case ui_state_drag_slider       :   {
                    currInput.consumedMouse.delta = 1;

                    switch(uiData->dragging_slider->type){

                        case ui_element_types::ui_element_type_vertical_scrollbar:{
                            adjust_slider(uiData->dragging_slider, uiData->dragging_slider->height, mouse_delta.y);
                        }break;

                        case ui_element_types::ui_element_type_horizontal_scrollbar:{
                            adjust_slider(uiData->dragging_slider, uiData->dragging_slider->width, mouse_delta.x);
                        }break;

                        case ui_element_types::ui_element_type_horizontal_slider_float:{
                            // printf("handle debug slider moving!\n");    
                            adjust_debug_slider(uiData->dragging_slider, uiData->dragging_slider->width, mouse_delta.x);

                        }break;
                        case ui_element_types::ui_element_type_vertical_slider_float:{
                            // printf("handle debug slider moving!\n");    
                            adjust_debug_slider(uiData->dragging_slider, uiData->dragging_slider->height, -mouse_delta.y);

                        }break;

                    }
                    
           
                }break;
                case ui_state_chat_input        :   {}break;
                case ui_state_text_editor_input :   {}break;
                case ui_state_none              ://none falls through to default
                default: {}break;
            }
        }

        //if clicked
        if(mouse_clicked){
            uiData->hovered_itemID = 0;
            //need to recalculate moused over elements to determine if we are hovering over another item
            ui_mouse_moved(GameState, uiData, currInput, mouse_pos, mouse_delta, mouse_clicked);
            ui_window_click(GameState, uiData, currInput, mouse_pos);
            switch (uiData->current_state){

                case ui_state_drag_item         :   {
                    printf("current state item drag, CLICK\n");
                    if(uiData->selected_element){
                        uiData->selected_element->clicked = true;
                        switch (uiData->selected_element->type){
                            case ui_element_types::ui_element_type_inventory_slot:{
                                if(uiData->hovered_item_slot){

                                    item_data& item = GameState->entityComponent->inventory_items[uiData->dragged_itemID].data;



                                    if(uiData->hovered_item_slot_type == ui_element_inventory_types::ui_element_inventory_type_item_slot ||
                                        uiData->hovered_item_slot_type == ui_element_inventory_types::ui_element_inventory_type_hotbar_slot){

                                        //what if the slot we want to drop into is occupied?? we need to swap!
                                        if(uiData->hovered_itemID){
                                            //store currently held item in temp memory
                                            uiData->dragging_item_memory = *uiData->selected_element;
                                            u32 swappedItemID = uiData->dragged_itemID;
                                            drag_item(GameState, uiData);
                                            *uiData->hovered_item_slot = swappedItemID;
                                            uiData->dragged_item_original_slot = nullptr;
                                            printf("SWAPPING ITEMS IN ITEM SLOT!\n");
                                            
                                        }else{//dropping into empty slot
                                            //any item can go here
                                            *uiData->hovered_item_slot = uiData->dragged_itemID;
                                            uiData->dragged_itemID = 0;
                                            uiData->dragged_item_original_slot = nullptr;
                                            transition_ui_state(uiData, ui_state_types::ui_state_none);
                                            printf("PLACING ITEM INTO ITEM SLOT!\n");
                                        }


                                    
                                    }else if(uiData->hovered_item_slot_type == ui_element_inventory_types::ui_element_inventory_type_trinket_slot){
                                        
                                        //only trinkets can go here
                                        
                                        if(item.type == item_types::item_type_trinket){
                                        
                                            *uiData->hovered_item_slot = uiData->dragged_itemID;
                                            uiData->dragged_itemID = 0;
                                            uiData->dragged_item_original_slot = nullptr;
                                            transition_ui_state(uiData, ui_state_types::ui_state_none);
                                            printf("PLACING ITEM INTO TRINKET SLOT!\n");
                                        
                                        }else{
                                            printf("INVALID DESTINATION TRINKET SLOT FOR CURRENT ITEM!\n");
                                            append_feedback_text_element(uiData, "CANT PLACE ITEM IN TRINKET SLOT!");

                                        }
          
                                    }else if(uiData->hovered_item_slot_type == ui_element_inventory_types::ui_element_inventory_type_equipment_slot){
                                        //only equipment can go here

                                        if(item.type == item_types::item_type_equipment){
                                            *uiData->hovered_item_slot = uiData->dragged_itemID;
                                            uiData->dragged_itemID = 0;
                                            uiData->dragged_item_original_slot = nullptr;
                                            transition_ui_state(uiData, ui_state_types::ui_state_none);
                                            printf("PLACING ITEM INTO EQUIPMENT SLOT!\n");

                                        }else{
                                            printf("INVALID DESTINATION EQUIPMENT SLOT FOR CURRENT ITEM!\n");
                                            append_feedback_text_element(uiData, "CANT PLACE ITEM IN EQUIPMENT SLOT!");
                                        }
                                    
                                    }else{
                                        printf("INVALID DESTINATION SLOT FOR CURRENT ITEM!\n");
                                    }

                                }else{
                                    printf("INVALID DESTINATION SLOT FOR CURRENT ITEM!\n");
                                }
                            
                            }break;
                        }
                    }else{
                        //return item to original slot
                        if(WAS_PRESSED(currInput, mouse.left, InputTypes::input_mouse_left)) currInput.consumedMouse.left = 1;
                        if(!uiData->dragged_item_original_slot){
                            //original slot is invalid, need to find the first empty slot to place the item
                            //scan player inventory for first free slot
                            u32 invIndex = GameState->entityComponent->entityToInventoryMap[GameState->localPlayerEntityIDs[0]];
                            InventoryComp& invComp = GameState->entityComponent->InventoryComps[invIndex != NULL_ENTITY ? invIndex : 2046];//give it some kind of valid value just in case 
                            if(!uiData->dragged_item_original_slot){
                                for(u32 i = 0; i < 5 ; i++){//left hand slots
                                    if(invComp.left_hand[i] == 0){
                                        uiData->dragged_item_original_slot = invComp.left_hand + i;
                                        break;
                                    }
                                }
                            }
                            if(!uiData->dragged_item_original_slot){
                                for(u32 i = 0; i < 5 ; i++){//right hand slots
                                    if(invComp.right_hand[i] == 0){
                                        uiData->dragged_item_original_slot = invComp.right_hand + i;
                                        break;
                                    }
                                }   
                            }
                            if(!uiData->dragged_item_original_slot){
                                for(u32 i = 0; i < 32; i++){//general inventory slots
                                    if(invComp.items[i] == 0){
                                        uiData->dragged_item_original_slot = invComp.items + i;
                                        break;
                                    }
                                }

                            }

                            if(!uiData->dragged_item_original_slot){
                                printf("NO VALID SLOT FOUND TO RETURN ITEM TO!!\n");
                                break;//break out of switch statement
                            }
              
                            //we could collapse this, but I'd rather keep it verbose for now in case we need more cases for this edge case
                            *uiData->dragged_item_original_slot = uiData->dragged_itemID;
                            uiData->dragged_itemID = 0;
                            uiData->dragged_item_original_slot = nullptr;
                            transition_ui_state(uiData, ui_state_types::ui_state_none);

                        }else{
                            *uiData->dragged_item_original_slot = uiData->dragged_itemID;
                            uiData->dragged_itemID = 0;
                            uiData->dragged_item_original_slot = nullptr;
                            transition_ui_state(uiData, ui_state_types::ui_state_none);
                        }

                    
                    }

                }break;
                // case ui_state_resize_window     :
                // case ui_state_drag_window       :
                // case ui_state_drag_slider       :
                // case ui_state_drag_scrollbar    :
                // case ui_state_chat_input        :
                // case ui_state_text_editor_input ://falls through to default
                // case ui_state_none              :
                default: {


                    if(uiData->selected_element && uiData->selected_element->type == ui_element_types::ui_element_type_editable_text){
                        printf("EDITABLE TEXT SLOT SELECTED!\n");
                        GameState->text_dest = &uiData->windows[ui_window_types::window_type_book_text].data.book_text.text_input;
                        GameState->textInputEnabled = true;
                        GameState->text_dest->show_cursor = true;
                        transition_ui_state(uiData, ui_state_types::ui_state_text_editor_input);
        
                    }else{//automatically direct text input back to the chat window
                        GameState->text_dest->show_cursor = false;
                        GameState->text_dest = &uiData->windows[ui_window_types::window_type_chat].data.chat.text_input;
                        GameState->textInputEnabled = false;
                        transition_ui_state(uiData, ui_state_types::ui_state_none);
                    }
                    if(uiData->selected_element){

         
               

                        uiData->selected_element->clicked = true;
                        if(uiData->selected_window && uiData->selected_window->resize_widget_selected){
                            transition_ui_state(uiData, ui_state_types::ui_state_resize_window);

                        }
                        else{
                            switch (uiData->selected_element->type){
                                case ui_element_types::ui_element_type_inventory_slot:{
                                    if(uiData->hovered_itemID){ 
                                        drag_item(GameState, uiData);
                                    }else{
                                        //item slot emtpy, drag window instead
                                        transition_ui_state(uiData, ui_state_types::ui_state_drag_window);
                                    }
                                    
                                }break;
    
                                case ui_element_types::ui_element_type_horizontal_scrollbar:
                                case ui_element_types::ui_element_type_vertical_scrollbar:
                                case ui_element_types::ui_element_type_horizontal_slider_float:
                                case ui_element_types::ui_element_type_vertical_slider_float:{
                                    if(uiData->mouse_intersects_slider){
                                        uiData->dragging_slider = uiData->selected_element;
                                        transition_ui_state(uiData, ui_state_types::ui_state_drag_slider);
                                    }else{
                                        transition_ui_state(uiData, ui_state_types::ui_state_drag_window);
                                    }
                                    
                                }break;
                                
                                case ui_element_types::ui_element_type_button:{
                                    uiData->selected_element->data.button.clicked = true;
                                    //if we dont want to hash the label, we would just need more static memory bools to pass in as pointers to change here
                                    // (*uiData->selected_element->data.button.value) = !(*uiData->selected_element->data.button.value);
                                    push_table(&uiData->label_table, uiData->selected_element->data.button.label, uiData->selected_element->data, uiData->selected_element->type);
                                    if(uiData->selected_window)transition_ui_state(uiData, ui_state_types::ui_state_drag_window);
                                }break;
                                case ui_element_types::ui_element_type_checkbox:{
                                    uiData->selected_element->data.checkbox.clicked = true;
                                    // (*uiData->selected_element->data.checkbox.value) = !(*uiData->selected_element->data.checkbox.value);
                                    push_table(&uiData->label_table, uiData->selected_element->data.checkbox.label, uiData->selected_element->data, uiData->selected_element->type);
                                    if(uiData->selected_window)transition_ui_state(uiData, ui_state_types::ui_state_drag_window);
                                }break;
                                case ui_element_types::ui_element_type_bar:{
                                    // printf("clicked a bar element!\n");
                                    switch(uiData->selected_element->data.bar.type){
                                        case bargraph_profile_event:{
                                            // printf("bargraph profile event clock selected! clock: %zu\n", uiData->selected_element->data.bar.data.eventClock);
                                            uiData->clicked_element = uiData->selected_element;
                                        }break;
                                        case bargraph_profile_history_snapshot:{
                                            // printf("bargraph profile history index selected! index: %u\n", uiData->selected_element->data.bar.data.eventIndex);
                                            uiData->clicked_element = uiData->selected_element;
                                        }break;
                                    }
                                }//flow into the window drag logic

                                case ui_element_types::ui_element_type_right_book_page:    
                                case ui_element_types::ui_element_type_none:    
                                case ui_element_types::ui_element_type_text:
                                case ui_element_types::ui_element_type_texture:
                                case ui_element_types::ui_element_type_inline:
                                case ui_element_types::ui_element_type_bar_graph:
                                case ui_element_types::ui_element_type_window_base:{
                                    if(uiData->selected_window){
                                        if(uiData->selected_window->resize_widget_selected)transition_ui_state(uiData, ui_state_types::ui_state_resize_window);
                                        transition_ui_state(uiData, ui_state_types::ui_state_drag_window);
                                    } 
                                }break;
                                //disabling text input at the top seems to work better instead of tucking it in here
                                // case ui_element_types::ui_element_type_editable_text:{
                                //     printf("EDITABLE TEXT SLOT SELECTED!\n");
                                //     GameState->text_dest = &uiData->windows[ui_window_types::window_type_book_text].data.book_text.text_input;
                                //     SDL_StartTextInput();
                                //     GameState->input->textInput = true;
                                //     GameState->sharedState->textInput = true;
                                //     GameState->text_dest->show_cursor = true;
                                //     transition_ui_state(uiData, ui_state_types::ui_state_text_editor_input);
                                // }break;
                            }
                        }
                     
                    }else if(uiData->selected_window){
                        transition_ui_state(uiData, ui_state_types::ui_state_drag_window);

                    }
                }break;
            }

        
        }

        //if mouse up
        if(WAS_RELEASED(currInput, mouse.left, InputTypes::input_mouse_left) || WAS_RELEASED(currInput, mouse.sideFront, InputTypes::input_mouse_sideFront)){
            ui_mouse_released(GameState, uiData, currInput, mouse_pos);
            switch (uiData->current_state){
                case ui_state_text_editor_input :   
                case ui_state_resize_window     :   {                    
                    uiData->selected_window = nullptr;
                    transition_ui_state(uiData, ui_state_types::ui_state_none);
                }break;
                case ui_state_drag_window       :   {

                    ui_window* windows = uiData->windows;
  
                    if(uiData->selected_window){
                        for(int i = 1; i < MAX_WINDOWS; i++){
                            if(windows[i].base.active && windows[i].dockable && windows[i].base.hovered && (!windows[i].docked && !uiData->selected_window->docked && uiData->selected_window->dockable) && (uiData->selected_window != (windows + i))){
                                printf("released window ON TOP OF OTHER WINDOW!\n");
                                windows[i].original = windows[i].base; //save original data for later restoration
                                if(windows[i].type == ui_window_types::window_type_none){printf("TRYING TO DOCK TO WINDOW TYPE NONE?? ERROR! BREAK!\n"); break;}

                                dock_to_window(GameState, windows[i], *uiData->selected_window);
                                float width  = 50.0f;
                                float height = 50.0f;
                                float normPosX = (uiData->selected_window->base.maxx  + (width  * 0.5f)) /  uiData->selected_window->base.parentWidth ;//want this window to hug the bottom right corner 
                                float normPosY = (uiData->selected_window->base.miny  + (height * 0.5f)) / uiData->selected_window->base.parentHeight;
                                resize_base_window(GameState, windows[i], normPosX, normPosY, width, height);
                                break;
                            }
                        }
                    

                        if(uiData->selected_window->docked){
                            ui_window& swapped_window = *uiData->selected_window->docked_to;
                            if(swapped_window.dockable){
                                undock_window (GameState, *uiData->selected_window, swapped_window);
                                float width     = swapped_window.base.width;
                                float height    = swapped_window.base.height;
                                float normPosX  = swapped_window.base.normPosX;//want this window to hug the bottom right corner 
                                float normPosY  = swapped_window.base.normPosY;
                                resize_base_window(GameState, *uiData->selected_window, normPosX, normPosY, width, height);
                                
                                swapped_window.original = swapped_window.base; //save original data for later restoration
                                dock_to_window(GameState, swapped_window, *uiData->selected_window);
                                width  = 50.0f;
                                height = 50.0f;
                                normPosX = (uiData->selected_window->base.maxx  + (width * 0.5f)) /  uiData->selected_window->base.parentWidth ;//want this window to hug the bottom right corner 
                                normPosY = (uiData->selected_window->base.miny  + (height * 0.5f)) / uiData->selected_window->base.parentHeight;
                                resize_base_window(GameState, swapped_window, normPosX, normPosY, width, height);
                            }
                    
                        }
                    }

                    uiData->selected_window = nullptr;
                    transition_ui_state(uiData, ui_state_types::ui_state_none);

                }break;
                case ui_state_drag_item         :   {}break;
                case ui_state_drag_slider       :   {
                    uiData->dragging_slider = nullptr;
                    transition_ui_state(uiData, ui_state_types::ui_state_none);
                }break;
                case ui_state_chat_input        :   {}break;
                case ui_state_none              ://none falls through to default
                default: {}break;
            }
        }

        if(currInput.mouse_wheel){

            if(uiData->hovered_window && uiData->hovered_window->vertical_scroll_height && uiData->hovered_window->interactable){
                currInput.consumedMouse.wheel = 1;
                if(uiData->selected_element){
                    switch(uiData->selected_element->type){
                        case ui_element_types::ui_element_type_horizontal_slider_float:{
                                scroll_debug_slider(uiData->selected_element, uiData->selected_element->width, currInput.mouse_wheel * 3.0f);
                        }break;
                        case ui_element_types::ui_element_type_vertical_slider_float:{
                                scroll_debug_slider(uiData->selected_element, uiData->selected_element->height, currInput.mouse_wheel * 3.0f);
                        }break;
                        case ui_element_types::ui_element_type_horizontal_scrollbar:{
                                adjust_slider(uiData->selected_element, uiData->selected_element->width, currInput.mouse_wheel * 3.0f);
                        }break;
                        default:{//if its an unhandled selected element case, then just scroll the window
                            switch(uiData->hovered_window->type){
                                case ui_window_types::window_type_book_text:{
                                    adjust_slider(&uiData->hovered_window->data.book_text.left_vertical_scrollbar, *uiData->hovered_window->vertical_scroll_height, -currInput.mouse_wheel * 3.0f);
                                }break;
                                default:{
                                    adjust_slider(&uiData->hovered_window->vertical_scrollbar, *uiData->hovered_window->vertical_scroll_height, -currInput.mouse_wheel * 3.0f);
                                }break;
                            }
                        }break;
                    }
                
                }

            }
            //update positions/selected elements after scroll has been applied
            ui_mouse_moved(GameState, uiData, currInput, mouse_pos, mouse_delta, mouse_clicked);


        }
        //broken arrow key selection feature
        #if 1
        if(WAS_PRESSED(currInput, function.up, InputTypes::input_key_arrow_up)){}
        if(WAS_PRESSED(currInput, function.down, InputTypes::input_key_arrow_down)){}
        if(WAS_PRESSED(currInput, function.left, InputTypes::input_key_arrow_left)){
            if(uiData->previously_selected_parent_element && uiData->previously_selected_parent_element->type == ui_element_type_bar_graph){
                uiData->previously_selected_element_index--;
                uiData->previously_selected_element_index = uiData->previously_selected_element_index % 300;
                uiData->clicked_element = uiData->previously_selected_parent_element->data.barGraph.start + uiData->previously_selected_element_index;
            
            }
        
        }
        if(WAS_PRESSED(currInput, function.right, InputTypes::input_key_arrow_right)){
            if(uiData->previously_selected_parent_element && uiData->previously_selected_parent_element->type == ui_element_type_bar_graph){
                uiData->previously_selected_element_index++;
                uiData->previously_selected_element_index = uiData->previously_selected_element_index % 300;
                //make this the new selected element
                uiData->clicked_element = uiData->previously_selected_parent_element->data.barGraph.start + uiData->previously_selected_element_index;
            
            }

        }
        #endif

        //specific UI logic for right click/middle mouse wheel click
        if(WAS_PRESSED(currInput, mouse.right, InputTypes::input_mouse_right) || WAS_PRESSED(currInput, mouse.middle, InputTypes::input_mouse_middle)){//just check if the right mouse clicked anywhere so we can consume it
            if(!(mouse_moved))ui_mouse_moved(GameState, uiData, currInput, mouse_pos, mouse_delta, mouse_clicked);
            ui_mouse_window_intersect_test(GameState, uiData, currInput, mouse_pos);
            switch (uiData->current_state){

                case ui_state_drag_item         :   {}break;
                default:{
                    if(uiData->selected_element){

                        uiData->selected_element->clicked = true;
                        if(uiData->selected_window && uiData->selected_window->resize_widget_selected){
                            transition_ui_state(uiData, ui_state_types::ui_state_resize_window);

                        }
                        else{
                            switch (uiData->selected_element->type){

                                // case ui_element_types::ui_element_type_horizontal_scrollbar:
                                // case ui_element_types::ui_element_type_vertical_scrollbar:
                                case ui_element_types::ui_element_type_vertical_slider_float:
                                case ui_element_types::ui_element_type_horizontal_slider_float:{
                                    if(uiData->mouse_intersects_slider){
                                        uiData->dragging_slider = uiData->selected_element;
                                        //reset slider to originalValue
                                        // printf("reset slider to original value!\n");
                                        *uiData->selected_element->data.debug_slider_float.current_value = (uiData->selected_element->data.debug_slider_float.originalValue);
                                    }
                                    
                                }break;
                                case ui_element_types::ui_element_type_button:{
                                    uiData->selected_element->data.button.rightClicked = true;
                                    push_table(&uiData->label_table, uiData->selected_element->data.button.label, uiData->selected_element->data, uiData->selected_element->type);
                                }break;
                            }
                        }
                    }
                }break;
            }
        }
        if(currInput.mouse.right){
            if(!(mouse_moved))ui_mouse_moved(GameState, uiData, currInput, mouse_pos, mouse_delta, mouse_clicked);
            ui_mouse_window_intersect_test(GameState, uiData, currInput, mouse_pos);
        }
        if(uiData->mouse_intersects_element || uiData->current_state == ui_state_types::ui_state_drag_item){
            if(WAS_PRESSED(currInput, mouse.sideFront, InputTypes::input_mouse_sideFront)) currInput.consumedMouse.sideFront = 1;
            if(WAS_PRESSED(currInput, mouse.left, InputTypes::input_mouse_left)){
                currInput.consumedMouse.left = 1;
            } 
            if(WAS_PRESSED(currInput, mouse.right, InputTypes::input_mouse_right) || currInput.mouse.right) currInput.consumedMouse.right = 1;
        }
        if(WAS_PRESSED(currInput, mouse.sideBack, InputTypes::input_mouse_sideBack)){
            uiData->freezeTooltipWindow = !uiData->freezeTooltipWindow;
            // printf("freeze tooltip window: %d\n" ,uiData->freezeTooltipWindow);
        }
        if(!uiData->freezeTooltipWindow){
            uiData->toolTipPos = mouse_pos;
        }
    }







    int ui_prepare_debug_request(ui_data* uiData, char* label, int char_max, int element_count, int max_elements, char* dest){
        int len = handmade_strlen(label); 
        if(len >= char_max){
            printf("DEBUG ELEMENT LABEL %s TOO LARGE! FAILURE!\n", label);
            return -1;
        }
        if(element_count >= max_elements){
            printf("TOO MANY DEBUG ELEMENTS! %d FAILURE!\n", element_count);
            return -1;
        }
        if(len == 0){
            printf("LABEL IS 0? FAILURE!\n");
            return -1;
        }

        //set the label if everything went well
        handmade_strcpy(dest, label);
        dest[len] = 0;

        return len;
    }


    ui_window* ui_begin_window    (game_state* GameState, char* label, bool dockable = true){
        ui_data* uiData = GameState->uiData;
        
        int len = ui_prepare_debug_request(uiData, label, DEBUG_ELEMENT_LABEL, uiData->debug_window_count, MAX_DEBUG_WINDOWS, uiData->debug_window_labels[uiData->debug_window_count]);
        if(len <= 0){
            printf("INVALID DEBUG WINDOW PARAMS!");
            return nullptr;
        }

        ui_window* window = uiData->windows + (ui_window_types::window_type_count + uiData->debug_window_count);
        window->selected_element = 0;
        window->selected_bounds = 0;
        window->dockable = dockable;
        window->base.hashEntry = UIHashElement(uiData, label, &window->base);
        window->base.parentHash = 0;

        //set where the window is getting/storing/setting its debug elements
        window->data.debug.elements = uiData->debug_elements + uiData->debug_element_count;

        //track number of elements across frames so we know if theres more/less and we need to resize the window
        window->data.debug.last_frame_element_count = window->data.debug.element_count;

        window->data.debug.element_count = 0;
        return window;

    }

    //TODO: make the height of the element directly tied to the text, if its a line, just the height of a character, multiple lines -> add it all up
    void ui_text            (game_state* GameState, const char* fmt, ...){
        ui_data* uiData = GameState->uiData;

        //replace with custom system? just see if it works first
        Assert(fmt && strlen(fmt) > 0);
        va_list args;
        va_start(args, fmt);
        char label[1024];
        vsnprintf(label, sizeof(label), fmt, args);
        va_end(args); 

        char* dest = uiData->debug_element_labels[uiData->debug_element_count];

        int len = ui_prepare_debug_request(uiData, label, DEBUG_ELEMENT_LABEL, uiData->debug_element_count, MAX_DEBUG_ELEMENTS, dest);
        if(len <= 0){
            printf("INVALID DEBUG ELEMENT PARAMS!");
            return;
        }
        float textWidth = len * (GameState->RenderCommandData->monospacedScreenFont.maxCharWidth * GameState->RenderCommandData->monospacedScreenFont.scale);


        ui_element* element = uiData->debug_elements + uiData->debug_element_count++;
        *element = {};

        ui_window* curr_window = uiData->windows + (ui_window_types::window_type_count + uiData->debug_window_count);
        element->type = ui_element_types::ui_element_type_text;
        element->data.text.label = dest;
        element->height = 30.0f;
        element->width = textWidth;

        curr_window->data.debug.element_count++;
        

    }

    
    void ui_text_highlight            (game_state* GameState, float height, u32 highlightStart, u32 highlightLen, const char* fmt, ...){
        ui_data* uiData = GameState->uiData;

        //replace with custom system? just see if it works first
        Assert(fmt && strlen(fmt) > 0);
        va_list args;
        va_start(args, fmt);
        char label[1024];
        vsnprintf(label, sizeof(label), fmt, args);
        va_end(args); 

        char* dest = uiData->debug_element_labels[uiData->debug_element_count];

        int len = ui_prepare_debug_request(uiData, label, DEBUG_ELEMENT_LABEL, uiData->debug_element_count, MAX_DEBUG_ELEMENTS, dest);
        if(len <= 0){
            printf("INVALID DEBUG ELEMENT PARAMS!");
            return;
        }

        ui_element* element = uiData->debug_elements + uiData->debug_element_count++;
        *element = {};

        ui_window* curr_window = uiData->windows + (ui_window_types::window_type_count + uiData->debug_window_count);
        element->type = ui_element_types::ui_element_type_text_highlight;
        element->data.text.label = dest;
        element->data.text.highlightStart = highlightStart;
        element->data.text.highlightLen = highlightLen;
        element->height = height;

        curr_window->data.debug.element_count++;
        

    }

    void ui_texture            (game_state* GameState, float height = 30.0f, u32 index = 0){
        ui_data* uiData = GameState->uiData;

        if(uiData->debug_element_count >= MAX_DEBUG_ELEMENTS){
            printf("TOO MANY DEBUG ELEMENTS! %d FAILURE!\n", uiData->debug_element_count);
            return;
        }


        ui_element* element = uiData->debug_elements + uiData->debug_element_count++;
        *element = {};

        ui_window* curr_window = uiData->windows + (ui_window_types::window_type_count + uiData->debug_window_count);
        element->type = ui_element_types::ui_element_type_texture;
        element->height = height;
        element->width = height;
        element->data.texture.textureIndex = index;

        curr_window->data.debug.element_count++;
        

    }

    
    void ui_color            (game_state* GameState, vec4 color, float height = 30.0f){
        ui_data* uiData = GameState->uiData;

        if(uiData->debug_element_count >= MAX_DEBUG_ELEMENTS){
            printf("TOO MANY DEBUG ELEMENTS! %d FAILURE!\n", uiData->debug_element_count);
            return;
        }


        ui_element* element = uiData->debug_elements + uiData->debug_element_count++;
        *element = {};

        ui_window* curr_window = uiData->windows + (ui_window_types::window_type_count + uiData->debug_window_count);
        element->type = ui_element_types::ui_element_type_color;
        element->height = height;
        element->width = curr_window->base.width;

        element->data.color.color = color;

        curr_window->data.debug.element_count++;
        

    }


    bool ui_checkbox        (game_state* GameState, bool& value, const char* fmt, ...){
        ui_data* uiData = GameState->uiData;

        va_list args;
        va_start(args, fmt);
        char label[128];
        vsnprintf(label, sizeof(label), fmt, args);
        va_end(args);

        char* dest = uiData->debug_element_labels[uiData->debug_element_count];
        
        int len = ui_prepare_debug_request(uiData, label, DEBUG_ELEMENT_LABEL, uiData->debug_element_count, MAX_DEBUG_ELEMENTS, dest);
        if(len <= 0){
            printf("INVALID DEBUG ELEMENT PARAMS!");
            return false;
        }

        ui_element* element = uiData->debug_elements + uiData->debug_element_count++;
        *element = {};

        ui_window* curr_window = uiData->windows + (ui_window_types::window_type_count + uiData->debug_window_count);
        element->type = ui_element_types::ui_element_type_checkbox;
        element->data.checkbox.label = dest;
        element->data.checkbox.clicked = false;
        element->data.checkbox.value = &value;
        element->height = 30.0f;
        element->width = 30.0f;
        
        ui_element_data prev_data = push_table(&uiData->label_table, dest, element->data, element->type);
        // ui_element_data prev_data = {};
        if(prev_data.checkbox.clicked){
            *element->data.checkbox.value = !*element->data.checkbox.value;
            
        }


        curr_window->data.debug.element_count++;

        return prev_data.checkbox.clicked;


    }

    bool ui_slider_float        (game_state* GameState, float& minRange, float& maxRange, float& value, float origValue, const char* fmt, ...){
        ui_data* uiData = GameState->uiData;

        va_list args;
        va_start(args, fmt);
        char label[128];
        vsnprintf(label, sizeof(label), fmt, args);
        va_end(args);

        char* dest = uiData->debug_element_labels[uiData->debug_element_count];
        
        int len = ui_prepare_debug_request(uiData, label, DEBUG_ELEMENT_LABEL, uiData->debug_element_count, MAX_DEBUG_ELEMENTS, dest);
        if(len <= 0){
            printf("INVALID DEBUG ELEMENT PARAMS!");
            return false;
        }

        ui_element* element = uiData->debug_elements + uiData->debug_element_count++;
        *element = {};

        ui_window* curr_window = uiData->windows + (ui_window_types::window_type_count + uiData->debug_window_count);

        element->type = ui_element_types::ui_element_type_horizontal_slider_float;
        element->data.debug_slider_float.label = dest;
        element->data.debug_slider_float.visible_range = 0.1f;
        element->data.debug_slider_float.min = minRange;
        element->data.debug_slider_float.max = maxRange;
        element->data.debug_slider_float.norm_value = (value - minRange) / (maxRange - minRange);
        element->data.debug_slider_float.current_value = (&value);
        element->data.debug_slider_float.originalValue = origValue;

        element->height = 30.0f;
        element->width = curr_window->base.width;
        element->color = {0,1,0,1};
        
        ui_element_data prev_data = push_table(&uiData->label_table, dest, element->data, element->type);
        // ui_element_data prev_data = {};

        if(prev_data.debug_slider_float.clicked){
            // *element->data.slider_float.value = !*element->data.slider_float.value; //cant use this, from checkbox version
            
        }


        curr_window->data.debug.element_count++;

        return prev_data.debug_slider_float.clicked;


    }


    bool ui_set_button          (game_state* GameState, const char* fmt, ...){

        ui_data* uiData = GameState->uiData;

        va_list args;
        va_start(args, fmt);
        char label[128];
        vsnprintf(label, sizeof(label), fmt, args);
        va_end(args);
        
        char* dest = uiData->debug_element_labels[uiData->debug_element_count];
        int len = ui_prepare_debug_request(uiData, label, DEBUG_ELEMENT_LABEL, uiData->debug_element_count, MAX_DEBUG_ELEMENTS, dest);
        if(len <= 0){
            printf("INVALID DEBUG ELEMENT PARAMS!");
            return false;
        }

        ui_element* element = uiData->debug_elements + uiData->debug_element_count++;
        *element = {};

        ui_window* curr_window = uiData->windows + (ui_window_types::window_type_count + uiData->debug_window_count);

        element->type = ui_element_types::ui_element_type_button;
        element->data.button.label = dest;
        element->data.button.clicked = false;
        element->height = 30.0f;
        element->width = 30.0f;

        ui_element_data prev_data = push_table(&uiData->label_table, dest, element->data, element->type);
        // ui_element_data prev_data = {};

        curr_window->data.debug.element_count++;

        return prev_data.button.clicked;

    }

    void ui_end_window      (game_state* GameState, vec4 posInfo = {-1, 0, 0, 0}, bool interactable = true){
        ui_data* uiData = GameState->uiData;

        //resize window if it hasnt been created or used already
        ui_window* window = uiData->windows + (ui_window_types::window_type_count + uiData->debug_window_count);
        if(window->type == ui_window_types::window_type_none){//check if the window was previously unused and set a base size for it
             window->type = ui_window_types::window_type_debug;
             window->fixed = true;//fix the window initially, slop for easier window placement
            float width     = 400.0f;
            float height    = 300.0f;
            float normPosX  = (uiData->debug_window_count + 1) / 10.0f;
            float normPosY  = 0.3f;
            resize_base_window(GameState, *window, normPosX, normPosY, width, height);
            window->base.active = uiData->display_UI_window;

        }else{

            if(posInfo.x == -1){
                window->base.normPosX   =  window->base.normPosX   ;
                window->base.normPosY   =  window->base.normPosY   ;
                window->base.width      =  window->base.width      ;
                window->base.height     =  window->base.height     ;
            }else{
                window->base.normPosX   = posInfo.x;
                window->base.normPosY   = posInfo.y;
                window->base.width      = posInfo.z;
                window->base.height     = posInfo.w;
                window->fixed = true;
            }
            //always need to resize debug elements since other windows debug elements can increase and reset the sizes for these
            // if(uiData->last_frame_debug_element_count != uiData->debug_element_count)

            //TODO: can we just submit the draw calls here?
            resize_base_window(GameState, *window);
            // if(window->data.debug.last_frame_element_count != window->data.debug.element_count        ){
                // new element since last frame, resize window
            // }
        }

        
        window->type = ui_window_types::window_type_debug;
        window->interactable = interactable;

        uiData->debug_window_count++;

    }

    void ui_flush_debug     (game_state* GameState){
        ui_data* uiData = GameState->uiData;
        uiData->last_frame_debug_window_count = uiData->debug_window_count;
        uiData->last_frame_debug_element_count = uiData->debug_element_count;
        uiData->debug_window_count = 0;
        uiData->debug_element_count = 0;
        uiData->debug_bargraph_element_count = 0;
        uiData->debug_inline_element_count = 0;

    }


    ui_element* ui_begin_barGraph(game_state* GameState, float normalizedValue, float height, const char* fmt, ...){
        ui_data* uiData = GameState->uiData;

        //replace with custom system? just see if it works first
        va_list args;
        va_start(args, fmt);
        char label[256];
        vsnprintf(label, sizeof(label), fmt, args);
        va_end(args);

        char* dest = uiData->debug_element_labels[uiData->debug_element_count];

        int len = ui_prepare_debug_request(uiData, label, DEBUG_ELEMENT_LABEL, uiData->debug_element_count, MAX_DEBUG_ELEMENTS, dest);
        if(len <= 0){
            printf("INVALID DEBUG ELEMENT PARAMS!");
            return nullptr;
        }

        ui_element* element = uiData->debug_elements + uiData->debug_element_count++;
        *element = {};

        ui_window* curr_window = uiData->windows + (ui_window_types::window_type_count + uiData->debug_window_count);
        element->type = ui_element_types::ui_element_type_bar_graph;
        element->data.barGraph.label = dest;
        element->data.barGraph.normalizedValue = normalizedValue;
        element->data.barGraph.currentValue = 0;
        element->data.barGraph.elementCount = 0; //clear the element count for the next frame
        element->height = height;
        element->width = curr_window->base.width;

        curr_window->data.debug.element_count++;

        //set the start of the array to read graph data from
        element->data.barGraph.start = uiData->debug_bargraph_elements + uiData->debug_bargraph_element_count;
        
        return element; 
    }
    
    void ui_append_barGraph(game_state* GameState, float minx, float maxx, float miny, float maxy, ui_element* barGraph, bargraph_data_types type, bargraph_data* data, vec4* color = 0){
        ui_data* uiData = GameState->uiData;
    
        if(uiData->debug_bargraph_element_count >= MAX_DEBUG_ELEMENTS){
            printf("TOO MANY DEBUG BARGRAPH ELEMENTS! %d FAILURE!\n", uiData->debug_bargraph_element_count);
            return;
        }

        
        ui_element* element = uiData->debug_bargraph_elements + uiData->debug_bargraph_element_count++;
        *element = {};

        ui_window* curr_window = uiData->windows + (ui_window_types::window_type_count + uiData->debug_window_count);
        
        element->type = ui_element_types::ui_element_type_bar;
        element->data.bar.type = type;
        element->data.bar.data = *data;
        element->minx = minx;
        element->maxx = maxx;
        element->miny = miny;
        element->maxy = maxy;
        if(color){
            element->color = *color;
        }else{
            element->color = {1,.5,.5,1.5};
        }

        element->height = barGraph->height;

        //do we include the graph element count in the total window element count? 
        // curr_window->data.debug.element_count++;
        
        barGraph->data.barGraph.elementCount++;
        barGraph->data.barGraph.currentValue += (maxx - minx);
    }
    
    void ui_end_barGraph(game_state* GameState){

    }

    ui_hash_entry* ui_begin_inline(game_state* GameState, float height, char* label){
        ui_data* uiData = GameState->uiData;

        if(uiData->debug_element_count >= MAX_DEBUG_ELEMENTS){
            printf("TOO MANY DEBUG ELEMENTS! %d FAILURE!\n", uiData->debug_element_count);
            Assert(!"What do we do when this happens?");
            return nullptr;
        }

        ui_element* element = uiData->debug_elements + uiData->debug_element_count++;
        *element = {};

        ui_window* curr_window = uiData->windows + (ui_window_types::window_type_count + uiData->debug_window_count);
        element->type = ui_element_types::ui_element_type_inline;
        element->height = height;
        element->width = 0.0f;//no width until we start adding elements

        curr_window->data.debug.element_count++;
        element->hashEntry = UIHashElement(uiData, label, element);
        element->parentHash = curr_window->base.hashEntry;

        //set the start of the array to read graph data from
        element->data.inlineElement.start = uiData->debug_inline_elements + uiData->debug_inline_element_count;
        element->data.inlineElement.elementCount = 0;
        element->data.inlineElement.currentWidth = 0; 
        
        //needed this to reset the elements between uses in case a previous element
        //was converted to an inline element, in which case the selected element would be invalid
        element->data.inlineElement.selected_element = 0;
        
        return element->hashEntry; 
    }


    ui_hash_entry* ui_append_inline(game_state* GameState, ui_element_types type, char* label, bool& value, float minx, float maxx, float miny, float maxy, ui_hash_entry* parentHash, ui_element_data& appendedData, vec4* color = 0, u32 textureIndex = 0, u32* textureMem = 0){
        ui_data* uiData = GameState->uiData;

        if(uiData->debug_inline_element_count >= MAX_DEBUG_ELEMENTS){
            printf("TOO MANY DEBUG ELEMENTS! %d FAILURE!\n", uiData->debug_inline_element_count);
            Assert(!"What do we do when this happens?");
            return nullptr;
        }
        
        ui_window* curr_window = uiData->windows + (ui_window_types::window_type_count + uiData->debug_window_count);
        ui_element* element = uiData->debug_inline_elements + uiData->debug_inline_element_count;
        ui_element* parent = parentHash->element;
        parent->data.inlineElement.elementCount++;
        element->type = type;
        element->minx = minx + parent->data.inlineElement.currentWidth;
        element->maxx = maxx + parent->data.inlineElement.currentWidth;
        element->miny = miny;
        element->maxy = maxy;

        float height = maxy - miny;
        element->height = height;
        element->width = maxx - minx;
        parent->width += element->width;

        switch(type){
            default:
            case ui_element_type_text:      {
                char* dest = uiData->debug_inline_labels[uiData->debug_inline_element_count];
                int len = ui_prepare_debug_request(uiData, label, DEBUG_ELEMENT_LABEL, uiData->debug_inline_element_count, MAX_DEBUG_ELEMENTS, dest);
                if(len <= 0){
                    printf("INVALID DEBUG ELEMENT PARAMS!");
                    Assert(!"What do we do when this happens?");
                    return false;
                }
                element->data.text.label = dest;

            }break;
            case ui_element_type_button:    {
                char* dest = uiData->debug_inline_labels[uiData->debug_inline_element_count];
                int len = ui_prepare_debug_request(uiData, label, DEBUG_ELEMENT_LABEL, uiData->debug_inline_element_count, MAX_DEBUG_ELEMENTS, dest);
                if(len <= 0){
                    printf("INVALID DEBUG ELEMENT PARAMS!");
                    Assert(!"What do we do when this happens?");
                    return false;
                }

                element->data.button.label = dest;
                element->data.button.clicked = false;
                element->data.button.rightClicked = false;
                element->data.button.value = &value;
                //could just toggle the value stored in the UI element when it gets clicked
                //this would require is to keep stuff like GameState->(bool)state to track if its been clicked or not, instead of using a more expensive hashmap
                appendedData = push_table(&uiData->label_table, dest, element->data, element->type);
            }break;
            case ui_element_type_checkbox:  {

                char* dest = uiData->debug_inline_labels[uiData->debug_inline_element_count];
                int len = ui_prepare_debug_request(uiData, label, DEBUG_ELEMENT_LABEL, uiData->debug_inline_element_count, MAX_DEBUG_ELEMENTS, dest);
                if(len <= 0){
                    printf("INVALID DEBUG ELEMENT PARAMS!");
                    Assert(!"What do we do when this happens?");
                    return false;
                }
                element->data.checkbox.label = dest;
                element->data.checkbox.clicked = false;
                element->data.checkbox.value = &value;
                
                appendedData = push_table(&uiData->label_table, dest, element->data, element->type);

                if(appendedData.checkbox.clicked){
                    *element->data.checkbox.value = !*element->data.checkbox.value;
                    
                }


            }break;
            case ui_element_type_texture:{
                element->data.texture.textureIndex = textureIndex;
                element->data.texture.textureMemory = textureMem;
            }break;

                Assert(!"unsupported type!");
                return nullptr;
        }

        uiData->debug_inline_element_count++;
        element->hashEntry = UIHashElement(uiData, label, element);
        element->parentHash = parentHash;


        if(color){
            element->color = *color;
        }else{
            element->color = {1,.5,.5,1.5};
        }
        parent->data.inlineElement.prevWidth = parent->data.inlineElement.currentWidth;
        parent->data.inlineElement.currentWidth += (element->maxx - element->minx);
        parent->data.inlineElement.latestHeight = element->height;

        return element->hashEntry;

    }



    ui_element* ui_append_inline_slider_float(game_state* GameState, char* label, ui_element_types slider_type, float minx, float maxx, float miny, float maxy, ui_hash_entry* parentHash, ui_element_data& appendedData, float minRange, float maxRange, float& value, float origValue, bool addToRight = true, vec4* color = nullptr, bool* adjusted = 0){ 
        ui_data* uiData = GameState->uiData;

        if(uiData->debug_inline_element_count >= MAX_DEBUG_ELEMENTS){
            printf("TOO MANY DEBUG ELEMENTS! %d FAILURE!\n", uiData->debug_inline_element_count);
            Assert(!"What do we do when this happens?");
            return nullptr;
        }
        
        ui_window* curr_window = uiData->windows + (ui_window_types::window_type_count + uiData->debug_window_count);
        ui_element* element = uiData->debug_inline_elements + uiData->debug_inline_element_count;
        ui_element* parent = parentHash->element;
        parent->data.inlineElement.elementCount++;
        element->type = slider_type;
        
        element->minx = minx;
        element->maxx = maxx;
        element->width = maxx - minx;

        element->miny = miny;
        element->maxy = maxy;
        if(addToRight){
            element->minx += parent->data.inlineElement.currentWidth;
            element->maxx += parent->data.inlineElement.currentWidth;
            element->width = maxx - minx;
            parent->width += element->width;

            parent->data.inlineElement.prevWidth = parent->data.inlineElement.currentWidth;
            parent->data.inlineElement.currentWidth += (element->maxx - element->minx);

        }else{
            element->minx += parent->data.inlineElement.prevWidth;
            element->maxx += parent->data.inlineElement.prevWidth;
            element->width = maxx - minx;
        }

        float height = maxy - miny;
        element->height = height;
        element->scale = {element->width, element->height};

        //need to copy it into another buffer in case we hot reload, otherwise if its a static char we will crash
        char* dest = uiData->debug_inline_labels[uiData->debug_inline_element_count];
        int len = ui_prepare_debug_request(uiData, label, DEBUG_ELEMENT_LABEL, uiData->debug_inline_element_count, MAX_DEBUG_ELEMENTS, dest);
        if(len <= 0){
            printf("INVALID DEBUG ELEMENT PARAMS!");
            return false;
        }
        element->data.debug_slider_float.label = dest;
        element->data.debug_slider_float.visible_range = 0.1f;
        element->data.debug_slider_float.min = minRange;
        element->data.debug_slider_float.max = maxRange;
        element->data.debug_slider_float.norm_value = (value - minRange) / (maxRange - minRange);
        element->data.debug_slider_float.current_value = (&value);
        element->data.debug_slider_float.originalValue = origValue;
        element->data.debug_slider_float.adjusted = adjusted;
        element->hashEntry = UIHashElement(uiData, label, element);
        element->parentHash = parentHash;

        
        // ui_element_data prev_data = push_table(&uiData->label_table, label, element->data, element->type);

        uiData->debug_inline_element_count++;



        if(color){
            element->color = *color;
        }else{
            element->color = {0,1,0,1};
        }
        parent->data.inlineElement.latestHeight = element->height;
        return element;

    }


    











//currently unused dumb bullshit
#if 0 
struct UIVertex {
    float x, y;
    float u, v;
};

// Helper to add a quad to our vertex collection
void addQuad(std::vector<UIVertex>& vertices, 
             float x, float y, float width, float height,
             float u1, float v1, float u2, float v2) {
    vertices.push_back({x,         y,          u1, v1}); // top-left
    vertices.push_back({x + width, y,          u2, v1}); // top-right
    vertices.push_back({x,         y + height, u1, v2}); // bottom-left
    vertices.push_back({x + width, y + height, u2, v2}); // bottom-right
}

void createNineSlicePanel(std::vector<UIVertex>& vertices, 
                         float panelWidth, float panelHeight,
                         int cornerSize,  // size in pixels of corners
                         int textureWidth,  // full texture width
                         int textureHeight) {
    // Convert texture coordinates to UV (0-1) space
    float cornerU = (float)cornerSize / textureWidth;
    float cornerV = (float)cornerSize / textureHeight;
    
    // Top row
    addQuad(vertices,
            0, 0, cornerSize, cornerSize,              // position and size
            0.0f, 0.0f, cornerU, cornerV);            // top-left corner
    
    addQuad(vertices,
            cornerSize, 0, panelWidth - cornerSize*2, cornerSize,  // middle top
            cornerU, 0.0f, 1.0f - cornerU, cornerV);
    
    addQuad(vertices,
            panelWidth - cornerSize, 0, cornerSize, cornerSize,    // top-right
            1.0f - cornerU, 0.0f, 1.0f, cornerV);

    // Middle row
    addQuad(vertices,
            0, cornerSize, cornerSize, panelHeight - cornerSize*2,  // left edge
            0.0f, cornerV, cornerU, 1.0f - cornerV);
    
    addQuad(vertices,
            cornerSize, cornerSize, 
            panelWidth - cornerSize*2, panelHeight - cornerSize*2,  // center
            cornerU, cornerV, 1.0f - cornerU, 1.0f - cornerV);
    
    addQuad(vertices,
            panelWidth - cornerSize, cornerSize,
            cornerSize, panelHeight - cornerSize*2,                 // right edge
            1.0f - cornerU, cornerV, 1.0f, 1.0f - cornerV);

    // Bottom row
    addQuad(vertices,
            0, panelHeight - cornerSize, cornerSize, cornerSize,    // bottom-left
            0.0f, 1.0f - cornerV, cornerU, 1.0f);
    
    addQuad(vertices,
            cornerSize, panelHeight - cornerSize,
            panelWidth - cornerSize*2, cornerSize,                  // bottom edge
            cornerU, 1.0f - cornerV, 1.0f - cornerU, 1.0f);
    
    addQuad(vertices,
            panelWidth - cornerSize, panelHeight - cornerSize,
            cornerSize, cornerSize,                                 // bottom-right
            1.0f - cornerU, 1.0f - cornerV, 1.0f, 1.0f);
}

// Usage example:
void createPanel() {
    std::vector<UIVertex> vertices;
    
    // Create a panel 200x150 pixels, with 16-pixel corners
    // Assuming source texture is 48x48 pixels
    createNineSlicePanel(vertices, 200, 150, 16, 48, 48);
    
    // Create vertex buffer
    bgfx::VertexBufferHandle vbh = bgfx::createVertexBuffer(
        bgfx::makeRef(vertices.data(), vertices.size() * sizeof(UIVertex)),
        UIVertex::layout  // You'll need to define this
    );
}
#endif



//multiple tiles  
#if 0
void createTiledNineSlicePanel(std::vector<UIVertex>& vertices, 
                              float panelWidth, float panelHeight,
                              int cornerSize,       // size of corner pieces
                              int tileSize,         // size of one tile
                              int textureWidth,     // full texture width
                              int textureHeight) {
    // UV coordinates for corners
    float cornerU = (float)cornerSize / textureWidth;
    float cornerV = (float)cornerSize / textureHeight;
    
    // Calculate how many tiles we need
    int numHorizTiles = ceil((panelWidth - cornerSize*2) / tileSize);
    int numVertTiles = ceil((panelHeight - cornerSize*2) / tileSize);
    
    // Top row
    // Left corner
    addQuad(vertices,
            0, 0, cornerSize, cornerSize,
            0.0f, 0.0f, cornerU, cornerV);
            
    // Top edge tiles
    for(int i = 0; i < numHorizTiles; i++) {
        float x = cornerSize + (i * tileSize);
        float width = min(tileSize, panelWidth - cornerSize*2 - i*tileSize);
        
        addQuad(vertices,
                x, 0, width, cornerSize,
                cornerU, 0.0f, cornerU + (width/textureWidth), cornerV);
    }
    
    // Right corner
    addQuad(vertices,
            panelWidth - cornerSize, 0, cornerSize, cornerSize,
            1.0f - cornerU, 0.0f, 1.0f, cornerV);

    // Middle rows
    for(int j = 0; j < numVertTiles; j++) {
        float y = cornerSize + (j * tileSize);
        float height = min(tileSize, panelHeight - cornerSize*2 - j*tileSize);
        
        // Left edge
        addQuad(vertices,
                0, y, cornerSize, height,
                0.0f, cornerV, cornerU, cornerV + (height/textureHeight));
                
        // Center tiles
        for(int i = 0; i < numHorizTiles; i++) {
            float x = cornerSize + (i * tileSize);
            float width = min(tileSize, panelWidth - cornerSize*2 - i*tileSize);
            
            addQuad(vertices,
                    x, y, width, height,
                    cornerU, cornerV, 
                    cornerU + (width/textureWidth), 
                    cornerV + (height/textureHeight));
        }
        
        // Right edge
        addQuad(vertices,
                panelWidth - cornerSize, y, cornerSize, height,
                1.0f - cornerU, cornerV, 1.0f, cornerV + (height/textureHeight));
    }

    // Bottom row
    // Left corner
    addQuad(vertices,
            0, panelHeight - cornerSize, cornerSize, cornerSize,
            0.0f, 1.0f - cornerV, cornerU, 1.0f);
            
    // Bottom edge tiles
    for(int i = 0; i < numHorizTiles; i++) {
        float x = cornerSize + (i * tileSize);
        float width = min(tileSize, panelWidth - cornerSize*2 - i*tileSize);
        
        addQuad(vertices,
                x, panelHeight - cornerSize, width, cornerSize,
                cornerU, 1.0f - cornerV, cornerU + (width/textureWidth), 1.0f);
    }
    
    // Right corner
    addQuad(vertices,
                panelWidth - cornerSize, panelHeight - cornerSize, cornerSize, cornerSize,
                1.0f - cornerU, 1.0f - cornerV, 1.0f, 1.0f);
}
#endif