//
// Created by tianyang on 2018/11/13.
//
#include "doom.h"
#include "jni.h"

/**
 * http://androidxref.com/7.0.0_r1/xref/art/runtime/gc/heap.h
 */

typedef struct Nougat_Heap{
    int next_gc_type_;

    // Maximum size that the heap can reach.
    size_t capacity_;

    // The size the heap is limited to. This is initially smaller than capacity, but for largeHeap
    // programs it is "cleared" making it the same as capacity.
    size_t growth_limit_;

    // When the number of bytes allocated exceeds the footprint TryAllocate returns null indicating
    // a GC should be triggered.
    size_t max_allowed_footprint_;

    // The watermark at which a concurrent GC is requested by registerNativeAllocation.
    size_t native_footprint_gc_watermark_;

    // Whether or not we need to run finalizers in the next native allocation.
    bool native_need_to_run_finalization_;

    // When num_bytes_allocated_ exceeds this amount then a concurrent GC should be requested so that
    // it completes ahead of an allocation failing.
    size_t concurrent_start_bytes_;
} NougatHeap;

void dumpNougatHeap(NougatHeap *heap){
    if(heap){
        DOOM_LOG("NougatHeap capacity_=%um growth_limit_=%um max_allowed_footprint_=%um concurrent_start_bytes_=%um next_gc_type_=%d",heap->capacity_/SIZE_M,heap->growth_limit_/SIZE_M,heap->max_allowed_footprint_/SIZE_M,heap->concurrent_start_bytes_/SIZE_M,heap->next_gc_type_);
    }
}

NougatHeap * nougatHeap;

GcType (*oldNougatCollectGarbageInternal)(void *heap, GcType gcType, int gcCause, bool clear_soft_references);

GcType nougatCollectGarbageInternal(void *heap, GcType gcType, int gcCause, bool clear_soft_references) {
    if(dooming && !nougatHeap){
        int heapAddr = reinterpret_cast<int>(heap);
        int growth_index = search(heapAddr,initial_growth_limit,100);
        if(growth_index > 0){
            int *pHeap = static_cast<int *>(heap);
            nougatHeap = reinterpret_cast<NougatHeap *>(pHeap + (growth_index - 2) );
            if(nougatHeap->capacity_ < nougatHeap->growth_limit_ || nougatHeap->next_gc_type_  >4){
                nougatHeap = reinterpret_cast<NougatHeap *>(pHeap + (growth_index - 1));
                DOOM_LOG("try fix growth_index - 2 to growth_index - 1 ");
            }
            if(nougatHeap->capacity_ < nougatHeap->growth_limit_ || nougatHeap->next_gc_type_  >4){
                DOOM_LOG("nougatCollectGarbageInternal invaild heap struct");
                dumpNougatHeap(nougatHeap);
                dooming = false;
                nougatHeap = NULL;
                return oldNougatCollectGarbageInternal(heap, gcType, gcCause, clear_soft_references);
            }
            dumpNougatHeap(nougatHeap);
            int max_allowed_footprint_value = nougatHeap->max_allowed_footprint_/SIZE_M;
            int concurrent_start_bytes_value = nougatHeap->concurrent_start_bytes_/SIZE_M;
            if(max_allowed_footprint_value > 0 && concurrent_start_bytes_value > 0){
                nougatHeap->max_allowed_footprint_ = nougatHeap->growth_limit_;
                initial_concurrent_start_bytes = nougatHeap->concurrent_start_bytes_;
                nougatHeap->concurrent_start_bytes_ = 1000 * SIZE_M;
                DOOM_LOG("nougatCollectGarbageInternal modify heap");
            } else {
                DOOM_LOG("nougatCollectGarbageInternal invaild heap struct");
                dumpNougatHeap(nougatHeap);
                dooming = false;
                nougatHeap = NULL;
            }
            return GcType::kGcTypeSticky;
        } else {
            DOOM_LOG("nougatCollectGarbageInternal can't find growth_index");
            dooming = false;
        }
    }

    DOOM_LOG("nougatCollectGarbageInternal GcType=%d GcCause=%d",gcType,gcCause);
    return oldNougatCollectGarbageInternal(heap, gcType, gcCause, clear_soft_references);
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_doom_Doom_initDoomNougat(JNIEnv *env, jclass type,jint growthLimit,jint maxAllowedFootprint) {
    initial_growth_limit = growthLimit;

    char *allocateJavaSymbol = "_ZN3art2gc4Heap22CollectGarbageInternalENS0_9collector6GcTypeENS0_7GcCauseEb";

    jint result = hook("libart.so", allocateJavaSymbol, (void *) nougatCollectGarbageInternal, (void **) &oldNougatCollectGarbageInternal);

    return result ? JNI_TRUE : JNI_FALSE;

}

extern "C" JNIEXPORT void JNICALL Java_com_doom_Doom_doomNougat(JNIEnv *env, jclass type) {
    dooming = true;
}

extern "C" JNIEXPORT void JNICALL Java_com_doom_Doom_unDoomNougat(JNIEnv *env, jclass type) {
    if(dooming){
        DOOM_LOG("unDoomNougat");
        nougatHeap->max_allowed_footprint_ = 2 * SIZE_M;
        nougatHeap->concurrent_start_bytes_ = initial_concurrent_start_bytes;
        dumpNougatHeap(nougatHeap);
    }
    nougatHeap = NULL;
    dooming = false;
}