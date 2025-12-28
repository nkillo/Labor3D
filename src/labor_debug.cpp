
#include "labor_debug.h"


void writeToBMP(const char* filename, u32* pixels, int width, int height){
    FILE* f = fopen(filename, "wb");
    if(!f)return;

    int rowSize = ((width * 3 + 3) & ~3); //pad to multiple of 4
    int pixelDataSize = rowSize * height;
    int fileSize = 14 + 40 + pixelDataSize;
    
    //bmp header
    u8 fileHeader[14] = {
        'B', 'M', //signature
        (u8)fileSize, (u8)(fileSize>>8), (u8)(fileSize>>16), (u8)(fileSize>>24),
        0,0,0,0,//reserved
        54,0,0,0//pixel data offset
    };
    fwrite(fileHeader, 1, 14, f);
    //dib header
    u8 infoHeader[40] = {
        40,0,0,0,//dib header size
        (u8)(width ),        (u8)(width>>8 ),         (u8)(width>>16 ),        (u8)(width>>24 ),
        (u8)(height),        (u8)(height>>8),         (u8)(height>>16),        (u8)(height>>24),
        1,0,
        24,0,
        0,0,0,0,
        (u8)(pixelDataSize), (u8)(pixelDataSize>>8),  (u8)(pixelDataSize>>16), (u8)(pixelDataSize>>24),
        0,0,0,0,
        0,0,0,0,
        0,0,0,0,
        0,0,0,0,

    };
    fwrite(infoHeader, 1, 40, f);
    //pixel data (BGR, bottom up)
    for(int y = height - 1; y >= 0; y--){
        for(int x = 0; x < width; x++){
            uint32_t pixel = pixels[y*width+x];
            u8 r = (pixel >> 16);
            u8 g = (pixel >> 8);
            u8 b = (pixel >> 0);
            fwrite(&r, 1, 1, f);
            fwrite(&g, 1, 1, f);
            fwrite(&b, 1, 1, f);
        }
        //padding
        uint8_t padding[3] = {0,0,0};
        fwrite(padding, 1, rowSize - width * 3, f);
    }
    fclose(f);
}

u64 HashPtr(const void* p) {
    uintptr_t v = (uintptr_t)p; // convert pointer to integer type
    v ^= v >> 33;
    v *= 0xff51afd7ed558ccdULL;
    v ^= v >> 33;
    v *= 0xc4ceb9fe1a85ec53ULL;
    v ^= v >> 33;
    return v;
}


void shuffleTiledPermTables(game_state* GameState){

        for(int i = 255; i > 0; i--){
            int j = rng_next_u32(&GameState->perlinRandState) % (i + 1);
            u8 temp = GameState->chunkData->permutations2d[i];
            GameState->chunkData->permutations2d[i] = GameState->chunkData->permutations2d[j];
            GameState->chunkData->permutations2d[j] = temp;    
            GameState->chunkData->permutations3d[i] = GameState->chunkData->permutations3d[j];
            GameState->chunkData->permutations3d[j] = temp;
            GameState->chunkData->permutations4d[i] = GameState->chunkData->permutations4d[j];
            GameState->chunkData->permutations4d[j] = temp;
            
        }
}

//must be set by the platform layer
//set in labor.cpp GAME_INIT()
debug_table* GlobalDebugTable = NULL;


inline b32 EventsMatch(debug_event& A, debug_event& B) {
    b32 result = (A.threadID == B.threadID);
    return result;
}

static void debug_start(game_state* GameState, debug_state* DebugState) {
    if (!DebugState->initialized) {

        size_t totalMemorySize = DebugGlobalMemory->DebugStorageSize - sizeof(debug_state);
        InitializeArena(&DebugState->debugArena, totalMemorySize, DebugState + 1);//DebugState + 1 offsets to the beginning of where we want the arena to be


        DebugState->initialized = true;
    }
}

inline void AppendFrameMarker(debug_state* DebugState, debug_event* event){
    //copy all the current events to the main buffer, reset the index, and add this one to the top.
    //record the time as well

    DebugState->prevFrame = DebugState->curFrame;
    DebugState->prevFrame.endClock = event->clock;
    DebugState->curFrame.beginClock = event->clock;

    //all open blocks must be closed by now
    // Assert(!DebugState->threadOpenEventCount[0]);
    
    u32 index = ((DebugState->currentEventHistoryIndex) % 300);

    //if collation is unpaused, save all information to current index
    //only save information if there were actually any events to save (currentFrameEventCount is event count)
    if (!DebugState->pauseCollation && DebugState->currentFrameEventCount) {
        DebugState->currentEventHistoryIndex = DebugState->eventHistoryIndex++;
        index = ((DebugState->currentEventHistoryIndex) % 300);
        DEBUG_PRINT("saving events to slot: %d\n", index);

        memcpy(DebugState->barEventHistory[index], DebugState->debugElementBarTest, sizeof(u64) * MAX_EVENTS_PER_FRAME);

        memcpy(DebugState->eventHistory[index], DebugState->currentFrameEvents, sizeof(histogram_debug_event) * MAX_EVENTS_PER_FRAME);

        DebugState->cumulativeCycleHistory[index] = DebugState->currentFrameCumulativeCycles;
        DebugState->prevFrameHistory[index] = DebugState->prevFrame;
        DebugState->curFrameHistory[index] = DebugState->curFrame;
        DebugState->timerHistory[index] = event->secondsElapsed;
        DebugState->eventHistoryCount[index] = DebugState->currentFrameEventCount;
        DebugState->barHistoryCount[index] = DebugState->debugElementBarCount;

        //assuming the last index of any frame is the total time for that frame
        DebugState->totalCycleHistory[index] = DebugState->eventHistory[index][DebugState->eventHistoryCount[index] - 1].event.clock;
            
    }
    DebugState->currentFrameCumulativeCycles = 0;
    DebugState->debugElementBarCount = 0;
    DebugState->currentFrameEventCount = 0;

    //clear per thread info
    DebugState->openThreadIDCount = 0;
    memset(DebugState->threadOpenEventCount, 0, sizeof(u16) * MAX_WORKER_THREADS);
    memset(DebugState->threadOpenEvents, 0, sizeof(debug_event_stack) * MAX_WORKER_THREADS * 256);

    memset(DebugState->GUIDHash, 0, sizeof(GUIDHashEntry) * 1024 * 4);
    //dont store a bar for the frame marker, do store the event
    memset(DebugState->currentFrameEvents, 0, sizeof(histogram_debug_event) * MAX_EVENTS_PER_FRAME);

    //used to store but found that we can omit it without problem
    // DebugState->currentFrameEvents[DebugState->currentFrameEventCount++].event = *event;


}

inline void AppendBeginBlock(debug_state* DebugState, debug_event* event){
    //append event to unresolved timing blocks per thread
    bool found = false;
    int threadIndex = 0;

    for(u16 activeThreads = 0; activeThreads < DebugState->openThreadIDCount; activeThreads++){
        if(event->threadID == DebugState->openThreadIDs[activeThreads]){
            found = true;
            threadIndex = activeThreads;
            break;
        }
    }
    if(!found){
        threadIndex = DebugState->openThreadIDCount++;
        DebugState->openThreadIDs[threadIndex] = event->threadID;
    }

    #if 1
    u64 hash = HashPtr(event->GUID);
    //so we have the hash, now we need to check the position in the table
    GUIDHashEntry* guidHash = NULL;

    u32 eventStackIndex = 0; 
    // bool firstEventOfItsKind = false;
    for(int i = 0; i < 4; i++){//check each bucket at this hash position
        if(!DebugState->GUIDHash[hash & 1023][i].GUID){
            // DEBUG_PRINT("|%s| added to guid hash\n", event->GUID);
            guidHash = DebugState->GUIDHash[hash & 1023] + i;
            guidHash->GUID = event->GUID;
            // firstEventOfItsKind = true;
            break;
        }else if(DebugState->GUIDHash[hash & 1023][i].GUID == event->GUID){
            guidHash = DebugState->GUIDHash[hash & 1023] + i;
            // DEBUG_PRINT("|%s| already has hash entry\n", event->GUID);
            break;
        }
    }
    Assert(guidHash);
    #endif
    int threadOpenEventCount = DebugState->threadOpenEventCount[threadIndex]++; 
    Assert(DebugState->threadOpenEventCount[threadIndex] < 256);
    
    DEBUG_PRINT("\tthreadOpenEventCount: %d \n",threadOpenEventCount);

    // u64 cumulativeChildTimings = DebugState->threadOpenEvents[threadIndex][threadOpenEventCount].event.cumulativeChildTimings;
    DebugState->threadOpenEvents[threadIndex][threadOpenEventCount].event = *event;
    //save cumulative timings between the same event getting pasted over and over
    // if(!firstEventOfItsKind){
    //     DEBUG_PRINT("\t|%s| not first event of its kind, saving cumulativeChildTimings: %zu\n",DebugState->threadOpenEvents[threadIndex][threadOpenEventCount].event.GUID, cumulativeChildTimings);
    //     DebugState->threadOpenEvents[threadIndex][threadOpenEventCount].event.cumulativeChildTimings = cumulativeChildTimings;

    // }

    DebugState->threadOpenEvents[threadIndex][threadOpenEventCount].guidHashEntry = guidHash;

    //should we push them onto the current events? or should we wait until they are resolved by their EndBlock counterparts?
    //wait until they are resolved by their EndBlock counterparts
    // DebugState->currentFrameEvents[DebugState->currentFrameEventCount++] = *event;
    // DebugState->debugElementBarTest[DebugState->debugElementBarCount++] = event->clock - DebugState->curFrame.beginClock;
 }


inline void AppendEndBlock(debug_state* DebugState, debug_event* endEvent){
    bool found = false;
    int threadIndex = 0;
    for(u16 activeThreads = 0; activeThreads < DebugState->openThreadIDCount; activeThreads++){
        if(endEvent->threadID == DebugState->openThreadIDs[activeThreads]){
            found = true;
            threadIndex = activeThreads;
            break;
        }
    }
    if(!found){
        // Assert(!"Error! Unmatched EndBlock debug profile event!");
    }else{
        //enable this if we truly cant find a way to keep from crashing during hot reloading
        // if(!DebugState->threadOpenEventCount[threadIndex]){
            // we hot reloaded, the count is 0, we need to exit out of here
            // return;
        // }
        int threadOpenEventCount = --DebugState->threadOpenEventCount[threadIndex];    
        
        debug_event_stack stackEntry = DebugState->threadOpenEvents[threadIndex][threadOpenEventCount];
        debug_event beginEvent = stackEntry.event;
        GUIDHashEntry* guidHash = stackEntry.guidHashEntry;
        Assert(guidHash);
        
        //pick up where we left off with the actual name of the function we get from the guidhash
        u64 totalBlockTime = endEvent->clock - beginEvent.clock;

        DEBUG_PRINT("|%s|! frame event count: %d, totalBlockTime: %zu\n",guidHash->GUID, DebugState->currentFrameEventCount, totalBlockTime);


        guidHash->cumulativeTime += totalBlockTime;
        guidHash->hits++;

        //try to add cumulative timing to parent block
        debug_event* parentEvent = 0;
        GUIDHashEntry* parentGuidHashEntry = 0;
        if((threadOpenEventCount - 1) >= 0){
            debug_event_stack* parentEventStack = DebugState->threadOpenEvents[threadIndex] + (threadOpenEventCount - 1);
            parentEvent = &parentEventStack->event;
            parentGuidHashEntry = parentEventStack->guidHashEntry;
            u64 lastCumulativeTiming = parentEvent->cumulativeChildTimings;
            parentGuidHashEntry->cumulativeChildTimings += totalBlockTime;
            DEBUG_PRINT("\t|%s| PARENT EVENT cumulativeChildTimings: %zu\n", parentEvent->GUID, parentGuidHashEntry->cumulativeChildTimings);
            if(parentGuidHashEntry->cumulativeChildTimings > parentEvent->clock){
                DEBUG_PRINT("cumulative child timings larger than event's timing?\n");
                __debugbreak();
            }
        }

        DebugState->debugElementBarTest[DebugState->debugElementBarCount++] = totalBlockTime;
        
        beginEvent.clock = totalBlockTime;
        beginEvent.cumulativeChildTimings = guidHash->cumulativeChildTimings;
        // if(beginEvent.cumulativeChildTimings > totalBlockTime){
            // DEBUG_PRINT("cumulative child timings larger than event's timing?\n");
            // __debugbreak();
        // }
        u32 currentFrameIndex = 0;
        if(guidHash->hits > 1){
            currentFrameIndex = guidHash->frameEventIndex;
        }else{
            currentFrameIndex = DebugState->currentFrameEventCount++;
            guidHash->frameEventIndex = currentFrameIndex;
        }
        DebugState->currentFrameEvents[currentFrameIndex].event = beginEvent;
        DebugState->currentFrameEvents[currentFrameIndex].cumulativeTime = guidHash->cumulativeTime;
        DebugState->currentFrameEvents[currentFrameIndex].hits = guidHash->hits;
        DEBUG_PRINT("\t|%s| event cumulativeChildTimings: %zu, event total time: %zu, cumulativeTime: %zu, HITS:!! %zu !!\n", guidHash->GUID, beginEvent.cumulativeChildTimings, totalBlockTime, guidHash->cumulativeTime, guidHash->hits);
        Assert(guidHash->GUID == beginEvent.GUID);
        // Assert(beginEvent.cumulativeChildTimings <= guidHash->cumulativeTime);


    }


}

struct barVal {
    u64 min;
    u64 max;
};

extern "C" DEBUG_GAME_FRAME_END(DEBUGGameFrameEnd) {

    GlobalDebugTable->CurrentEventArrayIndex = !GlobalDebugTable->CurrentEventArrayIndex;//swap buffers
    u64 ArrayIndex_EventIndex = AtomicExchangeU64(&GlobalDebugTable->EventArrayIndex_EventIndex, (u64)GlobalDebugTable->CurrentEventArrayIndex << 32);

    u32 EventArrayIndex = ArrayIndex_EventIndex >> 32;
    Assert(EventArrayIndex <= 1);
    u32 EventCount = ArrayIndex_EventIndex & 0xFFFFFFFF;
    debug_state* DebugState = (debug_state*)Memory->DebugStorage;
    bool discardNonFixedEvents = DebugState->last_frame_was_non_fixed_update;
    bool reachedFrameMarker = false;
    //need to reset all state/skip all events from the past few frames whenever a hot reload occurs to avoid garbage memory
    if (Memory->executable_reloaded) {
        DebugState->debugElementBarCount = 0;
        DebugState->currentFrameEventCount = 0;
        memset(DebugState->eventHistory, 0, sizeof(debug_event) * 300 * MAX_EVENTS_PER_FRAME);
        memset(GlobalDebugTable->Events[EventArrayIndex], 0, sizeof(debug_event) * 16*65536);
        EventCount = 0;
        DebugState->hotReloaded = true;
        discardNonFixedEvents = true;
        reachedFrameMarker = true;
        DebugState->openThreadIDCount = 0;
        DebugState->currentFrameCumulativeCycles = 0;
        DebugState->debugElementBarCount = 0;
        DebugState->currentFrameEventCount = 0;
        printf("debug hot reload, reset all memory\n");
        memcpy(DebugState->barEventHistory, DebugState->debugElementBarTest, 300 * sizeof(u64) * MAX_EVENTS_PER_FRAME);
        memcpy(DebugState->eventHistory, DebugState->currentFrameEvents, 300 * sizeof(histogram_debug_event) * MAX_EVENTS_PER_FRAME);
        memset(DebugState->threadOpenEventCount, 0, sizeof(u16) * MAX_WORKER_THREADS);
        memset(DebugState->threadOpenEvents, 0, sizeof(debug_event_stack) * MAX_WORKER_THREADS * 256);
        memset(DebugState->GUIDHash, 0, sizeof(GUIDHashEntry) * 1024 * 4);
        memset(DebugState->currentFrameEvents, 0, sizeof(histogram_debug_event) * MAX_EVENTS_PER_FRAME);
        DebugState->framesToSkip = 2;
    }

    game_state* GameState = (game_state*)Memory->PermanentStorage;
    u16 mainThreadID = Memory->mainThreadID;
    if (GameState->fixed_update) {
        DEBUG_PRINT("FIXED COLLATE\n");
        DebugState->last_frame_was_fixed_update = true;

        ui_data* uiData = GameState->uiData;

        char window_label[32] = "PROFILE DEBUG";
        ui_window* window = ui_begin_window(GameState, window_label);

        if (DebugState) {
            
            debug_start(GameState, DebugState);
            //collate debug frames here
            
            DebugState->lastFixedFrameCount = DebugState->curFrameCount;
            
            //BEGIN COLLATION LOOP
            
            u32 index = ((DebugState->currentEventHistoryIndex) % 300);
            DEBUG_PRINT("INDEX: %d\n", index);
            if(DebugState->framesToSkip){
                discardNonFixedEvents = true;
                // printf("skip this frame! framesToSkip: %d\n", DebugState->framesToSkip);
            }
            if(DebugState->framesToSkip > 0)DebugState->framesToSkip--;

            for (u32 i = 0; i < EventCount; i++) {
                
                debug_event* event = GlobalDebugTable->Events[EventArrayIndex] + i;

                // if(handmade_strcmp(event->GUID, "Chunk Visibility Destroy")){
                //     // __debugbreak();
                // }
                //we want to skip all non fixed events, including the frame marker, as long as we are discarding
                //but if we hit a non event frame marker, we want to set discardNonFixedEvents to false, but still skip
                if(discardNonFixedEvents){
                    if(event->type == DebugType_FrameMarker) discardNonFixedEvents = false;
                    continue;
                }

                
                switch (event->type) {

                case DebugType_FrameMarker: {
                    DEBUG_PRINT("fixed collate frame marker! frame event count: %d\n", DebugState->currentFrameEventCount);
                    AppendFrameMarker(DebugState, event);
                }break;

                case DebugType_BeginBlock: {
                    DEBUG_PRINT("fixed collate BEGIN block |%s|! frame event count: %d\n",event->GUID, DebugState->currentFrameEventCount);
                    AppendBeginBlock(DebugState, event);
                }break;
                case DebugType_EndBlock: {
                    DEBUG_PRINT("fixed collate END block   "); //completed in the next function
                    AppendEndBlock(DebugState, event);
                

                }break;
                }

            }

            DebugState->last_frame_was_non_fixed_update = false;
    



            debug_frame snapCurFrame = DebugState->curFrameHistory[index];
            debug_frame snapPrevFrame = DebugState->prevFrameHistory[index];

            //TODO: (nate) only for my main PC, how do we make this specific to the computer we run on?
            u64 cyclesPerSecond = 3610000000;
            float MSPerCycle = 1.0f / (cyclesPerSecond * 0.001f);
            
            //track the events before sorting
            //assumed the last event to be popped is the total encapsulating frame time
            u64 frameCycles = DebugState->totalCycleHistory[index];
            u64 cumulativeFrameCycles = 0;
            float frameMS = frameCycles * MSPerCycle;

            // for(u32 i = 0; i < DebugState->eventHistoryCount[index]; i++) {
    
            //     for(u32 j = 0; j < DebugState->eventHistoryCount[index] - i; j++) {
            //         u64 jTiming  = DebugState->eventHistory[index][j].cumulativeTime - DebugState->eventHistory[index][j].event.cumulativeChildTimings;
            //         u64 j1Timing = DebugState->eventHistory[index][j+1].cumulativeTime - DebugState->eventHistory[index][j+1].event.cumulativeChildTimings;
            //         if(jTiming < j1Timing) {
            //             // Swap
            //             histogram_debug_event temp = DebugState->eventHistory[index][j];
            //             DebugState->eventHistory[index][j] = DebugState->eventHistory[index][j + 1];
            //             DebugState->eventHistory[index][j + 1] = temp;
            //         }
            //     }
            // }



            float normalizedValue = 0.016f; //16 ms, 1 simulation tick is our time limit per frame
            //the normalized value is in seconds, what we do is divide cycle count by max cycles to get seconds as well
            //internally in the append bargraph function, we divide by the normalized value

            //3, 610, 000, 000, 3.610 gigahertz
            //take the cycles, divide by cyclesPerSecond, and then by 60 to get the duration

            float horBarGraphHeight = 60.0f;
            ui_element* barGraph = ui_begin_barGraph(GameState, normalizedValue, horBarGraphHeight, "bar graph test");

            ui_data* uiData = GameState->uiData;
            if (uiData->clicked_element) {
                DEBUG_PRINT("DebugState is processing uiData clicked element!\n");
                switch (uiData->clicked_element->type) {
                case ui_element_types::ui_element_type_bar: {
                    DEBUG_PRINT("clicked a bar element!\n");
                    switch (uiData->clicked_element->data.bar.type) {
                    case bargraph_profile_event: {
                        DEBUG_PRINT("bargraph profile event clock selected! clock: %zu\n", uiData->clicked_element->data.bar.data.eventClock);
                    }break;
                    case bargraph_profile_history_snapshot: {
                        DEBUG_PRINT("bargraph profile history index selected! index: %u\n", uiData->clicked_element->data.bar.data.eventIndex);
                        DebugState->currentEventHistoryIndex = uiData->clicked_element->data.bar.data.eventIndex;
                        index = ((DebugState->currentEventHistoryIndex) % 300);
                        DebugState->pauseCollation = true;
                    }break;
                    }
                }break;
                }

                uiData->clicked_element = nullptr;
            }
            ui_window* curr_window = uiData->windows + (ui_window_types::window_type_count + uiData->debug_window_count);
            DebugState->profileWindowBase = &curr_window->base;

            Assert(barGraph);
            DebugState->eventHistoryMax[index] = 0;
            bargraph_data barData = {};
            vec4 color = { 0, 1, 0, 1 }; //move from green to red as performance spikes

            for (u32 i = 0; i < DebugState->barHistoryCount[index]; i++) {

                float duration = (float)(((float)DebugState->barEventHistory[index][i] / ((float)cyclesPerSecond))) / normalizedValue;

                //set the max amount for each collation
                if (DebugState->eventHistoryMax[index] < DebugState->barEventHistory[index][i]) {
                    DebugState->eventHistoryMax[index] = DebugState->barEventHistory[index][i];
                }

                // DEBUG_PRINT("duration: %f\n", duration);
                barData.eventClock = DebugState->barEventHistory[index][i];
                color = { duration, 1 - duration, 0, 1 };

                ui_append_barGraph(GameState, 0, duration * barGraph->width, 0, barGraph->height, barGraph, bargraph_profile_event, &barData, &color);

            }

            //draw history in another bargraph beneath it
            float vertBarGraphHeight = 60.0f;
            ui_element* vertBarGraph = ui_begin_barGraph(GameState, normalizedValue, vertBarGraphHeight, "bar graph history");
            DebugState->historyGraph = vertBarGraph;
            float posx = 0;
            float width = barGraph->width / 300;
            for (u32 i = 0; i < 300; i++) {
                posx = i * width;
                float duration = (float)(((float)DebugState->totalCycleHistory[i] / ((float)cyclesPerSecond))) / normalizedValue;
                barData.eventIndex = i;
                color = { duration, 1 - duration, 0, 1 };
                ui_append_barGraph(GameState, posx, posx + width, 0, duration * barGraph->height, vertBarGraph, bargraph_profile_history_snapshot, &barData, &color);
            }
#if 1
            //timer histogram, should match the cycle histogram
            ui_element* timerBarGraph = ui_begin_barGraph(GameState, normalizedValue, vertBarGraphHeight, "timer graph history");
            posx = 0;
            width = barGraph->width / 300;
            for (u32 i = 0; i < 300; i++) {
                posx = i * width;
                // float duration = (float)(DebugState->timerHistory[i]) / normalizedValue;
                float duration = (float)(DebugState->timerHistory[i]) / normalizedValue;
                float compare = (float)(((float)DebugState->eventHistoryMax[i] / ((float)cyclesPerSecond))) / normalizedValue;
                // if(duration != compare)__debugbreak();

                color = { duration, 1 - duration, 0, 1 };
                ui_append_barGraph(GameState, posx, posx + width, 0, duration * barGraph->height, timerBarGraph, bargraph_profile_history_snapshot, &barData, &color);
            }
#endif

            //radix sort
            //find max time
            u32 eventCount = DebugState->eventHistoryCount[index];
            u64 max = 0;
            u64 maxPos = 0;
            for (u32 i = 0; i < eventCount; i++) {
                histogram_debug_event* histEvent = DebugState->eventHistory[index] + i;
                histEvent->remainingTime = histEvent->cumulativeTime - histEvent->event.cumulativeChildTimings;
                u64 time = histEvent->remainingTime;
                if(max < time){max = time; maxPos = i;}
                DebugState->radixTimes[i] = time;
            }
            
            for(int byte = 0; byte < 8; byte++){
                int count[256] = {};
                //count frequencies for this byte
                for(u32 i = 0; i < eventCount; i++){
                    u32 digit = (DebugState->eventHistory[index][i].remainingTime >> (byte * 8)) & 0xFF;
                    count[digit]++;
                }
                //prefix sum
                // for(int i = 1; i < 256; i++){
                //     count[i] += count[i-1];
                // }
                for(int i = 254; i >= 0; i--){
                    count[i] += count[i+1];
                }
                //build output (go backwards to preserve stability)
                for(int i = eventCount - 1; i >= 0; i--){
                    u32 digit = (DebugState->eventHistory[index][i].remainingTime >> (byte * 8)) & 0xFF;
                    DebugState->radixTemp[--count[digit]] = DebugState->eventHistory[index][i];
                }

                //copy back
                for(u32 i = 0; i < eventCount; i++){
                    DebugState->eventHistory[index][i] = DebugState->radixTemp[i];
                }


            }


            ui_text(GameState, "TOTAL FRAME TIME: %20zu, ms: %02.3f, frameIndex: %d", frameCycles, frameMS, index);
            DEBUG_PRINT("TOTAL FRAME TIME: %20zu, ms: %02.3f, frameIndex: %d\n", frameCycles, frameMS, index);

            
            if(DebugState->hotReloaded){
                DebugState->hotReloaded = false;
                //clear all possibly erroneous/nullified data here
                memset(DebugState->eventHistory, 0, sizeof(histogram_debug_event) * 300 * MAX_EVENTS_PER_FRAME);
                memset(DebugState->eventHistoryCount, 0, sizeof(u32) * 300);

                return GlobalDebugTable;
            }

            for (u32 i = 0; i < eventCount; i++) {

                //to get the timing of the current event since the start of the frame, use:
                // event->clock - snapPrevFrame.beginClock
                //not possible since we subtract the entTime - startTime


                //do we need 2 loops? One to sort, another to display?


                histogram_debug_event* histEvent = DebugState->eventHistory[index] + i;


                switch (histEvent->event.type) {

                    case DebugType_FrameMarker: {
                    }break;

                    case DebugType_BeginBlock: {

                        if(histEvent->event.cumulativeChildTimings){
                            //this block is a parent, contains sub block timings, need to subtract them from the total cycle count
                            //to determine if there is any leftover/uncaccounted for timing
                            u64 unaccountedCycles = histEvent->cumulativeTime - histEvent->event.cumulativeChildTimings;
                            if(handmade_strcmp(histEvent->event.GUID, "Chunk Visibility Destroy")){
                            // __debugbreak();
                                DEBUG_PRINT("CHUNK VIS DESTROY: cumulativeTime: %zu, cumulativeChildTimings: %zu\n", histEvent->cumulativeTime,  histEvent->event.cumulativeChildTimings);
                            }
                            if(histEvent->event.cumulativeChildTimings > histEvent->cumulativeTime){
                                DEBUG_PRINT("|%s| event cumulativeChildTimings: %zu, event timing: %zu\n", histEvent->event.GUID, histEvent->event.cumulativeChildTimings, histEvent->cumulativeTime);
                                int fuckTheDebugger = 0;
                            }
                            if(unaccountedCycles > 4000000000){
                                int fuckTheDebugger = 0;
                            }
                            ui_text(GameState, "RMNDR: %32s,  %10zu, hits: %4u | ms: %05.2f, %05.2f%%", histEvent->event.GUID, unaccountedCycles, histEvent->hits, unaccountedCycles * MSPerCycle, (float)(unaccountedCycles / (float)frameCycles)*100.0f);
                            cumulativeFrameCycles += unaccountedCycles;
                            DEBUG_PRINT("RMNDR: %32s,  %10zu, hits: %4u | ms: %05.2f, %05.2f%%\n", histEvent->event.GUID, unaccountedCycles, histEvent->hits, unaccountedCycles * MSPerCycle, (float)(unaccountedCycles / (float)frameCycles)*100.0f);
                        }
                        else{
                            ui_text(GameState, "BEGIN: %32s,  %10zu, hits: %4u | ms: %05.2f, %05.2f%%", histEvent->event.GUID, histEvent->cumulativeTime, histEvent->hits, histEvent->cumulativeTime * MSPerCycle, (float)(histEvent->cumulativeTime / (float)frameCycles)*100.0f);
                            cumulativeFrameCycles += histEvent->cumulativeTime;
                            DEBUG_PRINT("BEGIN: %32s,  %10zu, hits: %4u | ms: %05.2f, %05.2f%%\n", histEvent->event.GUID, histEvent->cumulativeTime, histEvent->hits, histEvent->cumulativeTime * MSPerCycle, (float)(histEvent->cumulativeTime / (float)frameCycles)*100.0f);

                        }
                            
                    }break;

                    case DebugType_EndBlock: {

                        ui_text(GameState, "END  : %32s,  %20zu, ms: %02.2f", histEvent->event.GUID, histEvent->cumulativeTime, histEvent->cumulativeTime * MSPerCycle);


                    }break;
                }

            }
            if(cumulativeFrameCycles != DebugState->totalCycleHistory[index]){
                printf("CUMULATIVE CYCLES FOR THIS FRAME %d DON'T MATCH! %zu != %zu !!!\n",index, cumulativeFrameCycles, DebugState->totalCycleHistory[index]);
                int debug = 0;
            }


            if(GameState->windowResized){
                GameState->windowResized = false;
                window->fixed = true;
            }

            if(window->fixed){//slop to fix the window on start, as soon as its moved/resized its no longer fixed
                ui_end_window(GameState, {0.4f, 0.5f, 1000.0f, 600.0f});
            }else{
                ui_end_window(GameState);
            }

            char noise_label[32] = "NOISE DEBUG";
            ui_window* noiseWindow = ui_begin_window(GameState, noise_label);
            
            ui_hash_entry* inlineElementHash = ui_begin_inline(GameState, 50.0f, "noiseInline1");
            // printf("first noise inline element: %p\n", inlineElementHash->element);
            
            bool testClicked = false;//not used for anything, needed for the function args because im retarded
            ui_element_data appendedData = {};
            ui_append_inline(GameState, ui_element_type_checkbox, "arrows", GameState->drawDerivArrows, 0, 50, 0, 50, inlineElementHash, appendedData);
            
            ui_append_inline(GameState, ui_element_type_checkbox, "PAUSE", DebugState->pauseCollation, 0, 50, 0, 50, inlineElementHash, appendedData);
            if (appendedData.checkbox.clicked) {
                DEBUG_PRINT("CLICKED PAUSE DEBUG COLLATION! %d\n", DebugState->pauseCollation);
            }
            ui_append_inline(GameState, ui_element_type_checkbox, "2dPRLN", GameState->perFramePerlin, 0, 50, 0, 50, inlineElementHash, appendedData);
            ui_append_inline(GameState, ui_element_type_checkbox, "Itrpl", GameState->interpolateCoarseNoise, 0, 50, 0, 50, inlineElementHash, appendedData);
            bool tempShuffleClick = false;
            bool tempSaveBmpClick = false;
            appendedData = {};
            // ui_append_inline(GameState, ui_element_type_button, "SHFL", tempShuffleClick, 0, 50, 0, 50, inlineElementHash, appendedData);
            char label[16] = {};
            label[0] = '0' + GameState->perlinDemoEnum;
            ui_append_inline(GameState, ui_element_type_button, label, tempShuffleClick, 0, 50, 0, 50, inlineElementHash, appendedData);
            if (appendedData.button.clicked) {
                // shuffleTiledPermTables(GameState);
                GameState->perlinDemoEnum++;
                GameState->perlinDemoEnum = GameState->perlinDemoEnum & 7;
            }
            if (appendedData.button.rightClicked) {
                // shuffleTiledPermTables(GameState);
                DEBUG_PRINT("button right clicked\n");
                GameState->perlinDemoEnum = 0;   
            }

            appendedData = {};
            ui_append_inline(GameState, ui_element_type_button, "BMP", tempSaveBmpClick, 0, 50, 0, 50, inlineElementHash, appendedData);
            if (appendedData.button.clicked) {
                writeToBMP("tiledNoise.bmp", GameState->textureTestMem, 512, 512);

            }
            ui_append_inline(GameState, ui_element_type_checkbox, "SIMD", GameState->perlinSIMD, 0, 50, 0, 50, inlineElementHash, appendedData);
            appendedData = {};
            //inline slider test
            vec4 sliderColor = { 0,1,0,1 };
            float minOffset = 0.0f;
            float maxOffset = 512.0f;
            float minTileScale = 1.0f;
            float maxTileScale = 8.0f;
            float minRedist = 0.5f;
            float maxRedist = 8.0f;
            float minWater = 0.0f;
            float maxWater = 2.0f;
            float derivBlendMin = 0.0f;
            float derivBlendMax = 1.0f;
            float heightMin = 255.0f;
            float heightMax = 2047.0f;
            ui_append_inline_slider_float(GameState, "ToffX", ui_element_type_horizontal_slider_float, 0, 128, 0, 50, inlineElementHash, appendedData, minOffset, maxOffset, GameState->tiledOffsetX, 0.0, true, &sliderColor, &GameState->regenChunks);
            ui_append_inline_slider_float(GameState, "ToffY", ui_element_type_horizontal_slider_float, 0, 128, 0, 50, inlineElementHash, appendedData, minOffset, maxOffset, GameState->tiledOffsetY, 0.0, true, &sliderColor, &GameState->regenChunks);
            ui_append_inline_slider_float(GameState, "ToffZ", ui_element_type_horizontal_slider_float, 0, 128, 0, 50, inlineElementHash, appendedData, minOffset, maxOffset, GameState->tiledOffsetZ, 0.0, true, &sliderColor, &GameState->regenChunks);
            // ui_append_inline_slider_float(GameState, "ToffW", ui_element_type_horizontal_slider_float, 0, 128, 0, 50, inlineElementHash, appendedData, minOffset, maxOffset, GameState->tiledOffsetW, 0.0, true, &sliderColor);
            ui_append_inline_slider_float(GameState, "TScl", ui_element_type_horizontal_slider_float, 0, 128, 0, 50, inlineElementHash, appendedData, minTileScale, maxTileScale, GameState->tiledScale, 6.0, true, &sliderColor);
            ui_append_inline_slider_float(GameState, "Redist", ui_element_type_horizontal_slider_float, 0, 128, 0, 50, inlineElementHash, appendedData, minRedist, maxRedist, GameState->perlinRedistribution, 1.0, true, &sliderColor);
            ui_append_inline_slider_float(GameState, "Water", ui_element_type_horizontal_slider_float, 0, 128, 0, 50, inlineElementHash, appendedData, minWater, maxWater, GameState->perlinWaterLevel, 0.0, true, &sliderColor);
            ui_append_inline_slider_float(GameState, "Hght", ui_element_type_horizontal_slider_float, 0, 128, 0, 50, inlineElementHash, appendedData, heightMin, heightMax, GameState->scaleFactor, 1023.0f, true, &sliderColor, &GameState->regenChunks );
            // ui_append_inline_slider_float(GameState, "dxBlnd", ui_element_type_horizontal_slider_float, 0, 128, 0, 50, inlineElementHash, appendedData, derivBlendMin, derivBlendMax, GameState->xDerivBlend, 0.0, true,  &sliderColor);
            // ui_append_inline_slider_float(GameState, "dyBlnd", ui_element_type_horizontal_slider_float, 0, 128, 0, 50, inlineElementHash, appendedData, derivBlendMin, derivBlendMax, GameState->yDerivBlend, 0.0, true,  &sliderColor);
            // ui_append_inline_slider_float(GameState, "dzBlnd", ui_element_type_horizontal_slider_float, 0, 128, 0, 50, inlineElementHash, appendedData, derivBlendMin, derivBlendMax, GameState->zDerivBlend, 0.0, true,  &sliderColor);
            ui_append_inline_slider_float(GameState, "time", ui_element_type_horizontal_slider_float, 0, 128, 0, 50, inlineElementHash, appendedData, derivBlendMin, derivBlendMax, GameState->timeScale, 0.0,true,  &sliderColor);



            ui_hash_entry* textureInlineElementHash = ui_begin_inline(GameState, 512.0f, "noiseInlineTexture0");
            ui_append_inline(GameState, ui_element_type_texture, "texture", testClicked, 0, 512, 0, 512,  textureInlineElementHash, appendedData, &color, 1, GameState->textureTestMem);
            ui_append_inline(GameState, ui_element_type_texture, "texture", testClicked, 0, 512, 0, 512, textureInlineElementHash, appendedData, &color, 1, GameState->textureTestMem);
            ui_append_inline(GameState, ui_element_type_texture, "texture", testClicked, 0, 512, 0, 512, textureInlineElementHash, appendedData, &color, 1, GameState->textureTestMem);
            float minScale = 0.001f;
            float maxScale = 0.25f;
            ui_append_inline_slider_float(GameState, "SCALE", ui_element_type_horizontal_slider_float, 0, 128, 0, 256, textureInlineElementHash, appendedData, minScale, maxScale, GameState->perlinScale, 0.009953125f, true, &sliderColor);
            vec4 testCol = { 1,0,0,1 };
            float minCoarse = 1.0f;
            float maxCoarse = 64.0f;
            ui_append_inline_slider_float(GameState, "CrsX", ui_element_type_horizontal_slider_float, 0, 128, 256, 512, textureInlineElementHash, appendedData, minCoarse, maxCoarse, GameState->coarsePerlinX, 8.0f, false, &testCol);
            ui_append_inline_slider_float(GameState, "CrsY", ui_element_type_vertical_slider_float, 0, 128, 0, 256, textureInlineElementHash, appendedData, minCoarse, maxCoarse, GameState->coarsePerlinY, 8.0f, true, &sliderColor);
            float minPeriod = 64.0f;
            float maxPeriod = 512.0f;
            ui_append_inline_slider_float(GameState, "Prd", ui_element_type_horizontal_slider_float, 0, 128, 256, 512, textureInlineElementHash, appendedData, minPeriod, maxPeriod, GameState->perlinPeriod, 8.0f, false, &testCol);
            // ui_append_inline_slider_float(GameState, "SLDRY", ui_element_type_vertical_slider_float, 0, 128, 256, 384, textureInlineElementHash, appendedData, GameState->perlinSliderMinRange, GameState->perlinSliderMaxRange, GameState->perlinYSlider, 0.0, false, &testCol);
            // ui_append_inline_slider_float(GameState, "SLDRY", ui_element_type_vertical_slider_float, 0, 128, 384, 512, textureInlineElementHash, appendedData, GameState->perlinSliderMinRange, GameState->perlinSliderMaxRange, GameState->perlinYSlider, 0.0, false, &testCol);
            // ui_texture(GameState, 512.0f, 1);
            //END COLLATION LOOP

            // ui_slider_float(GameState, GameState->perlinSliderMinRange, GameState->perlinSliderMaxRange, GameState->perlinXSlider, 0.0f, "SLIDER");
            //for 3x3 grid to see tiled noise results
            ui_hash_entry* textureInlineElementHash2 = ui_begin_inline(GameState, 512.0f,"noiseInlineTexture1");
            ui_append_inline(GameState, ui_element_type_texture, "texture", testClicked, 0, 512, 0, 512,  textureInlineElementHash2, appendedData, &color, 1, GameState->textureTestMem);
            ui_append_inline(GameState, ui_element_type_texture, "texture", testClicked, 0, 512, 0, 512,  textureInlineElementHash2, appendedData, &color, 1, GameState->textureTestMem);
            ui_append_inline(GameState, ui_element_type_texture, "texture", testClicked, 0, 512, 0, 512,  textureInlineElementHash2, appendedData, &color, 1, GameState->textureTestMem);
            ui_hash_entry* textureInlineElementHash3 = ui_begin_inline(GameState, 512.0f,"noiseInlineTexture1");
            ui_append_inline(GameState, ui_element_type_texture, "texture", testClicked, 0, 512, 0, 512,  textureInlineElementHash3, appendedData, &color, 1, GameState->textureTestMem);
            ui_append_inline(GameState, ui_element_type_texture, "texture", testClicked, 0, 512, 0, 512,  textureInlineElementHash3, appendedData, &color, 1, GameState->textureTestMem);
            ui_append_inline(GameState, ui_element_type_texture, "texture", testClicked, 0, 512, 0, 512,  textureInlineElementHash3, appendedData, &color, 1, GameState->textureTestMem);

        }
        ui_end_window(GameState);


        uint32_t new_index = GameState->currentTick & (SNAPSHOT_BUFFER_SIZE - 1);
        player_input& newInput = GameState->playerInputs[0][new_index];
        //ui debug tooltip
        char tooltip_label[32] = "TOOLTIP";

        #if 0
        ui_begin_window(GameState, tooltip_label, false);

        //TODO (nate): if we move the mouse out of this, the tooltip window vanishes, can just leave it as is, not really a big problem
        if (uiData->inline_element_selected) {
            if (uiData->selected_inline_element_memory.type == ui_element_type_texture) {
                vec2 localTextureCoords = {
                    floorf(fabs(uiData->inline_root.x + (float)uiData->selected_inline_element_memory.minx - (float)uiData->toolTipPos.x)),
                    floorf(fabs(uiData->inline_root.y + (float)uiData->selected_inline_element_parent->miny - (float)uiData->toolTipPos.y))
                };
                localTextureCoords.x = fmod(localTextureCoords.x,  512);//in case the mouse is over a neighboring region of the texture, map it back
                localTextureCoords.y = fmod(localTextureCoords.y,  512);//because the texture being used here is tiled
                u32 flippedY = (u32)((uiData->selected_inline_element_memory.height - 1) - localTextureCoords.y);
                ui_text(GameState, "%2.2f %2.2f", localTextureCoords.x, (float)flippedY);

                //print out texture color
                if (uiData->selected_inline_element_memory.data.texture.textureMemory) {
                    // ui_text(GameState, "%d", uiData->selected_inline_element_memory.data.texture.textureMemory[(u32)localTextureCoords.y * (u32)uiData->selected_inline_element_memory.width + (u32)localTextureCoords.y]);
                    u32 color = uiData->selected_inline_element_memory.data.texture.textureMemory[(u32)localTextureCoords.y * (u32)uiData->selected_inline_element_memory.width + (u32)localTextureCoords.x];
                    u8 r = color >> 16;
                    u8 g = color >> 8;
                    u8 b = color;
                    u8 a = color >> 24;
                    ui_text(GameState, "%d %d %d %d", r, g, b, a);

                    vec4 uicolor = { (float)b / 255.0f,(float)g / 255.0f,(float)r / 255.0f,(float)a / 255.0f };
                    //tweaked values for proper sRGB correction
                    uicolor.x = pow(uicolor.x, 2.2f);
                    uicolor.y = pow(uicolor.y, 2.2f);
                    uicolor.z = pow(uicolor.z, 2.2f);
                    ui_color(GameState, uicolor);

                    // float scale = 0.009953125f;
                    // // float val2d = (fnlPerlin2d(GameState->chunkData, {((float)(u32)localTextureCoords.x * scale) /* - scaledTime*0.5f */, ((float)(u32)localTextureCoords.y *scale)/*  - scaledTime*0.5f */})+1.0f) * 0.5f;
                    // // u8 noise2d = ((u8)(255 * val2d));


                    // vec2 vec = { ((float)(u32)localTextureCoords.x * scale) + GameState->perlinXSlider, ((float)(u32)localTextureCoords.y * scale) + GameState->perlinYSlider };
                    // int x0 = vec.x >= 0 ? (int)vec.x : ((int)vec.x - 1);
                    // int y0 = vec.y >= 0 ? (int)vec.y : ((int)vec.y - 1);
                    // float xf0 = (float)(vec.x - x0);
                    // float yf0 = (float)(vec.y - y0);
                    // float xf1 = xf0 - 1;
                    // float yf1 = yf0 - 1;

                    // float xs = noiseFade(xf0);
                    // float ys = noiseFade(yf0);
                    // ui_text(GameState, "inputs    %f %f", vec.x, vec.y);
                    // ui_text(GameState, "x0 : y0   %d %d", x0, y0);
                    // ui_text(GameState, "P %d %d", PRIME_X, PRIME_Y);
                    // ui_text(GameState, "xf0:yf0   %f %f", xf0, yf0);
                    // ui_text(GameState, "xf1:yf1   %f %f", xf1, yf1);
                    // ui_text(GameState, "xs : ys   %f %f", xs, ys);

                    // x0 *= PRIME_X;
                    // y0 *= PRIME_Y;

                    // int x1 = x0 + PRIME_X;
                    // int y1 = y0 + PRIME_Y;

                    // ui_text(GameState, "x0y0P %d %d", x0, y0);
                    // ui_text(GameState, "x1 : y1  %d %d", x1, y1);





                }



            }
        }

        float tooltipWidth = 400;
        float tooltipHeight = 200;
        float tooltipHalfWidth = tooltipWidth * 0.5f;
        float tooltipHalfHeight = tooltipHeight * 0.5f;
        ui_end_window(GameState, { ((float)uiData->toolTipPos.x + tooltipHalfWidth) / (float)*GameState->window_width, ((float)uiData->toolTipPos.y - tooltipHalfHeight) / (float)*GameState->window_height, tooltipWidth, tooltipHeight }, uiData->freezeTooltipWindow);
        #endif

    }
    else {//still collate
        DEBUG_PRINT("NON FIXED COLLATE\n");
        DebugState->last_frame_was_non_fixed_update = true;
        if (DebugState && DebugState->last_frame_was_fixed_update) {
            DebugState->last_frame_was_fixed_update = false;

    
            debug_start(GameState, DebugState);
            //collate debug frames here
            DebugState->lastNonFixedFrameCount = DebugState->curFrameCount;

            
            if(DebugState->framesToSkip){
                discardNonFixedEvents = true;
                // printf("skip this frame! framesToSkip: %d\n", DebugState->framesToSkip);
            }
            if(DebugState->framesToSkip > 0)DebugState->framesToSkip--;
            //BEGIN COLLATION LOOP
            DEBUG_PRINT("collate frames here!, event count: %u\n", EventCount);
            for (u32 i = 0; i < EventCount; i++) {

                if(reachedFrameMarker)break;//dont record anything past frame marker

                debug_event* event = GlobalDebugTable->Events[EventArrayIndex] + i;
                if(discardNonFixedEvents){
                    if(event->type == DebugType_FrameMarker) discardNonFixedEvents = false;
                    continue;
                }

                switch (event->type) {
                    case DebugType_FrameMarker: {
                        reachedFrameMarker = true;
                        DEBUG_PRINT("NON fixed collate frame marker! frame event count: %d\n", DebugState->currentFrameEventCount);
                        AppendFrameMarker(DebugState, event);

                    }break;

                    //still need to append events for consistency between frames and fixed update ticks
                    case DebugType_BeginBlock: {
                        DEBUG_PRINT("NON fixed collate BEGIN block |%s| frame event count: %d\n",event->GUID, DebugState->currentFrameEventCount);
                        AppendBeginBlock(DebugState, event);
                    }break;
                    case DebugType_EndBlock: {
                        DEBUG_PRINT("NON fixed collate END block   ");
                        AppendEndBlock(DebugState, event);
                    }break;
                }

            }

        }
    }
    DebugState->prevFrameCount = DebugState->curFrameCount++;


    return GlobalDebugTable;

}