//
// Created by 1 on 2019/11/23.
//

#include "AudioChannel.h"

AudioChannel::AudioChannel(AVCodecContext *context, int stream_index, AVRational time_base)
        : BaseChannel(context,
                      stream_index, time_base) {
    //初始化 缓冲 out_buffers
    //44100 16bit 双声道
    out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    out_sample_size = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    out_sample_rate = 44100;
    out_buffers_size = out_sample_rate * out_sample_size * out_channels;
    out_buffers = static_cast<uint8_t *>(malloc(out_buffers_size));
    memset(out_buffers, 0, out_buffers_size);

    swr_ctx = swr_alloc_set_opts(0, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, out_sample_rate,
                                 codecContext->channel_layout, codecContext->sample_fmt,
                                 codecContext->sample_rate, 0, 0);
    swr_init(swr_ctx);
}

AudioChannel::~AudioChannel() {
    if (swr_ctx) {
        swr_free(&swr_ctx);
        swr_ctx = 0;
    }
    DELETE(out_buffers);
}

void *task_audio_decode(void *args) {
    AudioChannel *audio_channel = (AudioChannel *) (args);
    audio_channel->_decode();
    return 0;
};

void *task_audio_play(void *args) {
    AudioChannel *audio_channel = (AudioChannel *) (args);
    audio_channel->_play();
    return 0;
};

//opengl播放音频回调函数
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    AudioChannel *audio_channel = (AudioChannel *) (context);
    int pcm_size = audio_channel->getPCM();
    //缓冲数组
    if (pcm_size > 0)
        (*bq)->Enqueue(bq, audio_channel->out_buffers, pcm_size);
}

/**
 * 获取pcm长度
 * @return
 */
int AudioChannel::getPCM() {
    int pcm_data_size = 0;
    AVFrame *frame = 0;
    while (isPlaying) {
        int ret = frames.pop(frame);
        if (!isPlaying)break;
        if (!ret)continue;
        //音频重采样
        int dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, frame->sample_rate) +
                                            frame->nb_samples, out_sample_rate, frame->sample_rate,
                                            AV_ROUND_UP);
        int samples_per_channel = swr_convert(swr_ctx, &out_buffers, dst_nb_samples,
                                              (const uint8_t **) frame->data,
                                              frame->nb_samples);
//        pcm_data_size = samples_per_channel*out_sample_size;获取每个声道的数据
        pcm_data_size = samples_per_channel * out_sample_size * out_channels;
        //获取音频当前时间
        audio_time = frame->best_effort_timestamp * av_q2d(time_base);
        //回调进度
        if (jni_callback_helper) {
            jni_callback_helper->onProgress(THREAD_CHILD, audio_time);
        }
        break;
    }//end while
    releaseAVFrame(&frame);
    return pcm_data_size;
};

void AudioChannel::start() {
    //开启解码和播放线程
    isPlaying = 1;
    packets.setWork(1);
    frames.setWork(1);

    pthread_create(&pid_audio_decode, 0, task_audio_decode, this);
    pthread_create(&pid_audio_play, 0, task_audio_play, this);
    LOGE("音频开始");
}

void AudioChannel::stop() {
    isPlaying = 0;
    packets.setWork(0);
    frames.setWork(0);
    pthread_join(pid_audio_decode, 0);
    pthread_join(pid_audio_play, 0);
    //release
    //change to stop
    if (bqPlayerPlay) {
        (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_STOPPED);
        bqPlayerPlay = 0;
    }
    //destroy player
    if (bqPlayerObject) {
        (*bqPlayerObject)->Destroy(bqPlayerObject);
        bqPlayerObject = 0;
    }
    //destroy mixer
    if (outputMixObject) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = 0;
    }
    //destroy engine
    if (engineObject) {
        (*engineObject)->Destroy(engineObject);
        engineObject = 0;
    }
}

void AudioChannel::_decode() {
    AVPacket *packet;
    while (isPlaying) {
        if (isPlaying && frames.size() > 100) {
            //等待消费
            av_usleep(10 * 1000);
            continue;
        }
        int ret = packets.pop(packet);
        if (!isPlaying) {
            //中途停止。
            break;
        }
        if (!ret) {
            continue;
        }
        //开始解码
        ret = avcodec_send_packet(codecContext, packet);
        if (ret != 0) {
            break;
        }
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

void AudioChannel::_play() {
    /**
  * 1、创建引擎并获取引擎接口
  */
    SLresult result;
    // 1.1 创建引擎对象：SLObjectItf engineObject
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    // 1.2 初始化引擎
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    // 1.3 获取引擎接口 SLEngineItf engineInterface
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineInterface);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }

    /**
     * 2、设置混音器
     */
    // 2.1 创建混音器：SLObjectItf outputMixObject
    result = (*engineInterface)->CreateOutputMix(engineInterface, &outputMixObject, 0,
                                                 0, 0);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    // 2.2 初始化混音器
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    //不启用混响可以不用获取混音器接口
    // 获得混音器接口
    //result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
    //                                         &outputMixEnvironmentalReverb);
    //if (SL_RESULT_SUCCESS == result) {
    //设置混响 ： 默认。
    //SL_I3DL2_ENVIRONMENT_PRESET_ROOM: 室内
    //SL_I3DL2_ENVIRONMENT_PRESET_AUDITORIUM : 礼堂 等
    //const SLEnvironmentalReverbSettings settings = SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;
    //(*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
    //       outputMixEnvironmentalReverb, &settings);
    //}

    /**
     * 3、创建播放器
     */
    //3.1 配置输入声音信息
    //创建buffer缓冲类型的队列 2个队列
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                       2};
    //pcm数据格式
    //SL_DATAFORMAT_PCM：数据格式为pcm格式
    //2：双声道
    //SL_SAMPLINGRATE_44_1：采样率为44100，应用最广的兼容性最好的（b/s）
    //SL_PCMSAMPLEFORMAT_FIXED_16：采样格式为16bit (2字节)
    //SL_PCMSAMPLEFORMAT_FIXED_16：数据大小为16bit
    //SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT：左右声道（双声道）
    //SL_BYTEORDER_LITTLEENDIAN：小端模式
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1,
                                   SL_PCMSAMPLEFORMAT_FIXED_16,
                                   SL_PCMSAMPLEFORMAT_FIXED_16,
                                   SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                                   SL_BYTEORDER_LITTLEENDIAN};

    //数据源 将上述配置信息放到这个数据源中
    SLDataSource audioSrc = {&loc_bufq, &format_pcm};

    //3.2 配置音轨（输出）
    //设置混音器
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, NULL};
    //需要的接口 操作队列的接口
    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    //3.3 创建播放器 SLObjectItf bqPlayerObject
    result = (*engineInterface)->CreateAudioPlayer(engineInterface, &bqPlayerObject, &audioSrc,
                                                   &audioSnk, 1, ids, req);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    //3.4 初始化播放器：SLObjectItf bqPlayerObject
    result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    //3.5 获取播放器接口：SLPlayItf bqPlayerPlay
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    /**
     * 4、设置播放回调函数
     */
    //4.1 获取播放器队列接口：SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE, &bqPlayerBufferQueue);

    //4.2 设置回调 void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
    (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, this);
    /**
     * 5、设置播放器状态为播放状态
     */
    (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
    /**
     * 6、手动激活回调函数
     */
    bqPlayerCallback(bqPlayerBufferQueue, this);
}


//void AudioChannel::_play() {
//    // create engine
//    SLresult result = 0;
//    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
//    if (SL_RESULT_SUCCESS != result)return;
//    // realize the engine
//    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
//    if (SL_RESULT_SUCCESS != result)return;
//    // get the engine interface, which is needed in order to create other objects
//    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineInterface);
//    if (SL_RESULT_SUCCESS != result)return;
//    //create output mix
//    result = (*engineInterface)->CreateOutputMix(engineInterface, &outputMixObject, 0,
//                                                 0, 0);
//    if (SL_RESULT_SUCCESS != result)return;
//    //init output mixer
//    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
//    if (SL_RESULT_SUCCESS != result) {
//        return;
//    }
////    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
////                                              &outputMixEnvironmentalReverb);
////    SL_I3DL2_ENVIRONMENT_PRESET_ROOM 室内
////    SL_I3DL2_ENVIRONMENT_PRESET_AUDITORIUM 礼堂
////    result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
////            outputMixEnvironmentalReverb, &Settings);
//
//    //Configure
//    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
//                                                       2};
//    //set PCM format
//    //SL_DATAFORMAT_PCM 播放格式为PCM
//    //2 双声道
//    //SL_SAMPLINGRATE_44_1 采样率为44100
//    //SL_PCMSAMPLEFORMAT_FIXED_16 采样率格式为16bit
//    //SL_PCMSAMPLEFORMAT_FIXED_16 数据大小为16bit
//    //     SL_SPEAKER_FRONT_LEFT|SL_SPEAKER_FRONT_RIGHT (前)左右声道
//    //SL_BYTEORDER_LITTLEENDIAN 小端模式
//    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1,
//                                   SL_PCMSAMPLEFORMAT_FIXED_16,
//                                   SL_PCMSAMPLEFORMAT_FIXED_16,
//                                   SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
//                                   SL_BYTEORDER_LITTLEENDIAN};
//    //make date sources effective
//    SLDataSource audioSrc = {&loc_bufq, &format_pcm};
//    //Configure mixer
//    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
//    SLDataSink audioSnk = {&loc_outmix, NULL};
//    //Configure queue interface
//    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
//    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
//
//    //create audio player
//    result = (*engineInterface)->CreateAudioPlayer(engineInterface, &bqPlayerObject, &audioSrc,
//                                                   &audioSnk, 1, ids, req);
//    if (SL_RESULT_SUCCESS != result) {
//        return;
//    }
//    //    //init audio player
//    result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
//    if (SL_RESULT_SUCCESS != result)return;
//    //get audio interface
//    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
//    if (SL_RESULT_SUCCESS != result)return;
//
//    //get queue interface
//    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE, &bqPlayerBufferQueue);
//    //set callback function
//    (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, this);
//    //change player status
//    (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
//    //excute callback
//    bqPlayerCallback(bqPlayerBufferQueue, this);
//}
//

