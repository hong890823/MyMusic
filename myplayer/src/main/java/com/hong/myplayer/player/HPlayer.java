package com.hong.myplayer.player;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.text.TextUtils;

import com.hong.myplayer.HTimeInfoBean;
import com.hong.myplayer.listener.HonCompleteListener;
import com.hong.myplayer.listener.HonErrorListener;
import com.hong.myplayer.listener.HonLoadListener;
import com.hong.myplayer.listener.HonPauseResumeListener;
import com.hong.myplayer.listener.HonPcmInfoListener;
import com.hong.myplayer.listener.HonPreparedListener;
import com.hong.myplayer.listener.HonRecordListener;
import com.hong.myplayer.listener.HonTimeInfoListener;
import com.hong.myplayer.log.LogUtil;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

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

    private static String source;
    private static HTimeInfoBean timeInfoBean;//线程独享，防止混乱
    private static boolean isNext;//是否播放下一曲

    private HonPreparedListener onPreparedListener;
    private HonLoadListener onLoadListener;
    private HonPauseResumeListener onPauseResumeListener;
    private HonTimeInfoListener onTimeInfoListener;
    private HonErrorListener onErrorListener;
    private HonCompleteListener onCompleteListener;
    private HonRecordListener onRecordListener;
    private HonPcmInfoListener onPcmInfoListener;


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
    public void setOnRecordListener(HonRecordListener onRecordListener) {
        this.onRecordListener = onRecordListener;
    }
    public void setOnPcmInfoListener(HonPcmInfoListener onPcmInfoListener) {
        this.onPcmInfoListener = onPcmInfoListener;
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
        timeInfoBean = null;
        stopRecord();
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

    public void playNext(String url){
        isNext = true;
        source = url;
        stop();
    }

    public int getDuration(){
        return n_get_duration();
    }

    public void setVolume(int percent){
        if(percent>=0 && percent<=100)
        n_volume(percent);
    }

    //改变声道
    public void setMuteSolo(int mute){
        n_mute(mute);
    }

    //改变音调音速
    public void setPitchAndSpeed(float pitch,float speed){
        n_pitch(pitch);
        n_speed(speed);
    }

    public void startRecord(File outFile){
        if(!isStartRecord){
            isStartRecord = true;
            audioSamplerate = n_sample_rate();
            if(audioSamplerate>0){
                initMediaCodec(audioSamplerate,outFile);
                n_start_stop_record(true);
                LogUtil.logD("开始录音");
            }
        }

    }

    public void stopRecord(){
        if(isStartRecord){
            n_start_stop_record(false);
            releaseMediaCodec();
            LogUtil.logD("停止录音");
        }
    }

    public void pauseRecord(){
        n_start_stop_record(false);
        LogUtil.logD("暂停录音");

    }

    public void resumeRecord(){
        n_start_stop_record(true);
        LogUtil.logD("继续录音");
    }

    public void cutAudioPlay(int startTime,int endTime,boolean isReturnPcm){
        if(n_cut_audio_play(startTime,endTime,isReturnPcm)){
            start();
        }else{
            onCallError(2001,"cut audio is wrong");
        }
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
    public void onCallNext(){
        if(isNext){
            isNext = false;
            prepare();
        }
    }
    public void onCallPcmInfo(byte[] buffer,int size){
        if(onPcmInfoListener!=null){
            onPcmInfoListener.onPcmInfo(buffer,size);
        }
    }
    public void onCallPcmRate(int sampleRate){
        if(onPcmInfoListener != null){
            onPcmInfoListener.onPcmRate(sampleRate, 16, 2);
        }
    }

    public native void n_prepare(String source);
    public native void n_start();
    public native void n_pause();
    public native void n_resume();
    public native void n_stop();
    public native void n_seek(int seconds);
    public native int n_get_duration();
    public native void n_volume(int percent);
    public native void n_mute(int mute);
    public native void n_pitch(float pitch);
    public native void n_speed(float speed);
    public native int n_sample_rate();
    public native void n_start_stop_record(boolean isStartRecord);
    public native boolean n_cut_audio_play(int startTime,int endTime,boolean isReturnPcm);//调用流程是先设置了是否裁剪之后才播放

    //-------MediaCodec硬编码，边播边录-------
    private MediaCodec encoder;//里面有输入和输出两个队列
    private MediaFormat encoderFormat;
    private MediaCodec.BufferInfo codecBufferInfo = null;
    private FileOutputStream fos;
    private int perPcmSize = 0;
    private byte[] outByteBuffer;
    private int codecSampleRate;
    private boolean isStartRecord;
    private double recordTime = 0;
    private int audioSamplerate = 0;

    private void initMediaCodec(int sampleRate,File outFile){
        try {
            recordTime = 0;
            codecSampleRate = getAdtsSampleRate(sampleRate);
            encoderFormat = MediaFormat.createAudioFormat(MediaFormat.MIMETYPE_AUDIO_AAC,sampleRate,2);
            encoderFormat.setInteger(MediaFormat.KEY_BIT_RATE, 96000);
            encoderFormat.setInteger(MediaFormat.KEY_AAC_PROFILE, MediaCodecInfo.CodecProfileLevel.AACObjectLC);
            //这个大小决定MediaCodec中ByteBuffer的大小，如果小于pcm数据包的大小会导致byteBuffer.put方法崩溃。所以需要进行c++层pcm的分包。
            encoderFormat.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, 4096);
            encoder = MediaCodec.createEncoderByType(MediaFormat.MIMETYPE_AUDIO_AAC);
            codecBufferInfo = new MediaCodec.BufferInfo();
            if(encoder==null)LogUtil.logE("HPlayer.initMediaCodec encoder is null");
            encoder.configure(encoderFormat,null,null,MediaCodec.CONFIGURE_FLAG_ENCODE);
            fos = new FileOutputStream(outFile);
            encoder.start();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    /**
     * @param size pcm的大小
     * @param  buffer pcm的数据
     * */
    private void encodecPcmToAac(int size, byte[] buffer){
        if(buffer!=null && encoder!=null){
            recordTime += size*1.0/audioSamplerate*2*(16/8);
            if(onRecordListener!=null)onRecordListener.onRecordTime((int) recordTime);
            int inputBufferIndex = encoder.dequeueInputBuffer(0);
            if(inputBufferIndex>=0){
                ByteBuffer byteBuffer = encoder.getInputBuffer(inputBufferIndex);
                byteBuffer.clear();
                byteBuffer.put(buffer);
                encoder.queueInputBuffer(inputBufferIndex,0,size,0,0);
            }
            int index = encoder.dequeueOutputBuffer(codecBufferInfo,0);
            while(index>=0){
                perPcmSize = codecBufferInfo.size+7;
                outByteBuffer = new byte[perPcmSize];

                ByteBuffer byteBuffer = encoder.getOutputBuffer(index);
                byteBuffer.position(codecBufferInfo.offset);
                byteBuffer.limit(codecBufferInfo.offset+codecBufferInfo.size);

                addAdtsHeader(outByteBuffer,perPcmSize,codecSampleRate);

                byteBuffer.get(outByteBuffer, 7, codecBufferInfo.size);//前面7个是头，后面是出队的数据
                byteBuffer.position(codecBufferInfo.offset);
                try {
                    fos.write(outByteBuffer, 0, perPcmSize);
                } catch (IOException e) {
                    e.printStackTrace();
                }

                encoder.releaseOutputBuffer(index, false);//输出数据不渲染
                index = encoder.dequeueOutputBuffer(codecBufferInfo, 0);
                outByteBuffer = null;
            }
        }
    }

    private void addAdtsHeader(byte[] packet, int packetLen, int sampleRate){
        int profile = 2; // AAC LC
        int freqIdx = sampleRate; // samplerate
        int chanCfg = 2; // CPE

        packet[0] = (byte) 0xFF; // 0xFFF(12bit) 这里只取了8位，所以还差4位放到下一个里面
        packet[1] = (byte) 0xF9; // 第一个t位放F
        packet[2] = (byte) (((profile - 1) << 6) + (freqIdx << 2) + (chanCfg >> 2));
        packet[3] = (byte) (((chanCfg & 3) << 6) + (packetLen >> 11));
        packet[4] = (byte) ((packetLen & 0x7FF) >> 3);
        packet[5] = (byte) (((packetLen & 7) << 5) + 0x1F);
        packet[6] = (byte) 0xFC;
    }

    //根据不同的采样率定义不同ADTS头部需要传入的rate值
    private int getAdtsSampleRate(int sampleRate){
        int rate = 4;
        switch (sampleRate){
            case 96000:
                rate = 0;
                break;
            case 88200:
                rate = 1;
                break;
            case 64000:
                rate = 2;
                break;
            case 48000:
                rate = 3;
                break;
            case 44100:
                rate = 4;
                break;
            case 32000:
                rate = 5;
                break;
            case 24000:
                rate = 6;
                break;
            case 22050:
                rate = 7;
                break;
            case 16000:
                rate = 8;
                break;
            case 12000:
                rate = 9;
                break;
            case 11025:
                rate = 10;
                break;
            case 8000:
                rate = 11;
                break;
            case 7350:
                rate = 12;
                break;
        }
        return rate;
    }

    private void releaseMediaCodec(){
        if(encoder==null)return;
        try {
            recordTime = 0;
            fos.close();
            fos = null;
            encoder.stop();
            encoder.release();
            encoder = null;
            encoderFormat = null;
            outByteBuffer = null;
            codecBufferInfo = null;
        } catch (IOException e) {
            e.printStackTrace();
        }finally {
            isStartRecord = false;
            try {
                if(fos!=null)fos.close();
                fos = null;
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

}
