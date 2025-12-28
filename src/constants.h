#ifndef CONSTANTS_H
#define CONSTANTS_H

#define MAX_ENTITIES 2048
#define MAX_COMPONENTS 32
#define MAX_NAME 16
#define NULL_ENTITY MAX_ENTITIES - 1
#define MAX_CONTACTS 1
#define MAX_STATE_ACTIONS 16

#define COLLISION_HASH 1024
#define HASH_BUCKETS 4
#define MAX_COLLISIONS 1024
#define NULL_CONTACT 15 //the best axes go from 0-14 for the box-box SAT

#define MAX_PLAYERS 16
#define NULL_PLAYER MAX_PLAYERS

#define MAX_LOCAL_PLAYERS 4
#define SNAPSHOT_BUFFER_SIZE 64

#define MAX_ITEMS 32
#define MAX_TRINKETS 32
#define MAX_EQUIPMENT 10
#define MAX_ITEMS_PER_HAND 5
#define MAX_HOTBAR 10
#define MAX_FEEDBACK_ELEMENTS 64

#define SNAPSHOT_BUFFER_SIZE 64
#define ID_BUFFER_SIZE 128
#define MAX_SEND_ATTEMPTS 4

#define MAX_CLIENTS 16
#define MAX_LOCAL_PLAYERS 4
#define MAX_CORES 32
#define MAX_FACES 786432

#define aabb_dirty  0x01  // Bit 0
#define obb_dirty   0x01  // Bit 0
#define pos_dirty   0x01  // Bit 0
#define rot_dirty   0x02  // Bit 1
#define scale_dirty 0x04  // Bit 2
#define collision_resolution 0x08  // Bit 4
#define trans_dirty (pos_dirty | rot_dirty | scale_dirty | collision_resolution)

#define FPT_MAX_CAMERA_ZOOM 655360

//chunk flags
#define chunk_active 0x01
#define chunk_visible 0x02
#define chunk_queued_for_voxelization 0x04
#define has_voxels 0x08
#define is_meshed 0x10
#define unused_chunk_flag1 0x20
#define unused_chunk_flag2 0x40
#define unused_chunk_flag3 0x80

// Set flags
// flags[chunkID] |= chunk_active;    // Turn on just that bit
// flags[chunkID] &= ~chunk_visible;  // Turn off just that bit

// Check flags
// if(flags[chunkID] & has_voxels) {  // Is that bit set?
    // ...
// }


#define NULL_CHUNK UINT32_MAX

#define FACE_MAX 786432



#define MAX_VERTICES 8192
#define MAX_INDICES 24576 // 8192 * 3
#define MAX_LINE_INDICES 49152 // 8192 * 6
#define MAX_MESHES 256
#define MAX_NAME_LENGTH 256
#define NULL_MESH 255 //last mesh will be uninitialized/all 0

#define MAX_HIGH_RES 32
#define MAX_MID_RES 128
#define MAX_LOW_RES 512

// #define MAX_CHUNKS   1024
#define MAX_CHUNKS   512
#define DEBUG_CHUNKS 16
//7*7*7 = 343
#define MAX_CHUNK_DRAW_DISTANCE 343

#define CHUNK_SIZE 62
#define VOX_MAX_INDEX 63
#define INVERSE_CHUNK_SIZE 0.01612903225
#define HALF_CHUNK_SIZE 31
#define INVERSE_HALF_CHUNK_SIZE 0.03225806451

#define FPT_CHUNK_SIZE 4063232
#define FPT_INVERSE_CHUNK_SIZE 1057
#define FPT_HALF_CHUNK_SIZE 2031616
#define FPT_INVERSE_HALF_CHUNK_SIZE 2114

#define FPT_CHUNK_SIZE_PADDED 4194304
#define FPT_HALF_CHUNK_SIZE_PADDED 2097152
#define FPT_INVERSE_CHUNK_SIZE_PADDED 1024
#define FPT_INVERSE_HALF_CHUNK_SIZE_PADDED 2048


#define FPT_VOXEL_SCALE 65536
#define FPT_HALF_VOXEL_SCALE 32768

#define TOTAL_VOXELS   64
#define PADDED_VOXELS  63
#define USABLE_VOXELS  62

// #define FPT_MID_VOXEL_SCALE 32768
// #define FPT_HIGH_VOXEL_SCALE 16384

// #define CHUNK_SIZE 64
// #define INVERSE_CHUNK_SIZE 0.015625
// #define HALF_CHUNK_SIZE 32
// #define INVERSE_HALF_CHUNK_SIZE 0.03125

// #define FPT_CHUNK_SIZE 4194304
// #define FPT_INVERSE_CHUNK_SIZE 1024
// #define FPT_HALF_CHUNK_SIZE 2097152
// #define FPT_INVERSE_HALF_CHUNK_SIZE 2048

// #define FPT_VOXEL_SCALE 67650
// #define FPT_MID_VOXEL_SCALE 33825
// #define FPT_HIGH_VOXEL_SCALE 16912




#define LOW_VOXEL_SCALE 1.0f
#define MID_VOXEL_SCALE 0.5f
#define HIGH_VOXEL_SCALE 0.25f

#define TWOPI 6.28318530718f
#define PI 3.14159265358979323846

#define HIGH_RES_BRICKMAPS 64
#define MID_RES_BRICKMAPS 8
#define LOW_RES_BRICKMAPS 1

#define MAX_MESSAGES 16
#define MAX_MSG_LEN 99

#define TEXTILESIZE 64 //for 512x512 perlin noise textures

// Regular enum
enum MeshTypes {                                //layouts:
    mesh_cube = 0, //cube mesh is the default   //LineVertex (vec3 pos, u32 color)
    mesh_quad,                                  //Vertex     (vec3 pos, vec3 color, vec2 texCoord)
    mesh_screenQuad,                            //test_vertex(vec3 pos, vec3 color)
    mesh_Axes,                                  //LineVertex (vec3 pos, u32 color)
    mesh_ColorCube,
    mesh_WhiteCube,
    mesh_RedCube,
    mesh_Frustum,
    mesh_uiBorder,                              //TextVertex (vec3 pos, vec2 texCoord)
    mesh_uiBox,                                 //TextVertex (vec3 pos, vec2 texCoord)
    mesh_Sphere,
    mesh_RedSphere,
    mesh_Capsule,
    mesh_Hemisphere,
    mesh_ForwardWidget,
    mesh_UpWidget,
    mesh_RightWidget,
    mesh_DirectionWidgets,
    mesh_ThickLine,
    mesh_ray,
    mesh_skybox,

    mesh_count,
};

enum TopologyTypes{
    topology_lines,
    topology_triangles,
};

enum WINDOW_VIEWS{
    worldSpace = 0,
    imgui,
    ui,
    offscreen,
};
//eventually we should move to uniforms/instancing for same object/different instances/different colors

enum DynamicMeshTypes{
    dyn_mesh_none = 0,
    dyn_mesh_Line,
    dyn_mesh_ChunkSystemRay,
    dyn_mesh_CameraFrustum,
};

enum model_type{
    model_none = 0,
    model_humanoid,
    model_sword,
    model_shield,
};

enum animation_type{
    anim_none = 0,
    
    anim_idle,
    anim_walk,
    anim_run,
    anim_jump,
    anim_airborn,
    anim_land,
    anim_attack,
    anim_cast,
    anim_left_punch,
    anim_right_punch,
    anim_left_light_weapon,
    anim_right_light_weapon,
    anim_left_kick,
    anim_right_kick,
    anim_left_block,
    anim_right_block,
    anim_pull_bow,
    anim_release_bow,
    anim_dodge_roll,


    anim_count,
};


enum entity_body_parts{
    entity_part_none = 0,

    humanoid_part_head,
    humanoid_part_torso,
    humanoid_part_right_upper_arm,
    humanoid_part_right_lower_arm,
    humanoid_part_left_upper_arm,
    humanoid_part_left_lower_arm,
    humanoid_part_right_upper_leg,
    humanoid_part_right_lower_leg,
    humanoid_part_left_upper_leg,
    humanoid_part_left_lower_leg,

    entity_part_count,

};



enum entity_bone_types{
    entity_bone_none = 0,

    humanoid_bone_head,
    humanoid_bone_torso,
    humanoid_bone_right_upper_arm,
    humanoid_bone_right_lower_arm,
    humanoid_bone_left_upper_arm,
    humanoid_bone_left_lower_arm,
    humanoid_bone_right_upper_leg,
    humanoid_bone_right_lower_leg,
    humanoid_bone_left_upper_leg,
    humanoid_bone_left_lower_leg,

    entity_bone_count,

};

enum equipment_socket_type{
    equip_socket_head,
    equip_socket_torso,
    
    equip_socket_left_fore_arm,
    equip_socket_right_fore_arm,
    equip_socket_left_upper_arm,
    equip_socket_right_upper_arm,
    equip_socket_left_thigh,
    equip_socket_right_thigh,
    equip_socket_left_shin,
    equip_socket_right_shin,
};

//brainstorming slop, don't have it tied to anything yet

enum inventory_item_modifier_type{
    item_mod_type_none,

    item_mod_type_magic,
    item_mod_type_poison,
    item_mod_type_fire,
    item_mod_type_lightning,
    item_mod_type_frost,
    item_mod_type_enchanted,
    item_mod_type_custom,

};


enum tool_types{
    tool_type_none = 0,
    tool_type_sword,
    tool_type_shield,
    tool_type_bow,
    tool_type_book,
    tool_type_unique,
};

enum equipment_types{
    equipment_type_none = 0,
    equipment_type_helmet,
    equipment_type_torso,
    equipment_type_upper_arm,
    equipment_type_upper_leg,
    equipment_type_lower_arm,
    equipment_type_lower_leg,
};

enum trinket_types{
    trinket_type_none = 0,
    trinket_type_amulet,
    trinket_type_ring,
    trinket_type_bracelet,
    trinket_type_artifact,
};

enum item_types{
    item_type_none = 0,
    item_type_tool,
    item_type_equipment,
    item_type_trinket,
    item_type_voxel,
    item_type_consumable,

};

#include "math.h"


#define MAX_SSBO_ENTITIES 2048

#define MAX_TEXT_CHARS 32768

struct PerEntitySSBO{
    mat4 model;
    vec4 color;
};

struct skeletal_mesh_command{
    uint32_t indexOffset;
    uint32_t indexCount;
    mat4 model;
    vec4 color;
};

struct entity_draw_command_buffer{
    MeshTypes mesh_type;
    TopologyTypes topology_type;
};

struct ScreenPushConstants {
    vec2 position; 
    vec2 scale;    
    vec4 color;    
    vec4 texCoords;    
    uvec4 misc;//x is a flag to draw the texture or not
    //misc x = draw texture flag
    //misc y = shape to draw for ui element. 6 is text
    //misc z = texture index, 4 is for standard text, 5 is for monospaced text
    //misc w = unused
    vec4 misc2;
    //misc2 z = z depth, vestigial
};


//used as the type to send to the push constant to be processed in the fragment shader
enum ScreenUITypes{
    ui_box,
    ui_border,
    ui_border_box,
    ui_rippleborder,
    ui_rippleborder_box,
};  



struct ScreenDrawCommands {
    ScreenPushConstants push;
    uint8_t center_text;
    vec4 scissor;    
    uvec4 misc;         //x = 1 is highlight mode, y = highlight start, z = highlight length, w = unused
    vec4 miscColor;     //highlight color
};

struct WindowsAlignedQuad{
    float x0;//pos
    float y0;
    float x1;
    float y1;
    float s0;//texture
    float t0;
    float s1;
    float t1;
};


struct WindowsCharData{
    float x0;
    float y0;
    float x1;
    float y1;
    float s0;
    float t0;
    float s1;
    float t1;
    float xadvance;
    float xoff;
    float yoff;
    int width;
    int height;
};



struct font_data{
    uint32_t Bitmap[1024 * 1024];
    WindowsCharData windowsCharData[96];
    int32_t fontKerningTable[96*96];
    float ascent;
    float descent;
    float doubleDescent;
    float lineGap;
    float scale;
    float lineAdvance;
    float maxCharWidth;
};



struct WorldTextPushConstants{
    mat4 model;
    vec4 color;
    float scale;
};

struct FaceData {
    uint32_t faceInfo1;  // x,y,z,width,height,flipped
    uint32_t faceInfo2; // ao,type
};

//voxel drawing commands
struct chunk_voxel_ssbo_entry{
    uint32_t ssboIndex;
    uint32_t faceCount;
};
struct chunk_create_command{
    uint32_t chunkID;
    FaceData* faceMemory;
    uint32_t faceCount;
    bool edit;
    uint32_t faceMemoryIndex;
};
struct texture_update_command{
    uint32_t  textureID;
    uint32_t* textureMemory;
    uint32_t  textureWidth;
    uint32_t  textureHeight;
};
struct chunk_destroy_command{
    uint32_t chunkID;
};
struct chunk_voxel_draw_command{
    uint32_t chunkID;
    mat4 model;
};

struct Vertex{
    vec3 pos;
    vec3 color;
    vec2 texCoord;
};

struct TextVertex {
    vec3 pos;    // Position
    vec2 texCoord;       // Texture coordinates
};


//global struct that contains all draw commands from game layer to platform layer/renderer
struct render_command_data{

    PerEntitySSBO entityDrawCommandsSSBO[MAX_SSBO_ENTITIES];
    entity_draw_command_buffer entityDrawCommands[MAX_SSBO_ENTITIES];
    size_t entityDrawCount;
    
    //skeletal mesh commands
    skeletal_mesh_command skeletalMeshCommands[MAX_SSBO_ENTITIES];
    uint32_t skelMeshDrawCount;

    //offscreen skeletal mesh commands
    skeletal_mesh_command offscreenSkeletalMeshCommands[MAX_SSBO_ENTITIES];
    uint32_t offcreenSkelMeshDrawCount;
    //end skeletal mesh commands

    ScreenDrawCommands screenElementDrawCommands[MAX_SSBO_ENTITIES];
    size_t screenElementDrawCount;

    
    WorldTextPushConstants worldTextDrawCommandsSSBO[MAX_SSBO_ENTITIES];
    size_t worldTextDrawCount;
    TextVertex world_transient_text[MAX_TEXT_CHARS];
    char   world_transient_char_buffer[MAX_TEXT_CHARS];
    uint32_t    world_transient_char_count;
    uint32_t    world_transient_char_offsets[MAX_SSBO_ENTITIES];
    float  world_transient_char_widths [MAX_SSBO_ENTITIES];
    float  world_transient_char_heights[MAX_SSBO_ENTITIES];
    size_t world_transient_text_entry_vertex_count[MAX_SSBO_ENTITIES];
    size_t world_transient_text_entry_offset[MAX_SSBO_ENTITIES];
    size_t world_transient_text_entries;
    size_t world_transient_text_vertex_count;

    TextVertex screen_transient_text[MAX_TEXT_CHARS];
    char   screen_transient_char_buffer[MAX_TEXT_CHARS];
    uint32_t    screen_transient_char_count;
    uint32_t    screen_transient_char_offsets[MAX_SSBO_ENTITIES];
    float  screen_transient_char_widths [MAX_SSBO_ENTITIES];
    float  screen_transient_char_heights[MAX_SSBO_ENTITIES];
    size_t screen_transient_text_entry_vertex_count[MAX_TEXT_CHARS];
    size_t screen_transient_text_entry_offset[MAX_TEXT_CHARS];
    size_t screen_transient_text_entries;
    size_t screen_transient_text_vertex_count;
    ScreenDrawCommands screenTextDrawCommands[MAX_SSBO_ENTITIES];
    size_t screenTextDrawCount;


    chunk_create_command        chunkCreateCommands     [MAX_CORES];
    chunk_destroy_command       chunkDestroyCommands    [MAX_CHUNK_DRAW_DISTANCE];
    chunk_voxel_draw_command    chunkDrawCommands       [MAX_CHUNK_DRAW_DISTANCE];
    uint32_t chunkCreateCommandCount;
    uint32_t chunkDestroyCommandCount;
    uint32_t chunkDrawCommandCount;


    texture_update_command screenSpaceTextureUpdateCommands[MAX_CORES];
    uint32_t screenSpaceTextureUpdateCommandCount;


    font_data monospacedScreenFont;
    font_data defaultScreenFont;

    //test textures for heightmaps
    uint32_t thorns[128*128];
    uint32_t cracked[64*64];

    uvec3                      chunk_draw_distance; //3, 5, or 7 cubed chunks centered at the camera.

};

struct user_input{
    uint32_t transitionCount;
    uint32_t endedDown;
};

enum InputTypes{
            //keys

            input_mouse_left ,
            input_mouse_middle ,
            input_mouse_right ,
            input_mouse_sideFront,
            input_mouse_sideBack ,
        
            input_mouse_consumed_left    ,
            input_mouse_consumed_middle    ,
            input_mouse_consumed_right ,
            input_mouse_consumed_sideFront ,
            input_mouse_consumed_sideBack,
            input_mouse_consumed_delta ,
            input_mouse_consumed_wheel ,

            input_key_up        ,
            input_key_down      ,
            input_key_left      ,
            input_key_right     ,
            input_key_forward   ,
            input_key_back      ,
            input_key_attack    ,
            input_key_jump      ,
            input_key_interact  ,
        

            input_key_0,
            input_key_1,
            input_key_2,
            input_key_3,

            input_key_4 ,
            input_key_5 ,
            input_key_6 ,
            input_key_7 ,
            
            input_key_8 ,
            input_key_9 ,
            
            input_key_alt    ,
            input_key_equals ,
            input_key_minus  ,
            input_key_Q      ,
            input_key_escape ,
            input_key_tab    ,
            input_key_ctrl   ,
            input_key_shift  ,
            input_key_return  ,
            input_key_arrow_up  ,
            input_key_arrow_down  ,
            input_key_arrow_left  ,
            input_key_arrow_right  ,
            input_key_arrow_pageUp  ,
            input_key_arrow_pageDown  ,
            input_key_arrow_home  ,
            input_key_arrow_end  ,


    input_count,
};

enum VoxelTypes{
    vox_none = 0,   //0
    vox_dirt,       //1
    vox_stone,      //2
    vox_grass,      //3
    vox_tree,       //4
    vox_leaves,     //5
    vox_tin,        //6
    vox_copper,     //7
};

#if 0
#define PRINT(format, ...) printf(format, ##__VA_ARGS__)
#else
#define PRINT(format, ...) 
#endif


//may want to define NO_COLOR flag for compilers that cant handle this
//#ifdef NO_COLOR
//#define COLOR ""
//etc...


// Text styles
#define BOLD        "\033[1m"
#define DIM         "\033[2m"
#define ITALIC      "\033[3m"    // Not widely supported
#define UNDERLINE   "\033[4m"
#define BLINK       "\033[5m"    // Not widely supported
#define REVERSE     "\033[7m"    // Inverts fg/bg colors
#define HIDDEN      "\033[8m"
#define STRIKE      "\033[9m"    // Not widely supported

// Regular colors
#define BLACK       "\033[30m"
#define RED         "\033[31m"
#define GREEN       "\033[32m"
#define YELLOW      "\033[33m"
#define BLUE        "\033[34m"
#define MAGENTA     "\033[35m"
#define CYAN        "\033[36m"
#define WHITE       "\033[37m"
#define DEFAULT     "\033[39m"

// Bold/bright colors
#define BBLACK      "\033[1;30m"
#define BRED        "\033[1;31m"
#define BGREEN      "\033[1;32m"
#define BYELLOW     "\033[1;33m"
#define BBLUE       "\033[1;34m"
#define BMAGENTA    "\033[1;35m"
#define BCYAN       "\033[1;36m"
#define BWHITE      "\033[1;37m"

// Background colors
#define BG_BLACK    "\033[40m"
#define BG_RED      "\033[41m"
#define BG_GREEN    "\033[42m"
#define BG_YELLOW   "\033[43m"
#define BG_BLUE     "\033[44m"
#define BG_MAGENTA  "\033[45m"
#define BG_CYAN     "\033[46m"
#define BG_WHITE    "\033[47m"
#define BG_DEFAULT  "\033[49m"

#define RESET       "\033[0m"

#endif //CONSTANTS_H