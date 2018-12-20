//
// Created by tianyang on 2018/11/13.
//

#ifndef NATIVE_DOOM_H
#define NATIVE_DOOM_H

#include "jni.h"
#include "stddef.h"
#include "android/log.h"
extern bool dooming;
extern jclass doomClass;
extern int initial_growth_limit;
extern int initial_concurrent_start_bytes;
extern void initDoom(JNIEnv * env);
extern int hook(char *libSo, char *targetSymbol, void *newFunc, void **oldFunc);
extern int search(int addr, int targetValue, int maxSearchTime);
#define SIZE_M  (1024*1024)
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

#endif //NATIVE_DOOM_H
