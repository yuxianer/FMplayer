//
// Created by 1 on 2019/11/23.
//


#include "FFmplayer.h"

FFmplayer::FFmplayer(JniCallbackHelper *p_helper, const char *_url) {
    this->jni_callback_helper = p_helper;
    this->url = new char[strlen(_url) + 1];
    strcpy(this->url, _url);
}

FFmplayer::~FFmplayer() {
    delete this->url;
    url = 0;
}

//准备线程执行函数
void *(thread_prepare_task)(void *args) {
    FFmplayer *player = (FFmplayer *) (args);
    player->_prepare();
    return 0;
}

//开始线程执行函数
void *(thread_start_task)(void *args) {
    FFmplayer *player = (FFmplayer *) (args);
    player->_start();
    return 0;
}

//进行解封装
//耗时操作 需要子线程
void FFmplayer::prepare() {
    pthread_create(&pid_task_prepare, 0, thread_prepare_task, this);
}

void FFmplayer::start() {
    isPlaying = 1;
    if (video_channel) {
        video_channel->start();
    }
    if (audio_channel) {
        LOGE("音频开启");
        audio_channel->start();
    }
    pthread_create(&pid_task_start, 0, thread_start_task, this);
}

void FFmplayer::stop() {

}

void FFmplayer::release() {

}

/**
 * 解封装函数
 */
void FFmplayer::_prepare() {
    /**
     * 1，打开媒体地址（文件路径/直播地址）
     */
    //封装了媒体流的格式信息
    formatContext = avformat_alloc_context();
    //字典（键值对）
    AVDictionary *dictionary = 0;
    //设置超时（5秒）
    av_dict_set(&dictionary, "timeout", "5000000", 0);//单位微秒
    /**
     * 1， AVFormatContext
     * 2，url:文件路径或直播地址
     * 3，输入的封装格式，
     * 4，参数
     */
    int ret = avformat_open_input(&formatContext, url, 0, &dictionary);
    //释放字典
    av_dict_free(&dictionary);
    if (ret) {
        //        jni_callback_helper->onError(THREAD_CHILD,);
        return;
    }
    /**
     * 2，查找媒体中的音视频流的信息
     */
    ret = avformat_find_stream_info(formatContext, 0);

    if (ret < 0) {
        //        jni_callback_helper->onError(THREAD_CHILD,);
        return;
    }
    int64_t duration = formatContext->duration / AV_TIME_BASE;//获取的 duration 单位是：秒

    /**
     * 3，根据流信息中流的个数来循环查找
     */
    for (int stream_index = 0; stream_index < formatContext->nb_streams; ++stream_index) {
        /**
         * 4, 获取媒体流（视频/音频）
         */
        AVStream *stream = formatContext->streams[stream_index];
        /**
         * 5，从流中获取解码这段流的参数
         */
        AVCodecParameters *codecParameters = stream->codecpar;

        /**
         * 6, 通过流的编解码参数中编解码id，来获取当前流的解码器
         */
        AVCodec *codec = avcodec_find_decoder(codecParameters->codec_id);
        if (!codec) {
            //        jni_callback_helper->onError(THREAD_CHILD,);
            return;
        }
        /**
         * 7, 创建解码器的上下文
         */
        AVCodecContext *codecContext = avcodec_alloc_context3(codec);
        if (!codecContext) {
            //        jni_callback_helper->onError(THREAD_CHILD,);
            return;
        }
        /**
         * 8，设置上下文的参数
         */
        ret = avcodec_parameters_to_context(codecContext, codecParameters);
        if (ret < 0) {
            //        jni_callback_helper->onError(THREAD_CHILD,);
            return;
        }
        /**
         * 9, 打开解码器
         */
        ret = avcodec_open2(codecContext, codec, 0);
        if (ret) {
            //        jni_callback_helper->onError(THREAD_CHILD,);
            return;
        }
        AVRational time_base = stream->time_base;
        /**
         * 10, 从编码器参数中获取流类型 codec_type
         */
        if (codecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            //音频
            audio_channel = new AudioChannel(codecContext, stream_index);
        } else if (codecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_channel = new VideoChannnel(codecContext, stream_index);
            //设置渲染回调
            video_channel->setRenderCallback(renderCallback);
        }
    }//end for
    if (!audio_channel && !video_channel) {
//        jni_callback_helper->onError(THREAD_CHILD,);
        return;
    }
    /**
     * 12， 准备完了，通知java可以开始播放
     */
    if (jni_callback_helper) {
        jni_callback_helper->onPrepared(THREAD_CHILD);
    }
}

/**
 * 解码函数
 */
void FFmplayer::_start() {
    while (isPlaying) {
        AVPacket *packet = av_packet_alloc();
        int ret = av_read_frame(formatContext, packet);
        if (!ret) {
            if (video_channel && video_channel->stream_index == packet->stream_index) {
                //视频帧
                video_channel->packets.push(packet);
            } else if (audio_channel && audio_channel->stream_index == packet->stream_index) {
                //音频帧
                audio_channel->packets.push(packet);
            }
        } else if (ret == EOF) {
            //读取完毕
            //可以设置停止或者重新播放
        } else {
            //获取帧错误。
            break;
        }

    }//end while
    isPlaying = 0;
    video_channel->stop();
    audio_channel->stop();
}

void FFmplayer::setRenderCallback(RenderCallback renderCallback) {
    this->renderCallback = renderCallback;
}
