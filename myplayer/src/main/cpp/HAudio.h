//
// Created by Hong on 2018/10/16.
//

#ifndef MYMUSIC_HAUDIO_H
#define MYMUSIC_HAUDIO_H

#include "HQueue.h"
#include "HPlayStatus.h"
#include "HCallJava.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include <libswresample/swresample.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
};

class HAudio {
public:
    int streamIndex = -1;
    //编码器上下文
//    AVCodecContext *codecCtx = NULL;
    AVCodecParameters *codecPar = NULL;
    //解码器上下文
    AVCodecContext *deCodecCtx = NULL;
    HQueue *queue = NULL;
    HPlayStatus *status = NULL;
    HCallJava *callJava = NULL;

    pthread_t thread_play;
    AVPacket *avPacket = NULL;
    AVFrame *avFrame = NULL;
    int ret = 0;
    uint8_t *buffer = NULL;
    int data_size = 0;
    int sample_rate = 0;

    int duration = 0;
    AVRational time_base;
    double clock;//总的播放时长
    double now_time;//当前frame时间
    double last_tiem; //上一次调用时间

    // 引擎接口
    SLObjectItf engineObject = NULL;
    SLEngineItf engineEngine = NULL;

    //混音器
    SLObjectItf outputMixObject = NULL;
    SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;
    SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

    //pcm播放器
    SLObjectItf pcmPlayerObject = NULL;
    SLPlayItf pcmPlayerPlay = NULL;

    //缓冲器队列接口
    SLAndroidSimpleBufferQueueItf pcmBufferQueue = NULL;
public:
    HAudio( HPlayStatus *status,int sample_rate,HCallJava *callJava);
    ~HAudio();

    void play();
    int resampleAudio();

    void initOpenSLES();

    int getCurrentSampleRateForOpensles(int sample_rate);

    void pause();
    void resume();
    void stop();
    void release();
};


#endif //MYMUSIC_HAUDIO_H
