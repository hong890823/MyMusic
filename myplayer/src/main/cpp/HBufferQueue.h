//
// Created by ywl on 2017-12-3.
//

#ifndef HPLAYER_BUFFERQUEUE_H
#define HPLAYER_BUFFERQUEUE_H

#include "deque"
#include "HPlayStatus.h"
#include "HPcmBean.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include "pthread.h"
};

class HBufferQueue {

public:
    std::deque<HPcmBean *> queueBuffer;
    pthread_mutex_t mutexBuffer;
    pthread_cond_t condBuffer;
    HPlayStatus *playStatus = NULL;

public:
    HBufferQueue(HPlayStatus *playStatus);
    ~HBufferQueue();
    int putBuffer(SAMPLETYPE *buffer, int size);
    int getBuffer(HPcmBean **pcmBean);
    int clearBuffer();

    void release();
    int getBufferSize();

    int noticeThread();
};


#endif //HPLAYER_BUFFERQUEUE_H
