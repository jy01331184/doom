//
// Created by tianyang on 2021/5/11.
//

#include <dlfcn.h>
#include "doom.h"
#include "dalvik.h"
#include "art.h"

typedef void (*SetJniCheckArtFunc)(void* env,bool check);
typedef void (*SetJniCheckDalvikFunc)(void* env);
typedef void* (*VoidJniFunc)();

bool VerifyAccess(void* self, void* obj, void* declaring_class, uint32_t access_flags){
    if(hookLog)
        DOOM_LOG("VerifyAccess");
    return true;
}

bool VerifyObjectIsClass(void* o, void* c) {
    if(hookLog)
        DOOM_LOG("VerifyObjectIsClass");
    return true;
}

SetJniCheckDalvikFunc vmDalvikSetFunc;
SetJniCheckDalvikFunc envDalvikSetFunc;
SetJniCheckArtFunc vmArtSetFunc;
SetJniCheckArtFunc envArtSetFunc;

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_doom_Doom_initCheckJNIL2M(JNIEnv *env, jclass clazz) {
    void *libart = dlopen("libart.so", RTLD_LAZY);
    if(!libart){
        return JNI_FALSE;
    }

    envArtSetFunc = (SetJniCheckArtFunc)dlsym(libart,"_ZN3art9JNIEnvExt18SetCheckJniEnabledEb");
    vmArtSetFunc = (SetJniCheckArtFunc)dlsym(libart,"_ZN3art9JavaVMExt18SetCheckJniEnabledEb");
    void* refMethod = dlsym(libart,"_ZN3art12VerifyAccessEPNS_6ThreadEPNS_6mirror6ObjectEPNS2_5ClassEj");
    hook(refMethod,(void*)VerifyAccess,(void**) NULL);

    refMethod = dlsym(libart,"_ZN3art19VerifyObjectIsClassEPNS_6mirror6ObjectEPNS0_5ClassE");
    hook(refMethod,(void*)VerifyObjectIsClass,(void**) NULL);

    DOOM_INFO("envSetFunc=%p,vmSetFunc=%p",envArtSetFunc,vmArtSetFunc);
    if(!envArtSetFunc || !vmArtSetFunc){
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_doom_Doom_initCheckJNIDalvik(JNIEnv *env, jclass clazz) {
    void *libdvm = dlopen("libdvm.so", RTLD_LAZY);
    if(!libdvm){
        return JNI_FALSE;
    }

    envDalvikSetFunc = (SetJniCheckDalvikFunc)dlsym(libdvm,"_Z19dvmUseCheckedJniEnvP9JNIEnvExt");
    vmDalvikSetFunc = (SetJniCheckDalvikFunc)dlsym(libdvm,"_Z18dvmUseCheckedJniVmP9JavaVMExt");
    VoidJniFunc enbale = (VoidJniFunc)dlsym(libdvm,"_Z23dvmLateEnableCheckedJniv");
    DvmJniGlobals* dvmJni = (DvmJniGlobals*)dlsym(libdvm,"gDvmJni");
    DOOM_INFO("dvmJni=%p envSetFunc=%p,vmSetFunc=%p",dvmJni,envDalvikSetFunc,vmDalvikSetFunc);
    if(!envDalvikSetFunc || !vmDalvikSetFunc){
        return JNI_FALSE;
    }

    JavaVM *javaVm;
    env->GetJavaVM(&javaVm);
    JNIEnvExt *envExt = reinterpret_cast<JNIEnvExt *>(env);
    JavaVMExt *vmExt = reinterpret_cast<JavaVMExt *>(javaVm);

    DOOM_INFO("checkjni=%d,vm=%p:%p",dvmJni->useCheckJni,dvmJni->jniVm,javaVm);
    DOOM_INFO("func=%p,%p base=%p,%p",envExt->funcTable,vmExt->funcTable,envExt->baseFuncTable,vmExt->baseFuncTable);
    DOOM_INFO("func=%p,%p",env->functions,javaVm->functions);
    dvmJni->useCheckJni = 1;
    return JNI_TRUE;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_doom_Doom_nativeCheckJNI(JNIEnv *env, jclass clazz, jboolean open) {

    JavaVM *javaVm;
    env->GetJavaVM(&javaVm);
    DOOM_INFO("check jni:%d vm=%p,env=%p",open,javaVm,env);

    if(vmArtSetFunc && envArtSetFunc){
        vmArtSetFunc(javaVm,open);
        envArtSetFunc(env,open);
    } else if(vmDalvikSetFunc && envDalvikSetFunc){
        //vmDalvikSetFunc(javaVm);
        //envDalvikSetFunc(env);
    }

}