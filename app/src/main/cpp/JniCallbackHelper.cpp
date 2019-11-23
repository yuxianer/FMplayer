#include "JniCallbackHelper.h"

JniCallbackHelper::JniCallbackHelper(JavaVM *javaVM, JNIEnv *env, jobject instance_) {
    this->javaVM = javaVM;
    this->env = env;

    this->instance = env->NewGlobalRef(instance_);
    jclass clazz = env->GetObjectClass(this->instance);
    jmd_prepared = env->GetMethodID(clazz, "onPrepared", "()V");
    jmd_onError = env->GetMethodID(clazz, "onError", "(I)V");
    jmd_onProgress = env->GetMethodID(clazz, "onProgress", "(I)V");
}

JniCallbackHelper::~JniCallbackHelper() {
    javaVM = 0;
    env->DeleteGlobalRef(instance);
    instance = 0;
    env = 0;
}

/**
 * 子线程！
 */
void JniCallbackHelper::onPrepared(int thread_mode) {
    if (thread_mode == THREAD_MAIN){
        env->CallVoidMethod(instance, jmd_prepared);
    }else{
        JNIEnv *env_child;
        javaVM->AttachCurrentThread(&env_child, 0);
        env_child->CallVoidMethod(instance, jmd_prepared);
        javaVM->DetachCurrentThread();
    }
}

void JniCallbackHelper::onError(int thread_mode, int error_code) {
    if (thread_mode == THREAD_MAIN) {
        env->CallVoidMethod(instance, jmd_onError, error_code);
    } else {
        JNIEnv *env_child;
        javaVM->AttachCurrentThread(&env_child, 0);
        env_child->CallVoidMethod(instance, jmd_onError, error_code);
        javaVM->DetachCurrentThread();
    }
}

void JniCallbackHelper::onProgress(int thread_mode, int progress) {
    if (thread_mode == THREAD_MAIN) {
        env->CallVoidMethod(instance, jmd_onProgress, progress);
    } else {
        JNIEnv *env_child;
        javaVM->AttachCurrentThread(&env_child, 0);
        env_child->CallVoidMethod(instance, jmd_onProgress, progress);
        javaVM->DetachCurrentThread();
    }
}


