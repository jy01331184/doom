package com.doom;

import android.content.Context;
import android.os.Build;
import java.util.logging.Logger;


public class Doom {
    private static final int STATE_UNINITED = 0, STATE_AVAILABLE = 1, STATE_INAVAILABLE = -1;
    private static int init = STATE_UNINITED;
    private static int verifyAvailable,gcAvailable,checkJNIAvailable = STATE_UNINITED;
    private static Logger logger = Logger.getLogger("doom");

    public static boolean init(Context context) {
        if (init == STATE_UNINITED) {
            init = STATE_INAVAILABLE;
            try {
                logger = Logger.getLogger("doom");
                System.loadLibrary("doom");
                init = initGlobal(context == null?null:context.getApplicationContext(),logger) ? STATE_AVAILABLE : init;
            } catch (Throwable t){
                logger.severe("Doom init fail:"+t.getMessage());
            }
        }
        return init == STATE_AVAILABLE;
    }

    public static boolean pauseVerify(){
        if (init == STATE_UNINITED) {
            throw new IllegalStateException("Doom has not init");
        } else if(init == STATE_INAVAILABLE){
            return false;
        }

        if(verifyAvailable == STATE_UNINITED){
            if(Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT || Build.VERSION.SDK_INT > Build.VERSION_CODES.M){
                verifyAvailable = STATE_INAVAILABLE;
            }
            if(Build.VERSION.SDK_INT == Build.VERSION_CODES.KITKAT){
                verifyAvailable = initVerifyKitkat() ? STATE_AVAILABLE : STATE_INAVAILABLE;
            } else {
                verifyAvailable = initVerifyL2M() ? STATE_AVAILABLE : STATE_INAVAILABLE;
            }
        }

        if(verifyAvailable != STATE_AVAILABLE){
            logger.severe("pauseVerify NOT AVAILABLE");
            return false;
        }
        pauseVerify(true);
        return true;
    }

    public static boolean resumeVerify(){
        if (init == STATE_UNINITED) {
            throw new IllegalStateException("Doom has not init");
        } else if(init == STATE_INAVAILABLE){
            return false;
        }
        if(verifyAvailable != STATE_AVAILABLE){
            return false;
        }
        pauseVerify(false);
        return true;
    }

    public static boolean pauseGc() {
        if (init == STATE_UNINITED) {
            throw new IllegalStateException("Doom has not init");
        } else if(init == STATE_INAVAILABLE){
            return false;
        }

        if (gcAvailable == STATE_UNINITED) {
            gcAvailable = STATE_INAVAILABLE;
            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT) {
                gcAvailable = STATE_INAVAILABLE;
            } else if(Build.VERSION.SDK_INT == Build.VERSION_CODES.KITKAT){
                gcAvailable = initGcDalvik(Runtime.getRuntime().maxMemory(),VMUtil.getTargetHeapUtilization()) ? STATE_AVAILABLE : STATE_INAVAILABLE;
            } else if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.M) {
                gcAvailable = initGcMarshmallow(Runtime.getRuntime().maxMemory()) ? STATE_AVAILABLE : STATE_INAVAILABLE;
            } else if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.N_MR1) {
                //gcAvailable = initDoomNougat((int) Runtime.getRuntime().maxMemory(), (int) Runtime.getRuntime().totalMemory()) ? STATE_AVAILABLE : STATE_INAVAILABLE;
            } else if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.P) {
                //gcAvailable = initDoomOreo((int) Runtime.getRuntime().maxMemory(), (int) Runtime.getRuntime().totalMemory()) ? STATE_AVAILABLE : STATE_INAVAILABLE;
            }
        }

        if (gcAvailable != STATE_AVAILABLE) {
            logger.severe("pauseGc NOT AVAILABLE");
            return false;
        }

        if( Build.VERSION.SDK_INT == Build.VERSION_CODES.KITKAT){
            pauseGcDalvik();
        } else if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.M) {
            pauseGcMarshmallow();
        } else if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.N_MR1) {
            //doomNougat();
        } else if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.P) {
            //doomOreo();
        }
        return true;
    }

    public static boolean resumeGc() {
        if (init == STATE_UNINITED) {
            throw new IllegalStateException("Doom has not init");
        } else if(init == STATE_INAVAILABLE){
            return false;
        }
        if (gcAvailable != STATE_AVAILABLE) {
            return false;
        }

        if( Build.VERSION.SDK_INT == Build.VERSION_CODES.KITKAT){
            resumeGcDalvik();
        } else if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.M) {
            resumeGcMarshmallow();
        } else if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.N_MR1) {
            //unDoomNougat();
        } else if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.P) {
            //unDoomOreo();
        }
        Runtime.getRuntime().gc();
        return true;
    }

    public static boolean checkJNI(boolean open){
        if (init == STATE_UNINITED) {
            throw new IllegalStateException("Doom has not init");
        } else if(init == STATE_INAVAILABLE){
            return false;
        }

        if(checkJNIAvailable == STATE_UNINITED){
            if(Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT || Build.VERSION.SDK_INT > Build.VERSION_CODES.M){
                checkJNIAvailable = STATE_INAVAILABLE;
            }
            if(Build.VERSION.SDK_INT == Build.VERSION_CODES.KITKAT){
                checkJNIAvailable =  STATE_INAVAILABLE;
            } else {
                checkJNIAvailable = initCheckJNIL2M() ? STATE_AVAILABLE : STATE_INAVAILABLE;
            }
        }

        if(checkJNIAvailable != STATE_AVAILABLE){
            logger.severe("checkJNI NOT AVAILABLE");
            return false;
        }

        nativeCheckJNI(open);

        return true;
    }

    /**
     * global init
     */
    private static native boolean initGlobal(Context context,Logger logger);

    private static native boolean initGcDalvik(long growthLimit,float targetHeapUtilization);

    private static native void pauseGcDalvik();

    private static native void resumeGcDalvik();

    /**
     * 5.0、5.1、6.0
     */
    private static native boolean initGcMarshmallow(long growthLimit);

    private static native void pauseGcMarshmallow();

    private static native void resumeGcMarshmallow();

    /**
     * 7.0、7.1
     */
    private static native boolean initDoomNougat(int growthLimit, int maxAllowedFootprint);

    private static native void doomNougat();

    private static native void unDoomNougat();

    /**
     * 8.0、8.1、9.0
     */
    private static native boolean initDoomOreo(int growthLimit, int maxAllowedFootprint);

    private static native void doomOreo();

    private static native void unDoomOreo();


    /**
     * class verify
     */
    private static native boolean initVerifyKitkat();

    private static native boolean initVerifyL2M();

    private static native void pauseVerify(boolean pause);

    /**
     *  checkjni
     */
    private static native boolean initCheckJNIL2M();

    private native static void nativeCheckJNI(boolean open);

    /**
     * others
     */
    public static Logger getLogger() {
        return logger;
    }

    static native void setHookLogEnable(boolean enable);

    static native void dump();
}
