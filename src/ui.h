#pragma once
#include "constants.h"


#define MIN_WINDOW_DIMENSIONS 20
#define ITEM_SLOT_COUNT 32
#define MAX_PAGE_COUNT 32
#define MAX_PAGE_CHARS 2048
#define MAX_BOOK_SLOTS 32
#define MAX_STAT_SLOTS 32
#define MAX_HOTBAR_SLOTS 10
#define MAX_WINDOWS 20
#define MAX_UI_HASH_SLOTS (1024 * 64)
#define MAX_UI_HASH_BUCKETS 4
#define MAX_DEBUG_ELEMENTS 1024
#define DEBUG_ELEMENT_LABEL 256
#define MAX_DEBUG_WINDOWS 8

#define LABEL_TABLE_ENTRIES 1024
#define LABEL_TABLE_BUCKETS 4

struct ui_element; //forward declaration

enum ui_state_types{
    ui_state_none,
    
    ui_state_resize_window,
    ui_state_drag_window,
    ui_state_drag_item,
    ui_state_drag_slider,
    ui_state_chat_input,
    ui_state_text_editor_input,
};


enum ui_item_types{
    ui_item_type_none = 0,
    ui_item_type_sword,
    ui_item_type_shield,
    ui_item_type_helm,
};

enum ui_element_inventory_types{
    ui_element_inventory_type_none,

    ui_element_inventory_type_item_slot,
    ui_element_inventory_type_trinket_slot,
    ui_element_inventory_type_equipment_slot,
    ui_element_inventory_type_hotbar_slot,

    //for easier labelling in renderSystem
    ui_element_inventory_type_left_hotbar_slot,
    ui_element_inventory_type_right_hotbar_slot,
};

enum ui_element_types{
    ui_element_type_none,

    
    ui_element_type_bounds,
    ui_element_type_resize,
    ui_element_type_drag,
    ui_element_type_checkbox,
    ui_element_type_window_base,
    ui_element_type_text,
    ui_element_type_text_highlight,
    ui_element_type_slider_float,
    ui_element_type_slider_int,
    ui_element_type_button,

    ui_element_type_editable_text,


    //book functionality
    ui_element_type_left_book_page,
    ui_element_type_right_book_page,
    ui_element_type_right_arrow,
    ui_element_type_left_arrow,

    ui_element_type_inventory_slot,



    ui_element_type_horizontal_scrollbar,
    ui_element_type_vertical_scrollbar,

    ui_element_type_bar_graph,
    ui_element_type_bar,
    ui_element_type_inline,
    ui_element_type_texture,
    ui_element_type_color,
    ui_element_type_horizontal_slider_float,
    ui_element_type_vertical_slider_float,
};

enum bargraph_data_types{
    bargraph_profile_event,
    bargraph_profile_history_snapshot,
};

struct bargraph_data{
    union{
        u64 eventClock;
        u32 eventIndex;
    };
};

struct bar_data{
    float minx;
    float miny;
    float maxx; //used as the scale for each horizontal element in a bar graph
    float maxy;
    
    bargraph_data_types type;
    bargraph_data data;

};

struct slider_float_data{
    float min;
    float max;
    float current_value;
    float visible_range;
    float* memory;
};

struct slider_int_data{
    int min;
    int max;
    int current_value;
    int visible_range;
    int* memory;
};

struct bounds_data          {


};

struct resize_data          {


};

struct drag_data            {


};

struct text_data        {
    char* label;
    u32 highlightStart;
    u32 highlightLen;
};

struct checkbox_data        {
    char* label;
    bool* value;
    bool clicked;
    u32 hash;
    u32 bucket;
};

struct button_data          {
    char* label;
    bool clicked;
    bool rightClicked;
    bool* value;
    u32 hash;
    u32 bucket;
};

struct left_book_page_data  {


};

struct right_book_page_data {


};

struct right_arrow_data     {


};

struct left_arrow_data      {


};

struct bar_graph{
    char* label;
    float normalizedValue;
    float currentValue;//to append to
    ui_element* start; //array of bar elements
    u32 elementCount;
};

struct inline_elements{
    ui_element* start;
    ui_element* selected_element;
    u32 elementCount;
    float currentWidth; //used to figure out where to append the next element to
    float latestHeight;//to slot elements beneath, used if not appending to the right
    float prevWidth;//to slot elements beneath, used if not appending to the right
};

struct inventory_slot_data     {
    ui_element_inventory_types inventory_type;
    ui_item_types item_type;
};

struct ui_texture{
    u32 textureIndex;
    u32* textureMemory;
};


struct ui_color{
    vec4 color;
};

struct debug_slider_float_data{
    float min;
    float max;
    float* current_value;
    float norm_value;
    float visible_range;
    float* memory;
    char* label;
    bool clicked;
    float originalValue;
    bool* adjusted;
};

union ui_element_data{
    slider_float_data slider_float;
    slider_int_data slider_int;

    debug_slider_float_data debug_slider_float;
    
    inventory_slot_data inventory_slot;
    
    text_data text;
    checkbox_data checkbox;
    button_data button;
    
    bar_graph barGraph;
    bar_data bar;

    inline_elements inlineElement;
    ui_texture texture;
    ui_color color;
};


struct ui_label_table_entry{
    char* name;
    
    ui_element_data data;
    ui_element_types type;

};

struct ui_label_table{
    ui_label_table_entry entries[LABEL_TABLE_ENTRIES][LABEL_TABLE_BUCKETS]; 
    int entry_count[LABEL_TABLE_ENTRIES];
    int total_entry_count;
    char labels[LABEL_TABLE_ENTRIES][LABEL_TABLE_BUCKETS][DEBUG_ELEMENT_LABEL];
};


struct ui_hash_entry{
    ui_element* element;
    char* label;
    u32 versionID;
    ui_element_types prevType;    
};


struct ui_element{
    ui_element_types type;
    ui_element_data data;
    ui_hash_entry* hashEntry;
    u32 hash;
    ui_hash_entry* parentHash;

    float parentWidth;
    float parentHeight;
    float normPosX; // 0 to 1
    float normPosY;
    float baseScale;

    float posx;
    float posy; //top left corner
    float windowScale;
    float ratio;
    float scalex;
    float scaley;
    vec4 color;
    vec4 uvCoords;
    vec2 scale;

    float minx;
    float miny;
    float maxx;
    float maxy;

    float width;
    float height;

    float pixelWidth;
    float pixelHeight;

    bool hovered;
    bool clicked;
    bool active;
    bool widget_selected;
    bool dragged;

};


enum ui_window_types{
    window_type_none                ,
    window_type_chat                ,
    window_type_inventory_hotbar    ,
    window_type_inventory           ,
    window_type_inventory_other     ,
    window_type_inventory_storage   ,
    window_type_book_text           ,
    window_type_book_cover          ,
    window_type_stats               ,
    window_type_feedback            ,
    window_type_debug               ,




    window_type_count               ,
    //everything at count and up is a debug window

};

// enum window_region_type{
//     region_type_none = 0,
//     region_type_equipment,
//     region_type_trinket,
//     region_type_inventory_items,
//     region_type_hotbar,
//     region_type_text_left_page,
//     region_type_text_right_page,
//     region_type_chat,
// };

// union window_region_data{
//     struct equipment_data{
//         ui_element body_slots[entity_body_parts::entity_part_count];

//     }equipment;
// };

union ui_window_data{
    struct inventory_data{
        ui_element equipment_slot_bounds;
        ui_element body_slots[MAX_EQUIPMENT];
        
        ui_element trinket_slot_bounds;
        ui_element trinket_slots[ITEM_SLOT_COUNT];
    
        ui_element inventory_slot_bounds;
        ui_element item_slots[ITEM_SLOT_COUNT];

        ui_element hotbar_bounds;
        ui_element hotbar_slots[MAX_HOTBAR_SLOTS];
    }inventory;

    struct inventory_hotbar_data{
        ui_element hotbar_bounds;
        ui_element hotbar_slots[MAX_HOTBAR_SLOTS];
    }inventory_hotbar;
    
    struct chat_data{
        ui_element base;  // inherit basic UI properties
        char input_buffer[MAX_MSG_LEN];
        vec2 cursor_positions[MAX_MSG_LEN];
        size_t newLines[MAX_MSG_LEN];

        char messages[MAX_MESSAGES][MAX_MSG_LEN];

        TextInputState text_input;

        int message_count;
        bool input_focused;
    }chat;

    struct feedback_data{
        ui_element elements[MAX_FEEDBACK_ELEMENTS];
        char text_labels[MAX_FEEDBACK_ELEMENTS][DEBUG_ELEMENT_LABEL];

        int element_count;
        int write_slot;
        int last_frame_element_count;

    }feedback;

    struct book_text_data{
        ui_element left_page    ;
        ui_element right_page   ;

        ui_element top_border;
        ui_element bottom_border;
        ui_element left_border;
        ui_element right_border;
    
        char chars[1024];
        vec2 cursor_positions[1024];
        size_t newLines[1024];

        TextInputState text_input;

        TextInputState text_input_history[64];
        u32 text_input_history_max;
        u32 text_input_history_index;
        u32 text_input_history_size;

        ui_element left_horizontal_scrollbar;
        ui_element left_vertical_scrollbar;
        
        float horizontal_offset;
        float vertical_offset;



        int page_index;
        int page_count;
    
        char left_page_text     [MAX_PAGE_COUNT][MAX_PAGE_CHARS];
        char right_page_text    [MAX_PAGE_COUNT][MAX_PAGE_CHARS];
    
    }book_text;

    struct book_cover_data{
        ui_element slots[MAX_BOOK_SLOTS];
    }book_cover;
    
    struct stats_data{
        ui_element slots[MAX_STAT_SLOTS];
    }stats;

    struct debug_data{
        ui_element* elements;
        int element_count;
        int last_frame_element_count;
    }debug;
};




struct ui_window{
    ui_window_types type;
    ui_window_data  data;
    bool fixed;
    //base window data
    ui_element      base;
    ui_element      original;

    ui_element* selected_element;
    float selected_element_rootx;
    float selected_element_rooty;

    //the broader bounds encapsulating whichever selected element
    ui_element* selected_bounds;
    float selected_bounds_rootx;
    float selected_bounds_rooty;

    b32 dockable;
    ui_element vertical_scrollbar;
    float* vertical_scroll_height;//dimension to send to the scrollbar adjust function

    ui_element horizontal_scrollbar;
    float* horizontal_scroll_width;//dimension to send to the scrollbar adjust function
    float largestElementWidth;

    b32 horizontal_scrollable;
    b32 visible;
    b32 docked;
    b32 dock_widget_selected;
    bool interactable; //for tooltip/transient windows we dont want the mouse to get consumed by
    
    ui_window* docked_to;
    ui_window* docked_child;

    b32 resize_widget_selected;
};

struct ui_data{
    ui_state_types current_state;
    ui_state_types previous_state;

    b32 display_UI_window;

    ui_element* selected_element;
    ui_element* previously_selected_element;
    ui_element* previously_selected_parent_element;
    u32 previously_selected_element_index;//for bargraph elements
    ui_element* clicked_element;
    ui_element* dragging_item;
    ui_element  dragging_item_memory;
    ui_element* dragging_item_original_slot;
    u32         dragged_itemID;
    u32*        dragged_item_original_slot;
    u32         hovered_itemID;
    u32*        hovered_item_slot;
    ui_element_inventory_types hovered_item_slot_type;
    float       dragged_item_scale_x;
    float       dragged_item_scale_y;
    
    ui_element* selected_bar_element;
    ui_element* selected_bargraph_element;
    ui_element** selected_inline_element;
    
    bool inline_element_selected;
    ui_hash_entry*  selected_inline_element_hash;
    ui_hash_entry*  selected_inline_element_parent_hash;
    vec2 inline_root;
    vec4 inline_scissor;


    bool debug_element_selected;
    float debug_scrolly_offset;
    float debug_scrollx_offset;
    ui_element  selected_debug_element_memory;
    ui_element  selected_debug_element_window_base_memory;
    ui_element* selected_debug_element_parent;
    vec2 debug_root;
    vec4 debug_scissor;

    bool mouse_intersects_slider;

    bool mouse_intersects_element;

    ui_element* dragging_slider;

    u32 hovered_debug_hash;
    u32 clicked_debug_hash;
    ui_element* hovered_debug_element;
    ui_element* clicked_debug_element;

    ui_window* selected_window;
    ui_element* selected_window_bounds;

    bool checkboxtest1;
    bool checkboxtest2;
    bool checkboxtest3;
    bool checkboxtest4;

    ui_window windows[MAX_WINDOWS];

    int debug_window_count;
    int last_frame_debug_window_count;
    char debug_window_labels[MAX_DEBUG_WINDOWS][DEBUG_ELEMENT_LABEL];
    ui_element debug_elements[MAX_DEBUG_ELEMENTS];
    ui_element debug_bargraph_elements[MAX_DEBUG_ELEMENTS];
    ui_element debug_inline_elements[MAX_DEBUG_ELEMENTS];
    char debug_element_labels[MAX_DEBUG_ELEMENTS][DEBUG_ELEMENT_LABEL];
    char debug_bargraph_labels[MAX_DEBUG_ELEMENTS][DEBUG_ELEMENT_LABEL];
    char debug_inline_labels[MAX_DEBUG_ELEMENTS][DEBUG_ELEMENT_LABEL];
    int last_frame_debug_element_count;
    int debug_element_count;
    int debug_bargraph_element_count;
    int debug_inline_element_count;
    
    char tooltipText[32];
    int tooltipTextLen;

    ui_label_table label_table;

    ui_element information_panel; //pops up to provide details on the current selected/hovered element
    char information_panel_text[128];//total slop just to print something to the info panel when an element is hovered
    int information_panel_size;
    ui_element* hovered_element;
    ui_window* hovered_window;

    bool freezeTooltipWindow;
    vec2 toolTipPos;

    ui_hash_entry uiHash[MAX_UI_HASH_SLOTS][MAX_UI_HASH_BUCKETS];
    u8 uiHashBucketCount[MAX_UI_HASH_SLOTS];
    u32 curVersionID;
};




    void ui_resizeElements(game_state* GameState);
    void ui_start(game_state* GameState);
    void ui_update(game_state* GameState);
    void enable_network_buttons(game_state* GameState);
    void disable_network_buttons(game_state* GameState);
    void ui_process_input(game_state* GameState, player_input& prevInput, player_input& currInput);

    void ui_text            (game_state* GameState, const char* fmt, ...);
    bool ui_checkbox        (game_state* GameState, bool& value, const char* fmt, ...);
    bool ui_set_button       (game_state* GameState, const char* fmt, ...);
    void ui_flush_debug     (game_state* GameState);
    const char* ui_state_to_str(ui_state_types state);

    ui_element* ui_begin_barGraph(game_state* GameState, float normalizedValue, const char* fmt, ...);
    void ui_append_barGraph(game_state* GameState, float value, vec4* color = 0);
    void ui_end_barGraph(game_state* GameState);


static inline const char* entity_body_parts_to_string(entity_body_parts type){
    switch(type){
        case humanoid_part_head: return "head";
        case humanoid_part_torso: return "torso";
        case humanoid_part_right_upper_arm: return "R_up_arm";
        case humanoid_part_right_lower_arm: return "R_lo_arm";
        case humanoid_part_left_upper_arm: return " L_up_arm";
        case humanoid_part_left_lower_arm: return " L_lo_arm";
        case humanoid_part_right_upper_leg: return "R_up_leg";
        case humanoid_part_right_lower_leg: return "R_lo_leg";
        case humanoid_part_left_upper_leg: return " L_up_leg";
        case humanoid_part_left_lower_leg: return " L_lo_leg";
        default: return "NONE??";
    }

};

