package com.hong.myplayer.listener;

public interface HonPcmInfoListener {
    void onPcmInfo(byte[] buffer,int size);
    //sampleRate采样率 bit采样的位数 channels声道数
    void onPcmRate(int sampleRate, int bit, int channels);
}
