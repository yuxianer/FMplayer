//
// Created by 1 on 2019/11/23.
//

#ifndef FFMPLAYER_BASECHANNEL_H
#define FFMPLAYER_BASECHANNEL_H

#include "safe_queue.h"

extern "C" {
#include <libavcodec/avcodec.h>
};

class BaseChannel {
public:
    BaseChannel(AVCodecContext *codecContext, int stream_index) : stream_index(stream_index),
                                                                  codecContext(codecContext) {
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

    int stream_index;
    SafeQueue<AVPacket *> packets;
    SafeQueue<AVFrame *> frames;
    bool isPlaying = 0;
    AVCodecContext *codecContext = 0;
};

#endif //FFMPLAYER_BASECHANNEL_H
