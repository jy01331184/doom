//
// Created by tianyang on 2021/4/30.
//

#ifndef NATIVE_DALVIK_H
#define NATIVE_DALVIK_H

#include <string>
#include <vector>
#include "stdint.h"

struct GcSpec {
    /* If true, only the application heap is threatened. */
    bool isPartial;
    /* If true, the trace is run concurrently with the mutator. */
    bool isConcurrent;
    /* Toggles for the soft reference clearing policy. */
    bool doPreserve;
    /* A name for this garbage collection mode. */
    const char *reason;
};


union IRTSegmentState {
    uint32_t          all;
    struct {
        uint32_t      topIndex:16;            /* index of first unused entry */
        uint32_t      numHoles:16;            /* #of holes in entire table */
    } parts;
};

struct ReferenceTable {
    void**        nextEntry;          /* top of the list */
    void**        table;              /* bottom of the list */

    int             allocEntries;       /* #of entries we have space for */
    int             maxEntries;         /* max #of entries allowed */
};

struct IndirectRefTable {
public:

    IRTSegmentState segmentState;

    void* table_;
    /* bit mask, ORed into all irefs */
    int kind_;
    /* #of entries we have space for */
    size_t          alloc_entries_;
    /* max #of entries allowed */
    size_t          max_entries_;
};


struct Heap {
    /* The mspace to allocate from.
     */
    void* msp;

    /* The largest size that this heap is allowed to grow to.
     */
    size_t maximumSize;

    /* Number of bytes allocated from this mspace for objects,
     * including any overhead.  This value is NOT exact, and
     * should only be used as an input for certain heuristics.
     */
    size_t bytesAllocated;

    /* Number of bytes allocated from this mspace at which a
     * concurrent garbage collection will be started.
     */
    size_t concurrentStartBytes;

    /* Number of objects currently allocated from this mspace.
     */
    size_t objectsAllocated;

    /*
     * The lowest address of this heap, inclusive.
     */
    char *base;

    /*
     * The highest address of this heap, exclusive.
     */
    char *limit;

    /*
     * If the heap has an mspace, the current high water mark in
     * allocations requested via dvmHeapSourceMorecore.
     */
    char *brk;
};

struct HeapSource {
    /* Target ideal heap utilization ratio; range 1..HEAP_UTILIZATION_MAX
     */
    size_t targetUtilization;

    /* The starting heap size.
     */
    size_t startSize;

    /* The largest that the heap source as a whole is allowed to grow.
     */
    size_t maximumSize;

    /*
     * The largest size we permit the heap to grow.  This value allows
     * the user to limit the heap growth below the maximum size.  This
     * is a work around until we can dynamically set the maximum size.
     * This value can range between the starting size and the maximum
     * size but should never be set below the current footprint of the
     * heap.
     */
    size_t growthLimit;

    /* The desired max size of the heap source as a whole.
     */
    size_t idealSize;

    /* The maximum number of bytes allowed to be allocated from the
     * active heap before a GC is forced.  This is used to "shrink" the
     * heap in lieu of actual compaction.
     */
    size_t softLimit;

    /* Minimum number of free bytes. Used with the target utilization when
     * setting the softLimit. Never allows less bytes than this to be free
     * when the heap size is below the maximum size or growth limit.
     */
    size_t minFree;

    /* Maximum number of free bytes. Used with the target utilization when
     * setting the softLimit. Never allows more bytes than this to be free
     * when the heap size is below the maximum size or growth limit.
     */

    size_t maxFree;

    /* The heaps; heaps[0] is always the active heap,
     * which new objects should be allocated from.
     */
    Heap heaps[2];

    /* The current number of heaps.
     */
    size_t numHeaps;
};

struct GcHeap {
    HeapSource *heapSource;
};


struct DvmGlobals {
    /*
     * Some options from the command line or environment.
     */
    char*       bootClassPathStr;
    char*       classPathStr;

    size_t      heapStartingSize;
    size_t      heapMaximumSize;
    size_t      heapGrowthLimit;
    bool        lowMemoryMode;
    double      heapTargetUtilization;
    size_t      heapMinFree;
    size_t      heapMaxFree;
    size_t      stackSize;
    size_t      mainThreadStackSize;

    bool        verboseGc;
    bool        verboseJni;
    bool        verboseClass;
    bool        verboseShutdown;

    bool        jdwpAllowed;        // debugging allowed for this process?
    bool        jdwpConfigured;     // has debugging info been provided?
    int jdwpTransport;
    bool        jdwpServer;
    char*       jdwpHost;
    int         jdwpPort;
    bool        jdwpSuspend;

    int profilerClockSource;

    /*
     * Lock profiling threshold value in milliseconds.  Acquires that
     * exceed threshold are logged.  Acquires within the threshold are
     * logged with a probability of $\frac{time}{threshold}$ .  If the
     * threshold is unset no additional logging occurs.
     */
    uint32_t          lockProfThreshold;

    int         (*vfprintfHook)(FILE*, const char*, va_list);
    void        (*exitHook)(int);
    void        (*abortHook)(void);
    bool        (*isSensitiveThreadHook)(void);

    char*       jniTrace;
    bool        reduceSignals;
    bool        noQuitHandler;
    bool        verifyDexChecksum;
    char*       stackTraceFile;     // for SIGQUIT-inspired output

    bool        logStdio;

    int    dexOptMode;
    int  classVerifyMode;

    bool        generateRegisterMaps;
    int     registerMapMode;

    bool        monitorVerification;

    bool        dexOptForSmp;

    /*
     * GC option flags.
     */
    bool        preciseGc;
    bool        preVerify;
    bool        postVerify;
    bool        concurrentMarkSweep;
    bool        verifyCardTable;
    bool        disableExplicitGc;

    int         assertionCtrlCount;
    void*   assertionCtrl;

    int   executionMode;

    bool        commonInit; /* whether common stubs are generated */
    bool        constInit; /* whether global constants are initialized */

    /*
     * VM init management.
     */
    bool        initializing;
    bool        optimizing;

    /*
     * java.lang.System properties set from the command line with -D.
     * This is effectively a set, where later entries override earlier
     * ones.
     */
    std::vector<std::string>* properties;

    /*
     * Where the VM goes to find system classes.
     */
    void* bootClassPath;
    /* used by the DEX optimizer to load classes from an unfinished DEX */
    void*     bootClassPathOptExtra;
    bool        optimizingBootstrapClass;

    /*
     * Loaded classes, hashed by class name.  Each entry is a void*,
     * allocated in GC space.
     */
    void*  loadedClasses;

    /*
     * Value for the next class serial number to be assigned.  This is
     * incremented as we load classes.  Failed loads and races may result
     * in some numbers being skipped, and the serial number is not
     * guaranteed to start at 1, so the current value should not be used
     * as a count of loaded classes.
     */
    volatile int classSerialNumber;

    /*
     * Classes with a low classSerialNumber are probably in the zygote, and
     * their InitiatingLoaderList is not used, to promote sharing. The list is
     * kept here instead.
     */
    void* initiatingLoaderList;

    /*
     * Interned strings.
     */

    /* A mutex that guards access to the interned string tables. */
    pthread_mutex_t internLock;

    /* Hash table of strings interned by the user. */
    void*  internedStrings;

    /* Hash table of strings interned by the class loader. */
    void*  literalStrings;

    /*
     * Classes constructed directly by the vm.
     */

    /* the class Class */
    void* classJavaLangClass;

    /* synthetic classes representing primitive types */
    void* typeVoid;
    void* typeBoolean;
    void* typeByte;
    void* typeShort;
    void* typeChar;
    void* typeInt;
    void* typeLong;
    void* typeFloat;
    void* typeDouble;

    /* synthetic classes for arrays of primitives */
    void* classArrayBoolean;
    void* classArrayByte;
    void* classArrayShort;
    void* classArrayChar;
    void* classArrayInt;
    void* classArrayLong;
    void* classArrayFloat;
    void* classArrayDouble;

    /*
     * Quick lookups for popular classes used internally.
     */
    void* classJavaLangClassArray;
    void* classJavaLangClassLoader;
    void* classJavaLangObject;
    void* classJavaLangObjectArray;
    void* classJavaLangString;
    void* classJavaLangThread;
    void* classJavaLangVMThread;
    void* classJavaLangThreadGroup;
    void* classJavaLangStackTraceElement;
    void* classJavaLangStackTraceElementArray;
    void* classJavaLangAnnotationAnnotationArray;
    void* classJavaLangAnnotationAnnotationArrayArray;
    void* classJavaLangReflectAccessibleObject;
    void* classJavaLangReflectConstructor;
    void* classJavaLangReflectConstructorArray;
    void* classJavaLangReflectField;
    void* classJavaLangReflectFieldArray;
    void* classJavaLangReflectMethod;
    void* classJavaLangReflectMethodArray;
    void* classJavaLangReflectProxy;
    void* classJavaLangSystem;
    void* classJavaNioDirectByteBuffer;
    void* classLibcoreReflectAnnotationFactory;
    void* classLibcoreReflectAnnotationMember;
    void* classLibcoreReflectAnnotationMemberArray;
    void* classOrgApacheHarmonyDalvikDdmcChunk;
    void* classOrgApacheHarmonyDalvikDdmcDdmServer;
    void* classJavaLangRefFinalizerReference;

    /*
     * classes representing exception types. The names here don't include
     * packages, just to keep the use sites a bit less verbose. All are
     * in java.lang, except where noted.
     */
    void* exAbstractMethodError;
    void* exArithmeticException;
    void* exArrayIndexOutOfBoundsException;
    void* exArrayStoreException;
    void* exClassCastException;
    void* exClassCircularityError;
    void* exClassFormatError;
    void* exClassNotFoundException;
    void* exError;
    void* exExceptionInInitializerError;
    void* exFileNotFoundException; /* in java.io */
    void* exIOException;           /* in java.io */
    void* exIllegalAccessError;
    void* exIllegalAccessException;
    void* exIllegalArgumentException;
    void* exIllegalMonitorStateException;
    void* exIllegalStateException;
    void* exIllegalThreadStateException;
    void* exIncompatibleClassChangeError;
    void* exInstantiationError;
    void* exInstantiationException;
    void* exInternalError;
    void* exInterruptedException;
    void* exLinkageError;
    void* exNegativeArraySizeException;
    void* exNoClassDefFoundError;
    void* exNoSuchFieldError;
    void* exNoSuchFieldException;
    void* exNoSuchMethodError;
    void* exNullPointerException;
    void* exOutOfMemoryError;
    void* exRuntimeException;
    void* exStackOverflowError;
    void* exStaleDexCacheError;    /* in dalvik.system */
    void* exStringIndexOutOfBoundsException;
    void* exThrowable;
    void* exTypeNotPresentException;
    void* exUnsatisfiedLinkError;
    void* exUnsupportedOperationException;
    void* exVerifyError;
    void* exVirtualMachineError;

    /* method offsets - Object */
    int         voffJavaLangObject_equals;
    int         voffJavaLangObject_hashCode;
    int         voffJavaLangObject_toString;

    /* field offsets - String */
    int         offJavaLangString_value;
    int         offJavaLangString_count;
    int         offJavaLangString_offset;
    int         offJavaLangString_hashCode;

    /* field offsets - Thread */
    int         offJavaLangThread_vmThread;
    int         offJavaLangThread_group;
    int         offJavaLangThread_daemon;
    int         offJavaLangThread_name;
    int         offJavaLangThread_priority;
    int         offJavaLangThread_uncaughtHandler;
    int         offJavaLangThread_contextClassLoader;

    /* method offsets - Thread */
    int         voffJavaLangThread_run;

    /* field offsets - ThreadGroup */
    int         offJavaLangThreadGroup_name;
    int         offJavaLangThreadGroup_parent;

    /* field offsets - VMThread */
    int         offJavaLangVMThread_thread;
    int         offJavaLangVMThread_vmData;

    /* method offsets - ThreadGroup */
    int         voffJavaLangThreadGroup_removeThread;

    /* field offsets - Throwable */
    int         offJavaLangThrowable_stackState;
    int         offJavaLangThrowable_cause;

    /* method offsets - ClassLoader */
    int         voffJavaLangClassLoader_loadClass;

    /* direct method pointers - ClassLoader */
    void*     methJavaLangClassLoader_getSystemClassLoader;

    /* field offsets - java.lang.reflect.* */
    int         offJavaLangReflectConstructor_slot;
    int         offJavaLangReflectConstructor_declClass;
    int         offJavaLangReflectField_slot;
    int         offJavaLangReflectField_declClass;
    int         offJavaLangReflectMethod_slot;
    int         offJavaLangReflectMethod_declClass;

    /* field offsets - java.lang.ref.Reference */
    int         offJavaLangRefReference_referent;
    int         offJavaLangRefReference_queue;
    int         offJavaLangRefReference_queueNext;
    int         offJavaLangRefReference_pendingNext;

    /* field offsets - java.lang.ref.FinalizerReference */
    int offJavaLangRefFinalizerReference_zombie;

    /* method pointers - java.lang.ref.ReferenceQueue */
    void* methJavaLangRefReferenceQueueAdd;

    /* method pointers - java.lang.ref.FinalizerReference */
    void* methJavaLangRefFinalizerReferenceAdd;

    /* constructor method pointers; no vtable involved, so use void* */
    void*     methJavaLangStackTraceElement_init;
    void*     methJavaLangReflectConstructor_init;
    void*     methJavaLangReflectField_init;
    void*     methJavaLangReflectMethod_init;
    void*     methOrgApacheHarmonyLangAnnotationAnnotationMember_init;

    /* static method pointers - android.lang.annotation.* */
    void*
            methOrgApacheHarmonyLangAnnotationAnnotationFactory_createAnnotation;

    /* direct method pointers - java.lang.reflect.Proxy */
    void*     methJavaLangReflectProxy_constructorPrototype;

    /* field offsets - java.lang.reflect.Proxy */
    int         offJavaLangReflectProxy_h;

    /* direct method pointer - java.lang.System.runFinalization */
    void*     methJavaLangSystem_runFinalization;

    /* field offsets - java.io.FileDescriptor */
    int         offJavaIoFileDescriptor_descriptor;

    /* direct method pointers - dalvik.system.NativeStart */
    void*     methDalvikSystemNativeStart_main;
    void*     methDalvikSystemNativeStart_run;

    /* assorted direct buffer helpers */
    void*     methJavaNioDirectByteBuffer_init;
    int         offJavaNioBuffer_capacity;
    int         offJavaNioBuffer_effectiveDirectAddress;

    /* direct method pointers - org.apache.harmony.dalvik.ddmc.DdmServer */
    void*     methDalvikDdmcServer_dispatch;
    void*     methDalvikDdmcServer_broadcast;

    /* field offsets - org.apache.harmony.dalvik.ddmc.Chunk */
    int         offDalvikDdmcChunk_type;
    int         offDalvikDdmcChunk_data;
    int         offDalvikDdmcChunk_offset;
    int         offDalvikDdmcChunk_length;

    /*
     * Thread list.  This always has at least one element in it (main),
     * and main is always the first entry.
     *
     * The threadListLock is used for several things, including the thread
     * start condition variable.  Generally speaking, you must hold the
     * threadListLock when:
     *  - adding/removing items from the list
     *  - waiting on or signaling threadStartCond
     *  - examining the Thread struct for another thread (this is to avoid
     *    one thread freeing the Thread struct while another thread is
     *    perusing it)
     */
    void*     threadList;
    pthread_mutex_t threadListLock;

    pthread_cond_t threadStartCond;

    /*
     * The thread code grabs this before suspending all threads.  There
     * are a few things that can cause a "suspend all":
     *  (1) the GC is starting;
     *  (2) the debugger has sent a "suspend all" request;
     *  (3) a thread has hit a breakpoint or exception that the debugger
     *      has marked as a "suspend all" event;
     *  (4) the SignalCatcher caught a signal that requires suspension.
     *  (5) (if implemented) the JIT needs to perform a heavyweight
     *      rearrangement of the translation cache or JitTable.
     *
     * Because we use "safe point" self-suspension, it is never safe to
     * do a blocking "lock" call on this mutex -- if it has been acquired,
     * somebody is probably trying to put you to sleep.  The leading '_' is
     * intended as a reminder that this lock is special.
     */
    pthread_mutex_t _threadSuspendLock;

    /*
     * Guards Thread->suspendCount for all threads, and
     * provides the lock for the condition variable that all suspended threads
     * sleep on (threadSuspendCountCond).
     *
     * This has to be separate from threadListLock because of the way
     * threads put themselves to sleep.
     */
    pthread_mutex_t threadSuspendCountLock;

    /*
     * Suspended threads sleep on this.  They should sleep on the condition
     * variable until their "suspend count" is zero.
     *
     * Paired with "threadSuspendCountLock".
     */
    pthread_cond_t  threadSuspendCountCond;

    /*
     * Sum of all threads' suspendCount fields. Guarded by
     * threadSuspendCountLock.
     */
    int  sumThreadSuspendCount;

    /*
     * MUTEX ORDERING: when locking multiple mutexes, always grab them in
     * this order to avoid deadlock:
     *
     *  (1) _threadSuspendLock      (use lockThreadSuspend())
     *  (2) threadListLock          (use dvmLockThreadList())
     *  (3) threadSuspendCountLock  (use lockThreadSuspendCount())
     */


    /*
     * Thread ID bitmap.  We want threads to have small integer IDs so
     * we can use them in "thin locks".
     */
    void*  threadIdMap;

    /*
     * Manage exit conditions.  The VM exits when all non-daemon threads
     * have exited.  If the main thread returns early, we need to sleep
     * on a condition variable.
     */
    int         nonDaemonThreadCount;   /* must hold threadListLock to access */
    pthread_cond_t  vmExitCond;

    /*
     * The set of DEX files loaded by custom class loaders.
     */
    void*  userDexFiles;

    /*
     * JNI global reference table.
     */
    IndirectRefTable jniGlobalRefTable;
    IndirectRefTable jniWeakGlobalRefTable;
    pthread_mutex_t jniGlobalRefLock;
    pthread_mutex_t jniWeakGlobalRefLock;

    /*
     * JNI pinned object table (used for primitive arrays).
     */
    ReferenceTable  jniPinRefTable;
    pthread_mutex_t jniPinRefLock;

    /*
     * Native shared library table.
     */
    void*  nativeLibs;

    /*
     * GC heap lock.  Functions like gcMalloc() acquire this before making
     * any changes to the heap.  It is held throughout garbage collection.
     */
    pthread_mutex_t gcHeapLock;

    /*
     * Condition variable to queue threads waiting to retry an
     * allocation.  Signaled after a concurrent GC is completed.
     */
    pthread_cond_t gcHeapCond;

    /* Opaque pointer representing the heap. */
    GcHeap*     gcHeap;

};
#endif //NATIVE_DALVIK_H
