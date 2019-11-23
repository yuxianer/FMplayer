//
// Created by 1 on 2019/11/23.
//

#ifndef FFMPLAYER_BASECHANNEL_H
#define FFMPLAYER_BASECHANNEL_H

#include "safe_queue.h"
#include <android/log.h>

#include "macro.h"
#include "JniCallbackHelper.h"

extern "C" {
#include <libavcodec/avcodec.h>
};

class BaseChannel {
public:
    BaseChannel(AVCodecContext *codecContext, int stream_index, AVRational time_base)
            : stream_index(stream_index),
              codecContext(codecContext),
              time_base(time_base) {
        packets.setReleaseCallback(releaseAVPacket);
        frames.setReleaseCallback(releaseAVFrame);
    }

    virtual ~BaseChannel() {
        packets.clear();
        frames.clear();
    }

    static void releaseAVPacket(AVPacket **packet) {
        if (packet) {
            av_packet_free(packet);
            *packet = 0;
        }
    }

    static void releaseAVFrame(AVFrame **frame) {
        if (frame) {
            av_frame_free(frame);
            *frame = 0;
        }
    }

    void setJniCallbackHelper(JniCallbackHelper *jni_callback_helper) {
        this->jni_callback_helper = jni_callback_helper;
    };


    int stream_index;
    SafeQueue<AVPacket *> packets;
    SafeQueue<AVFrame *> frames;
    bool isPlaying = 0;
    AVCodecContext *codecContext = 0;
    AVRational time_base;
    double audio_time;
    JniCallbackHelper *jni_callback_helper = 0;
};

#endif //FFMPLAYER_BASECHANNEL_H
