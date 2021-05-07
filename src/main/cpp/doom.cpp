//
// Created by tianyang on 2018/11/14.
//
#include <dlfcn.h>
#include <sys/time.h>
#include "doom.h"

typedef int (*PUDGE_HOOK_FUNCTION)(char *libSo, char *targetSymbol, void *newFunc, void **oldFunc);
typedef int (*PUDGE_HOOK_FUNCTION_DIRECT)(void* funcPtr,void* newFunc, void ** oldFunc);
typedef bool (*PUDGE_IS_GOOD_PTR_FUNCTION)(void* ptr);
PUDGE_HOOK_FUNCTION pudgeHookFunction = 0;
PUDGE_HOOK_FUNCTION_DIRECT pudgeHookFunctionDirect = 0;
PUDGE_IS_GOOD_PTR_FUNCTION pudgeIsGoodPtrFunction = 0;

typedef int (*PUDGE_SEARCH_FUNCTION)(int addr, int target, int maxSearch);
PUDGE_SEARCH_FUNCTION pudgeSearchFunction = 0;
int initial_growth_limit;
int initial_concurrent_start_bytes;
double cost = 0;
bool dooming = false;

const char * PUDGE_SO = "libpudge.so";


int hook(char *libSo, char *targetSymbol, void *newFunc, void **oldFunc) {
    if (pudgeHookFunction) {
        return pudgeHookFunction(libSo, targetSymbol, newFunc, oldFunc);
    }
    return 0;
}

int hook(void *funcPtr, void *newFunc, void **oldFunc){
    if(pudgeHookFunctionDirect){
        return pudgeHookFunctionDirect(funcPtr,newFunc,oldFunc);
    }
    return 0;
}

int search(int addr, int targetValue, int maxSearchTime) {
    if (pudgeSearchFunction) {
        return pudgeSearchFunction(addr, targetValue, maxSearchTime);
    }
    return -1;
}

bool isGoodPtr(void* ptr){
    if (pudgeIsGoodPtrFunction){
        return pudgeIsGoodPtrFunction(ptr);
    }
    return false;
}

double getCurrentTime(){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000.0;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_doom_Doom_initGlobal(JNIEnv *env, jclass clazz) {
    void *handle = dlopen(PUDGE_SO, RTLD_LAZY);
    if (handle) {
        DOOM_LOG("initDoom dlopen success");
        pudgeHookFunction = (PUDGE_HOOK_FUNCTION)dlsym(handle, "_ZN5pudge12hookFunctionEPcS0_PvPS1_");
        pudgeHookFunctionDirect = (PUDGE_HOOK_FUNCTION_DIRECT)dlsym(handle,"_ZN5pudge12hookFunctionEPvS0_PS0_");
        pudgeSearchFunction = (PUDGE_SEARCH_FUNCTION)dlsym(handle, "_ZN5pudge6searchEiii");
        pudgeIsGoodPtrFunction = (PUDGE_IS_GOOD_PTR_FUNCTION)dlsym(handle,"_ZN5pudge9isGoodPtrEPv");

        DOOM_LOG("pudgeHookFunction %p,pudgeSearchFunction %p",pudgeHookFunction,pudgeSearchFunction);
        if(pudgeHookFunction && pudgeHookFunctionDirect && pudgeSearchFunction){
            return JNI_TRUE;
        }

    } else {
        DOOM_LOG("initDoom dlopen fail");
    }

    return JNI_FALSE;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_doom_Doom_dump(JNIEnv *env, jclass clazz) {
    DOOM_LOG("doom total gc cost:%llf",cost);
}