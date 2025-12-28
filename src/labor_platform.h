#if !defined(LABOR_PLATFORM_H)

#ifdef __cplusplus
extern "C"{
#endif

#define global_variable static

#if !defined(COMPILER_MSVC)
#define COMPILER_MSVC 0
#endif

#if !defined(COMPILER_LLVM)
#define COMPILER_LLVM 0
#endif

#if !COMPILER_MSVC && !COMPILER_LLVM
#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#else
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif
#endif

#if COMPILER_MSVC
#include <intrin.h>
#endif

// #pragma once

// #ifdef _WIN32
//     #ifdef engine_GAME_EXPORTS
//         #define DLL_EXPORT __declspec(dllexport)
//     #else
//         #define DLL_EXPORT __declspec(dllimport)
//     #endif
// #else
//     #define DLL_EXPORT
// #endif

#define introspect(params)

// #include "networkState.h"
typedef struct thread_context
{
    int Placeholder;
} thread_context;

#if COMPILER_MSVC
inline uint64_t AtomicExchangeU64(uint64_t volatile *Value, uint64_t New){
    uint64_t Result = _InterlockedExchange64((__int64 volatile*)Value, New);
    return Result; 
}

inline uint64_t AtomicAddU64(uint64_t volatile* Value, uint64_t Addend){
    uint64_t Result =InterlockedExchangeAdd64((__int64 volatile*)Value, Addend);
    return Result;
}

inline uint32_t GetThreadID(void){
    uint8_t* ThreadLocalStorage = (uint8_t*)__readgsqword(0x30);
    uint32_t ThreadID = *(uint32_t*)(ThreadLocalStorage + 0x48);
    return ThreadID;
}

#elif COMPILER_LLVM
inline uint64_t AtomicExchangeU64(uint64_t volatile *Value, uint64_t New){
    uint64_t Result = __sync_lock_test_and_set(Value, New);
    return Result; 
}

inline uint64_t AtomicAddU64(uint64_t volatile* Value, uint64_t Addend){
    //returns result prior to adding
    uint64_t Result = __sync_fetch_and_add(Value, Addend);
    return Result;
}

inline u32 GetThreadID(void)
{
    u32 ThreadID;
#if defined(__APPLE__) && defined(__x86_64__)
    asm("mov %%gs:0x00,%0" : "=r"(ThreadID));
#elif defined(__i386__)
    asm("mov %%gs:0x08,%0" : "=r"(ThreadID));
#elif defined(__x86_64__)
    asm("mov %%fs:0x10,%0" : "=r"(ThreadID));
#else
#error Unsupported architecture
#endif
#endif
#define Pi32 3.14159265359f


#ifdef _MSC_VER
    #define SIMD_ALIGN(x) __declspec(align(x))
#elif defined(__GNUC__)
    #define SIMD_ALIGN(x) __attribute__((aligned(x)))
#else
    #define SIMD_ALIGN(x) alignas(x)  // C++11 fallback
#endif

// struct MyStruct {
//     SIMD_ALIGN(32) uint8_t simd_data[1024];
//     uint8_t normal_data[100];
// };


#define InvalidCodePath Assert(!"InvalidCodePath")
#define InvalidDefaultCase default: {InvalidCodePath;} break
#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)


//returns the number of elements in an array
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef int32_t b32;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float  f32;
typedef double r64;

#if LABOR_WIN
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif


#define BITMAP_BYTES_PER_PIXEL 4
#define BUFFER_WIDTH 640
#define BUFFER_HEIGHT 360

#define BITMAP_WIDTH 1920
#define BITMAP_HEIGHT 1080

#define MAX_BITMAPS 32

    struct debug_read_file_result
    {
        uint32_t ContentsSize;
        void *Contents;
    };

#define PLATFORM_ALLOCATE_MEMORY(name) void* name(size_t size)
typedef PLATFORM_ALLOCATE_MEMORY(platform_allocate_memory);
#define PLATFORM_DEALLOCATE_MEMORY(name) void name(void* memory)
typedef PLATFORM_DEALLOCATE_MEMORY(platform_deallocate_memory);


#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(void *Memory, uint32_t size)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(char* Filename, void* TargetMemory)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) b32 name(char* Filename, u32 MemorySize, void *Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

//ENABLE CYCLE COUNTER
#define labor_INTERNAL false

#if labor_INTERNAL
enum {
    DebugCycleCounter_GameUpdate,
    DebugCycleCounter_VoxelEdit,
    DebugCycleCounter_VoxelizeBrickmap,
    DebugCycleCounter_EntityBroadPhaseCollision,
    DebugCycleCounter_VoxelBroadPhaseCollision,
    DebugCycleCounter_Count,
};

typedef struct debug_cycle_counter{
    uint64_t cycle_count;
    uint32_t hit_count;
} debug_cycle_counter;
// extern struct game_memory* DebugGlobalMemory;
#if _MSC_VER
#define BEGIN_TIMED_BLOCK(ID) uint64_t StartCycleCount##ID = __rdtsc();
#define END_TIMED_BLOCK(ID) DebugGlobalMemory->Counters[DebugCycleCounter_##ID].cycle_count += __rdtsc() - StartCycleCount##ID; ++DebugGlobalMemory->Counters[DebugCycleCounter_##ID].hit_count;
#else
#define BEGIN_TIMED_BLOCK(ID)
#define END_TIMED_BLOCK(ID) 
#endif
#else
#define BEGIN_TIMED_BLOCK(ID)
#define END_TIMED_BLOCK(ID) 
#endif



    inline uint32_t
    SafeTruncateUInt64(uint64_t Value){
        //TODO defines for maximum values
        Assert(Value <= 0xFFFFFFF);
        uint32_t Result = (uint32_t)Value;
        return Result;
    }


inline char* get_token(char* str, char delim, char** remainder) {
    if (!str || !remainder) return NULL;
    
    // Find first occurrence of delimiter
    char* delim_pos = strchr(str, delim);
    
    if (delim_pos) {
        // Null terminate at delimiter
        *delim_pos = '\0';
        // Set remainder to just past delimiter
        *remainder = delim_pos + 1;
        return str;
    }
    
    // No delimiter found - whole string is the token
    *remainder = NULL;
    return str;
}

    struct game_memory;
    struct ServerSockets;

#ifndef SERVER_BUILD

    struct ClientSockets;

    #define PROCESS_CLIENT_COMMAND(name) void name(char* cmdString)
    typedef PROCESS_CLIENT_COMMAND(process_client_command);

    // #define PUSH_PLAYER_INPUT_TO_NETWORK(name) void name(player_input* playerInput)
    // typedef PUSH_PLAYER_INPUT_TO_NETWORK(push_player_input_to_network);

    #define START_LISTEN_SERVER(name) void name(game_memory* GameMemory)
    typedef START_LISTEN_SERVER(start_listen_server);

    #define START_CLIENT(name) void name(game_memory* GameMemory)
    typedef START_CLIENT(start_client);

#endif

// #define PUSH_ENTITY_STATE_TO_NETWORK(name) void name(uint32_t entityID,uint32_t inputTime, uint32_t timeProcessed, fpt_quat rotation, fpt_vec3 position, fpt_vec3 pos_in_chunk, fpt_vec3 forward, fpt_vec3 up, fpt_vec3 right, fpt speed, fpt brushSize, fpt angleH, fpt angleV)
// typedef PUSH_ENTITY_STATE_TO_NETWORK(push_entity_state_to_network);

// #define APPEND_TO_PACKET(name) void name(uint16_t clientID, char* buffer, int size)
// typedef APPEND_TO_PACKET(append_to_packet);

// #define APPEND_TO_ALL_PACKETS(name) void name(char* buffer, int size)
// typedef APPEND_TO_ALL_PACKETS(append_to_all_packets);

// #define SEND_PACKET(name) void name(uint16_t clientID)
// typedef SEND_PACKET(send_packet);

// #define SEND_ALL_PACKETS(name) void name(uint16_t sequence)
// typedef SEND_ALL_PACKETS(send_all_packets);

struct game_input;
struct SDL_Window;
struct shader_data;
struct EntityComponent;
struct SoftwareRenderState;
struct TransSoftRenderState;
struct ui_data;
struct physics_data;
struct gltf_data;
    struct dispatcher;
    struct chunk_data;
    struct mesh_data;
    struct Camera;
}
    struct Generator;


#if LABOR_INTERNAL
    struct debug_table;
#endif





struct platform_work_queue;
#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(platform_work_queue *Queue, void *Data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(platform_work_queue_callback);

typedef void platform_add_entry(platform_work_queue *Queue, platform_work_queue_callback *Callback, void *Data);
typedef void platform_complete_all_work(platform_work_queue *Queue);
typedef bool platform_check_all_work_completed(platform_work_queue *Queue);

// typedef void GenNoiseFunc(FastNoise::Generator*, float*, int, int, int, int, int, int, float, int);

typedef int32_t Platform_fpt_asin(int32_t sine_value);

#if LABOR_INTERNAL
    extern struct game_memory *DebugGlobalMemory;
#endif

enum text_edit_commands{
    text_edit_up,
    text_edit_down,
    text_edit_left,
    text_edit_right,
    text_edit_pageUp,
    text_edit_pageDown,
    text_edit_home,
    text_edit_end,
    text_edit_shift,
    text_edit_ctrl,
    text_edit_count,
};

struct game_memory
{
    int32_t IsInitialized;
    size_t PlatformMemorySize;
    uint64_t PermanentStorageSize;
    void *PermanentStorage; // required to be cleared to zero at startup

    uint64_t TransientStorageSize;
    void *TransientStorage; // required to be cleared to zero at startup
    
    uint64_t DebugStorageSize;
    void *DebugStorage; 
    
    #if LABOR_INTERNAL
        debug_table* GlobalDebugTablePtr;
    #endif

    platform_work_queue *HighPriorityQueue;
    platform_add_entry *PlatformAddEntry;
    platform_complete_all_work *PlatformCompleteAllWork;
    platform_check_all_work_completed *PlatformCheckAllWorkCompleted;
    // GenNoiseFunc *PlatformGenerateNoise;
    Platform_fpt_asin* platform_fpt_asin;

    shader_data* shaderData;

    // FastNoise::Generator** noiseGenerators;
    uint32_t generatorCount;
    uint32_t threadCount;


    
    debug_platform_free_file_memory *DEBUGPlatformFreeFileMemory;
    debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;
    debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile;

    uint32_t mainWindowID;

    wchar_t textInput[256];
    u32 codePointInput[256];
    u32  textInputLen;
    b32  textInputEnabled;
    u32 pendingHighSurrogate;


    ServerSockets* serverSockets;
    
    b32 executable_reloaded;

    float deltaTime;
    uint32_t width;
    uint32_t height;
    b32 window_resized;
    
    mat4 axis_model;
    vec3 camPos;
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    mat4 invViewProj;

    player_input new_input;
    player_input old_input;
    // #ifndef SERVER_BUILD
    //     ClientSockets* clientSockets;
    //     process_client_command* PlatformProcessClientCommand;
    //     // push_player_input_to_network* PlatformPushPlayerInputToNetwork;
    //     start_listen_server* PlatformStartListenServer;
    //     start_client* PlatformStartClient;
    // #endif
    
    // // push_entity_state_to_network*   PlatformPushEntityStateToNetwork;
    // append_to_packet*      platform_append_to_packet;
    // append_to_all_packets* platform_append_to_all_packets;
    // send_packet*           platform_send_packet;
    // send_all_packets*      platform_send_all_packets;
    gltf_data* gltfData;

    char* path;

    render_command_data* RenderCommandData;



    float doubleDescent;
    float lineAdvance;
    float maxCharWidth;

    #if labor_INTERNAL
    debug_cycle_counter Counters[DebugCycleCounter_Count];
    #endif

    u64* frameCount;
    u16 mainThreadID;
    b32 fixedUpdate;

    u32* thorns;
    u32* cracked;
} ;

struct TextInputState{
    char* buffer;
    vec2* cursor_positions;
    float* curPosX;
    float* curPosY;
    size_t buffer_size;
    size_t length;
    size_t cursor_num_positions;
    size_t cursor_pos;
    float cursor_blink_timer;
    bool show_cursor;
    
    size_t gapStart;
    size_t gapEnd;

    size_t* newLines;
    size_t lineCount;
    size_t charsSinceNewLine;

    //for snapshot reloading
    TextInputState* history;
    u32 historyIndex;
    u32 historyMax;
    u32 historySize;
    
};






#define GAME_UPDATE_AND_RENDER(name)  void name( game_memory *Memory/* , game_input *Input, game_offscreen_buffer *Buffer */)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_FIXED_UPDATE(name)  void name( game_memory *Memory/* , game_input *Input, game_offscreen_buffer *Buffer */)
typedef GAME_FIXED_UPDATE(game_fixed_update);


#define GAME_DRAW_UI(name)  void name( game_memory *Memory/* , game_input *Input, game_offscreen_buffer *Buffer */)
typedef GAME_DRAW_UI(game_draw_ui);


#define GAME_GET_SOUND_SAMPLES(name)  void name( game_memory *Memory/* , game_input *Input, game_offscreen_buffer *Buffer */)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);


#define GAME_INIT_NETWORK(name) void name(game_memory *Memory/* , game_input *Input, game_offscreen_buffer *Buffer */)
typedef GAME_INIT_NETWORK(game_init_network);

#define GAME_INIT(name) void name( game_memory *Memory/* , game_input *Input, game_offscreen_buffer *Buffer */)
typedef GAME_INIT(game_init);

#define GAME_SHUTDOWN(name)  void name( game_memory *Memory/* , game_input *Input, game_offscreen_buffer *Buffer */)
typedef GAME_SHUTDOWN(game_shutdown);

#define GAME_RELOAD(name)  void name( game_memory *Memory/* , game_input *Input, game_offscreen_buffer *Buffer */)
typedef GAME_RELOAD(game_reload);

#define GAME_PRE_RELOAD(name)  void name( game_memory *Memory/* , game_input *Input, game_offscreen_buffer *Buffer */)
typedef GAME_PRE_RELOAD(game_pre_reload);

#define GAME_WINDOW_RESIZED(name)  void name( game_memory *Memory/* , game_input *Input, game_offscreen_buffer *Buffer */)
typedef GAME_WINDOW_RESIZED(game_window_resized);


#define GAME_SNAPSHOT_SAVED(name)  void name( game_memory *Memory/* , game_input *Input, game_offscreen_buffer *Buffer */)
typedef GAME_SNAPSHOT_SAVED(game_snapshot_saved);


#define GAME_SNAPSHOT_LOADED(name)  void name( game_memory *Memory/* , game_input *Input, game_offscreen_buffer *Buffer */)
typedef GAME_SNAPSHOT_LOADED(game_snapshot_loaded);


struct replay_buffer{
    void* FileHandle;
    void* MemoryMap;
    char  FileName[256];
    void* MemoryBlock;
};

struct platform_state{
    uint64_t TotalSize;
    uint64_t GameSize;
    void *TotalMemoryBlock;
    void *GameMemoryBlock;
    bool recording = false;
    bool playback = false;
    bool endOfPlayback = false;
    replay_buffer ReplayBuffers[4];

    char exe_filename[MAX_PATH];
    char* one_past_last_exe_filename_slash;

    u32 width;
    u32 height;
};

struct gamepad_input{
    b32 is_connected;
    b32 is_analog;
    b32 stick_average_x;
    b32 stick_average_y;

    union {
        uint16_t buttons;     // The whole byte
        struct {
            uint16_t move_up            : 1;    
            uint16_t move_down          : 1;  
            uint16_t move_left          : 1;  
            uint16_t move_right         : 1; 
            uint16_t action_up          : 1;
            uint16_t action_down        : 1;
            uint16_t action_left        : 1;
            uint16_t action_right       : 1; //block? other inputs? just key presses we interpret as actions?

            uint16_t left_shoulder      : 1;    
            uint16_t right_shoulder     : 1;  
            uint16_t back               : 1;  
            uint16_t start              : 1; 
            uint16_t unused3    : 1;
            uint16_t unused4    : 1;
            uint16_t unused5    : 1;
            uint16_t unused6    : 1;

        } bits; //need to name it to do something like button.bits.up to access individual bits
    };

};

struct game_controllers{
    //0 is reserved for keyboard
    gamepad_input gamepads[5];
    u32 gamepad_count;
};

struct game_input{
    game_controllers controllers;
};



#define Minimum(A, B) ((A < B) ? (A) : (B))
#define Maximum(A, B) ((A > B) ? (A) : (B))



enum input_mouse_buttons{
    input_mouse_button_left,
    input_mouse_button_middle,
    input_mouse_button_right,
    input_mouse_button_side_back,
    input_mouse_button_side_front,

    input_mouse_button_count,
};


PLATFORM_ALLOCATE_MEMORY(Win32AllocateMemory)
{
    void* result = VirtualAlloc(0, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

    return(result);
}

PLATFORM_DEALLOCATE_MEMORY(Win32DeallocateMemory)
{
    if(memory)
    {
        VirtualFree(memory, 0, MEM_RELEASE);
    }
}

struct debug_file{
    void* memory;
    LARGE_INTEGER filesize;
    HANDLE mapping;
};



global_variable u32 g_allocs = 0;
global_variable u64 g_allocs_bytes = 0;

void* plat_alloc_mem(size_t size){
    void* result = VirtualAlloc(0, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    if(result){
        g_allocs++;
        g_allocs_bytes += size;
    }
    return(result);
}

void plat_dealloc_mem(void* memory, size_t size){
    if(memory)
    {
        BOOL result = VirtualFree(memory, 0, MEM_RELEASE);
        if(result){
            printf("successfully deallocated memory!\n");
            g_allocs--;
            g_allocs_bytes -= size;
        }else{
            printf("FAILED TO DEALLOCATE MEMORY!\n");

        }
    }
}


//TODO: this will FAIL on files 4GB or larger
debug_file Win32ReadFile(char* filename){
    debug_file debugFile = {};
    LARGE_INTEGER filesize;
    HANDLE file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if(file != INVALID_HANDLE_VALUE){
        if(GetFileSizeEx(file, &filesize)){
            debugFile.memory = VirtualAlloc(0, filesize.QuadPart, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            debugFile.filesize = filesize;
            DWORD bytesRead;
            if(ReadFile(file, debugFile.memory, filesize.QuadPart, &bytesRead, 0) && bytesRead == filesize.QuadPart){
                printf("successfully read file in Win32\n");
            }else{
                printf("win32 file read FAILED\n");
                VirtualFree(debugFile.memory, 0, MEM_RELEASE);
            }
        }else{
            printf("FAILED TO GET FILESIZE!\n");
        }
    }else{
        printf("FAILED TO OPEN FILE!\n");
    }
    CloseHandle(file);

    return debugFile;
}

//TODO: this will FAIL on files 4GB or larger
debug_file Win32ReadFileToGivenBuffer(char* filename, char* buffer = nullptr, size_t* max_size = 0){
    debug_file debugFile = {};
    LARGE_INTEGER filesize;
    HANDLE file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if(file != INVALID_HANDLE_VALUE){
        if(GetFileSizeEx(file, &filesize)){

            bool using_provided_buffer = false;
            if((buffer && filesize.QuadPart > *max_size) || !buffer){
                Assert(!"FILE TOO LARGE!");
                printf("read file is too large, or no buffer provided! allocating memory instead!\n");
                debugFile.memory = VirtualAlloc(0, filesize.QuadPart, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
                debugFile.filesize = filesize;

            }else if(buffer){
                using_provided_buffer = true;
                debugFile.memory = buffer;
                debugFile.filesize = filesize;
                *max_size = debugFile.filesize.QuadPart;
            }


            DWORD bytesRead;
            if(ReadFile(file, debugFile.memory, filesize.QuadPart, &bytesRead, 0) && bytesRead == filesize.QuadPart){
                printf("successfully read file in Win32\n");
            }else{
                printf("win32 file read FAILED\n");
                if(!using_provided_buffer){
                    VirtualFree(debugFile.memory, 0, MEM_RELEASE);
                }
            }
        }else{
            printf("FAILED TO GET FILESIZE!\n");
        }
    }else{
        printf("FAILED TO OPEN FILE!\n");
    }
    CloseHandle(file);

    return debugFile;
}

debug_file Win32MapFile(char* filename){
    debug_file debugFile = {};
    LARGE_INTEGER filesize;
    HANDLE file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    HANDLE mapping = NULL;
    if(file != INVALID_HANDLE_VALUE){
        if(GetFileSizeEx(file, &filesize)){
            mapping = CreateFileMappingA(file, 0, PAGE_READONLY, 0, 0, 0);
            if(mapping){
                debugFile.memory = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
                debugFile.filesize = filesize;
                debugFile.mapping = mapping;
            }else{
                printf("FAILED TO GET MAPPING!\n");
            }
        }else{
            printf("FAILED TO GET FILESIZE!\n");
        }
    }else{
        printf("FAILED TO OPEN FILE!\n");
    }
    CloseHandle(file);

    return debugFile;
}


void Win32UnmapFile(debug_file* file){
    if(file){
        if(file->memory){
            UnmapViewOfFile(file->memory);
            file->memory = NULL;
        }
        if(file->mapping){
            CloseHandle(file->mapping);
            file->mapping = NULL;
        }
    }
}

void Win32FreeFile(debug_file* file){
    if(file && file->memory){
        BOOL result = VirtualFree(file->memory, 0, MEM_RELEASE);
        if(result) {
            // Memory freed successfully
            file->memory = NULL;
        } else {
            // Failed to free memory
            DWORD error = GetLastError();
            printf("Failed to free memory. Error code: %lu\n", error);
        }
    }
}
//need top check all these
//test them
//see how they work

struct memory_arena
{
    size_t size;
    uint8_t *base;
    size_t used;
    
    s32 tempCount;
};

struct temporary_memory
{
    memory_arena *arena;
    size_t used;
};

#define ZeroStruct(instance) ZeroSize(sizeof(instance), &(instance))
#define ZeroArray(count, pointer) ZeroSize(count*sizeof((pointer[0]), pointer)
inline void ZeroSize(size_t size, void* ptr){
    u8* byte = (u8*)ptr;
    while(size--){
        *byte++ = 0;
    }
}


#define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &(Instance))
// inline void
// ZeroSize(size_t Size, void *Ptr)
// {
//     //check for performance
//     uint8_t* Byte = (uint8_t*)Ptr;
//     while(Size--)
//     {
//         *Byte++ = 0;
//     }
// }



inline size_t GetAlignmentOffset(memory_arena* arena, size_t alignment){
    size_t alignmentOffset = 0;

    size_t resultPointer = (size_t)arena->base + arena->used;
    size_t alignmentMask = alignment - 1;
    if(resultPointer & alignmentMask){
        alignmentOffset = alignment - (resultPointer & alignmentMask);
    }
    return alignmentOffset;
}





inline void initArena(memory_arena* arena, size_t size, void* base){
    arena->size = size;
    arena->base = (u8*)base;
    arena->used = 0;
    arena->tempCount = 0;
}

enum arena_push_flag{
    ArenaFlag_ClearToZero = 0x1,
};

struct arena_push_params{
    u32 flags;
    u32 alignment;
};

inline arena_push_params DefaultArenaParams(void){
    arena_push_params params = {};
    params.flags = ArenaFlag_ClearToZero;
    params.alignment = 4;
    return params;
}

inline arena_push_params alignNoClear(u32 alignment){
    arena_push_params params = DefaultArenaParams();
    params.flags &= ~ArenaFlag_ClearToZero;
    params.alignment = alignment;
    return params;
}

inline arena_push_params align(u32 alignment, b32 clear){
    arena_push_params params = DefaultArenaParams();
    if(clear){
        params.flags |= ArenaFlag_ClearToZero;
    }else{
        params.flags &= ~ArenaFlag_ClearToZero;
    }
    params.alignment = alignment;
    return params;
}

inline arena_push_params NoClear(void){
    arena_push_params params = DefaultArenaParams();
    params.flags &= ~ArenaFlag_ClearToZero;
    return params;
}

inline size_t GetArenaSizeRemaining(memory_arena* arena, arena_push_params params = DefaultArenaParams()){
    size_t result = arena->size - (arena->used + GetAlignmentOffset(arena, params.alignment));
    return result;
}

#define PushStruct(arena, type, ...) (type*)pushSize(arena, sizeof(type), ##__VA_ARGS__)
#define PushArray(arena, count, type, ...)(type*)pushSize(arena, (count)*sizeof(type), ##__VA_ARGS__)
inline size_t GetEffectiveSizeFor(memory_arena* arena, size_t sizeInit, arena_push_params params = DefaultArenaParams()){
    size_t size = sizeInit;

    size_t alignmentOffset = GetAlignmentOffset(arena, params.alignment);
    size += alignmentOffset;
    return size;
}
inline b32 ArenaHasRoomFor(memory_arena* arena, size_t sizeInit, arena_push_params params = DefaultArenaParams()){
    size_t size = GetEffectiveSizeFor(arena, sizeInit, params);
    b32 result = ((arena->used + size) <= arena->size);
    return result;
}
inline void* pushSize(memory_arena* arena, size_t sizeInit, arena_push_params params = DefaultArenaParams()){
    size_t size = GetEffectiveSizeFor(arena, sizeInit, params);
    Assert((arena->used + size) <= arena->size);

    size_t alignmentOffset = GetAlignmentOffset(arena, params.alignment);
    void* result = arena->base + arena->used + alignmentOffset;
    arena->used += size;

    assert(size >= sizeInit);

    if(params.flags & ArenaFlag_ClearToZero){
        // ZeroSize(sizeInit, result); //this is rather slow for large blocks of memory, using memset for faster clearing for now
        memset(result, 0, sizeInit);

    }

    return result;
}


inline FILETIME Win32GetLastWriteTime(char* filename){
    FILETIME lastWriteTime = {};

    WIN32_FILE_ATTRIBUTE_DATA data;
    if(GetFileAttributesEx(filename, GetFileExInfoStandard, &data)){
        lastWriteTime = data.ftLastWriteTime;
        // printf("lowTime: %u, highTime: %u\n", lastWriteTime.dwLowDateTime, lastWriteTime.dwHighDateTime);
    }
    return lastWriteTime;
}




inline void
InitializeArena(memory_arena *Arena, size_t Size, void *Base)
{
    Arena->size = Size;
    Arena->base = (uint8_t*)Base;
    Arena->used = 0;
    Arena->tempCount = 0;
}

inline temporary_memory
BeginTemporaryMemory(memory_arena *Arena)
{
    temporary_memory Result;

    Result.arena = Arena;
    Result.used = Arena->used;

    ++Arena->tempCount;

    return(Result);
}

inline void
EndTemporaryMemory(temporary_memory TempMem)
{
    memory_arena *Arena = TempMem.arena;
    Assert(Arena->used >= TempMem.used);
    Arena->used = TempMem.used;
    Assert(Arena->tempCount > 0);
    --Arena->tempCount;
}

inline void
CheckArena(memory_arena *Arena)
{
    Assert(Arena->tempCount == 0);
}

// 
static inline void HandleDebugCycleCounters(game_memory* Memory){
    #if labor_INTERNAL
    printf("DEBUG CYCLE COUNTS:\n");
    for(int i = 0; i < ArrayCount(Memory->Counters); i++){
        debug_cycle_counter *Counter = Memory->Counters + i;

        if(Counter->hit_count){
            char TextBuffer[256];
            _snprintf_s(TextBuffer, sizeof(TextBuffer),
            "  %d: %I64ucy %uh %I64ucy/h\n",

            i,
            Counter->cycle_count,
            Counter->hit_count,
            Counter->cycle_count / Counter->hit_count);
            printf("%s", TextBuffer);
            Counter->hit_count = 0;
            Counter->cycle_count = 0;
        }
    }

    #endif
}

     
static const int32_t fpt_asin_table[1025] = {
    0x00000000, 0x00000040, 0x00000080, 0x000000C0, 0x00000100, 0x00000140, 0x00000180, 0x000001C0, 
    0x00000200, 0x00000240, 0x00000280, 0x000002C0, 0x00000300, 0x00000340, 0x00000380, 0x000003C0, 
    0x00000400, 0x00000440, 0x00000480, 0x000004C0, 0x00000500, 0x00000540, 0x00000580, 0x000005C0, 
    0x00000600, 0x00000640, 0x00000680, 0x000006C0, 0x00000700, 0x00000740, 0x00000780, 0x000007C0, 
    0x00000800, 0x00000840, 0x00000880, 0x000008C0, 0x00000900, 0x00000940, 0x00000980, 0x000009C0, 
    0x00000A00, 0x00000A40, 0x00000A80, 0x00000AC0, 0x00000B00, 0x00000B40, 0x00000B80, 0x00000BC1, 
    0x00000C01, 0x00000C41, 0x00000C81, 0x00000CC1, 0x00000D01, 0x00000D41, 0x00000D81, 0x00000DC1, 
    0x00000E01, 0x00000E41, 0x00000E81, 0x00000EC2, 0x00000F02, 0x00000F42, 0x00000F82, 0x00000FC2, 
    0x00001002, 0x00001042, 0x00001082, 0x000010C3, 0x00001103, 0x00001143, 0x00001183, 0x000011C3,
    0x00001203, 0x00001243, 0x00001284, 0x000012C4, 0x00001304, 0x00001344, 0x00001384, 0x000013C5,
    0x00001405, 0x00001445, 0x00001485, 0x000014C5, 0x00001506, 0x00001546, 0x00001586, 0x000015C6,
    0x00001606, 0x00001647, 0x00001687, 0x000016C7, 0x00001707, 0x00001748, 0x00001788, 0x000017C8,
    0x00001809, 0x00001849, 0x00001889, 0x000018C9, 0x0000190A, 0x0000194A, 0x0000198A, 0x000019CB,
    0x00001A0B, 0x00001A4B, 0x00001A8C, 0x00001ACC, 0x00001B0C, 0x00001B4D, 0x00001B8D, 0x00001BCD,
    0x00001C0E, 0x00001C4E, 0x00001C8F, 0x00001CCF, 0x00001D0F, 0x00001D50, 0x00001D90, 0x00001DD1,
    0x00001E11, 0x00001E52, 0x00001E92, 0x00001ED3, 0x00001F13, 0x00001F54, 0x00001F94, 0x00001FD4,
    0x00002015, 0x00002055, 0x00002096, 0x000020D7, 0x00002117, 0x00002158, 0x00002198, 0x000021D9,
    0x00002219, 0x0000225A, 0x0000229A, 0x000022DB, 0x0000231C, 0x0000235C, 0x0000239D, 0x000023DE,
    0x0000241E, 0x0000245F, 0x0000249F, 0x000024E0, 0x00002521, 0x00002561, 0x000025A2, 0x000025E3,
    0x00002624, 0x00002664, 0x000026A5, 0x000026E6, 0x00002727, 0x00002767, 0x000027A8, 0x000027E9,
    0x0000282A, 0x0000286A, 0x000028AB, 0x000028EC, 0x0000292D, 0x0000296E, 0x000029AF, 0x000029EF,
    0x00002A30, 0x00002A71, 0x00002AB2, 0x00002AF3, 0x00002B34, 0x00002B75, 0x00002BB6, 0x00002BF7,
    0x00002C38, 0x00002C79, 0x00002CBA, 0x00002CFB, 0x00002D3C, 0x00002D7D, 0x00002DBE, 0x00002DFF,
    0x00002E40, 0x00002E81, 0x00002EC2, 0x00002F03, 0x00002F44, 0x00002F85, 0x00002FC6, 0x00003008,
    0x00003049, 0x0000308A, 0x000030CB, 0x0000310C, 0x0000314D, 0x0000318F, 0x000031D0, 0x00003211,
    0x00003252, 0x00003294, 0x000032D5, 0x00003316, 0x00003357, 0x00003399, 0x000033DA, 0x0000341B, 
    0x0000345D, 0x0000349E, 0x000034E0, 0x00003521, 0x00003562, 0x000035A4, 0x000035E5, 0x00003627,
    0x00003668, 0x000036AA, 0x000036EB, 0x0000372D, 0x0000376E, 0x000037B0, 0x000037F1, 0x00003833,
    0x00003874, 0x000038B6, 0x000038F8, 0x00003939, 0x0000397B, 0x000039BC, 0x000039FE, 0x00003A40,
    0x00003A82, 0x00003AC3, 0x00003B05, 0x00003B47, 0x00003B89, 0x00003BCA, 0x00003C0C, 0x00003C4E,
    0x00003C90, 0x00003CD2, 0x00003D13, 0x00003D55, 0x00003D97, 0x00003DD9, 0x00003E1B, 0x00003E5D,
    0x00003E9F, 0x00003EE1, 0x00003F23, 0x00003F65, 0x00003FA7, 0x00003FE9, 0x0000402B, 0x0000406D,
    0x000040AF, 0x000040F1, 0x00004133, 0x00004176, 0x000041B8, 0x000041FA, 0x0000423C, 0x0000427E,
    0x000042C1, 0x00004303, 0x00004345, 0x00004387, 0x000043CA, 0x0000440C, 0x0000444E, 0x00004491,
    0x000044D3, 0x00004515, 0x00004558, 0x0000459A, 0x000045DD, 0x0000461F, 0x00004662, 0x000046A4,
    0x000046E7, 0x00004729, 0x0000476C, 0x000047AE, 0x000047F1, 0x00004834, 0x00004876, 0x000048B9,
    0x000048FC, 0x0000493E, 0x00004981, 0x000049C4, 0x00004A07, 0x00004A49, 0x00004A8C, 0x00004ACF,
    0x00004B12, 0x00004B55, 0x00004B98, 0x00004BDA, 0x00004C1D, 0x00004C60, 0x00004CA3, 0x00004CE6,
    0x00004D29, 0x00004D6C, 0x00004DAF, 0x00004DF2, 0x00004E36, 0x00004E79, 0x00004EBC, 0x00004EFF,
    0x00004F42, 0x00004F85, 0x00004FC9, 0x0000500C, 0x0000504F, 0x00005092, 0x000050D6, 0x00005119,
    0x0000515C, 0x000051A0, 0x000051E3, 0x00005227, 0x0000526A, 0x000052AE, 0x000052F1, 0x00005335,
    0x00005378, 0x000053BC, 0x000053FF, 0x00005443, 0x00005487, 0x000054CA, 0x0000550E, 0x00005552,
    0x00005595, 0x000055D9, 0x0000561D, 0x00005661, 0x000056A5, 0x000056E8, 0x0000572C, 0x00005770,
    0x000057B4, 0x000057F8, 0x0000583C, 0x00005880, 0x000058C4, 0x00005908, 0x0000594C, 0x00005990,
    0x000059D5, 0x00005A19, 0x00005A5D, 0x00005AA1, 0x00005AE5, 0x00005B2A, 0x00005B6E, 0x00005BB2,
    0x00005BF7, 0x00005C3B, 0x00005C7F, 0x00005CC4, 0x00005D08, 0x00005D4D, 0x00005D91, 0x00005DD6,
    0x00005E1A, 0x00005E5F, 0x00005EA4, 0x00005EE8, 0x00005F2D, 0x00005F72, 0x00005FB6, 0x00005FFB,
    0x00006040, 0x00006085, 0x000060CA, 0x0000610F, 0x00006153, 0x00006198, 0x000061DD, 0x00006222,
    0x00006267, 0x000062AC, 0x000062F1, 0x00006337, 0x0000637C, 0x000063C1, 0x00006406, 0x0000644B,
    0x00006491, 0x000064D6, 0x0000651B, 0x00006561, 0x000065A6, 0x000065EB, 0x00006631, 0x00006676,
    0x000066BC, 0x00006701, 0x00006747, 0x0000678C, 0x000067D2, 0x00006818, 0x0000685D, 0x000068A3,
    0x000068E9, 0x0000692F, 0x00006975, 0x000069BA, 0x00006A00, 0x00006A46, 0x00006A8C, 0x00006AD2,
    0x00006B18, 0x00006B5E, 0x00006BA4, 0x00006BEB, 0x00006C31, 0x00006C77, 0x00006CBD, 0x00006D03,
    0x00006D4A, 0x00006D90, 0x00006DD6, 0x00006E1D, 0x00006E63, 0x00006EAA, 0x00006EF0, 0x00006F37,
    0x00006F7D, 0x00006FC4, 0x0000700A, 0x00007051, 0x00007098, 0x000070DF, 0x00007125, 0x0000716C,
    0x000071B3, 0x000071FA, 0x00007241, 0x00007288, 0x000072CF, 0x00007316, 0x0000735D, 0x000073A4,
    0x000073EB, 0x00007432, 0x0000747A, 0x000074C1, 0x00007508, 0x00007550, 0x00007597, 0x000075DE,
    0x00007626, 0x0000766D, 0x000076B5, 0x000076FC, 0x00007744, 0x0000778C, 0x000077D3, 0x0000781B,
    0x00007863, 0x000078AB, 0x000078F3, 0x0000793B, 0x00007982, 0x000079CA, 0x00007A12, 0x00007A5B,
    0x00007AA3, 0x00007AEB, 0x00007B33, 0x00007B7B, 0x00007BC3, 0x00007C0C, 0x00007C54, 0x00007C9C, 
    0x00007CE5, 0x00007D2D, 0x00007D76, 0x00007DBE, 0x00007E07, 0x00007E50, 0x00007E98, 0x00007EE1,
    0x00007F2A, 0x00007F73, 0x00007FBC, 0x00008004, 0x0000804D, 0x00008096, 0x000080DF, 0x00008129,
    0x00008172, 0x000081BB, 0x00008204, 0x0000824D, 0x00008297, 0x000082E0, 0x00008329, 0x00008373,
    0x000083BC, 0x00008406, 0x00008450, 0x00008499, 0x000084E3, 0x0000852D, 0x00008576, 0x000085C0,
    0x0000860A, 0x00008654, 0x0000869E, 0x000086E8, 0x00008732, 0x0000877C, 0x000087C6, 0x00008811,
    0x0000885B, 0x000088A5, 0x000088F0, 0x0000893A, 0x00008984, 0x000089CF, 0x00008A19, 0x00008A64,
    0x00008AAF, 0x00008AF9, 0x00008B44, 0x00008B8F, 0x00008BDA, 0x00008C25, 0x00008C70, 0x00008CBB,
    0x00008D06, 0x00008D51, 0x00008D9C, 0x00008DE8, 0x00008E33, 0x00008E7E, 0x00008ECA, 0x00008F15,
    0x00008F61, 0x00008FAC, 0x00008FF8, 0x00009043, 0x0000908F, 0x000090DB, 0x00009127, 0x00009173,
    0x000091BF, 0x0000920B, 0x00009257, 0x000092A3, 0x000092EF, 0x0000933B, 0x00009388, 0x000093D4,
    0x00009420, 0x0000946D, 0x000094B9, 0x00009506, 0x00009553, 0x0000959F, 0x000095EC, 0x00009639,
    0x00009686, 0x000096D3, 0x00009720, 0x0000976D, 0x000097BA, 0x00009807, 0x00009854, 0x000098A2,
    0x000098EF, 0x0000993D, 0x0000998A, 0x000099D8, 0x00009A25, 0x00009A73, 0x00009AC1, 0x00009B0F,
    0x00009B5C, 0x00009BAA, 0x00009BF8, 0x00009C46, 0x00009C95, 0x00009CE3, 0x00009D31, 0x00009D7F,
    0x00009DCE, 0x00009E1C, 0x00009E6B, 0x00009EB9, 0x00009F08, 0x00009F57, 0x00009FA6, 0x00009FF4,
    0x0000A043, 0x0000A092, 0x0000A0E2, 0x0000A131, 0x0000A180, 0x0000A1CF, 0x0000A21F, 0x0000A26E,
    0x0000A2BD, 0x0000A30D, 0x0000A35D, 0x0000A3AC, 0x0000A3FC, 0x0000A44C, 0x0000A49C, 0x0000A4EC,
    0x0000A53C, 0x0000A58C, 0x0000A5DC, 0x0000A62D, 0x0000A67D, 0x0000A6CE, 0x0000A71E, 0x0000A76F,
    0x0000A7BF, 0x0000A810, 0x0000A861, 0x0000A8B2, 0x0000A903, 0x0000A954, 0x0000A9A5, 0x0000A9F6,
    0x0000AA48, 0x0000AA99, 0x0000AAEA, 0x0000AB3C, 0x0000AB8E, 0x0000ABDF, 0x0000AC31, 0x0000AC83,
    0x0000ACD5, 0x0000AD27, 0x0000AD79, 0x0000ADCB, 0x0000AE1E, 0x0000AE70, 0x0000AEC2, 0x0000AF15,
    0x0000AF67, 0x0000AFBA, 0x0000B00D, 0x0000B060, 0x0000B0B3, 0x0000B106, 0x0000B159, 0x0000B1AC,
    0x0000B1FF, 0x0000B253, 0x0000B2A6, 0x0000B2FA, 0x0000B34E, 0x0000B3A1, 0x0000B3F5, 0x0000B449,
    0x0000B49D, 0x0000B4F1, 0x0000B545, 0x0000B59A, 0x0000B5EE, 0x0000B643, 0x0000B697, 0x0000B6EC,
    0x0000B741, 0x0000B796, 0x0000B7EA, 0x0000B840, 0x0000B895, 0x0000B8EA, 0x0000B93F, 0x0000B995,
    0x0000B9EA, 0x0000BA40, 0x0000BA96, 0x0000BAEC, 0x0000BB42, 0x0000BB98, 0x0000BBEE, 0x0000BC44,
    0x0000BC9A, 0x0000BCF1, 0x0000BD47, 0x0000BD9E, 0x0000BDF5, 0x0000BE4C, 0x0000BEA3, 0x0000BEFA,
    0x0000BF51, 0x0000BFA8, 0x0000C000, 0x0000C057, 0x0000C0AF, 0x0000C107, 0x0000C15E, 0x0000C1B6,
    0x0000C20E, 0x0000C267, 0x0000C2BF, 0x0000C317, 0x0000C370, 0x0000C3C9, 0x0000C421, 0x0000C47A,
    0x0000C4D3, 0x0000C52C, 0x0000C585, 0x0000C5DF, 0x0000C638, 0x0000C692, 0x0000C6EC, 0x0000C745,
    0x0000C79F, 0x0000C7F9, 0x0000C854, 0x0000C8AE, 0x0000C908, 0x0000C963, 0x0000C9BE, 0x0000CA18,
    0x0000CA73, 0x0000CACE, 0x0000CB2A, 0x0000CB85, 0x0000CBE0, 0x0000CC3C, 0x0000CC98, 0x0000CCF4,
    0x0000CD50, 0x0000CDAC, 0x0000CE08, 0x0000CE64, 0x0000CEC1, 0x0000CF1D, 0x0000CF7A, 0x0000CFD7,
    0x0000D034, 0x0000D092, 0x0000D0EF, 0x0000D14C, 0x0000D1AA, 0x0000D208, 0x0000D266, 0x0000D2C4,
    0x0000D322, 0x0000D380, 0x0000D3DF, 0x0000D43E, 0x0000D49C, 0x0000D4FB, 0x0000D55A, 0x0000D5BA,
    0x0000D619, 0x0000D679, 0x0000D6D8, 0x0000D738, 0x0000D798, 0x0000D7F9, 0x0000D859, 0x0000D8B9,
    0x0000D91A, 0x0000D97B, 0x0000D9DC, 0x0000DA3D, 0x0000DA9E, 0x0000DB00, 0x0000DB62, 0x0000DBC3,
    0x0000DC25, 0x0000DC88, 0x0000DCEA, 0x0000DD4C, 0x0000DDAF, 0x0000DE12, 0x0000DE75, 0x0000DED8,
    0x0000DF3C, 0x0000DF9F, 0x0000E003, 0x0000E067, 0x0000E0CB, 0x0000E12F, 0x0000E194, 0x0000E1F9,
    0x0000E25D, 0x0000E2C3, 0x0000E328, 0x0000E38D, 0x0000E3F3, 0x0000E459, 0x0000E4BF, 0x0000E525,
    0x0000E58B, 0x0000E5F2, 0x0000E659, 0x0000E6C0, 0x0000E727, 0x0000E78F, 0x0000E7F6, 0x0000E85E,
    0x0000E8C6, 0x0000E92E, 0x0000E997, 0x0000EA00, 0x0000EA69, 0x0000EAD2, 0x0000EB3B, 0x0000EBA5,
    0x0000EC0F, 0x0000EC79, 0x0000ECE3, 0x0000ED4D, 0x0000EDB8, 0x0000EE23, 0x0000EE8E, 0x0000EEFA, 
    0x0000EF65, 0x0000EFD1, 0x0000F03E, 0x0000F0AA, 0x0000F117, 0x0000F183, 0x0000F1F1, 0x0000F25E,
    0x0000F2CC, 0x0000F33A, 0x0000F3A8, 0x0000F416, 0x0000F485, 0x0000F4F4, 0x0000F563, 0x0000F5D3,
    0x0000F642, 0x0000F6B2, 0x0000F723, 0x0000F793, 0x0000F804, 0x0000F875, 0x0000F8E7, 0x0000F959,
    0x0000F9CB, 0x0000FA3D, 0x0000FAAF, 0x0000FB22, 0x0000FB96, 0x0000FC09, 0x0000FC7D, 0x0000FCF1,
    0x0000FD66, 0x0000FDDA, 0x0000FE4F, 0x0000FEC5, 0x0000FF3B, 0x0000FFB1, 0x00010027, 0x0001009E,
    0x00010115, 0x0001018C, 0x00010204, 0x0001027C, 0x000102F5, 0x0001036D, 0x000103E7, 0x00010460,
    0x000104DA, 0x00010554, 0x000105CF, 0x0001064A, 0x000106C5, 0x00010741, 0x000107BD, 0x0001083A,
    0x000108B7, 0x00010934, 0x000109B2, 0x00010A30, 0x00010AAF, 0x00010B2E, 0x00010BAD, 0x00010C2D,
    0x00010CAD, 0x00010D2E, 0x00010DAF, 0x00010E31, 0x00010EB3, 0x00010F35, 0x00010FB8, 0x0001103C,
    0x000110C0, 0x00011144, 0x000111C9, 0x0001124F, 0x000112D5, 0x0001135B, 0x000113E2, 0x00011469,
    0x000114F1, 0x0001157A, 0x00011603, 0x0001168D, 0x00011717, 0x000117A1, 0x0001182D, 0x000118B9,
    0x00011945, 0x000119D2, 0x00011A60, 0x00011AEE, 0x00011B7D, 0x00011C0C, 0x00011C9C, 0x00011D2D,
    0x00011DBF, 0x00011E51, 0x00011EE4, 0x00011F77, 0x0001200B, 0x000120A0, 0x00012135, 0x000121CC,
    0x00012263, 0x000122FB, 0x00012393, 0x0001242C, 0x000124C6, 0x00012561, 0x000125FD, 0x0001269A,
    0x00012737, 0x000127D5, 0x00012874, 0x00012914, 0x000129B5, 0x00012A57, 0x00012AFA, 0x00012B9D,
    0x00012C42, 0x00012CE8, 0x00012D8E, 0x00012E36, 0x00012EDF, 0x00012F89, 0x00013034, 0x000130E0,
    0x0001318D, 0x0001323B, 0x000132EB, 0x0001339B, 0x0001344D, 0x00013501, 0x000135B5, 0x0001366B,
    0x00013722, 0x000137DB, 0x00013895, 0x00013950, 0x00013A0D, 0x00013ACC, 0x00013B8C, 0x00013C4E,
    0x00013D11, 0x00013DD6, 0x00013E9D, 0x00013F66, 0x00014030, 0x000140FD, 0x000141CB, 0x0001429B,
    0x0001436E, 0x00014442, 0x00014519, 0x000145F2, 0x000146CE, 0x000147AC, 0x0001488C, 0x0001496F,
    0x00014A55, 0x00014B3E, 0x00014C29, 0x00014D18, 0x00014E0A, 0x00014EFF, 0x00014FF8, 0x000150F4,
    0x000151F4, 0x000152F8, 0x00015400, 0x0001550D, 0x0001561E, 0x00015734, 0x0001584F, 0x00015970,
    0x00015A96, 0x00015BC3, 0x00015CF6, 0x00015E30, 0x00015F71, 0x000160BB, 0x0001620D, 0x00016369,
    0x000164CF, 0x00016640, 0x000167BE, 0x00016949, 0x00016AE4, 0x00016C91, 0x00016E51, 0x00017028,
    0x0001721A, 0x0001742C, 0x00017665, 0x000178D0, 0x00017B7D, 0x00017E85, 0x0001821F, 0x000186CF,
    0x0001921F
};


#ifdef __cplusplus
#endif

#include "labor_debug_interface.h"

#define LABOR_PLATFORM_H
#endif