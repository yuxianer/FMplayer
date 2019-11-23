#include <jni.h>
#include <string>
#include "FFmplayer.h"
#include "JniCallbackHelper.h"

JavaVM *javaVM = 0;

jint JNI_OnLoad(JavaVM *vm, void *args){
    javaVM = vm;
    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_jzy_ffmplayer_FFmplayer__1prepare(JNIEnv *env, jobject instance, jstring _url) {
    const char *url = env->GetStringUTFChars(_url, 0);
    JniCallbackHelper *jni_callback_helper = new JniCallbackHelper(javaVM, env, instance);
    FFmplayer *player = new FFmplayer(jni_callback_helper,url);
    player->prepare();
    env->ReleaseStringUTFChars(_url, url);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_jzy_ffmplayer_FFmplayer__1start(JNIEnv *env, jobject instance) {

}
extern "C"
JNIEXPORT void JNICALL
Java_com_jzy_ffmplayer_FFmplayer__1stop(JNIEnv *env, jobject instance) {

}
extern "C"
JNIEXPORT void JNICALL
Java_com_jzy_ffmplayer_FFmplayer__1release(JNIEnv *env, jobject instance) {

}