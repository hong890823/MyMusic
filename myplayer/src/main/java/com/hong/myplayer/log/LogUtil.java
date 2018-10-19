package com.hong.myplayer.log;

import android.util.Log;

public class LogUtil {
    private static final String TAG = "Hong";

    public static void logD(String log){
        Log.d(TAG, log);
    }

    public static void logE(String log){
        Log.e(TAG, log);
    }
}
