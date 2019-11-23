//
// Created by 1 on 2019/11/23.
//

#ifndef FFMPLAYER_FFMPLAYER_H
#define FFMPLAYER_FFMPLAYER_H

#include <cstring>
#include <pthread.h>
#include "AudioChannel.h"
#include "VideoChannnel.h"
#include "JniCallbackHelper.h"
#include "macro.h"
#include "android/log.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/time.h>
}

class FFmplayer {
    friend void *thread_stop_task(void *args);
public:
    FFmplayer(JniCallbackHelper *p_helper, const char *_url);

    ~FFmplayer();

    void prepare();

    void _prepare();

    void start();

    void _start();

    void stop();


    void release();

    void setRenderCallback(RenderCallback renderCallback);

    int getDuration();

    void seek(int progress);


private:
    char *url = 0;
    pthread_t pid_task_prepare;
    pthread_t pid_task_start;
    pthread_t pid_task_stop;
    JniCallbackHelper *jni_callback_helper;
    AudioChannel *audio_channel = 0;
    VideoChannnel *video_channel = 0;
    bool isPlaying = 0;
    AVFormatContext *formatContext = 0;
    RenderCallback renderCallback;
    int duration = 0;
    pthread_mutex_t seek_mutex;
};


#endif //FFMPLAYER_FFMPLAYER_H
