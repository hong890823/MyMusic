package com.hong.myplayer;

public class Demo {
//    这里面的各个库的加载顺序不能变，否则很可能有的库加载不上，会报错
    static {
        System.loadLibrary("avutil-54");
        System.loadLibrary("swresample-1");
        System.loadLibrary("avcodec-56");
        System.loadLibrary("avformat-56");
        System.loadLibrary("swscale-3");
        System.loadLibrary("postproc-53");
        System.loadLibrary("avfilter-5");
        System.loadLibrary("avdevice-56");
        System.loadLibrary("native-lib");
    }

    public native String stringFromJNI();

    public native void testFFmpeg();
}
