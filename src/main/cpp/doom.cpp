//
// Created by tianyang on 2018/11/14.
//
#include <dlfcn.h>
#include "doom.h"

typedef int (*PUDGE_HOOK_FUNCTION)(char *libSo, char *targetSymbol, void *newFunc, void **oldFunc);
typedef int (*PUDGE_HOOK_FUNCTION_DIRECT)(void* funcPtr,void* newFunc, void ** oldFunc);
PUDGE_HOOK_FUNCTION pudgeHookFunction = 0;
PUDGE_HOOK_FUNCTION_DIRECT pudgeHookFunctionDirect = 0;

typedef int (*PUDGE_SEARCH_FUNCTION)(int addr, int target, int maxSearch);
PUDGE_SEARCH_FUNCTION pudgeSearchFunction = 0;
int initial_growth_limit;
int initial_concurrent_start_bytes;
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

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_doom_Doom_initGlobal(JNIEnv *env, jclass clazz) {
    void *handle = dlopen(PUDGE_SO, RTLD_LAZY);
    if (handle) {
        DOOM_LOG("initDoom dlopen success");
        pudgeHookFunction = (PUDGE_HOOK_FUNCTION)dlsym(handle, "_ZN5pudge12hookFunctionEPcS0_PvPS1_");
        pudgeHookFunctionDirect = (PUDGE_HOOK_FUNCTION_DIRECT)dlsym(handle,"_ZN5pudge12hookFunctionEPvS0_PS0_");
        pudgeSearchFunction = (PUDGE_SEARCH_FUNCTION)dlsym(handle, "_ZN5pudge6searchEiii");
        DOOM_LOG("pudgeHookFunction %p,pudgeSearchFunction %p",pudgeHookFunction,pudgeSearchFunction);
        if(pudgeHookFunction && pudgeHookFunctionDirect && pudgeSearchFunction){
            return JNI_TRUE;
        }

    } else {
        DOOM_LOG("initDoom dlopen fail");
    }

    return JNI_FALSE;
}
