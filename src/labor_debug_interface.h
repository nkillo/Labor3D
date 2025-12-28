


struct debug_table;
#define DEBUG_GAME_FRAME_END(name)  debug_table* name( game_memory *Memory/* , game_input *Input, game_offscreen_buffer *Buffer */)
typedef DEBUG_GAME_FRAME_END(debug_game_frame_end);

#if LABOR_INTERNAL 

enum debug_type{
    DebugType_Unknown,
    DebugType_FrameMarker,
    DebugType_BeginBlock,
    DebugType_EndBlock,
    DebugType_,
};


struct debug_event{
    u64 clock;
    char* GUID;
    u16 threadID;
    u16 CoreIndex;
    u8 type;
    union{
        f32 secondsElapsed;
        u64 cumulativeChildTimings;
    };
};

struct histogram_debug_event{
    debug_event event;
    u64 cumulativeTime;
    u32 hits;
    u64 remainingTime;
};

struct debug_table{

    u32 CurrentEventArrayIndex;
    u64 volatile EventArrayIndex_EventIndex;
    debug_event Events[2][16*65536];
};

extern debug_table *GlobalDebugTable;

#define UniqueFileCounterString__(A, B, C, D) A "|" #B "|" #C "|" #D
#define UniqueFileCounterString_(A, B, C, D) UniqueFileCounterString__(A, B, C, D)
#define DEBUG_NAME(Name) UniqueFileCounterString_(__FILE__, __LINE__, __COUNTER__, Name)

#define RecordDebugEvent(EventType, GUIDINIT) \
    u64 ArrayIndex_EventIndex = AtomicAddU64(&GlobalDebugTable->EventArrayIndex_EventIndex, 1); \
    u32 EventIndex = ArrayIndex_EventIndex & 0xFFFFFFFF; \
    Assert(EventIndex < ArrayCount(GlobalDebugTable->Events[0])); \
    debug_event* Event = GlobalDebugTable->Events[ArrayIndex_EventIndex >> 32] + EventIndex; \
    Event->clock = __rdtsc(); \
    Event->type = (u8)EventType; \
    Event->CoreIndex = 0; \
    Event->secondsElapsed = 0; /*clear the union*/ \
    Event->threadID = (u16)GetThreadID(); \
    Event->GUID = GUIDINIT; 


//with additional naming data
// #define FRAME_MARKER(SecondsElapsedInit) \
// {RecordDebugEvent(DebugType_FrameMarker, DEBUG_NAME("Frame Marker")); \
//         Event->secondsElapsed = SecondsElapsedInit;}

#define FRAME_MARKER(SecondsElapsedInit) \
{RecordDebugEvent(DebugType_FrameMarker, "Frame Marker"); \
        Event->secondsElapsed = SecondsElapsedInit;}


// #define TIMED_BLOCK__(GUID, Number, ...) timed_block TimedBlock_##Number(GUID, ## __VA_ARGS__)
#define TIMED_BLOCK__(Name) timed_block TimedBlock_(Name)
// #define TIMED_BLOCK_(GUID, Number, ...) TIMED_BLOCK__(GUID, Number, ## __VA_ARGS__)
#define TIMED_BLOCK_(Name) TIMED_BLOCK__(Name)
// #define TIMED_BLOCK(Name, ...) TIMED_BLOCK_(DEBUG_NAME(Name), __COUNTER__, ## __VA_ARGS__)
#define TIMED_BLOCK(Name) TIMED_BLOCK_(Name)
#define TIMED_FUNCTION(...) TIMED_BLOCK_(DEBUG_NAME(__FUNCTION__), ## __VA_ARGS__)

#define BEGIN_BLOCK_(GUID) {RecordDebugEvent(DebugType_BeginBlock, GUID);}
#define END_BLOCK_(GUID) {RecordDebugEvent(DebugType_EndBlock, GUID);}

//this add additional data like file name/code line
// #define BEGIN_BLOCK(Name) BEGIN_BLOCK_(DEBUG_NAME(Name))
// #define END_BLOCK() END_BLOCK_(DEBUG_NAME("END_BLOCK_"))


#define BEGIN_BLOCK(Name) BEGIN_BLOCK_(Name)
#define END_BLOCK_NONAME() END_BLOCK_("END_BLOCK_")
#define END_BLOCK(Name) END_BLOCK_(Name)

struct timed_block{
    timed_block(char* Name){
        BEGIN_BLOCK_(Name)
    }

    ~timed_block(){
        END_BLOCK_NONAME();
    }
};

#else

#define TIMED_BLOCK(...)
#define TIMED_FUNCTION(...)
#define BEGIN_BLOCK(...)
#define END_BLOCK(...)
#define FRAME_MARKER(...)

#endif