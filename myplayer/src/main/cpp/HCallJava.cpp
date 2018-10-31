//
// Created by Hong on 2018/10/15.
//

#include "HCallJava.h"
#include "AndroidLog.h"

HCallJava::HCallJava(JavaVM *vm, JNIEnv *env, jobject obj) {
    this->jvm = vm;
    this->env = env;
    //把obj变成全局变量才能赋值给全局的jobject引用
    this->obj = env->NewGlobalRef(obj);

    jclass clz = this->env->GetObjectClass(this->obj);
    if(!clz){
        if(LOG_DEBUG)LOGE("get jclass wrong");
        return;
    }
    this->jmid_prepared = this->env->GetMethodID(clz,"onCallPrepared","()V");
    this->jmid_load = this->env->GetMethodID(clz,"onCallLoad","(Z)V");
    this->jmid_timeinfo = this->env->GetMethodID(clz,"onCallTimeInfo","(II)V");
    this->jmid_error = this->env->GetMethodID(clz,"onCallError","(ILjava/lang/String;)V");
    this->jmid_complete = this->env->GetMethodID(clz,"onCallComplete","()V");
    this->jmid_pcm2aac = this->env->GetMethodID(clz,"encodecPcmToAac","(I[B)V");
    this->jmid_pcminfo = this->env->GetMethodID(clz,"onCallPcmInfo","([BI)V");
    this->jmid_pcmrate = this->env->GetMethodID(clz,"onCallPcmRate","(I)V");
}

HCallJava::~HCallJava() {

}

void HCallJava::onCallPrepared(int type) {
    if(MAIN_THREAD==type){
        env->CallVoidMethod(obj,jmid_prepared);
    }
    else if(CHILD_THREAD==type){
        JNIEnv *env;
        if(jvm->AttachCurrentThread(&env,0)!=JNI_OK){
            if(LOG_DEBUG)LOGE("get child thread JNIEnv wrong")
            return;
        }
        env->CallVoidMethod(obj,jmid_prepared);
        jvm->DetachCurrentThread();
    }
}

void HCallJava::onCallLoad(int type, bool load) {
    if(MAIN_THREAD==type){
        env->CallVoidMethod(obj,jmid_load,load);
    }
    else if(CHILD_THREAD==type){
        JNIEnv *env;
        if(jvm->AttachCurrentThread(&env,0)!=JNI_OK){
            if(LOG_DEBUG)LOGE("get child thread JNIEnv wrong")
            return;
        }
        env->CallVoidMethod(obj,jmid_load,load);
        jvm->DetachCurrentThread();
    }
}

void HCallJava::onCallTimeInfo(int type, int clock, int total) {
    if(type == MAIN_THREAD){
        env->CallVoidMethod(obj, jmid_timeinfo, clock, total);
    }
    else if(type == CHILD_THREAD){
        JNIEnv *jniEnv;
        if(jvm->AttachCurrentThread(&jniEnv, 0) != JNI_OK){
            if(LOG_DEBUG){
                LOGE("call onCallTimeInfo wrong");
            }
            return;
        }
        jniEnv->CallVoidMethod(obj, jmid_timeinfo, clock, total);
        jvm->DetachCurrentThread();
    }
}

void HCallJava::onCallError(int type, int code, char *msg) {
    if(type == MAIN_THREAD){
        jstring jmsg = env->NewStringUTF(msg);
        env->CallVoidMethod(obj, jmid_error, code, jmsg);
        //防止内存泄露
        env->DeleteLocalRef(jmsg);
    }
    else if(type == CHILD_THREAD){
        JNIEnv *jniEnv;
        if(jvm->AttachCurrentThread(&jniEnv, 0) != JNI_OK){
            if(LOG_DEBUG){
                LOGE("call onCallError wrong");
            }
            return;
        }
        jstring jmsg = jniEnv->NewStringUTF(msg);
        jniEnv->CallVoidMethod(obj, jmid_error, code, jmsg);
        jniEnv->DeleteLocalRef(jmsg);
        jvm->DetachCurrentThread();
    }
}

void HCallJava::onCallComplete(int type) {
    if(type == MAIN_THREAD){
        env->CallVoidMethod(obj, jmid_complete);
    }
    else if(type == CHILD_THREAD){
        JNIEnv *jniEnv;
        if(jvm->AttachCurrentThread(&jniEnv, 0) != JNI_OK){
            if(LOG_DEBUG){
                LOGE("call onCallComplete wrong");
            }
            return;
        }
        jniEnv->CallVoidMethod(obj, jmid_complete);
        //该方法不可以在主线程中调用
        jvm->DetachCurrentThread();
    }
}

void HCallJava::onCallPcmToAac(int type, int size, void *buffer) {
    if(type == MAIN_THREAD){
        jbyteArray array = env->NewByteArray(size);
        env->SetByteArrayRegion(array, 0, size, static_cast<const jbyte *>(buffer));
        env->CallVoidMethod(obj, jmid_pcm2aac,size,array);
        env->DeleteLocalRef(array);
    }
    else if(type == CHILD_THREAD){
        JNIEnv *jniEnv;
        if(jvm->AttachCurrentThread(&jniEnv, 0) != JNI_OK){
            if(LOG_DEBUG){
                LOGE("call onCallPcmToAcc wrong");
            }
            return;
        }
        jbyteArray array = jniEnv->NewByteArray(size);
        jniEnv->SetByteArrayRegion(array, 0, size, static_cast<const jbyte *>(buffer));
        jniEnv->CallVoidMethod(obj, jmid_pcm2aac,size,array);
        jniEnv->DeleteLocalRef(array);
        jvm->DetachCurrentThread();
    }
}

void HCallJava::onCallPcmInfo(void *buffer, int size) {
    JNIEnv *jniEnv;
    if(jvm->AttachCurrentThread(&jniEnv,0)!=JNI_OK){
        if(LOG_DEBUG)LOGE("call onCallPcmInfo wrong");
        return;
    }
    jbyteArray array = jniEnv->NewByteArray(size);
    jniEnv->SetByteArrayRegion(array, 0, size, static_cast<const jbyte *>(buffer));
    jniEnv->CallVoidMethod(obj,jmid_pcminfo,array,size);
    jniEnv->DeleteLocalRef(array);
    jvm->DetachCurrentThread();
}

void HCallJava::onCallPcmRate(int sampleRate) {
    JNIEnv *jniEnv;
    if(jvm->AttachCurrentThread(&jniEnv, 0) != JNI_OK){
        if(LOG_DEBUG)LOGE("call onCallPcmRate worng");
        return;
    }
    jniEnv->CallVoidMethod(obj, jmid_pcmrate, sampleRate);
    jvm->DetachCurrentThread();
}
