//
// Created by Hong on 2018/10/16.
//

#include "HAudio.h"

HAudio::HAudio(HPlayStatus *status,int sample_rate,HCallJava *callJava) {
    this->status = status;
    this->sample_rate = sample_rate;
    this->callJava = callJava;
    this->queue = new HQueue(status);
    this->buffer = (uint8_t *) av_malloc(static_cast<size_t>(sample_rate * 2 * 2));//demo中sample_rate的值也是44100

    sampleBuffer = static_cast<SAMPLETYPE *>(malloc(static_cast<size_t>(sample_rate * 2 * 2)));
    this->soundTouch = new SoundTouch();
    soundTouch->setSampleRate(static_cast<uint>(sample_rate));
    soundTouch->setChannels(2);
    soundTouch->setPitch(pitch);//变调
    soundTouch->setTempo(speed);//变速

    this->isCut = false;
    this->endTime = 0;
    this->isReturnPcm = false;
}

HAudio::~HAudio() {

}

void *decodePlay(void *data){
    HAudio *audio = static_cast<HAudio *>(data);
    audio->initOpenSLES();
    pthread_exit(&audio->thread_play);
}

void *pcmCallBack(void *data){
    HAudio *audio = static_cast<HAudio *>(data);
    while(audio->status!=NULL && !audio->status->exit){
        HPcmBean *pcmBean = NULL;
        audio->bufferQueue->getBuffer(&pcmBean);
        if(pcmBean==NULL)continue;
        if(pcmBean->buffsize<=audio->defaultPcmSize){//不需要分包
            if(audio->isStartRecord){
                audio->callJava->onCallPcmToAac(CHILD_THREAD, pcmBean->buffsize, pcmBean->buffer);
            }
            if(audio->isReturnPcm){
                audio->callJava->onCallPcmInfo(pcmBean->buffer, pcmBean->buffsize);
            }
        }else{
            int pack_num = pcmBean->buffsize / audio->defaultPcmSize;
            int pack_sub = pcmBean->buffsize % audio->defaultPcmSize;

            for(int i = 0; i < pack_num; i++) {
                char *bf = static_cast<char *>(malloc(audio->defaultPcmSize));
                //参数二：起始地址+偏移量
                memcpy(bf, pcmBean->buffer + i * audio->defaultPcmSize, audio->defaultPcmSize);
                if(audio->isStartRecord) {
                    audio->callJava->onCallPcmToAac(CHILD_THREAD, audio->defaultPcmSize, bf);
                }
                if(audio->isReturnPcm){
                    audio->callJava->onCallPcmInfo(bf, audio->defaultPcmSize);
                }
                free(bf);
            }

            if(pack_sub > 0){
                char *bf = static_cast<char *>(malloc(pack_sub));
                memcpy(bf, pcmBean->buffer + pack_num * audio->defaultPcmSize, pack_sub);
                if(audio->isStartRecord) {
                    audio->callJava->onCallPcmToAac(CHILD_THREAD, pack_sub, bf);
                }
                if(audio->isReturnPcm){
                    audio->callJava->onCallPcmInfo(bf, pack_sub);
                }
            }
        }
        delete(pcmBean);
        pcmBean = NULL;
    }
    pthread_exit(&audio->pcmCallBackThread);
}

void HAudio::play() {
    bufferQueue = new HBufferQueue(status);
    pthread_create(&thread_play,NULL,decodePlay,this);
    pthread_create(&pcmCallBackThread,NULL,pcmCallBack,this);
}

//FILE *outFile = fopen("/storage/emulated/0/myheart.pcm","w");

int HAudio::resampleAudio(uint8_t **outBuffer) {
    data_size = 0;
    while(status!=NULL && !status->exit){
        if(status->seek){
            av_usleep(1000*100);
            continue;
        }

        if(queue->getQueueSize() == 0){//加载中
            if(!status->load) {
                status->load = true;
                callJava->onCallLoad(CHILD_THREAD, true);
            }
            av_usleep(1000*100);
            continue;
        } else{
            if(status->load){
                status->load = false;
                callJava->onCallLoad(CHILD_THREAD, false);
            }
        }
        //当全部读取完毕之后，在拿新的avPacket放大解码器中
        if(readFrameFinished){
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
        }

        avFrame = av_frame_alloc();
        //从解码器中接收packet解压缩到avFrame
        ret = avcodec_receive_frame(deCodecCtx, avFrame);
        if(ret==0){//成功
            readFrameFinished = false;
            //解决声道数和声道布局的异常情况
            if(avFrame->channels > 0&& avFrame->channel_layout == 0){
                avFrame->channel_layout = static_cast<uint64_t>(av_get_default_channel_layout(avFrame->channels));
            }
            else if(avFrame->channels == 0 && avFrame->channel_layout > 0) {
                avFrame->channels = av_get_channel_layout_nb_channels(avFrame->channel_layout);
            }

            //重采样初始化（不能改变采样率，要改变采样率需要AVFilter才可以）
            //那这里的重采样的意义是什么呢？一个是为了得到PCM数据，一个是为了统一采样位数以及声道布局
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
                readFrameFinished = true;
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
            //nb就是重采样后的单位时间的采样数据的大小（这个值基本上应该是小于1秒的采样率的值），实际数据存在了buffer中
            nb = swr_convert(
                    swr_ctx,
                    &buffer,//转码后输出的PCM数据；buffer定成1秒需要的内存空间就足够，真实的重采样根本不到1秒
                    avFrame->nb_samples,//输出采样个数（这里和输入的一样）
                    (const uint8_t **) avFrame->data,//输入的数据
                    avFrame->nb_samples//输入的采样个数
            );
            int out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
            data_size = nb * out_channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);//比如44100*2*2
            *outBuffer = buffer;

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
            readFrameFinished = true;
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

//OpenSLES里面也有变速功能，但是它的变速在同时也改变了音调，无法分开所以不采用
int HAudio::getSoundTouchData() {
    while(status!=NULL && !status->exit){
        outBuffer = NULL;
        if(finished){
            finished = false;
            data_size = resampleAudio(&outBuffer);
            if(data_size>0){
                for(int i = 0; i < data_size / 2 + 1; i++){
                    //SoundTouch要求的数据格式是16bit integer，而我们的outBuffer是uint8的，需要转换
                    sampleBuffer[i] = (outBuffer[i * 2] | ((outBuffer[i * 2 + 1]) << 8));
                }
                soundTouch->putSamples(sampleBuffer, nb);
                num = soundTouch->receiveSamples(sampleBuffer,data_size/(2*2));//data_size/(2*2)得到的值其实就是nb的值
            }else{
                soundTouch->flush();
            }
        }

        if(num==0){
            finished = true;
            continue;
        }else{//没有再需要进行转化的数据，把sampleBuffer中剩余的数据拿出来
            if(outBuffer==NULL){
                num = soundTouch->receiveSamples(sampleBuffer,data_size/4);
                if(num==0){
                    finished = true;
                    continue;
                }
            }
            return num;
        }

    }
    return 0;
}

void pcmBufferCallBack(SLAndroidSimpleBufferQueueItf bf, void * context) {
    HAudio *audio = static_cast<HAudio *>(context);
    if(audio != NULL){
//        int buffersize = audio->resampleAudio(&audio-> outBuffer); //对应普通播放
        int buffersize = audio->getSoundTouchData(); //对应特效播放
        if(buffersize > 0){
            audio->clock += buffersize / ((double)(audio->sample_rate * 2 * 2));
            if(audio->clock - audio->last_tiem >= 1){//不需要频繁回调，这里面限定1秒回调1次
                audio->last_tiem = audio->clock;
                audio->callJava->onCallTimeInfo(CHILD_THREAD, audio->clock, audio->duration); //回调应用层
            }
//            if(audio->isStartRecord)audio->callJava->onCallPcmToAac(CHILD_THREAD,buffersize*2*2,audio->sampleBuffer);
            audio->bufferQueue->putBuffer(audio->sampleBuffer, buffersize * 4);//pcm数据分包，防止应用层录音崩溃

            int db = audio->getPCMDB(reinterpret_cast<char *>(audio->sampleBuffer), buffersize * 4);
//            LOGD("分贝值是%i",db);

            //实际播放的方法
//            (* audio-> pcmBufferQueue)->Enqueue( audio->pcmBufferQueue, (char *) audio-> buffer, buffersize); //对应普通播放
            (* audio-> pcmBufferQueue)->Enqueue( audio->pcmBufferQueue, (char *) audio-> sampleBuffer, buffersize*2*2);  //对应特效播放

            //如果需要裁剪音频的话
            //更独立的裁剪可以放到一个单独的线程去仿照这个逻辑做，速度会快一些。
            if(audio->isCut){
//                if(audio->isReturnPcm){
//                    audio->callJava->onCallPcmInfo(audio->sampleBuffer, buffersize * 2 * 2);
//                }
                if(audio->clock > audio->endTime){
                    LOGE("裁剪退出...");
                    audio->status->exit = true;
                }
            }

        }
    }
}

void HAudio::initOpenSLES() {
    /*
     *引擎接口，混音器接口，播放器接口等创建的三部曲
     * create->realize->get
     * */

    //第一步，创建引擎
    SLresult result;
    result = slCreateEngine(&engineObject, 0, 0, 0, 0, 0);
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);

    //第二步，创建混音器
    const SLInterfaceID mids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean mreq[1] = {SL_BOOLEAN_FALSE};
    //param 1:mids数组的个数； mids：混音的方式吧
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

    // 第三步，创建播放器
    // 配置PCM格式信息
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

    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&outputMix, 0};

    //切记这里面一定要把相应的功能都加上
    const SLInterfaceID ids[4] = {SL_IID_BUFFERQUEUE,SL_IID_VOLUME,SL_IID_PLAYBACKRATE,SL_IID_MUTESOLO};
    const SLboolean req[4] = {SL_BOOLEAN_TRUE,SL_BOOLEAN_TRUE,SL_BOOLEAN_TRUE,SL_BOOLEAN_TRUE};
   //切记功能数目变成相应数量
    (*engineEngine)->CreateAudioPlayer(engineEngine, &pcmPlayerObject, &slDataSource, &audioSnk, 4, ids, req);
    //初始化播放器
    (*pcmPlayerObject)->Realize(pcmPlayerObject, SL_BOOLEAN_FALSE);

//    得到接口后调用  获取Player接口
    (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_PLAY, &pcmPlayerPlay);
    //初始化音量接口
    (*pcmPlayerObject)->GetInterface(pcmPlayerObject,SL_IID_VOLUME,&pcmPlayerVolume);
    //初始化声道接口
    (*pcmPlayerObject)->GetInterface(pcmPlayerObject,SL_IID_MUTESOLO,&pcmPlayerMuteSolo);

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
    if(bufferQueue != NULL)
    {
        bufferQueue->noticeThread();
        pthread_join(pcmCallBackThread, NULL);
        bufferQueue->release();
        delete(bufferQueue);
        bufferQueue = NULL;
    }

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
        pcmPlayerVolume = NULL;
        pcmPlayerMuteSolo = NULL;
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
    if(outBuffer != NULL){
        //这里面不用free掉outBuffer的内存
        //因为outBuffer我们就没有申请内存，它和buffer用的是一块内存
        outBuffer = NULL;
    }
    if(soundTouch!=NULL){
        delete(soundTouch);
        soundTouch = NULL;
    }
    if(sampleBuffer!=NULL){
        free(sampleBuffer);
        sampleBuffer=NULL;
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

void HAudio::setVolume(int percent) {
    if(pcmPlayerVolume!=NULL){
        //音量基本的分段算法（为了可以使音量更均衡的变化）
        if(percent > 30){
            (*pcmPlayerVolume)->SetVolumeLevel(pcmPlayerVolume,static_cast<SLmillibel>((100 - percent) * -20));
        }
        else if(percent > 25){
            (*pcmPlayerVolume)->SetVolumeLevel(pcmPlayerVolume,static_cast<SLmillibel>((100 - percent) * -22));
        }
        else if(percent > 20){
            (*pcmPlayerVolume)->SetVolumeLevel(pcmPlayerVolume,static_cast<SLmillibel>((100 - percent) * -25));
        }
        else if(percent > 15){
            (*pcmPlayerVolume)->SetVolumeLevel(pcmPlayerVolume,static_cast<SLmillibel>((100 - percent) * -28));
        }
        else if(percent > 10){
            (*pcmPlayerVolume)->SetVolumeLevel(pcmPlayerVolume,static_cast<SLmillibel>((100 - percent) * -30));
        }
        else if(percent > 5){
            (*pcmPlayerVolume)->SetVolumeLevel(pcmPlayerVolume,static_cast<SLmillibel>((100 - percent) * -34));
        }
        else if(percent > 3){
            (*pcmPlayerVolume)->SetVolumeLevel(pcmPlayerVolume,static_cast<SLmillibel>((100 - percent) * -37));
        }
        else if(percent > 0){
            (*pcmPlayerVolume)->SetVolumeLevel(pcmPlayerVolume,static_cast<SLmillibel>((100 - percent) * -40));
        }
        else{
            (*pcmPlayerVolume)->SetVolumeLevel(pcmPlayerVolume,static_cast<SLmillibel>((100 - percent) * -100));
        }
    }

}

/**
 * 右声道为0，左声道为1.（在戴耳机的时候感受最为明显）
 * */
void HAudio::setMute(int mute) {
    if(pcmPlayerMuteSolo!=NULL){
        if(0==mute){//右声道
            (*pcmPlayerMuteSolo)->SetChannelMute(pcmPlayerMuteSolo,0,SL_BOOLEAN_TRUE);
            (*pcmPlayerMuteSolo)->SetChannelMute(pcmPlayerMuteSolo,1,SL_BOOLEAN_FALSE);
        }
        else if(1==mute){//左声道
            (*pcmPlayerMuteSolo)->SetChannelMute(pcmPlayerMuteSolo,0,SL_BOOLEAN_FALSE);
            (*pcmPlayerMuteSolo)->SetChannelMute(pcmPlayerMuteSolo,1,SL_BOOLEAN_TRUE);
        }
        else if(2==mute){//立体声
            (*pcmPlayerMuteSolo)->SetChannelMute(pcmPlayerMuteSolo,0,SL_BOOLEAN_FALSE);
            (*pcmPlayerMuteSolo)->SetChannelMute(pcmPlayerMuteSolo,1,SL_BOOLEAN_FALSE);
        }
        LOGD("当前声道是%i",mute);
    }
}

void HAudio::setPitch(float pitch) {
    this->pitch = pitch;
    if(soundTouch!=NULL)soundTouch->setPitch(pitch);//变调
}

void HAudio::setSpeed(float speed) {
    this->speed = speed;
    if(soundTouch!=NULL)soundTouch->setTempo(speed);//变速
}

//计算分贝值(可能不太准...)
int HAudio::getPCMDB(char *pcmcata, size_t pcmsize) {
    int db = 0;
    short int pervalue = 0;
    double sum = 0;
    for(int i = 0; i < pcmsize; i+= 2){
        memcpy(&pervalue, pcmcata+i, 2);
        sum += abs(pervalue);
    }
    sum = sum / (pcmsize / 2);
    if(sum > 0)
    {
        db = static_cast<int>((int)20.0 * log10(sum));//计算振幅的方法
    }
    return db;
}

void HAudio::startOrStopRecord(bool isStartRecord) {
    this->isStartRecord = isStartRecord;
}


