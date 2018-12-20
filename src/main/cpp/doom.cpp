//
// Created by tianyang on 2018/11/14.
//
#include <dlfcn.h>
#include "doom.h"

jclass doomClass;
typedef int (*PUDGE_HOOK_FUNCTION)(char *libSo, char *targetSymbol, void *newFunc, void **oldFunc);
PUDGE_HOOK_FUNCTION pudgeHookFunction = 0;
typedef int (*PUDGE_SEARCH_FUNCTION)(int addr, int target, int maxSearch);
PUDGE_SEARCH_FUNCTION pudgeSearchFunction = 0;
int initial_growth_limit;
int initial_concurrent_start_bytes;
bool dooming = false;

const char * PUDGE_SO = "libpudge.so";

void initDoom(JNIEnv *env) {
    void *handle = dlopen(PUDGE_SO, RTLD_LAZY);
    if (handle) {
        DOOM_LOG("initDoom dlopen success");
        pudgeHookFunction = (PUDGE_HOOK_FUNCTION)dlsym(handle, "_ZN5pudge12hookFunctionEPcS0_PvPS1_");
        pudgeSearchFunction = (PUDGE_SEARCH_FUNCTION)dlsym(handle, "_ZN5pudge6searchEiii");
        DOOM_LOG("pudgeHookFunction %p,pudgeSearchFunction %p",pudgeHookFunction,pudgeSearchFunction);

    } else {
        DOOM_LOG("initDoom dlopen fail");
    }

    doomClass = static_cast<jclass>(env->NewGlobalRef(env->FindClass("com/doom/Doom")));
}

int hook(char *libSo, char *targetSymbol, void *newFunc, void **oldFunc) {
    if (pudgeHookFunction) {
        return pudgeHookFunction(libSo, targetSymbol, newFunc, oldFunc);
    }
    return 0;
}

int search(int addr, int targetValue, int maxSearchTime) {
    if (pudgeSearchFunction) {
        return pudgeSearchFunction(addr, targetValue, maxSearchTime);
    }
    return -1;
}


