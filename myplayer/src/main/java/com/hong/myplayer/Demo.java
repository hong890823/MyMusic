package com.hong.myplayer;

public class Demo {
    static {
        System.loadLibrary("native-lib");
        System.loadLibrary("avcodec-56");
        System.loadLibrary("avdevice-56");
        System.loadLibrary("avfilter-5");
        System.loadLibrary("avformat-56");
        System.loadLibrary("avutil-54");
        System.loadLibrary("postproc-53");
        System.loadLibrary("swresample-1");
        System.loadLibrary("swscale-3");
    }

    public native String stringFromJNI();

    public native void testFFmpeg();
}
