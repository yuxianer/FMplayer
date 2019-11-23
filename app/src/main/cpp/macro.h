//
// Created by 1 on 2019/11/23.
//

#ifndef FFMPLAYER_MACRO_H
#define FFMPLAYER_MACRO_H
//线程模式
#define THREAD_MAIN 1
#define THREAD_CHILD 2
//日志宏
#define LOG_TAG "logtest"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGT(...) __android_log_print(ANDROID_LOG_INFO,"alert",__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#endif //FFMPLAYER_MACRO_H
