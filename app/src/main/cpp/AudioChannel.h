//
// Created by 1 on 2019/11/23.
//

#ifndef FFMPLAYER_AUDIOCHANNEL_H
#define FFMPLAYER_AUDIOCHANNEL_H


#include "BaseChannel.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

extern "C" {
#include "libswresample/swresample.h"
}

class AudioChannel : public BaseChannel {
public:
    AudioChannel(AVCodecContext *codecContext, int stream_index);

    ~AudioChannel();

    void start();

    void _decode();

    void _play();

    void stop();

    int getPCM();

    uint8_t *out_buffers = 0;

    int out_sample_rate ;
    int out_channels ;
    int out_sample_size ;
    int out_buffers_size ;
private:
    pthread_t pid_audio_decode;
    pthread_t pid_audio_play;
    //引擎
    SLObjectItf engineObject;
    //引擎接口
    SLEngineItf engineInterface;
    //混音器
    SLObjectItf outputMixObject;
    //播放器
    SLObjectItf bqPlayerObject;
    //播放器接口
    SLPlayItf bqPlayerPlay;
    //播放器队列接口
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
    SwrContext *swr_ctx = 0;
};


#endif //FFMPLAYER_AUDIOCHANNEL_H
