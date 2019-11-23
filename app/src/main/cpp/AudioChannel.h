//
// Created by 1 on 2019/11/23.
//

#ifndef FFMPLAYER_AUDIOCHANNEL_H
#define FFMPLAYER_AUDIOCHANNEL_H


#include "BaseChannel.h"

class AudioChannel : public BaseChannel {
public:
    AudioChannel(AVCodecContext *codecContext, int stream_index);

    ~AudioChannel();

    void start();

    void stop();
};


#endif //FFMPLAYER_AUDIOCHANNEL_H
