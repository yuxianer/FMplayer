//
// Created by 1 on 2019/11/23.
//

#ifndef FFMPLAYER_VIDEOCHANNNEL_H
#define FFMPLAYER_VIDEOCHANNNEL_H


#include "BaseChannel.h"
#include "AudioChannel.h"

extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
};

typedef void (*RenderCallback)(uint8_t *, int, int, int);

class VideoChannnel : public BaseChannel {
public:
    VideoChannnel(AVCodecContext *codecContext, int stream_index, AVRational time_base, int fps);

    ~VideoChannnel();

    void start();

    void stop();

    void video_decode();

    void video_play();

    void setRenderCallback(RenderCallback renderCallback);

    void setAudioChannel(AudioChannel *audio_channel);

private:
    pthread_t pid_video_decode;
    pthread_t pid_video_play;
    RenderCallback renderCallback;
    int fps;
    AudioChannel *audio_channel = 0;
};


#endif //FFMPLAYER_VIDEOCHANNNEL_H
