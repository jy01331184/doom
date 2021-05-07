//
// Created by tianyang on 2018/11/13.
//

#ifndef NATIVE_DOOM_H
#define NATIVE_DOOM_H

#include "jni.h"
#include "stddef.h"
#include "android/log.h"
extern bool dooming;
extern double cost;
extern int initial_growth_limit;
extern int initial_concurrent_start_bytes;
extern int hook(char *libSo, char *targetSymbol, void *newFunc, void **oldFunc);
extern int hook(void *funcPtr, void *newFunc, void **oldFunc);
extern bool isGoodPtr(void* ptr);

extern double getCurrentTime();

extern int search(int addr, int targetValue, int maxSearchTime);
#define SIZE_M  (1024*1024)
#define SIZE_K  (1024)
#define  DOOM_LOG_TAG    "doom_log"
#define  DOOM_LOG(...)  __android_log_print(ANDROID_LOG_DEBUG,DOOM_LOG_TAG,__VA_ARGS__)


enum GcType {
    // Placeholder for when no GC has been performed.
            kGcTypeNone,
    // Sticky mark bits GC that attempts to only free objects allocated since the last GC.
            kGcTypeSticky,
    // Partial GC that marks the application heap but not the Zygote.
            kGcTypePartial,
    // Full GC that marks and frees in both the application and Zygote heap.
            kGcTypeFull,
    // Number of different GC types.
            kGcTypeMax,
};

enum GcCause {
    // GC triggered by a failed allocation. Thread doing allocation is blocked waiting for GC before
    // retrying allocation.
            kGcCauseForAlloc,
    // A background GC trying to ensure there is free memory ahead of allocations.
            kGcCauseBackground,
    // An explicit System.gc() call.
            kGcCauseExplicit,
    // GC triggered for a native allocation.
            kGcCauseForNativeAlloc,
    // GC triggered for a collector transition.
            kGcCauseCollectorTransition,
    // Not a real GC cause, used when we disable moving GC (currently for GetPrimitiveArrayCritical).
            kGcCauseDisableMovingGc,
    // Not a real GC cause, used when we trim the heap.
            kGcCauseTrim,
    // GC triggered for background transition when both foreground and background collector are CMS.
            kGcCauseHomogeneousSpaceCompact,
};


#endif //NATIVE_DOOM_H

