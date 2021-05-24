//
// Created by tianyang on 2018/11/14.
//
#include <dlfcn.h>
#include <sys/time.h>
#include <malloc.h>
#include <asm/signal.h>
#include "doom.h"
#include "sys/system_properties.h"

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
JavaVM* vm;
jclass doomClass;
jobject logger;
jmethodID logInfoMethod;
jmethodID logErrorMethod;
jmethodID onSigEventMethod;
bool hookLog = false;
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

JNIEnv* getEnv(){
    JNIEnv* env;
    if(vm->GetEnv((void **)&env,JNI_VERSION_1_6) != JNI_OK){
        vm->AttachCurrentThread(&env,0);
    }
    return env;
}

void doLog(int level,const char* message,...){
    va_list args;
    va_start(args,message);

    int mallocSize = 3;
    char* buf = static_cast<char *>(malloc(mallocSize));
    int needSize = vsnprintf(buf,mallocSize,message,args);

    if(needSize > mallocSize){
        buf = static_cast<char *>(realloc(buf, needSize + 1));
        vsnprintf(buf,needSize+1,message,args);
    }
    JNIEnv *env = getEnv();
    jstring jmessage = env->NewStringUTF(buf);
    if(level == ANDROID_LOG_ERROR){
        env->CallVoidMethod(logger, logErrorMethod, jmessage);
    } else if(level == ANDROID_LOG_INFO){
        env->CallVoidMethod(logger,logInfoMethod,jmessage);
    }
    env->DeleteLocalRef(jmessage);
    free(buf);
    va_end(args);
}
bool watched = false;
struct sigaction oldAbrtHandler;

void doomHandler(int sigNo){
    __android_log_print(ANDROID_LOG_ERROR,"fatal","doomHandler %d",sigNo);
    if(dooming){
        JNIEnv *env = getEnv();
        if(env){
            env->CallStaticVoidMethod(doomClass,onSigEventMethod,sigNo);
        }
    }
    if(sigNo == SIGABRT){
        oldAbrtHandler.sa_handler(sigNo);
    }
}

void watchSig(){
    if(!watched){
        struct sigaction newaction;
        newaction.sa_handler = doomHandler;
        newaction.sa_flags = 0;
        sigaction( SIGABRT, &newaction, &oldAbrtHandler);
        watched = true;
    }
}

void unwatchSig(){
    if(watched){
        sigaction( SIGABRT, &oldAbrtHandler, NULL);
        watched = false;
    }
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_doom_Doom_initGlobal(JNIEnv *env, jclass clazz,jobject ctx,jobject l) {
    logger = env->NewGlobalRef(l);
    jclass loggerCls = env->GetObjectClass(logger);
    doomClass = static_cast<jclass>(env->NewGlobalRef(clazz));
    logInfoMethod = env->GetMethodID(loggerCls,"info","(Ljava/lang/String;)V");
    logErrorMethod = env->GetMethodID(loggerCls, "severe", "(Ljava/lang/String;)V");
    onSigEventMethod = env->GetStaticMethodID(clazz,"onSigEvent", "(I)V");
    int ret = env->GetJavaVM(&vm);

//    char prop[256];
//    int hasProp = __system_property_get("debug.doom",prop);
//
//    if(hasProp){
//        DOOM_LOG("prop %s,%d",prop,hasProp);
//    }

    if(ret != JNI_OK){
        DOOM_ERROR("GetJavaVM fail");
        return JNI_FALSE;
    }

    void *handle = dlopen(PUDGE_SO, RTLD_LAZY);
    if (handle) {
        DOOM_INFO("initDoom dlopen success");
        pudgeHookFunction = (PUDGE_HOOK_FUNCTION)dlsym(handle, "_ZN5pudge12hookFunctionEPcS0_PvPS1_");
        pudgeHookFunctionDirect = (PUDGE_HOOK_FUNCTION_DIRECT)dlsym(handle,"_ZN5pudge12hookFunctionEPvS0_PS0_");
        pudgeSearchFunction = (PUDGE_SEARCH_FUNCTION)dlsym(handle, "_ZN5pudge6searchEiii");
        pudgeIsGoodPtrFunction = (PUDGE_IS_GOOD_PTR_FUNCTION)dlsym(handle,"_ZN5pudge9isGoodPtrEPv");
        DOOM_INFO("hook=%p,hookDir=%p,search=%p,isGood=%p",pudgeHookFunction,pudgeHookFunctionDirect,pudgeSearchFunction,pudgeIsGoodPtrFunction);
        if(pudgeHookFunction && pudgeHookFunctionDirect && pudgeSearchFunction && pudgeIsGoodPtrFunction){
            return JNI_TRUE;
        }
    } else {
        DOOM_ERROR("initDoom dlopen fail");
    }

    return JNI_FALSE;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_doom_Doom_dump(JNIEnv *env, jclass clazz) {
    //DOOM_LOG("doom total gc cost:%llf",cost);
}