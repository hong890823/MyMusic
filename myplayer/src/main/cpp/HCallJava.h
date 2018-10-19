//
// Created by Hong on 2018/10/15.
//

#ifndef MYMUSIC_HCALLJAVA_H
#define MYMUSIC_HCALLJAVA_H


#include <jni.h>

#define MAIN_THREAD 0
#define CHILD_THREAD 1

class HCallJava {
public:
    JavaVM *jvm = NULL;
    JNIEnv *env = NULL;
    jobject obj;

    jmethodID jmid_prepared;
    jmethodID jmid_load;
    jmethodID jmid_timeinfo;
    jmethodID jmid_error;
    jmethodID jmid_complete;
public:
    HCallJava(JavaVM *vm,JNIEnv *env,jobject obj);
    ~HCallJava();

    void onCallPrepared(int type);
    void onCallLoad(int type,bool load);
    void onCallTimeInfo(int type,int clock,int duration);
    void onCallError(int type,int code,char *msg);
    void onCallComplete(int type);
};


#endif //MYMUSIC_HCALLJAVA_H
