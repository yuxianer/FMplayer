#include "VideoChannnel.h"

/**
 * 丢packet帧
 * @param q
 */
void dropAVPacket(queue<AVPacket *> &q) {
    while (!q.empty()) {
        AVPacket *packet = q.front();
        if (packet->flags != AV_PKT_FLAG_KEY) {
            BaseChannel::releaseAVPacket(&packet);
            q.pop();

        } else {
            break;
        }
    }
}/**
 * 丢frame帧
 * @param q
 */
void dropAVFrame(queue<AVFrame *> &q) {
    if (!q.empty()) {
        AVFrame *frame = q.front();
        BaseChannel::releaseAVFrame(&frame);
        q.pop();
    }
}

VideoChannnel::VideoChannnel(AVCodecContext *codecContext, int stream_index, AVRational time_base,
                             int fps) : BaseChannel(
        codecContext, stream_index, time_base) {
    this->fps = fps;
    //设置同步操作
    packets.setSyncCallback(dropAVPacket);
    frames.setSyncCallback(dropAVFrame);
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
    isPlaying = 0;
    packets.setWork(0);
    frames.setWork(0);
    pthread_join(pid_video_decode,0);
    pthread_join(pid_video_play,0);

}

//解码操作
void VideoChannnel::video_decode() {
    AVPacket *packet;
    while (isPlaying) {
        if (isPlaying && frames.size() > 100) {
            //等待消费
            av_usleep(10 * 1000);
            continue;
        }
        int ret = packets.pop(packet);
        if (!isPlaying)break;
        if (!ret) continue;
        ret = avcodec_send_packet(codecContext, packet);
        if (ret != 0)break;
        releaseAVPacket(&packet);
        AVFrame *frame = av_frame_alloc();
        ret = avcodec_receive_frame(codecContext, frame);
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret != 0) {
            releaseAVFrame(&frame);
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

        //进行同步处理
//        frame->best_effort_timestamp*av_q2d(time_base);
        //获取delaytime(微秒)
        double delay_time = frame->repeat_pict / (2 * fps);//帧包含的延时时间
        double base_delay = 1.0 / fps;//播放每一帧需要的时间
        double real_delay = base_delay + delay_time;
        double video_time = frame->best_effort_timestamp * av_q2d(time_base);
        if (!audio_channel) {
            av_usleep(real_delay * 1000000);
            if (jni_callback_helper) {
                jni_callback_helper->onProgress(THREAD_CHILD, video_time);
            }
        } else {
            double audio_time = audio_channel->audio_time;
            //判断差值 同步播放
            double time_diff = video_time - audio_time;
            if (time_diff > 0) {
                //视频快 等待音频
                if (time_diff > 1) {
                    av_usleep(real_delay * 2 * 1000000);
                } else {
                    av_usleep((real_delay + time_diff) * 1000000);
                }
            } else if (time_diff < 0) {
                //视频慢 追音频 可以丢非关键帧
                if (fabs(time_diff) < time_diff) {
                    //时间差小 同步丢包
                    frames.sync();
                    continue;
                }
            }
            //通知AVNativeWindow进行渲染
            renderCallback(dst_data[0], codecContext->width, codecContext->height, dst_linesize[0]);
            releaseAVFrame(&frame);
        }
    }
    releaseAVFrame(&frame);
    isPlaying = 0;
    av_freep(&dst_data[0]);
    sws_freeContext(sws_ctx);
}

void VideoChannnel::setRenderCallback(RenderCallback renderCallback) {
    this->renderCallback = renderCallback;
}

void VideoChannnel::setAudioChannel(AudioChannel *audio_channel_) {
    this->audio_channel = audio_channel_;
}

