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
}
class FFmplayer {
public:
    FFmplayer(JniCallbackHelper *p_helper,const char *_url);

    ~FFmplayer();
    void prepare();
    void _prepare();
    void start();
    void stop();
    void release();
private:
    char *url = 0;
    pthread_t task_prepare ;
    pthread_t task_start ;
    JniCallbackHelper *jni_callback_helper=0;
    AudioChannel *audio_channel =0;
    VideoChannnel *video_channel =0;
};


#endif //FFMPLAYER_FFMPLAYER_H
