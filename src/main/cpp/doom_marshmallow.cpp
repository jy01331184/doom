//
// Created by tianyang on 2018/11/13.
//
#include <dlfcn.h>
#include "doom.h"
#include "jni.h"

/**
 * http://androidxref.com/6.0.1_r10/xref/art/runtime/gc/heap.h
 * http://androidxref.com/6.0.1_r10/xref/art/runtime/gc/heap-inl.h#418
 */

typedef struct Marshmallow_Heap{
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

    // Whether or not we currently care about pause times.
    int process_state_;
    // When num_bytes_allocated_ exceeds this amount then a concurrent GC should be requested so that
    // it completes ahead of an allocation failing.
    size_t concurrent_start_bytes_;
} MarshmallowHeap;

void dumpMarshmallowHeap(MarshmallowHeap *heap){
    if(heap){
        DOOM_LOG("MarshmallowHeap capacity_=%um growth_limit_=%um max_allowed_footprint_=%um native_need_to_run_finalization_=%d process_state_=%d concurrent_start_bytes_=%um next_gc_type_=%d",heap->capacity_/SIZE_M,heap->growth_limit_/SIZE_M,heap->max_allowed_footprint_/SIZE_M,heap->native_need_to_run_finalization_,heap->process_state_,heap->concurrent_start_bytes_/SIZE_M,heap->next_gc_type_);
    }
}
bool compensation = false;
MarshmallowHeap * marshmallowHeap;

GcType (*oldMarshmallowCollectGarbageInternal)(void *heap, GcType gcType, int gcCause, bool clear_soft_references);

GcType marshmallowCollectGarbageInternal(void *heap, GcType gcType, int gcCause, bool clear_soft_references) {
    if(dooming && !marshmallowHeap){
        int heapAddr = reinterpret_cast<int>(heap);
        int growth_index = search(heapAddr,initial_growth_limit,100);
        if(growth_index > 0){
            int *pHeap = static_cast<int *>(heap);
            marshmallowHeap = reinterpret_cast<MarshmallowHeap *>(pHeap + (growth_index - 2) );
            if(marshmallowHeap->capacity_ < marshmallowHeap->growth_limit_ || marshmallowHeap->process_state_ >=1 || marshmallowHeap->next_gc_type_  >4){
                marshmallowHeap = reinterpret_cast<MarshmallowHeap *>(pHeap + (growth_index - 1));
                DOOM_LOG("try fix growth_index - 2 to growth_index - 1 ");
            }
            if(marshmallowHeap->capacity_ < marshmallowHeap->growth_limit_ || marshmallowHeap->process_state_ >=1 || marshmallowHeap->next_gc_type_  >4){
                DOOM_LOG("marshmallowCollectGarbageInternal invalid heap struct");
                dumpMarshmallowHeap(marshmallowHeap);
                dooming = false;
                marshmallowHeap = NULL;
                return oldMarshmallowCollectGarbageInternal(heap, gcType, gcCause, clear_soft_references);
            }
            dumpMarshmallowHeap(marshmallowHeap);
            int max_allowed_footprint_value = marshmallowHeap->max_allowed_footprint_/SIZE_M;
            int concurrent_start_bytes_value = marshmallowHeap->concurrent_start_bytes_/SIZE_M;
            if(max_allowed_footprint_value > 0 && concurrent_start_bytes_value >= 0){
                marshmallowHeap->max_allowed_footprint_ = marshmallowHeap->growth_limit_;
                initial_concurrent_start_bytes = marshmallowHeap->concurrent_start_bytes_;
                marshmallowHeap->concurrent_start_bytes_ = 1000 * SIZE_M;
                DOOM_LOG("marshmallowCollectGarbageInternal valid heap struct");
            } else {
                DOOM_LOG("marshmallowCollectGarbageInternal invalid heap struct");
                dumpMarshmallowHeap(marshmallowHeap);
                dooming = false;
                marshmallowHeap = NULL;
            }
            return GcType::kGcTypeSticky;
        } else {
            dooming = false;
            DOOM_LOG("marshmallowCollectGarbageInternal can't find growth_index");
        }
    } else if(dooming && marshmallowHeap && compensation){
        DOOM_LOG("marshmallowCollectGarbageInternal compensation from %um to %um",marshmallowHeap->max_allowed_footprint_/SIZE_M,marshmallowHeap->growth_limit_/SIZE_M);
        marshmallowHeap->max_allowed_footprint_ = marshmallowHeap->growth_limit_;
        marshmallowHeap->concurrent_start_bytes_ = 1000 * SIZE_M;
        compensation = false;
        return GcType::kGcTypeSticky;
    }
//    size_t heapSize = marshmallowHeap ? marshmallowHeap->max_allowed_footprint_/SIZE_M:0;
//    DOOM_LOG("marshmallowCollectGarbageInternal heap:%um GcType=%d GcCause=%d",heapSize,gcType,gcCause);

    GcType ret = oldMarshmallowCollectGarbageInternal(heap, gcType, gcCause, clear_soft_references);
    compensation = true;
    return ret;
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_doom_Doom_initDoomMarshmallow(JNIEnv *env, jclass type,jlong growthLimit) {
    initial_growth_limit = growthLimit;

    void *artso = dlopen("libart.so",RTLD_LAZY);
    if(artso){
        void* func = (dlsym(artso, "_ZN3art2gc4Heap22CollectGarbageInternalENS0_9collector6GcTypeENS0_7GcCauseEb"));
        DOOM_LOG("Art gc func addr=%p",func);
        if(func){
            int result = hook(func,(void*)(marshmallowCollectGarbageInternal),(void**)(&oldMarshmallowCollectGarbageInternal));
            return result ? JNI_TRUE : JNI_FALSE;
        }
    }

    return JNI_FALSE;

}

extern "C" JNIEXPORT void JNICALL Java_com_doom_Doom_doomMarshmallow(JNIEnv *env, jclass type) {
    dooming = true;
}

extern "C" JNIEXPORT void JNICALL Java_com_doom_Doom_unDoomMarshmallow(JNIEnv *env, jclass type) {
    if(dooming){
        DOOM_LOG("unDoomMarshmallow");
        marshmallowHeap->max_allowed_footprint_ = 2 * SIZE_M;
        marshmallowHeap->concurrent_start_bytes_ = initial_concurrent_start_bytes;
        dumpMarshmallowHeap(marshmallowHeap);
    }
    marshmallowHeap = NULL;
    dooming = false;
}