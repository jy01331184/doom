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
extern bool hookLog;
extern int initial_growth_limit;
extern int initial_concurrent_start_bytes;
extern int hook(char *libSo, char *targetSymbol, void *newFunc, void **oldFunc);
extern int hook(void *funcPtr, void *newFunc, void **oldFunc);
extern bool isGoodPtr(void* ptr);

extern double getCurrentTime();
extern int search(int addr, int targetValue, int maxSearchTime);
extern void doLog(int level,const char* message,...);
extern void watchSig();
extern void unwatchSig();

#define SIZE_M  (1024*1024)
#define SIZE_K  (1024)
#define DOOM_LOG_TAG    "doom_log"
#define DOOM_LOG(...)  __android_log_print(ANDROID_LOG_DEBUG,DOOM_LOG_TAG,__VA_ARGS__)
#define DOOM_INFO(...) doLog(ANDROID_LOG_INFO,__VA_ARGS__)
#define DOOM_ERROR(...) doLog(ANDROID_LOG_ERROR,__VA_ARGS__)




#endif //NATIVE_DOOM_H

