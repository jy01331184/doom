//
// Created by yt on 10/26/21.
//

#include "xhook_ext.h"
#include "jni.h"
#include "atomic"
#include "doom.h"
#include <mutex>
#include <sys/mman.h>
#include <cinttypes>
#include <string.h>

#define RETURN_ON_COND(cond, ret_value) \
      do { \
        if ((cond)) { \
          DOOM_ERROR( "Hit cond: %s, return %s directly.", #cond, #ret_value); \
          return (ret_value); \
        } \
      } while (false)

enum {
    /**
    * When set, the `reserved_addr` and `reserved_size` fields must point to an
    * already-reserved region of address space which will be used to load the
    * library if it fits.
    *
    * If the reserved region is not large enough, loading will fail.
    */
            ANDROID_DLEXT_RESERVED_ADDRESS = 0x1,

    /**
     * Like `ANDROID_DLEXT_RESERVED_ADDRESS`, but if the reserved region is not large enough,
     * the linker will choose an available address instead.
     */
            ANDROID_DLEXT_RESERVED_ADDRESS_HINT = 0x2,
};

static const constexpr int LIBLOAD_SUCCESS = 0;
static const constexpr int LIBLOAD_FAILURE = 1;

typedef struct {
    uint64_t flags;
    void *reserved_addr;
    size_t reserved_size;
    int relro_fd;
    int library_fd;
    off64_t library_fd_offset;
    struct android_namespace_t *library_namespace;
} android_dlextinfo;

static void *(*sOriginalAndroidDlOpenExt)(const char *, int, void *);

static std::recursive_mutex sDlOpenLock;
static std::atomic_bool sDlOpenLockGate(false);

static const constexpr char *PROBE_REQ_TAG = "~~~MEMMISc_Pr0b3_7a9_00l~";
static const constexpr char *FAKE_RELRO_PATH = "/dev/null";
static const constexpr uint64_t PROBE_RESP_VALUE = 0xFFFBFFFCFFFDFFFELL;
static void *sProbedReservedSpaceStart = nullptr;
static size_t sProbedReservedSpaceSize = 0;
static std::atomic_bool sInstalled(false);

template<class TDtor>
class ScopedCleaner {
public:
    ScopedCleaner(const TDtor &dtor) : mDtor(dtor), mOmitted(false) {}

    ScopedCleaner(ScopedCleaner &&other) : mDtor(other.mDtor), mOmitted(other.mOmitted) {
        other.Omit();
    }

    ~ScopedCleaner() {
        if (!mOmitted) {
            mDtor();
            mOmitted = true;
        }
    }

    void Omit() {
        mOmitted = true;
    }

private:
    void *operator new(size_t) = delete;

    void *operator new[](size_t) = delete;

    ScopedCleaner(const ScopedCleaner &) = delete;

    ScopedCleaner &operator=(const ScopedCleaner &) = delete;

    ScopedCleaner &operator=(ScopedCleaner &&) = delete;

    TDtor mDtor;
    bool mOmitted;
};

template<class TDtor>
static ScopedCleaner<TDtor> MakeScopedCleaner(const TDtor &dtor) {
    return ScopedCleaner<TDtor>(std::forward<const TDtor &>(dtor));
}

template<class TMutex>
class GateMutexGuard {
public:
    GateMutexGuard(TMutex &mutex, std::atomic_bool &gate, bool change_gate)
            : mMutex(mutex), mGate(gate), mChangeGate(change_gate) {
        if (change_gate) {
            mMutex.lock();
            mGate.store(true);
        } else if (mGate.load()) {
            mMutex.lock();
        }
    }

    ~GateMutexGuard() {
        if (mChangeGate) {
            mGate.store(false);
            mMutex.unlock();
        } else if (mGate.load()) {
            mMutex.unlock();
        }
    }

private:
    GateMutexGuard(const GateMutexGuard &) = delete;

    GateMutexGuard &operator=(const GateMutexGuard &) = delete;

    GateMutexGuard &operator=(GateMutexGuard &&) = delete;

    void *operator new(size_t) = delete;

    void *operator new[](size_t) = delete;

    TMutex &mMutex;
    std::atomic_bool &mGate;
    bool mChangeGate;
};

static bool UnmapMemRegion(void *start, size_t size) {
    if (::munmap(start, size) == 0) {
        DOOM_INFO("Unmap region [%p, +%"
                          PRIu32
                          "] successfully.", start, size);
        return true;
    } else {
        int errcode = errno;
        DOOM_ERROR("Fail to unmap region [%p, +%"
                           PRIu32
                           "], errcode: %d", start, size, errcode);
        return false;
    }
}

static void HackDlExtInfoOnDemand(void *extinfo) {
    auto parsedExtInfo = reinterpret_cast<android_dlextinfo *>(extinfo);
    int extFlags = parsedExtInfo->flags;
    if ((extFlags & ANDROID_DLEXT_RESERVED_ADDRESS) != 0) {
        parsedExtInfo->reserved_addr = nullptr;
        parsedExtInfo->reserved_size = 0;
        extFlags = (extFlags & (~ANDROID_DLEXT_RESERVED_ADDRESS)) |
                   ANDROID_DLEXT_RESERVED_ADDRESS_HINT;
        parsedExtInfo->flags = extFlags;
    }
}

static void *HandleAndroidDlOpenExt(const char *filepath, int flags, void *extinfo) {
    GateMutexGuard<std::recursive_mutex> guard(sDlOpenLock, sDlOpenLockGate, false);
    if (strcmp(filepath, PROBE_REQ_TAG) == 0) {
        auto android_extinfo = reinterpret_cast<android_dlextinfo *>(extinfo);
        sProbedReservedSpaceStart = android_extinfo->reserved_addr;
        sProbedReservedSpaceSize = android_extinfo->reserved_size;
        DOOM_INFO("android_dlopen_ext received probe tag, found reserved region: [%p, +%"
                          PRIu32
                          "].",
                  sProbedReservedSpaceStart, sProbedReservedSpaceSize);
        return reinterpret_cast<void *>(PROBE_RESP_VALUE);
    }
    HackDlExtInfoOnDemand(extinfo);
    return sOriginalAndroidDlOpenExt(filepath, flags, extinfo);
}

static bool LocateReservedSpaceByProbing(JNIEnv *env,
                                         jint sdk_ver, jobject class_loader, void **start_out,
                                         size_t *size_out) {
    jclass webClass = env->FindClass("android/webkit/WebViewLibraryLoader");
    if (webClass == nullptr) {
        env->ExceptionClear();
        DOOM_ERROR("Cannot find loader class, try factory class next.");
        webClass = env->FindClass("android/webkit/WebViewFactory");
    }
    if (webClass != nullptr) {
        DOOM_INFO("Found loader/factory class.");
    } else {
        env->ExceptionClear();
        DOOM_ERROR("Cannot find loader/factory class, go failure directly.");
        return false;
    }

    const char *probeMethodName = "nativeLoadWithRelroFile";
    jstring jProbeTag = env->NewStringUTF(PROBE_REQ_TAG);
    RETURN_ON_COND(jProbeTag == nullptr, false);
    jstring jFakeRelRoPath = env->NewStringUTF(FAKE_RELRO_PATH);
    RETURN_ON_COND(jFakeRelRoPath == nullptr, false);
    jmethodID probeMethodID = nullptr;
    jint probeMethodRet = 0;

    if(sdk_ver <= 23){ //5.x、6.0
        if(sdk_ver <= 22){
            probeMethodID = env->GetStaticMethodID(webClass, probeMethodName, "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Z");
            env->ExceptionClear();
            if(probeMethodID != nullptr){
                probeMethodRet = env->CallStaticBooleanMethod(webClass,probeMethodID,jProbeTag, jProbeTag, jFakeRelRoPath, jFakeRelRoPath) ? LIBLOAD_SUCCESS : LIBLOAD_FAILURE;
            } else {
                DOOM_ERROR("can't find nativeLoadWithRelroFile method");
            }
        } else {
            probeMethodID = env->GetStaticMethodID(webClass, probeMethodName, "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I");
            env->ExceptionClear();
            if(probeMethodID != nullptr){
                probeMethodRet = env->CallStaticIntMethod(webClass,probeMethodID,jProbeTag, jProbeTag, jFakeRelRoPath, jFakeRelRoPath);
            } else {
                DOOM_ERROR("can't find nativeLoadWithRelroFile method");
            }
        }
    } else if(sdk_ver <= 27){//7.0、7.1、8.0、8.1
        probeMethodID = env->GetStaticMethodID(webClass, probeMethodName, "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/ClassLoader;)I");
        env->ExceptionClear();
        if(probeMethodID != nullptr){
            probeMethodRet = env->CallStaticIntMethod(webClass,probeMethodID,jProbeTag, jProbeTag, jFakeRelRoPath, jFakeRelRoPath,class_loader);
        } else {
            DOOM_ERROR("can't find nativeLoadWithRelroFile method");
        }
    } else if(sdk_ver <= 30){//9.0、10、11
        probeMethodID = env->GetStaticMethodID(webClass, probeMethodName, "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/ClassLoader;)I");
        env->ExceptionClear();
        if(probeMethodID != nullptr){
            probeMethodRet = env->CallStaticIntMethod(webClass,probeMethodID,jProbeTag, jFakeRelRoPath, class_loader);
        } else {
            DOOM_ERROR("can't find nativeLoadWithRelroFile method");
        }
    }

    if (probeMethodID == nullptr) {
        DOOM_ERROR("Fail to find probe method.");
        return false;
    }

    if (probeMethodRet == LIBLOAD_SUCCESS) {
        *start_out = sProbedReservedSpaceStart;
        *size_out = sProbedReservedSpaceSize;
        return true;
    } else {
        return false;
    }
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_doom_Doom_nativeClearWebviewReserveSpace(JNIEnv *env, jclass clazz, jint sdk_ver,
                                                  jobject loader) {

    if (sInstalled.load()) {
        DOOM_ERROR("Already installed.");
        return JNI_TRUE;
    }

    void *hLib = xhook_elf_open("libwebviewchromium_loader.so");
    if (hLib == nullptr) {
        DOOM_ERROR("Fail to open libwebviewchromium_loader.so.");
        return JNI_FALSE;
    }

    auto hLibCleaner = MakeScopedCleaner([hLib]() {
        if (hLib != nullptr) {
            xhook_elf_close(hLib);
        }
    });

    int ret = xhook_got_hook_symbol(hLib, "android_dlopen_ext",
                                    reinterpret_cast<void *>(HandleAndroidDlOpenExt),
                                    reinterpret_cast<void **>(&sOriginalAndroidDlOpenExt));
    if (ret != 0) {
        DOOM_ERROR("Fail to hook android_dlopen_ext, ret: %d", ret);
        return JNI_FALSE;
    }

    DOOM_INFO("android_dlopen_ext hook done, locate and unmap reserved space next.");

    bool result = false;
    {
        GateMutexGuard<std::recursive_mutex> guard(sDlOpenLock, sDlOpenLockGate, true);

        void *reservedSpaceStart = nullptr;
        size_t reservedSpaceSize = 0;

//        if (LocateReservedSpaceByParsingMaps(&reservedSpaceStart, &reservedSpaceSize)) {
//            DOOM_INFO("Reserved space located by parsing maps, start: %p, size: %" PRIu32, reservedSpaceStart, reservedSpaceSize);
//            result = true;
//        }

        if (!result && LocateReservedSpaceByProbing(env,
                                                    sdk_ver, loader, &reservedSpaceStart,
                                                    &reservedSpaceSize)) {
            DOOM_INFO("Reserved space located by probing, start: %p, size: %"
                              PRIu32, reservedSpaceStart, reservedSpaceSize);
            result = true;
        }

        if (result) {
            if (!UnmapMemRegion(reservedSpaceStart, reservedSpaceSize)) {
                result = false;
            }
        } else {
            DOOM_ERROR("Cannot locate reserved space.");
        }
    }


    if (result) {
        DOOM_INFO("Installed successfully.");
        sInstalled.store(true);
    } else {
        DOOM_ERROR("Failed, unhook android_dlopen_ext.");
        xhook_got_hook_symbol(hLib, "android_dlopen_ext",
                              reinterpret_cast<void *>(sOriginalAndroidDlOpenExt), nullptr);
    }

    return JNI_TRUE;
}