#include <jni.h>
#include <string>
#include "AndroidLog.h"
#include "HCallJava.h"
#include "HFFmpeg.h"



JavaVM *jvm = NULL;
HCallJava *callJava = NULL;
HFFmpeg *FFmpeg = NULL;
HPlayStatus *status = NULL;

bool n_exit = true;//防止重复调用stop方法出错
pthread_t thread_start;

extern "C"
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm,void *reserved){
    jint result = -1;
    jvm = vm;
    JNIEnv *env;
    if(vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_4) != JNI_OK){
        return result;
    }
    return JNI_VERSION_1_4;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_hong_myplayer_player_HPlayer_n_1prepare(JNIEnv *env, jobject instance,jstring source_) {
    const char *source = env->GetStringUTFChars(source_, JNI_FALSE);
    if(FFmpeg==NULL){
        if(callJava==NULL)callJava = new HCallJava(jvm,env,instance);
        callJava->onCallLoad(MAIN_THREAD, true);
        status = new HPlayStatus();
        FFmpeg = new HFFmpeg(status,callJava,source);
        FFmpeg->prepared();

        //这里不能释放source，否则在子线程操作HFFmpeg对象的source的时候，source不存在了
//        env->ReleaseStringUTFChars(source_,source);
    }

}

void *startCallBack(void *data){
    HFFmpeg *ffmpeg = static_cast<HFFmpeg *>(data);
    ffmpeg->start();
    pthread_exit(&thread_start);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_hong_myplayer_player_HPlayer_n_1start(JNIEnv *env, jobject instance) {
    pthread_create(&thread_start,NULL,startCallBack,FFmpeg);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_hong_myplayer_player_HPlayer_n_1pause(JNIEnv *env, jobject instance) {
    if(FFmpeg!=NULL)FFmpeg->pause();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_hong_myplayer_player_HPlayer_n_1resume(JNIEnv *env, jobject instance) {
    if(FFmpeg!=NULL)FFmpeg->resume();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_hong_myplayer_player_HPlayer_n_1stop(JNIEnv *env, jobject instance) {
    if(!n_exit)return;
    n_exit = false;
    if(FFmpeg != NULL){
        FFmpeg->release();
        delete(FFmpeg);
        FFmpeg = NULL;
        if(callJava != NULL) {
            delete(callJava);
            callJava = NULL;
        }
        if(status != NULL) {
            delete(status);
            status = NULL;
        }
    }
    n_exit = true;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_hong_myplayer_player_HPlayer_n_1seek(JNIEnv *env, jobject instance, jint seconds) {
    if(FFmpeg!=NULL)FFmpeg->seek(seconds);
}