//
// Created by tianyang on 2021/4/29.
//

#include <dlfcn.h>
#include <string.h>
#include "doom.h"
#include "jni.h"
#include "dalvik.h"

typedef void* (*dvmAddrFromCard)(uint8_t *cardAddr);
#define GC_CARD_SHIFT 7
#define HEAP_UTILIZATION_MAX 1024


GcHeap* globalHeap;
size_t originMaxFree;
size_t originTtargetUtilization;

void (*oldDvmCollectGarbageInternal)(const GcSpec* spec);

void hookedDvmCollectGarbageInternal(const GcSpec* spec){
//    double start = getCurrentTime();
    oldDvmCollectGarbageInternal(spec);
//    DOOM_LOG("hookedDvmCollectGarbageInternal: %s->part=%d concurront=%d",spec->reason,spec->isPartial,spec->isConcurrent);
//    if(0 == strcmp(spec->reason, "GC_BEFORE_OOM")){
//        DOOM_LOG("hookedDvmCollectGarbageInternal: before oom");
//        oldDvmCollectGarbageInternal(spec);
//    } else {
//        oldDvmCollectGarbageInternal(spec);
//        //DOOM_LOG("hookedDvmCollectGarbageInternal: pass");
//    }
//    cost += getCurrentTime() - start;
}

int locateByCardTableBase(int addr,uintptr_t cardRet,int time) {
    int *p_addr = reinterpret_cast<int *>(addr);
    uint8_t *tempPtr = 0;
    for (int i = 0; i <= time; ++i) {
        uint8_t * biasedCardTableBase = reinterpret_cast<uint8_t *>( *(p_addr + i));
        uintptr_t offset = tempPtr - biasedCardTableBase;
        uintptr_t ret = (offset << GC_CARD_SHIFT);
        if(ret == cardRet){
            return i-1;
        }
    }
    return -1;
}


extern "C"
JNIEXPORT jboolean JNICALL
Java_com_doom_Doom_initGcDalvik(JNIEnv *env, jclass clazz, jlong growth_limit,jfloat targetHeapUtilization) {

    if(globalHeap != NULL){
        return JNI_TRUE;
    }

    void *dvmso = dlopen("libdvm.so", RTLD_LAZY);
    if(dvmso == NULL){
        DOOM_ERROR("dlopen libdvm.so fail");
        return JNI_FALSE;
    }

    int* pDvm = static_cast<int *>(dlsym(dvmso, "gDvm"));
    if(pDvm == NULL){
        DOOM_ERROR("dlsym dvm fail");
        return JNI_FALSE;
    }

    int dvmAddr = reinterpret_cast<int>(pDvm);

    dvmAddrFromCard f = reinterpret_cast<dvmAddrFromCard>(dlsym(dvmso, "_Z15dvmAddrFromCardPKh"));
    if(f == NULL){
        DOOM_ERROR("can't locate dvmAddrFromCard");
        return JNI_FALSE;
    }

    uint8_t *tempPtr = 0;
    uintptr_t ret = reinterpret_cast<uintptr_t>(f(tempPtr));
    int index = locateByCardTableBase(dvmAddr ,ret,400);

    if(index < 0){
        DOOM_ERROR("can't locate gcHeap from dvm");
        return JNI_FALSE;
    }

    GcHeap * gcHeap = reinterpret_cast<GcHeap *>(*(pDvm + index));
    if(!isGoodPtr(gcHeap) || !isGoodPtr(gcHeap->heapSource)) {
        DOOM_ERROR("gcHeap inavailable offset=%d", index);
        return JNI_FALSE;
    }

    DOOM_INFO("located gcHeap at %p,offset=%d,heapSource=%p", gcHeap, index, gcHeap->heapSource);

    HeapSource* heapSource = gcHeap->heapSource;
    //validate gcHeap
    if(heapSource->growthLimit != growth_limit || heapSource->growthLimit > heapSource->maximumSize){
        DOOM_ERROR("gcHeap inavailable growthLimit %d:%d", heapSource->growthLimit,growth_limit);
        return JNI_FALSE;
    }

    if(heapSource->numHeaps <= 0 || heapSource->numHeaps > 2){
        DOOM_ERROR("gcHeap inavailable numHeaps %d", heapSource->numHeaps);
        return JNI_FALSE;
    }

    if(heapSource->minFree < 500 * SIZE_K || heapSource->minFree > 20 * SIZE_M || heapSource->maxFree < 1 * SIZE_M || heapSource->maxFree > 128 * SIZE_M || heapSource->maxFree < heapSource->minFree){
        DOOM_ERROR("gcHeap inavailable min=%d,max=%d", heapSource->minFree,heapSource->maxFree);
        return JNI_FALSE;
    }

    if(heapSource->targetUtilization != targetHeapUtilization * HEAP_UTILIZATION_MAX){
        DOOM_ERROR("gcHeap inavailable targetUtilization=%d,%lf", heapSource->targetUtilization,targetHeapUtilization);
        return JNI_FALSE;
    }

    DOOM_INFO("gcHeap targetUtilization=%d numHeap=%d", gcHeap->heapSource->targetUtilization,gcHeap->heapSource->numHeaps);
    DOOM_INFO("growth=%dMb,maxSize=%dMb,maxFree=%dMb,minfree=%uKb",
             gcHeap->heapSource->growthLimit / SIZE_M, gcHeap->heapSource->maximumSize / SIZE_M,
             gcHeap->heapSource->maxFree / SIZE_M, gcHeap->heapSource->minFree / SIZE_K);
    DOOM_INFO("heap softlimit:%u=%u", gcHeap->heapSource->softLimit, UINT32_MAX);
    DOOM_INFO("heap concurrent:%uMb",gcHeap->heapSource->heaps[0].concurrentStartBytes/SIZE_M);
    globalHeap = gcHeap;

//    void* func = (dlsym(dvmso, "_Z25dvmCollectGarbageInternalPK6GcSpec"));
//    if(func){
//        int result = hook(func,(void*)(hookedDvmCollectGarbageInternal),(void**)(&oldDvmCollectGarbageInternal));
//
//        return result ? JNI_TRUE : JNI_FALSE;
//    }
    return JNI_TRUE;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_doom_Doom_pauseGcDalvik(JNIEnv *env, jclass clazz) {
    if(globalHeap){
        DOOM_INFO("pauseGc");
        watchSig();
        dooming = true;
        originMaxFree = globalHeap->heapSource->maxFree;
        originTtargetUtilization = globalHeap->heapSource->targetUtilization;
        globalHeap->heapSource->heaps[0].concurrentStartBytes = 1024*SIZE_M;
        globalHeap->heapSource->targetUtilization = 1;
        globalHeap->heapSource->maxFree = 1024 * SIZE_M;
    } else {
        DOOM_ERROR("pauseGc globalHeap null,unreachable code");
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_doom_Doom_resumeGcDalvik(JNIEnv *env, jclass clazz) {
    if(globalHeap){
        DOOM_INFO("resumeGc");
        unwatchSig();
        dooming = false;
        globalHeap->heapSource->targetUtilization = originTtargetUtilization;
        globalHeap->heapSource->maxFree = originMaxFree;
    } else {

    }

//    DOOM_LOG("heap max:%dMb",globalHeap->heapSource->maximumSize/SIZE_M);
//    DOOM_LOG("heap maxfree=%uMb minfree=%uMb",globalHeap->heapSource->maxFree/SIZE_M,globalHeap->heapSource->minFree/SIZE_M);
//    DOOM_LOG("heap growth:%uMb",globalHeap->heapSource->growthLimit/SIZE_M);
//    DOOM_LOG("heap softlimit:%u=%u",globalHeap->heapSource->softLimit,UINT32_MAX);
//    DOOM_LOG("heap idealSize:%uMb",globalHeap->heapSource->idealSize/SIZE_M);
//    DOOM_LOG("heap[0] alloc:%dMb",globalHeap->heapSource->heaps[0].bytesAllocated/SIZE_M);
//    DOOM_LOG("heap[0] max:%dMb",globalHeap->heapSource->heaps[0].maximumSize/SIZE_M);
//    DOOM_LOG("heap[0] conccur:%dMb",globalHeap->heapSource->heaps[0].concurrentStartBytes/SIZE_M);
//    DOOM_LOG("heap[0] target:%u",globalHeap->heapSource->targetUtilization);
}