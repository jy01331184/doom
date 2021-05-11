//
// Created by tianyang on 2021/5/11.
//

#include <dlfcn.h>
#include "doom.h"

typedef void (*SetJniCheckFunc)(void* env,bool check);
typedef void* (*GetJniFunc)();

SetJniCheckFunc vmSetFunc;
SetJniCheckFunc envSetFunc;

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_doom_Doom_initCheckJNIL2M(JNIEnv *env, jclass clazz) {
    void *libart = dlopen("libart.so", RTLD_LAZY);
    if(!libart){
        return JNI_FALSE;
    }

    envSetFunc = (SetJniCheckFunc)dlsym(libart,"_ZN3art9JNIEnvExt18SetCheckJniEnabledEb");
    vmSetFunc = (SetJniCheckFunc)dlsym(libart,"_ZN3art9JavaVMExt18SetCheckJniEnabledEb");
    DOOM_INFO("envSetFunc=%p,vmSetFunc=%p",envSetFunc,vmSetFunc);
    if(!envSetFunc || !vmSetFunc){
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_doom_Doom_nativeCheckJNI(JNIEnv *env, jclass clazz, jboolean open) {
    DOOM_INFO("check jni:%d",open);
    JavaVM *javaVm;
    env->GetJavaVM(&javaVm);
    vmSetFunc(javaVm,open);
    envSetFunc(env,open);
}