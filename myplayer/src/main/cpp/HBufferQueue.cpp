//
// Created by ywl on 2017-12-3.
//

#include "HBufferQueue.h"
#include "AndroidLog.h"

HBufferQueue::HBufferQueue(HPlayStatus *playStatus) {
    this->playStatus = playStatus;
    pthread_mutex_init(&mutexBuffer, NULL);
    pthread_cond_init(&condBuffer, NULL);
}

HBufferQueue::~HBufferQueue() {
    playStatus = NULL;
    pthread_mutex_destroy(&mutexBuffer);
    pthread_cond_destroy(&condBuffer);
    if(LOG_DEBUG){
        LOGE("HBufferQueue 释放完了");
    }
}

void HBufferQueue::release() {
    if(LOG_DEBUG){
        LOGE("WlBufferQueue::release");
    }
    noticeThread();
    clearBuffer();
    if(LOG_DEBUG){
        LOGE("WlBufferQueue::release success");
    }
}

int HBufferQueue::putBuffer(SAMPLETYPE *buffer, int size) {
    pthread_mutex_lock(&mutexBuffer);
    HPcmBean *pcmBean = new HPcmBean(buffer, size);
    queueBuffer.push_back(pcmBean);
    pthread_cond_signal(&condBuffer);
    pthread_mutex_unlock(&mutexBuffer);
    return 0;
}

int HBufferQueue::getBuffer(HPcmBean **pcmBean) {
    pthread_mutex_lock(&mutexBuffer);
    while(playStatus != NULL && !playStatus->exit){
        if(queueBuffer.size() > 0) {
            *pcmBean = queueBuffer.front();
            queueBuffer.pop_front();
            break;
        } else{
            if(!playStatus->exit) {
                pthread_cond_wait(&condBuffer, &mutexBuffer);
            }
        }
    }
    pthread_mutex_unlock(&mutexBuffer);
    return 0;
}

int HBufferQueue::clearBuffer() {
    pthread_cond_signal(&condBuffer);
    pthread_mutex_lock(&mutexBuffer);
    while (!queueBuffer.empty()) {
        HPcmBean *pcmBean = queueBuffer.front();
        queueBuffer.pop_front();
        delete(pcmBean);
    }
    pthread_mutex_unlock(&mutexBuffer);
    return 0;
}

int HBufferQueue::getBufferSize() {
    int size = 0;
    pthread_mutex_lock(&mutexBuffer);
    size = queueBuffer.size();
    pthread_mutex_unlock(&mutexBuffer);
    return size;
}


int HBufferQueue::noticeThread() {
    pthread_cond_signal(&condBuffer);
    return 0;
}

