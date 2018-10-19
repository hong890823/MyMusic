//
// Created by Hong on 2018/10/16.
//

#include "HQueue.h"

HQueue::HQueue(HPlayStatus *playStatus) {
    this->playStatus = playStatus;
    pthread_mutex_init(&mutextPacket,NULL);
    pthread_cond_init(&condPacket,NULL);
}

HQueue::~HQueue() {
    clearPacket();
    pthread_mutex_destroy(&mutextPacket);
    pthread_cond_destroy(&condPacket);
}

//加锁解锁必须成对出现
int HQueue::putAvpacket(AVPacket *packet) {
    pthread_mutex_lock(&mutextPacket);
    queuePacket.push(packet);
    if(LOG_DEBUG){
//        LOGD("放入一个Avpacket到队列里面，个数为:%d",queuePacket.size());
    }
    pthread_cond_signal(&condPacket);
    pthread_mutex_unlock(&mutextPacket);
    return 0;
}

int HQueue::getAvpacket(AVPacket *packet) {
    pthread_mutex_lock(&mutextPacket);
    while(playStatus!=NULL && !playStatus->exit){
        if(queuePacket.size()>0){
            //拿出队头
            AVPacket *avPacket = queuePacket.front();
            //把队列头的avPacket引用拷贝给参数packet，引用数加一
            if(av_packet_ref(packet,avPacket)==0){
                queuePacket.pop();
            }
            //释放掉加一的引用数，data数据并没有释放掉，内存也没有释放掉。所以data数据依然可以给外部用
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            if(LOG_DEBUG){
//                LOGD("从队列里面取出一个Avpacket,还剩下%d个",queuePacket.size());
            }
            break;
        }else{
            pthread_cond_wait(&condPacket,&mutextPacket);
        }
    }
    pthread_mutex_unlock(&mutextPacket);
    return 0;
}

int HQueue::getQueueSize() {
    int size = 0;
    pthread_mutex_lock(&mutextPacket);
    size = queuePacket.size();
    pthread_mutex_unlock(&mutextPacket);
    return size;
}

void HQueue::clearPacket() {
    pthread_cond_signal(&condPacket);
    pthread_mutex_lock(&mutextPacket);
    while (!queuePacket.empty()){
        AVPacket *packet = queuePacket.front();
        queuePacket.pop();
        av_packet_free(&packet);
        av_free(packet);
        packet = NULL;
    }
    pthread_mutex_unlock(&mutextPacket);

}
