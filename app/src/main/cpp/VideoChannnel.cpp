//
// Created by 1 on 2019/11/23.
//

#include "VideoChannnel.h"

VideoChannnel::VideoChannnel(AVCodecContext *codecContext, int stream_index) : BaseChannel(
        codecContext, stream_index) {

}

VideoChannnel::~VideoChannnel() {

}

//解码线程函数
void *(task_video_decode)(void *args) {
    VideoChannnel *video_channel = (VideoChannnel *) (args);
    video_channel->video_decode();
    return 0;
}

//播放线程函数
void *(task_video_play)(void *args) {
    VideoChannnel *video_channel = (VideoChannnel *) (args);
    video_channel->video_play();
    return 0;
}

void VideoChannnel::start() {
    isPlaying = 1;
    packets.setWork(1);
    frames.setWork(1);
    pthread_create(&pid_video_decode, 0, task_video_decode, this);
    pthread_create(&pid_video_play, 0, task_video_play, this);
}

void VideoChannnel::stop() {

}

//解码操作
void VideoChannnel::video_decode() {
    AVPacket *packet;
    while (isPlaying) {
        int ret = packets.pop(packet);
        if (!isPlaying)break;
        if (!ret) {
            continue;
        }
        //开始解码
        ret = avcodec_send_packet(codecContext, packet);
        if (ret != 0) {
            break;
        }
        AVFrame *frame = av_frame_alloc();
        ret = avcodec_receive_frame(codecContext, frame);
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret != 0) {
            break;
        }
        frames.push(frame);
    }
    releaseAVPacket(&packet);
}

//播放操作
void VideoChannnel::video_play() {
    uint8_t *dst_data[4];
    int dst_linesize[4];
    AVFrame *frame = 0;

    //yuv->rgba
    SwsContext *sws_ctx = sws_getContext(codecContext->width, codecContext->height,
                                         codecContext->pix_fmt,
                                         codecContext->width, codecContext->height, AV_PIX_FMT_RGBA,
                                         SWS_BILINEAR, NULL, NULL, NULL);

    //为dst_data申请内存
    av_image_alloc(dst_data, dst_linesize,
                   codecContext->width, codecContext->height, AV_PIX_FMT_RGBA, 1);

    while (isPlaying) {
        int ret = frames.pop(frame);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }
        //格式转换 yuv -> rgba
        sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height,
                  dst_data, dst_linesize);
        //通知AVNativeWindow进行渲染
        renderCallback(dst_data[0], codecContext->width, codecContext->height, dst_linesize[0]);
        releaseAVFrame(&frame);
    }
    releaseAVFrame(&frame);
    isPlaying = 0;
    av_freep(&dst_data[0]);
    sws_freeContext(sws_ctx);
}

void VideoChannnel::setRenderCallback(RenderCallback renderCallback) {
    this->renderCallback = renderCallback;
}

