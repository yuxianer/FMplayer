//
// Created by 1 on 2019/11/23.
//


#include "FFmplayer.h"

FFmplayer::FFmplayer(JniCallbackHelper *p_helper, const char *_url) {
    this->jni_callback_helper = p_helper;
    this->url = new char[strlen(_url) + 1];
    strcpy(this->url, _url);

    pthread_mutex_init(&seek_mutex, 0);
}

FFmplayer::~FFmplayer() {
    DELETE(url);
    DELETE(jni_callback_helper);
    pthread_mutex_destroy(&seek_mutex);
}

//准备线程执行函数
void *thread_prepare_task(void *args) {
    FFmplayer *player = (FFmplayer *) (args);
    player->_prepare();
    return 0;
}

//开始线程执行函数
void *thread_start_task(void *args) {
    FFmplayer *player = (FFmplayer *) (args);
    player->_start();
    return 0;
}

//停止线程执行函数
void *thread_stop_task(void *args) {
    FFmplayer *player = (FFmplayer *) (args);
    player->isPlaying = 0;
    //等待start执行完毕
    pthread_join(player->pid_task_prepare, 0);
    pthread_join(player->pid_task_start, 0);
    if (player->formatContext) {
        avformat_close_input(&(player->formatContext));
        avformat_free_context(player->formatContext);
        player->formatContext = 0;
    }
    DELETE(player->audio_channel);
    DELETE(player->video_channel);
    DELETE(player);
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
        if (audio_channel)video_channel->setAudioChannel(audio_channel);
        video_channel->start();
    }
    if (audio_channel) {
        LOGE("音频开启");
        audio_channel->start();
    }
    pthread_create(&pid_task_start, 0, thread_start_task, this);
}

void FFmplayer::stop() {
    jni_callback_helper = 0;
    if (audio_channel) {
        audio_channel->jni_callback_helper = 0;
    }
    if (video_channel) {
        video_channel->jni_callback_helper = 0;
    }

    pthread_create(&pid_task_stop, 0, thread_stop_task, this);
}

void FFmplayer::release() {

}

/**
 * 解封装函数
 */
void FFmplayer::_prepare() {
    /**
     * 打开媒体地址（文件路径/直播地址）
     */
    //封装了媒体流的格式信息
    formatContext = avformat_alloc_context();
    AVDictionary *dictionary = 0;
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
     * 查找媒体中的音视频流的信息
     */
    ret = avformat_find_stream_info(formatContext, 0);

    if (ret < 0) {
        //        jni_callback_helper->onError(THREAD_CHILD,);
        return;
    }
    duration = formatContext->duration / AV_TIME_BASE;
    /**
     * 根据流信息中流的个数来循环查找
     */
    for (int stream_index = 0; stream_index < formatContext->nb_streams; ++stream_index) {
        /**
         * 获取媒体流（视频/音频）
         */
        AVStream *stream = formatContext->streams[stream_index];
        /**
         * 从流中获取解码这段流的参数
         */
        AVCodecParameters *codecParameters = stream->codecpar;
        /**
         * 通过流的编解码参数中编解码id，来获取当前流的解码器
         */
        AVCodec *codec = avcodec_find_decoder(codecParameters->codec_id);
        if (!codec) {
            //        jni_callback_helper->onError(THREAD_CHILD,);
            return;
        }
        /**
         * 创建解码器的上下文
         */
        AVCodecContext *codecContext = avcodec_alloc_context3(codec);
        if (!codecContext) {
            //        jni_callback_helper->onError(THREAD_CHILD,);
            return;
        }
        /**
         * 设置上下文的参数
         */
        ret = avcodec_parameters_to_context(codecContext, codecParameters);
        if (ret < 0) {
            //        jni_callback_helper->onError(THREAD_CHILD,);
            return;
        }
        /**
         * 打开解码器
         */
        ret = avcodec_open2(codecContext, codec, 0);
        if (ret) {
            //        jni_callback_helper->onError(THREAD_CHILD,);
            return;
        }
        //获得时间基
        AVRational time_base = stream->time_base;
        /**
         * 从编码器参数中获取流类型 codec_type
         */
        if (codecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            //音频
            audio_channel = new AudioChannel(codecContext, stream_index, time_base);
            if (0 != duration) {
                audio_channel->setJniCallbackHelper(jni_callback_helper);
            }
        } else if (codecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (stream->disposition & AV_DISPOSITION_ATTACHED_PIC) {
                //过滤封面
                continue;
            }
            AVRational frame_rate = stream->avg_frame_rate;
            int fps = av_q2d(frame_rate);
            video_channel = new VideoChannnel(codecContext, stream_index, time_base, fps);
            //设置渲染回调
            video_channel->setRenderCallback(renderCallback);
            if (0 != duration) {
                video_channel->setJniCallbackHelper(jni_callback_helper);
            }
        }
    }//end for
    if (!audio_channel && !video_channel) {
//        jni_callback_helper->onError(THREAD_CHILD,);
        return;
    }
    /**
     * 通知java可以开始播放
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
        if (video_channel && video_channel->packets.size() > 100) {
            //等待 packet使用
            av_usleep(10 * 1000);
            continue;
        }
        if (audio_channel && audio_channel->packets.size() > 100) {
            //等待 packet使用
            av_usleep(10 * 1000);
            continue;
        }
        AVPacket *packet = av_packet_alloc();
        pthread_mutex_lock(&seek_mutex);
        int ret = av_read_frame(formatContext, packet);
        pthread_mutex_unlock(&seek_mutex);
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

int FFmplayer::getDuration() {
    return duration;
}

void FFmplayer::seek(int progress) {
    if (progress < 0 || progress > duration) {
        return;
    }
    if (!audio_channel && !video_channel) {
        return;
    }
    if (!formatContext) {
        return;
    }
    pthread_mutex_lock(&seek_mutex);
    int ret = av_seek_frame(formatContext, -1, AV_TIME_BASE * progress, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        pthread_mutex_unlock(&seek_mutex);
        return;
    }
    //reset and play
    if (audio_channel) {
        audio_channel->packets.setWork(0);
        audio_channel->frames.setWork(0);
        audio_channel->packets.clear();
        audio_channel->frames.clear();
        audio_channel->packets.setWork(1);
        audio_channel->frames.setWork(1);
    }
    if (video_channel) {
        video_channel->packets.setWork(0);
        video_channel->frames.setWork(0);
        video_channel->packets.clear();
        video_channel->frames.clear();
        video_channel->packets.setWork(1);
        video_channel->frames.setWork(1);
    }

    pthread_mutex_unlock(&seek_mutex);
}
