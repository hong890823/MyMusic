//
// Created by Hong on 2018/10/16.
//

#ifndef MYMUSIC_HQUEUE_H
#define MYMUSIC_HQUEUE_H

#include "queue"
#include "pthread.h"
#include "AndroidLog.h"
#include "HPlayStatus.h"

extern "C"{
#include "libavcodec/avcodec.h"
};

class HQueue {
public:
    std::queue<AVPacket *> queuePacket;
    pthread_mutex_t mutextPacket;
    pthread_cond_t condPacket;
    HPlayStatus *playStatus;
public:
    HQueue(HPlayStatus *playStatus);
    ~HQueue();

    int putAvpacket(AVPacket *packet);
    int getAvpacket(AVPacket *packet);
    int getQueueSize();
    void clearPacket();
};


#endif //MYMUSIC_HQUEUE_H
