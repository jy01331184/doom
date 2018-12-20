//
// Created by tianyang on 2018/11/13.
//
#include "doom.h"
#include "jni.h"

/**
 * http://androidxref.com/8.0.0_r4/xref/art/runtime/gc/heap.h
 */

typedef struct Oreo_Heap{
    int next_gc_type_;

    // Maximum size that the heap can reach.
    size_t capacity_;

    // The size the heap is limited to. This is initially smaller than capacity, but for largeHeap
    // programs it is "cleared" making it the same as capacity.
    size_t growth_limit_;

    // When the number of bytes allocated exceeds the footprint TryAllocate returns null indicating
    // a GC should be triggered.
    size_t max_allowed_footprint_;

    // When num_bytes_allocated_ exceeds this amount then a concurrent GC should be requested so that
    // it completes ahead of an allocation failing.
    size_t concurrent_start_bytes_;
} OreoHeap;

void dumpOreoHeap(OreoHeap *heap){
    if(heap){
        DOOM_LOG("OreoHeap capacity_=%um growth_limit_=%um max_allowed_footprint_=%um concurrent_start_bytes_=%um next_gc_type_=%d",heap->capacity_/SIZE_M,heap->growth_limit_/SIZE_M,heap->max_allowed_footprint_/SIZE_M,heap->concurrent_start_bytes_/SIZE_M,heap->next_gc_type_);
    }
}

OreoHeap * oreoHeap;

GcType (*oldOreoCollectGarbageInternal)(void *heap, GcType gcType, int gcCause, bool clear_soft_references);

GcType oreoCollectGarbageInternal(void *heap, GcType gcType, int gcCause, bool clear_soft_references) {
    if(dooming && !oreoHeap){
        int heapAddr = reinterpret_cast<int>(heap);
        int growth_index = search(heapAddr,initial_growth_limit,100);
        if(growth_index > 0){
            int *pHeap = static_cast<int *>(heap);
            oreoHeap = reinterpret_cast<OreoHeap *>(pHeap + (growth_index - 2) );
            if(oreoHeap->capacity_ < oreoHeap->growth_limit_ || oreoHeap->next_gc_type_  >4){
                oreoHeap = reinterpret_cast<OreoHeap *>(pHeap + (growth_index - 1));
                DOOM_LOG("try fix growth_index - 2 to growth_index - 1 ");
            }
            if(oreoHeap->capacity_ < oreoHeap->growth_limit_ || oreoHeap->next_gc_type_  >4){
                DOOM_LOG("oreoCollectGarbageInternal invaild heap struct");
                dumpOreoHeap(oreoHeap);
                dooming = false;
                oreoHeap = NULL;
                return oldOreoCollectGarbageInternal(heap, gcType, gcCause, clear_soft_references);
            }
            dumpOreoHeap(oreoHeap);
            int max_allowed_footprint_value = oreoHeap->max_allowed_footprint_/SIZE_M;
            int concurrent_start_bytes_value = oreoHeap->concurrent_start_bytes_/SIZE_M;
            if(max_allowed_footprint_value > 0 && concurrent_start_bytes_value > 0){
                oreoHeap->max_allowed_footprint_ = oreoHeap->growth_limit_;
                initial_concurrent_start_bytes = oreoHeap->concurrent_start_bytes_;
                oreoHeap->concurrent_start_bytes_ = 1000 * SIZE_M;
                DOOM_LOG("oreoCollectGarbageInternal modify heap");
            } else {
                DOOM_LOG("oreoCollectGarbageInternal invaild heap struct");
                dumpOreoHeap(oreoHeap);
                dooming = false;
                oreoHeap = NULL;
            }
            return GcType::kGcTypeSticky;
        } else {
            DOOM_LOG("oreoCollectGarbageInternal can't find growth_index");
            dooming = false;
        }
    }

    DOOM_LOG("oreoCollectGarbageInternal GcType=%d GcCause=%d",gcType,gcCause);
    return oldOreoCollectGarbageInternal(heap, gcType, gcCause, clear_soft_references);
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_doom_Doom_initDoomOreo(JNIEnv *env, jobject type,jint growthLimit,jint maxAllowedFootprint) {
    initial_growth_limit = growthLimit;
    initDoom(env);

    char *allocateJavaSymbol = "_ZN3art2gc4Heap22CollectGarbageInternalENS0_9collector6GcTypeENS0_7GcCauseEb";

    jint result = hook("libart.so", allocateJavaSymbol, (void *) oreoCollectGarbageInternal, (void **) &oldOreoCollectGarbageInternal);

    return result ? JNI_TRUE : JNI_FALSE;

}

extern "C" JNIEXPORT void JNICALL Java_com_doom_Doom_doomOreo(JNIEnv *env, jobject type) {
    dooming = true;
}

extern "C" JNIEXPORT void JNICALL Java_com_doom_Doom_unDoomOreo(JNIEnv *env, jobject type) {
    if(dooming){
        DOOM_LOG("unDoomOreo");
        oreoHeap->max_allowed_footprint_ = 2 * SIZE_M;
        oreoHeap->concurrent_start_bytes_ = initial_concurrent_start_bytes;
        dumpOreoHeap(oreoHeap);
    }
    oreoHeap = NULL;
    dooming = false;
}