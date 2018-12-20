package com.doom;

import android.content.Context;
import android.os.Build;

public class Doom {

    private static boolean init = false;
    private static boolean available = false;

    static {
        try {
            System.loadLibrary("doom");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static boolean init(Context context) {
        if (!init) {
            init = true;
            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) {
                available = false;
                return available;
            } else if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.M) {
                available = initDoomMarshmallow((int) Runtime.getRuntime().maxMemory(), (int) Runtime.getRuntime().totalMemory());
            } else if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.N_MR1) {
                available = initDoomNougat((int) Runtime.getRuntime().maxMemory(), (int) Runtime.getRuntime().totalMemory());
            } else if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.P) {
                available = initDoomOreo((int) Runtime.getRuntime().maxMemory(), (int) Runtime.getRuntime().totalMemory());
            }
        }
        return available;
    }


    public static void pauseGc() {
        if (!init) {
            throw new IllegalStateException("Doom has not init");
        } else if (!available) {
            return;
        }
        if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.M) {
            doomMarshmallow();
        } else if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.N_MR1) {
            doomNougat();
        } else if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.P) {
            doomOreo();
        }
    }

    public static void resumeGc() {
        if (!init) {
            throw new IllegalStateException("Doom has not init");
        } else if (!available) {
            return;
        }
        if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.M) {
            unDoomMarshmallow();
        } else if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.N_MR1) {
            unDoomNougat();
        } else if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.P) {
            unDoomOreo();
        }
        Runtime.getRuntime().gc();
    }

    /**
     * 5.0、5.1、6.0
     */
    private static native boolean initDoomMarshmallow(int growthLimit, int maxAllowedFootprint);

    private static native void doomMarshmallow();

    private static native void unDoomMarshmallow();

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
}
