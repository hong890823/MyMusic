//
// Created by Hong on 2018/10/16.
//

#ifndef MYMUSIC_FFMPEG_H
#define MYMUSIC_FFMPEG_H

#include "HCallJava.h"
#include "pthread.h"
#include "AndroidLog.h"
#include "HAudio.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/time.h>
};

class HFFmpeg {
public:
    HCallJava *callJava = NULL;
    const char *source = NULL;
    pthread_t decodeThread;
    AVFormatContext *ctx = NULL;
    HAudio *audio = NULL;
    HPlayStatus *status = NULL;

    pthread_mutex_t init_mutex;
    bool exit = false;

    pthread_mutex_t seek_mutex;
    int duration;
public:
    HFFmpeg(HPlayStatus *status,HCallJava *callJava,const char *source);
    ~HFFmpeg();

    void prepared();
    void decodeFFmpegThread();
    void start();
    void pause();
    void resume();
    void release();
    void seek(int64_t seconds);
    void setVolume(int percent);
    void setMute(int mute);
    void setPicth(float picth);
    void setSpeed(float speed);
    int getSampleRate();
    void startOrStopRecord(bool isStartRecord);
};


#endif //MYMUSIC_FFMPEG_H
