//
// Created by Hong on 2018/10/16.
//

#include "HAudio.h"

HAudio::HAudio(HPlayStatus *status,int sample_rate,HCallJava *callJava) {
    this->status = status;
    this->sample_rate = sample_rate;
    this->callJava = callJava;
    this->queue = new HQueue(status);
    this->buffer = (uint8_t *) av_malloc(44100 * 2 * 2);
}

HAudio::~HAudio() {

}

void *decodePlay(void *data){
    HAudio *audio = static_cast<HAudio *>(data);
    audio->initOpenSLES();
    pthread_exit(&audio->thread_play);
}

void HAudio::play() {
    pthread_create(&thread_play,NULL,decodePlay,this);
}

//FILE *outFile = fopen("/storage/emulated/0/myheart.pcm","w");

int HAudio::resampleAudio() {
    while(status!=NULL && !status->exit){
        if(queue->getQueueSize() == 0){//加载中
            if(!status->load) {
                status->load = true;
                callJava->onCallLoad(CHILD_THREAD, true);
            }
            continue;
        } else{
            if(status->load){
                status->load = false;
                callJava->onCallLoad(CHILD_THREAD, false);
            }
        }

        avPacket = av_packet_alloc();
        if(queue->getAvpacket(avPacket)!=0){
            av_packet_free(&avPacket);//释放avPacket里面的内存
            av_free(avPacket); //释放avPacket本身的内存
            avPacket = NULL;
            continue;
        }
        //把packet放到解码器中
        ret = avcodec_send_packet(deCodecCtx, avPacket);
        if(ret != 0){
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            continue;
        }
        avFrame = av_frame_alloc();
        //从解码器中接收packet解压缩到avFrame
        ret = avcodec_receive_frame(deCodecCtx, avFrame);
        if(ret==0){//成功
            //解决声道数和声道布局的异常情况
            if(avFrame->channels > 0&& avFrame->channel_layout == 0){
                avFrame->channel_layout = static_cast<uint64_t>(av_get_default_channel_layout(avFrame->channels));
            }
            else if(avFrame->channels == 0 && avFrame->channel_layout > 0) {
                avFrame->channels = av_get_channel_layout_nb_channels(avFrame->channel_layout);
            }

            //重采样初始化（不能改变采样率，要改变采样率需要FFmpegFilter才可以）
            SwrContext *swr_ctx;
            swr_ctx = swr_alloc_set_opts(
                    NULL,
                    AV_CH_LAYOUT_STEREO,//输出声道布局（立体声）
                    AV_SAMPLE_FMT_S16,//输出重采样的位数
                    avFrame->sample_rate,//输出采样率
                    avFrame->channel_layout,//输入声道布局
                    static_cast<AVSampleFormat>(avFrame->format),//输入采样位数
                    avFrame->sample_rate,//输入采样率
                    NULL, NULL
            );
            if(!swr_ctx || swr_init(swr_ctx) <0){
                av_packet_free(&avPacket);
                av_free(avPacket);
                avPacket = NULL;
                av_frame_free(&avFrame);
                av_free(avFrame);
                avFrame = NULL;
                swr_free(&swr_ctx);
                swr_ctx = NULL;
                continue;
            }
            //nb就是重采样后的采样率大小，实际数据存在了buffer中
            int nb = swr_convert(
                    swr_ctx,
                    &buffer,//buffer定成1秒需要的内存空间就足够，真实的重采样根本不到1秒
                    avFrame->nb_samples,//输出采样个数（这里和输入的一样）
                    (const uint8_t **) avFrame->data,//输入的数据
                    avFrame->nb_samples//输入的采样个数
            );
            int out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
            data_size = nb * out_channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);//比如44100*2*2

            now_time = avFrame->pts * av_q2d(time_base);
            if(now_time < clock)
            {
                now_time = clock;
            }
            clock = now_time;

//            fwrite(buffer,data_size,1,outFile);
//            LOGE("data_size is %d", data_size);
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            av_frame_free(&avFrame);
            av_free(avFrame);
            avFrame = NULL;
            swr_free(&swr_ctx);
            swr_ctx = NULL;
            break;
        }else{
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            av_frame_free(&avFrame);
            av_free(avFrame);
            avFrame = NULL;
            continue;
        }
    }
//    fclose(outFile);
    return data_size;
}

void pcmBufferCallBack(SLAndroidSimpleBufferQueueItf bf, void * context) {
    HAudio *audio = static_cast<HAudio *>(context);
    if(audio != NULL){
        int buffersize = audio->resampleAudio();
        if(buffersize > 0){
            audio->clock += buffersize / ((double)(audio->sample_rate * 2 * 2));
            if(audio->clock - audio->last_tiem >= 1){//不需要频繁回调，这里面限定1秒回调1次
                audio->last_tiem = audio->clock;
                audio->callJava->onCallTimeInfo(CHILD_THREAD, audio->clock, audio->duration); //回调应用层
            }
            //实际播放的方法
            (* audio-> pcmBufferQueue)->Enqueue( audio->pcmBufferQueue, (char *) audio-> buffer, buffersize);

            //如果这里不停止播放，即使歌曲播放完了，pcmBufferCallBack接口也会一直被调用
//            if(audio->clock>audio->duration){
//                audio->stop();
//            }
        }
    }
}

void HAudio::initOpenSLES() {
    SLresult result;
    result = slCreateEngine(&engineObject, 0, 0, 0, 0, 0);
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);

    //第二步，创建混音器
    const SLInterfaceID mids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean mreq[1] = {SL_BOOLEAN_FALSE};
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, mids, mreq);
    (void)result;
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    (void)result;
    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB, &outputMixEnvironmentalReverb);
    if (SL_RESULT_SUCCESS == result) {
        result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
                outputMixEnvironmentalReverb, &reverbSettings);
        (void)result;
    }
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&outputMix, 0};


    // 第三步，配置PCM格式信息
    SLDataLocator_AndroidSimpleBufferQueue android_queue={SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,2};

    SLDataFormat_PCM pcm={
            SL_DATAFORMAT_PCM,//播放pcm格式的数据
            2,//2个声道（立体声）
            static_cast<SLuint32>(getCurrentSampleRateForOpensles(sample_rate)),
            SL_PCMSAMPLEFORMAT_FIXED_16,//位数 16位
            SL_PCMSAMPLEFORMAT_FIXED_16,//和位数一致就行
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,//立体声（前左前右）
            SL_BYTEORDER_LITTLEENDIAN//结束标志
    };
    SLDataSource slDataSource = {&android_queue, &pcm};


    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};

    (*engineEngine)->CreateAudioPlayer(engineEngine, &pcmPlayerObject, &slDataSource, &audioSnk, 1, ids, req);
    //初始化播放器
    (*pcmPlayerObject)->Realize(pcmPlayerObject, SL_BOOLEAN_FALSE);

//    得到接口后调用  获取Player接口
    (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_PLAY, &pcmPlayerPlay);

//    注册回调缓冲区 获取缓冲队列接口
    (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_BUFFERQUEUE, &pcmBufferQueue);
    //缓冲接口回调
    (*pcmBufferQueue)->RegisterCallback(pcmBufferQueue, pcmBufferCallBack, this);
//    获取播放状态接口
    (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay, SL_PLAYSTATE_PLAYING);
    pcmBufferCallBack(pcmBufferQueue, this);

}

int HAudio::getCurrentSampleRateForOpensles(int sample_rate) {
    int rate = 0;
    switch (sample_rate){
        case 8000:
            rate = SL_SAMPLINGRATE_8;
            break;
        case 11025:
            rate = SL_SAMPLINGRATE_11_025;
            break;
        case 12000:
            rate = SL_SAMPLINGRATE_12;
            break;
        case 16000:
            rate = SL_SAMPLINGRATE_16;
            break;
        case 22050:
            rate = SL_SAMPLINGRATE_22_05;
            break;
        case 24000:
            rate = SL_SAMPLINGRATE_24;
            break;
        case 32000:
            rate = SL_SAMPLINGRATE_32;
            break;
        case 44100:
            rate = SL_SAMPLINGRATE_44_1;
            break;
        case 48000:
            rate = SL_SAMPLINGRATE_48;
            break;
        case 64000:
            rate = SL_SAMPLINGRATE_64;
            break;
        case 88200:
            rate = SL_SAMPLINGRATE_88_2;
            break;
        case 96000:
            rate = SL_SAMPLINGRATE_96;
            break;
        case 192000:
            rate = SL_SAMPLINGRATE_192;
            break;
        default:
            rate =  SL_SAMPLINGRATE_44_1;
    }
    return rate;
}

void HAudio::pause() {
    if(pcmPlayerPlay!=NULL){
        (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay,SL_PLAYSTATE_PAUSED);
    }
}

void HAudio::resume() {
    if(pcmPlayerPlay!=NULL){
        (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay,SL_PLAYSTATE_PLAYING);
    }
}

void HAudio::stop() {
    if(pcmPlayerPlay!=NULL){
        (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay,SL_PLAYSTATE_STOPPED);
    }
}

void HAudio::release() {
    stop();
    if(queue!=NULL){
        //delete方法会触发HQueue对象的析构方法
        delete(queue);
        queue = NULL;
    }
    if(pcmPlayerObject != NULL){
        (*pcmPlayerObject)->Destroy(pcmPlayerObject);
        pcmPlayerObject = NULL;
        pcmPlayerPlay = NULL;
        pcmBufferQueue = NULL;
    }
    if(outputMixObject != NULL){
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = NULL;
        outputMixEnvironmentalReverb = NULL;
    }
    if(engineObject != NULL){
        (*engineObject)->Destroy(engineObject);
        engineObject = NULL;
        engineEngine = NULL;
    }
    if(buffer != NULL) {
        //真正的释放占用的内存
        free(buffer);
        buffer = NULL;
    }
    if(deCodecCtx != NULL){
        avcodec_close(deCodecCtx);
        avcodec_free_context(&deCodecCtx);
        deCodecCtx = NULL;
    }
    if(status != NULL){
        //这里面不用free status占用的内存。因为在HFFmpeg对象中还会用到？？？
        status = NULL;
    }
    if(callJava != NULL){
        callJava = NULL;
    }
}


