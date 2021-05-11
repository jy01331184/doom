//
// Created by tianyang on 2021/5/10.
//

#ifndef NATIVE_ART_H
#define NATIVE_ART_H
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
#endif //NATIVE_ART_H
