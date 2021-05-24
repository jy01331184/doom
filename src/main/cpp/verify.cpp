//
// Created by tianyang on 2021/4/19.
//

#include <dlfcn.h>
#include "doom.h"
#include "jni.h"

bool pauseVerify = false;


bool (*originKitkatVerifyClass)(void* clazz);

bool hookedKitkatVerifyClass(void* clazz){
    if(pauseVerify){
        if(hookLog){
            DOOM_LOG("hookedKitkatVerifyClass %p",clazz);
        }
    } else {
        return originKitkatVerifyClass(clazz);
    }
    return true;
}

bool (*originL2MVerifyClassOatFile)(void* linker);

bool hookedL2MVerifyClassOatFile(void* linker) {
    if(pauseVerify){
        if(hookLog){
            DOOM_LOG("hookedL2MVerifyClass");
        }
        return true;
    } else {
        bool ret = originL2MVerifyClassOatFile(linker);
        return ret;
    }
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_doom_Doom_initVerifyKitkat(JNIEnv *env, jclass clazz) {
    void *dvmso = dlopen("libdvm.so", RTLD_LAZY);
    if( dvmso ) {
        void* func = (dlsym(dvmso, "_Z14dvmVerifyClassP11ClassObject"));
        DOOM_INFO("Kitkat verify func addr=%p",func);
        if(func){
            int result = hook(func,(void*)(hookedKitkatVerifyClass),(void**)(&originKitkatVerifyClass));
            return result ? JNI_TRUE : JNI_FALSE;
        }
    }
    return JNI_FALSE;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_doom_Doom_pauseVerify(JNIEnv *env, jclass clazz,jboolean pause) {
    DOOM_INFO("change verify %d",pause);
    pauseVerify = pause;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_doom_Doom_initVerifyL2M(JNIEnv *env, jclass clazz) {
    void *artso = dlopen("libart.so",RTLD_LAZY);
    if(artso){
        void* func = (dlsym(artso, "_ZN3art11ClassLinker23VerifyClassUsingOatFileERKNS_7DexFileEPNS_6mirror5ClassERNS5_6StatusE"));
        DOOM_INFO("Art verify func addr=%p",func);
        if(func){
            int result = hook(func,(void*)(hookedL2MVerifyClassOatFile),(void**)(&originL2MVerifyClassOatFile));
            return result ? JNI_TRUE : JNI_FALSE;
        }
    }
    return JNI_FALSE;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_doom_Doom_setHookLogEnable(JNIEnv *env, jclass clazz, jboolean enable) {
    hookLog = enable;
}