#include <jni.h>
#include <string>
#include "FFmplayer.h"
#include "JniCallbackHelper.h"
#include <android/native_window_jni.h>

JavaVM *javaVM = 0;
ANativeWindow *window = 0;
FFmplayer *player;
JniCallbackHelper *jni_callback_helper;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

jint JNI_OnLoad(JavaVM *vm, void *args) {
    javaVM = vm;
    return JNI_VERSION_1_6;
}

//渲染图像
void WindowRender(uint8_t *src_data, int width, int height, int src_lineSize) {
    pthread_mutex_lock(&mutex);
    if (!window) {
        pthread_mutex_unlock(&mutex);

    }
    ANativeWindow_setBuffersGeometry(window, (uint32_t) width, (uint32_t) height,
                                     WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer window_buffer;
    if (ANativeWindow_lock(window, &window_buffer, 0)) {
        ANativeWindow_release(window);
        window = 0;
        pthread_mutex_unlock(&mutex);
        return;
    }
    //填充window_buff
    uint8_t *dst_data = static_cast<uint8_t *>(window_buffer.bits);
    //rgba四个分量  步长*4
    int dst_linesize = window_buffer.stride * 4;
    for (int i = 0; i < window_buffer.height; ++i) {
        //拷贝一行
        mempcpy(dst_data + i * dst_linesize, src_data + src_lineSize * i, dst_linesize);
    }
    ANativeWindow_unlockAndPost(window);
    pthread_mutex_unlock(&mutex);
};

extern "C"
JNIEXPORT void JNICALL
Java_com_jzy_ffmplayer_FFmplayer__1prepare(JNIEnv *env, jobject instance, jstring _url) {
    const char *url = env->GetStringUTFChars(_url, 0);
    jni_callback_helper = new JniCallbackHelper(javaVM, env, instance);
    player = new FFmplayer(jni_callback_helper, url);
    player->prepare();
    player->setRenderCallback(WindowRender);
    env->ReleaseStringUTFChars(_url, url);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_jzy_ffmplayer_FFmplayer__1start(JNIEnv *env, jobject instance) {
    if (player) {
        player->start();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_jzy_ffmplayer_FFmplayer__1stop(JNIEnv *env, jobject instance) {
    if (player) {
        player->stop();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_jzy_ffmplayer_FFmplayer__1release(JNIEnv *env, jobject instance) {
    pthread_mutex_lock(&mutex);
    if (window) {
        ANativeWindow_release(window);
        window = 0;
    }

    pthread_mutex_unlock(&mutex);
    DELETE(player);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_jzy_ffmplayer_FFmplayer_setSurfaceNative(JNIEnv *env, jobject instance, jobject surface) {
    pthread_mutex_lock(&mutex);
    //如果之前初始化过窗口对象，需要释放 并重新创建
    if (window) {
        ANativeWindow_release(window);
        window = 0;
    }
    window = ANativeWindow_fromSurface(env, surface);
    pthread_mutex_unlock(&mutex);

}
extern "C"
JNIEXPORT jint JNICALL
Java_com_jzy_ffmplayer_FFmplayer_getDurationNative(JNIEnv *env, jobject instance) {
    if (player) {
        return player->getDuration();
    }
    return 0;
}
extern "C"
JNIEXPORT void JNICALL
Java_com_jzy_ffmplayer_FFmplayer_seekNative(JNIEnv *env, jobject instance, jint progress) {
    if (player) {
        player->seek(progress);
    }
}