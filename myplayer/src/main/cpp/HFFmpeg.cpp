//
// Created by Hong on 2018/10/16.
//

#include "HFFmpeg.h"

HFFmpeg::HFFmpeg(HPlayStatus *status,HCallJava *callJava, const char *source) {
    this->status = status;
    this->callJava = callJava;
    this->source = source;
    this->exit = false;
    pthread_mutex_init(&init_mutex, NULL);
    pthread_mutex_init(&seek_mutex,NULL);
}

HFFmpeg::~HFFmpeg() {
    pthread_mutex_destroy(&init_mutex);
    pthread_mutex_destroy(&seek_mutex);
}

//该方法属于线程的回调方法和HFFmpeg对象无关，所以需要把HFFmpeg的对象传进来才能调用HFFmpeg的方法
void *decodeCallBack(void *data){
    HFFmpeg *FFmpeg = static_cast<HFFmpeg *>(data);
    FFmpeg->decodeFFmpegThread();
    //一样的
//    pthread_exit(&FFmpeg->decodeThread);
    pthread_exit(&(FFmpeg->decodeThread));
}

void HFFmpeg::prepared() {
    pthread_create(&decodeThread,NULL,decodeCallBack,this);
}

int avformat_callback(void *ctx) {
    HFFmpeg *fFmpeg = (HFFmpeg *) ctx;
    if(fFmpeg->status->exit){
        return AVERROR_EOF;
    }
    return 0;
}

//每个return之前都要解锁，否则会造成死锁（只加锁不解锁不就是死锁嘛）--锁要成对出现哦
void HFFmpeg::decodeFFmpegThread() {
    pthread_mutex_lock(&init_mutex);

    av_register_all();
    avformat_network_init();
    ctx = avformat_alloc_context();
    ctx->interrupt_callback.callback = avformat_callback;
    ctx->interrupt_callback.opaque = this;

    if(avformat_open_input(&ctx,source,NULL,NULL)!=0){
        LOGE("avformat open input failed");
        callJava->onCallError(CHILD_THREAD,1001,"HFFmpeg.decodeFFmpegThread-输入地址打开失败");
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }
    if(avformat_find_stream_info(ctx,NULL)<0){
        LOGE(" avformat find no stream info");
        callJava->onCallError(CHILD_THREAD,1002,"HFFmpeg.decodeFFmpegThread-输入流打开失败");
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }
    for(int i=0;i<ctx->nb_streams;i++){
        //ctx.streams[i].codec.codec_type 已经过时
        if(ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            if(audio==NULL){
                audio = new HAudio(status,ctx->streams[i]->codecpar->sample_rate,callJava);
                audio->streamIndex = i;
                audio->codecPar = ctx->streams[i]->codecpar;
                audio->duration = static_cast<int>(ctx->duration / AV_TIME_BASE);//总时间
                audio->time_base = ctx->streams[i]->time_base;//当前时间，需要累加
                duration = audio->duration;
                //这里别用break跳出循环了...可能崩溃
            }
        }
    }
    //获取编码器对应的解码器
    AVCodec *dec = avcodec_find_decoder(audio->codecPar->codec_id);
    if(!dec){
        LOGE("avcodec find decoder wrong");
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }
    //声明解码器的上下文
    audio->deCodecCtx = avcodec_alloc_context3(dec);
    if(!audio->deCodecCtx){
        LOGE("avcodec alloc context3 wrong");
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }
    //把编码器中的属性复制到解码器上下文中
    if(avcodec_parameters_to_context(audio->deCodecCtx,audio->codecPar)<0){
        LOGE("avcodec parameters to context wrong");
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }
    if(avcodec_open2(audio->deCodecCtx,dec,0)!=0){
        LOGE("avcodec open2 wrong");
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }

    if(callJava != NULL) {
        if(status != NULL && !status->exit) {
            callJava->onCallPrepared(CHILD_THREAD);
        } else{
            exit = true;
        }
    }
    pthread_mutex_unlock(&init_mutex);
}

void HFFmpeg::start() {
    if(audio==NULL)return;

    //重采样的方法
    audio->play();

    int count = 0;
    while(status!=NULL && !status->exit){
        if(status->seek)continue;
        if(audio->queue->getQueueSize()>40)continue;//不要一次性全部读帧。这样控制一下既可以减少内存压力，又可以防止seek出错。

        AVPacket *avPacket = av_packet_alloc();//此时avPacket是一个空包
        pthread_mutex_lock(&seek_mutex);//尽量减小锁的粒度
        int result = av_read_frame(ctx,avPacket); //av_read_frame读帧之后avPacket包才有了实质的内容
        pthread_mutex_unlock(&seek_mutex);

        if(result==0){
            if(avPacket->stream_index==audio->streamIndex){
                count++;
//                LOGD("读取第 %d 帧", count);
                audio->queue->putAvpacket(avPacket);
            }else{
                av_packet_free(&avPacket);
                av_free(avPacket);
                avPacket = NULL;
            }
        }else{
            //之前一直正确的读帧，当读不出来的时候就跳出循环
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;

            while(status!=NULL && !status->exit){
                if(audio->queue->getQueueSize()>0){
                    continue;
                }else{
                    status->exit = true;
                    break;
                }
            }

        }

    }

    if(callJava!=NULL)callJava->onCallComplete(CHILD_THREAD);
    exit = true;
}

void HFFmpeg::pause() {
    if(audio!=NULL)audio->pause();
}

void HFFmpeg::resume() {
    if(audio!=NULL)audio->resume();
}

//释放资源从这里开始，就会调用HAudio的release方法，就会调用了HQueue的析构函数
void HFFmpeg::release() {
    if(LOG_DEBUG) {
        LOGE("开始释放FFmpeg");
    }
    if(LOG_DEBUG) {
        LOGE("开始释放FFmpeg2");
    }
    status->exit = true;

    pthread_mutex_lock(&init_mutex);
    int sleepCount = 0;
    while (!exit){
        if(sleepCount > 1000) {
            exit = true;
        }
        if(LOG_DEBUG){
            LOGE("wait ffmpeg  exit %d", sleepCount);
        }
        sleepCount++;
        av_usleep(1000 * 10);//暂停10毫秒
    }

    if(LOG_DEBUG) {
        LOGE("释放 Audio");
    }
    if(audio != NULL) {
        audio->release();
        delete(audio);
        audio = NULL;
    }

    if(LOG_DEBUG) {
        LOGE("释放 封装格式上下文");
    }
    if(ctx != NULL) {
        avformat_close_input(&ctx);
        avformat_free_context(ctx);
        ctx = NULL;
    }
    if(LOG_DEBUG) {
        LOGE("释放 callJava");
    }
    if(callJava != NULL) {
        callJava = NULL;
    }
    if(LOG_DEBUG){
        LOGE("释放 playstatus");
    }
    if(status != NULL){
        status = NULL;
    }
    pthread_mutex_unlock(&init_mutex);
}

void HFFmpeg::seek(int64_t seconds) {
    if(duration<=0)return; //直播类的不需要seek功能
    if(seconds>0 && seconds<=duration){
        if(audio!=NULL){
            status->seek = true;
            audio->queue->clearPacket();
            audio->clock = 0;
            audio->last_tiem = 0;

            pthread_mutex_lock(&seek_mutex);
            ino64_t ts = static_cast<ino64_t>(seconds * AV_TIME_BASE);
            avformat_seek_file(ctx,-1,INT64_MIN,ts,INT64_MAX,0);
            pthread_mutex_unlock(&seek_mutex);

            status->seek = false;
        }
    }
}

void HFFmpeg::setVolume(int percent) {
    if(audio!=NULL)audio->setVolume(percent);
}

void HFFmpeg::setMute(int mute) {
    if(audio!=NULL)audio->setMute(mute);
}

void HFFmpeg::setPicth(float pitch) {
    if(audio!=NULL)audio->setPitch(pitch);
}

void HFFmpeg::setSpeed(float speed) {
    if(audio!=NULL)audio->setSpeed(speed);
}
