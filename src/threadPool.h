#pragma once

#ifdef _WIN32
    #include <windows.h>
    typedef HANDLE semaphore_handle;
    typedef LONG volatile atomic_uint32;
    #define CompletePastWritesBeforeFutureWrites _WriteBarrier(); _mm_sfence()
    #define CompletePastReadWritesBeforeFutureReadWrites _ReadWriteBarrier(); _mm_mfence()
    #define CompletePastReadsBeforeFutureReads _ReadBarrier()
#else
    #include <pthread.h>
    #include <semaphore.h>
    typedef sem_t* semaphore_handle;
    typedef volatile uint32_t atomic_uint32;
    #define CompletePastWritesBeforeFutureWrites asm volatile("" ::: "memory"); __sync_synchronize()
    #define CompletePastReadWritesBeforeFutureReadWrites asm volatile("" ::: "memory"); __sync_synchronize()
    #define CompletePastReadsBeforeFutureReads asm volatile("" ::: "memory"); __sync_synchronize()
#endif



inline uint32_t AtomicAdd(atomic_uint32* Value, int32_t Addend) {
#ifdef _WIN32
    return InterlockedAdd((LONG volatile*)Value, Addend);
#else
    return __sync_add_and_fetch(Value, Addend);
#endif
}

inline uint32_t AtomicIncrement(atomic_uint32* Value) {
#ifdef _WIN32
    return InterlockedIncrement((LONG volatile*)Value);
#else
    return __sync_add_and_fetch(Value, 1);
#endif
}

inline uint32_t AtomicDecrement(atomic_uint32* Value) {
#ifdef _WIN32
    return InterlockedAdd((LONG volatile*)Value, -1);
#else
    return __sync_add_and_fetch(Value, -1);
#endif
}

inline uint32_t AtomicCompareExchange(atomic_uint32* Destination, uint32_t Exchange, uint32_t Comperand) {
#ifdef _WIN32
    return InterlockedCompareExchange((LONG volatile*)Destination, Exchange, Comperand);
#else
    return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
#endif
}

inline uint32_t AtomicExchangeU32(atomic_uint32* Destination, uint32_t Exchange) {
#ifdef _WIN32
    return InterlockedExchange((LONG volatile*)Destination, Exchange);
#else
    //need some unix version of exchange
    // return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
#endif
}


inline uint32_t AtomicRead(atomic_uint32* Value) {
    return AtomicCompareExchange(Value, 0, 0); // Exchange with same value = safe read
}

inline semaphore_handle CreatePlatformSemaphore(uint32_t InitialCount, uint32_t MaxCount) {
#ifdef _WIN32
    return CreateSemaphoreEx(0, InitialCount, MaxCount, 0, 0, SEMAPHORE_ALL_ACCESS);
#else
    sem_t* Semaphore = new sem_t;
    sem_init(Semaphore, 0, InitialCount);
    return Semaphore;
#endif
}

inline void WaitOnPlatformSemaphore(semaphore_handle Semaphore) {
#ifdef _WIN32
    WaitForSingleObjectEx(Semaphore, INFINITE, FALSE);
#else
    sem_wait(Semaphore);
#endif
}

inline void ReleasePlatformSemaphore(semaphore_handle Semaphore) {
#ifdef _WIN32
    ReleaseSemaphore(Semaphore, 1, 0);
#else
    sem_post(Semaphore);
#endif
}

inline void DestroyPlatformSemaphore(semaphore_handle Semaphore) {
#ifdef _WIN32
    CloseHandle(Semaphore);
#else
    sem_destroy(Semaphore);
    delete Semaphore;  // Free the allocated sem_t
#endif
}

inline uint32_t GetOSThreadID() {
#ifdef _WIN32
    return GetCurrentThreadId();
#else
    return (uint32_t)syscall(SYS_gettid);
#endif
}

struct platform_work_queue_entry {
    platform_work_queue_callback *Callback;
    void* Data;
};

struct platform_work_queue{
    atomic_uint32 MaxEntryCount;
    atomic_uint32 CompletionGoal;
    atomic_uint32 CompletionCount;

    atomic_uint32 NextEntryToWrite;
    atomic_uint32 NextEntryToRead;
    semaphore_handle SemaphoreHandle;

    platform_work_queue_entry Entries[256];
    //limitation is that we can't queue too much work at once
};


struct thread_info {
    int LogicalThreadIndex;
    platform_work_queue *Queue;
};


// Static allocations
extern atomic_uint32 EntryCompletionCount;
extern atomic_uint32 NextEntryToDo;
extern atomic_uint32 EntryCount;
extern atomic_uint32 ActiveThreadCount;
// Add a shutdown flag
extern atomic_uint32 ShouldThreadsExit;



