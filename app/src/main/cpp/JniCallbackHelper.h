//
// Created by 1 on 2019/11/23.
//

#ifndef FFMPLAYER_JNICALLBACKHELPER_H
#define FFMPLAYER_JNICALLBACKHELPER_H


#include <jni.h>
#include "macro.h"

class JniCallbackHelper {
public:
    JniCallbackHelper(JavaVM *javaVM, JNIEnv *env, jobject instance);

    ~JniCallbackHelper();

    void onPrepared(int thread_mode);
    void onError(int thread_mode, int error_code);
    void onProgress(int thread_mode, int progress);

private:
    JavaVM *javaVM = 0;
    JNIEnv *env = 0;
    jobject instance;
    jmethodID jmd_prepared;
    jmethodID jmd_onError;
    jmethodID jmd_onProgress;
};


#endif //FFMPLAYER_JNICALLBACKHELPER_H
