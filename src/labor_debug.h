
#if 0
#define DEBUG_PRINT(format, ...) printf(format, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(format, ...)
#endif

struct debug_stored_event{
    union{
        debug_stored_event* Next;
        debug_stored_event* NextFree;
    };

    u32 FrameIndex;
    debug_event Event;
};

struct debug_element{
    char* OriginalGUID; //can never be printed, might point into unloaded DLL
    char* GUID;
    u32 FileNameCout;
    u32 LineNumber;
    u32 NameStartsAt;

    b32 ValueWasEdited;

    debug_element* NextInHash;
    debug_stored_event* OldestEvent;
    debug_stored_event* MostRecentEvent;
};

struct debug_variable_group;
struct debug_variable_link{
    debug_variable_link* Next;
    debug_variable_link* Prev;

    debug_variable_group* Children;
    debug_element* Element;
};


struct debug_variable_group{
    u32 NameLength;
    char* Name;
    debug_variable_link Sentinel;
};

struct open_debug_block{
    union{
        open_debug_block* Parent;
        open_debug_block* NextFree;
    };
    
    u32 StartingFrameIndex;
    debug_event* OpeningEvent;
    debug_element* element;

    debug_variable_group* Group;
};

struct debug_thread{
    union{
        debug_thread* Next;
        debug_thread* NextFree;
    };
    u32 id;
    u32 laneIndex;
    open_debug_block *FirstOpenCodeBlock;
    open_debug_block *FirstOpenDataBlock;
};

struct debug_frame{
    union{
        debug_frame* next;
        debug_frame* nextFree;
    };

    u64 beginClock;
    u64 endClock;
    f32 wallSecondsElapsed;
    f32 frameBarScale;
    u32 frameIndex;
};

#define MAX_EVENTS_PER_FRAME 1024

struct GUIDHashEntry{
    char* GUID;
    u64 cumulativeTime;
    u64 cumulativeChildTimings;
    u64 hits;
    u32 frameEventIndex;
};

struct debug_event_stack{
    debug_event event;
    GUIDHashEntry* guidHashEntry;
};

//size for this is allocated at the start, in win_labor.cpp, a pointer to this block is stored in Game Memory
struct debug_state{
    b32 initialized;
    memory_arena debugArena;
    memory_arena perFrameArena;
    debug_frame curFrame;
    debug_frame prevFrame;

    debug_event* firstBeginEvent; 
    debug_event* firstEndEvent; 
    
    ui_element* profileWindowBase;
    ui_element* historyGraph;

    histogram_debug_event sortedHistory[MAX_EVENTS_PER_FRAME];
    histogram_debug_event radixTemp[MAX_EVENTS_PER_FRAME];
    u64 radixTimes[MAX_EVENTS_PER_FRAME];
    u64 outputTimes[MAX_EVENTS_PER_FRAME];

    u64 barEventHistory [300][MAX_EVENTS_PER_FRAME];
    
    histogram_debug_event eventHistory[300][MAX_EVENTS_PER_FRAME];
    debug_frame curFrameHistory[300];
    u64 totalCycleHistory[300];
    u64 cumulativeCycleHistory[300];
    debug_frame prevFrameHistory[300];
    u64 eventHistoryMax [300];
    u32 eventHistoryCount[300];
    u32 barHistoryCount[300];
    f32 timerHistory[300];
    u32 eventHistoryIndex;
    u32 currentEventHistoryIndex;

    bool pauseCollation;
    u64 curFrameCount;
    u64 prevFrameCount;
    u64 lastNonFixedFrameCount;
    u64 lastFixedFrameCount;
    u64 currentFrameCumulativeCycles;

    histogram_debug_event currentFrameEvents[MAX_EVENTS_PER_FRAME];
    u32 currentFrameEventCount;
    
    u64 debugElementBarTest[MAX_EVENTS_PER_FRAME];
    u32 debugElementBarCount;
    u32 framesToSkip;
    bool hotReloaded;

    bool last_frame_was_fixed_update;
    bool last_frame_was_non_fixed_update;

    u16 openThreadIDs[MAX_WORKER_THREADS];
    u16 openThreadIDCount;

    debug_event_stack threadOpenEvents[MAX_WORKER_THREADS][256];
    u16 threadOpenEventCount[MAX_WORKER_THREADS];
    u16 threadOpenEventTotal[MAX_WORKER_THREADS];

    GUIDHashEntry GUIDHash[1024][4];
};