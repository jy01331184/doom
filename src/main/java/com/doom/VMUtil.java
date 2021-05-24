package com.doom;

import java.lang.reflect.Method;

class VMUtil {
    static float getTargetHeapUtilization(){
        Class cls = null;
        try {
            cls = Class.forName("dalvik.system.VMRuntime");
            Method getInstanceMethod = cls.getDeclaredMethod("getRuntime");
            Method getTargetHeapUtilizationMethod = cls.getDeclaredMethod("getTargetHeapUtilization");
            Object vm = getInstanceMethod.invoke(null);
            float perc = (float) getTargetHeapUtilizationMethod.invoke(vm);
            return perc;
        } catch (Exception e) {
            e.printStackTrace();
        }
        return -1;
    }
}
