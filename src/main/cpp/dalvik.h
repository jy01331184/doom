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

struct DvmJniGlobals {
    bool useCheckJni;
    bool warnOnly;
    bool forceCopy;

    // Provide backwards compatibility for pre-ICS apps on ICS.
    bool workAroundAppJniBugs;

    // Debugging help for third-party developers. Similar to -Xjnitrace.
    bool logThirdPartyJni;

    // We only support a single JavaVM per process.
    JavaVM*     jniVm;
};


struct JNIEnvExt {
    const struct JNINativeInterface* funcTable;     /* must be first */

    const struct JNINativeInterface* baseFuncTable;
};

struct JavaVMExt {
    const struct JNIInvokeInterface* funcTable;     /* must be first */

    const struct JNIInvokeInterface* baseFuncTable;
};

#endif //NATIVE_DALVIK_H
