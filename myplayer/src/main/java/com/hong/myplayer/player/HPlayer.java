package com.hong.myplayer.player;

import android.text.TextUtils;

import com.hong.myplayer.HTimeInfoBean;
import com.hong.myplayer.listener.HonCompleteListener;
import com.hong.myplayer.listener.HonErrorListener;
import com.hong.myplayer.listener.HonLoadListener;
import com.hong.myplayer.listener.HonPauseResumeListener;
import com.hong.myplayer.listener.HonPreparedListener;
import com.hong.myplayer.listener.HonTimeInfoListener;
import com.hong.myplayer.log.LogUtil;

public class HPlayer {
    //    这里面的各个库的加载顺序不能变，否则很可能有的库加载不上，会报错
    static {
        System.loadLibrary("avutil-55");
        System.loadLibrary("swresample-2");
        System.loadLibrary("avcodec-57");
        System.loadLibrary("avformat-57");
        System.loadLibrary("swscale-4");
        System.loadLibrary("postproc-54");
        System.loadLibrary("avfilter-6");
        System.loadLibrary("avdevice-57");
        System.loadLibrary("native-lib");
    }

    private static HTimeInfoBean timeInfoBean;//线程独享，防止混乱

    private String source;
    private HonPreparedListener onPreparedListener;
    private HonLoadListener onLoadListener;
    private HonPauseResumeListener onPauseResumeListener;
    private HonTimeInfoListener onTimeInfoListener;
    private HonErrorListener onErrorListener;
    private HonCompleteListener onCompleteListener;

    public HPlayer(){

    }

    public void setSource(String source) {
        this.source = source;
    }
    public void setOnPreparedListener(HonPreparedListener onPreparedListener) {
        this.onPreparedListener = onPreparedListener;
    }
    public void setOnLoadListener(HonLoadListener onLoadListener) {
        this.onLoadListener = onLoadListener;
    }
    public void setOnPauseResumeListener(HonPauseResumeListener onPauseResumeListener) {
        this.onPauseResumeListener = onPauseResumeListener;
    }
    public void setOnTimeInfoListener(HonTimeInfoListener onTimeInfoListener) {
        this.onTimeInfoListener = onTimeInfoListener;
    }
    public void setOnErrorListener(HonErrorListener onErrorListener) {
        this.onErrorListener = onErrorListener;
    }
    public void setOnCompleteListener(HonCompleteListener onCompleteListener) {
        this.onCompleteListener = onCompleteListener;
    }

    public void prepare(){
        if(TextUtils.isEmpty(source)){
            LogUtil.logD("准备的播放源为空");
            return;
        }
        new Thread(new Runnable() {
            @Override
            public void run() {
                n_prepare(source);
            }
        }).start();
    }

    public void start(){
        if(TextUtils.isEmpty(source)){
            LogUtil.logD("播放源为空");
            return;
        }
        new Thread(new Runnable() {
            @Override
            public void run() {
                n_start();
            }
        }).start();
    }

    public void pause(){
        n_pause();
        if(onPauseResumeListener !=null) onPauseResumeListener.onPause(true);
    }

    public void resume(){
        n_resume();
        if(onPauseResumeListener !=null) onPauseResumeListener.onPause(false);
    }

    public void stop(){
        new Thread(new Runnable() {
            @Override
            public void run() {
                n_stop();
            }
        }).start();
    }

    public void seek(int seconds){
        n_seek(seconds);
    }

    //jni调用该方法表示解码器已经准备好了
    public void onCallPrepared(){
        if(onPreparedListener!=null)onPreparedListener.onPrepared();
    }
    public void onCallLoad(boolean load){
        if(onLoadListener!=null)onLoadListener.onLoad(load);
    }
    public void onCallTimeInfo(int current,int total){
        if(onTimeInfoListener!=null){
            if(timeInfoBean==null)
            timeInfoBean = new HTimeInfoBean();
            timeInfoBean.setCurrentTime(current);
            timeInfoBean.setTotalTime(total);
            onTimeInfoListener.onTimeInfo(timeInfoBean);
        }
    }
    public void onCallError(int code,String msg){
        stop();
        if(onErrorListener!=null)onErrorListener.onError(code,msg);
    }
    public void onCallComplete(){
        stop();
        if(onCompleteListener!=null)onCompleteListener.onComplete();
    }


    public native void n_prepare(String source);
    public native void n_start();
    public native void n_pause();
    public native void n_resume();
    public native void n_stop();
    public native void n_seek(int seconds);
}
