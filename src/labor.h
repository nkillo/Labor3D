#if !defined(LABOR_H)

#include "labor_platform.h"



struct transient_state
{
    b32 IsInitialized;
    memory_arena TranArena;
};


enum PerlinDemoEnum{
    demo_basic2d = 0,
    demo_valueNoise,
    demo_fbm,
    demo_domainWarpedValueNoise,
    demo_flowArrows,
    demo_flow,
    demo_heightmap,
    demo_coarse3d
};

struct game_state
{
    network_state* NetworkState;
    player_input playerInputs[MAX_LOCAL_PLAYERS][SNAPSHOT_BUFFER_SIZE];
    // SDL_GameController* controllers[MAX_LOCAL_PLAYERS];
    uint32_t connected_controller_count;
    memory_arena WorldArena;
    game_memory* GameMemory;
    chunk_data* chunkData;
    physics_data* physicsData;
    ui_data* uiData;
    Camera* camera;
    dispatcher* dispatcher;
    shader_data* shaderData;
    mesh_data* meshData;
    gltf_data* gltfData;
    game_input* input;
    render_command_data* RenderCommandData;
    // SDL_Window* window;
    EntityComponent* entityComponent;
    EntityComponent* ecBuffer[SNAPSHOT_BUFFER_SIZE];
    size_t PlatformMemorySize;
    // FastNoise::Generator** noiseGenerators;
    uint32_t generatorCount;
    uint32_t threadCount;

    platform_work_queue* Queue;
    platform_add_entry* PlatformAddEntry;
    platform_complete_all_work* PlatformCompleteAllWork;
    platform_check_all_work_completed* PlatformCheckAllWorkCompleted;
    // GenNoiseFunc *PlatformGenerateNoise;

    Platform_fpt_asin* platform_fpt_asin;

    float last_tick;
    float deltaTime;
    float totalTime;
    float fixedTimeStep; // 1.0f / 60.0f // 60 updates per second
    uint32_t fixed_tick_rate; //60 ticks per second
    fpt   fptFixedTimeStep;
    float accumulator;
    int frameNumber = 0;
    bool showBgfxDebugOverlay = false;
    uint32_t selectedEntity = 0;

    //for fixed update/input access/simulation steps
    uint16_t currentTick;
    uint16_t tick;

    //imgui bools
    bool sceneWindow;
    bool chunkWindow;
    bool renderWindow;
    bool networkWindow;
    bool enableAutoMoveSystem;
    char projectPath[256];
    char assetsPath[256];

    char clipboard[256];
    u32 clipboardSize;
    TextInputState* text_dest;
    TextInputState* chat_text_dest;

    float currentUIDepth;   //didnt work, may be useful later
    float UIDepthIncrement; //didnt work, may be useful later

    bool updateSoftwareRenderer;
    uint32_t mainWindowID;

    bool pauseAnimTest;


#ifndef SERVER_BUILD
    process_client_command* PlatformProcessClientCommand;
    // push_player_input_to_network* PlatformPushPlayerInputToNetwork;
    start_listen_server* PlatformStartListenServer;
    start_client* PlatformStartClient;

#endif
    uint32_t localPlayerEntityIDs[MAX_LOCAL_PLAYERS];
    uint32_t local_player_count;
    uint32_t nonLocalPlayerEntityIDs[MAX_CLIENTS][MAX_LOCAL_PLAYERS];
    uint32_t totalPlayers;
    // push_entity_state_to_network* PlatformPushEntityStateToNetwork;

    // append_to_packet*      platform_append_to_packet;
    // append_to_all_packets* platform_append_to_all_packets;
    // send_packet*           platform_send_packet;
    // send_all_packets*      platform_send_all_packets;


    //time tracking experiment
    uint32_t last_second_time;
    uint32_t ticks_this_second;

    bool draw_aabb;
    bool draw_obb;
    bool draw_obbaabb;
    bool draw_bvh;

    
    float noiseResults[TEXTILESIZE * TEXTILESIZE];
    
    float particleX     [512];
    float particleY     [512];
    u8 particleR        [512];
    u8 particleG        [512];
    u8 particleB        [512];
    bool particleGrow   [512];
    float particleRad   [512];
    float particleSpeed [512];
    float particleDevX  [512];
    float particleDevY  [512];
    float particledDevX [512];
    float particledDevY [512];
    u32   particleShape [512];

    vec2 perlinFlow[512*512];
    vec2 perlinGradientFlow[512*512];
    float perlinVals[512*512];

    u32 textureTestMem[512 * 512];  //memory for 1d perlin noise plot
    u32 textureTestMem1[512 * 512]; //memory for 1d perlin noise
    u32 textureTestMem2[512 * 512]; //memory for 2d perlin noise texture
    float coarseNoiseGrid[576 * 576];
    vec3 valueNoiseDerivatives[576*576];
    u64* frameCount;




    float doubleDescent;
    float lineAdvance;
    float maxCharWidth;

    b32 fixed_update;//tracks whether a fixed update happened this frame, useful for debug collation
    b32  textInputEnabled;


    u32* window_width;
    u32* window_height;

    bool firstFrame; //for temporary testing stuff
    bool perFramePerlin; //for 2d perlin texture generation each frame
    textEditInputTEST textEditTimers[text_edit_count];

    float perlinSliderMinRange;
    float perlinSliderMaxRange;
    float perlinXSlider;
    float perlinYSlider;
    float perlinZSlider;
    float perlinScale;

    bool  interpolateCoarseNoise;
    float coarsePerlinX;
    float coarsePerlinY;
    float coarsePerlinZ;
    float perlinPeriod;
    float tiledOffsetX;
    float tiledOffsetY;
    float tiledOffsetZ;
    float tiledOffsetW;
    float tiledScale;
    u32 perlinSeed;
    u32 perlinRandState;


    float perlinRedistribution;
    float perlinWaterLevel;

    float xDerivMin;
    float xDerivMax;
    float yDerivMin;
    float yDerivMax;
    float zDerivMin;
    float zDerivMax;
    float valueNoiseMin;
    float valueNoiseMax;
    
    float xDerivBlend;
    float yDerivBlend;
    float zDerivBlend;
    float timeScale;
    bool drawDerivArrows;

    u64 valNoise3dCycles;
    u64 valNoise3dCyclesTotal;
    u64 valNoise3dHits;
    bool perlinSIMD;

    u64 drawTextureTotal;
    u64 drawTextureHits;
    u64 drawTextureCycles;

    bool regenChunks;
    bool firstGenState; //for initial noise generation, then we would run a second pass for larger features
    bool secondGenState;

    u32 consecutiveFixedUpdates;
    
    bool windowResized;
    bool hotReloaded;

    u32 perlinDemoEnum;
    
    u32* thorns;
    u32* cracked;

    float scaleFactor;

    float deltaTimeScale;

    float testEase;
    float testHalflife;
    float testEaseTarget;
};


#define LABOR_H
#endif