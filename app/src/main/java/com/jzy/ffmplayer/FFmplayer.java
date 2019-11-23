package com.jzy.ffmplayer;

import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class FFmplayer implements SurfaceHolder.Callback {
    static {
        System.loadLibrary("native-lib");
    }

    //准备过程错误码
    public static final int ERROR_CODE_FFMPEG_PREPARE = 1000;
    //播放过程错误码
    public static final int ERROR_CODE_FFMPEG_PLAY = 2000;

    //打不开视频
    public static final int FFMPEG_CAN_NOT_OPEN_URL = (ERROR_CODE_FFMPEG_PREPARE - 1);

    //找不到媒体流信息
    public static final int FFMPEG_CAN_NOT_FIND_STREAMS = (ERROR_CODE_FFMPEG_PREPARE - 2);

    //找不到解码器
    public static final int FFMPEG_FIND_DECODER_FAIL = (ERROR_CODE_FFMPEG_PREPARE - 3);

    //无法根据解码器创建上下文
    public static final int FFMPEG_ALLOC_CODEC_CONTEXT_FAIL = (ERROR_CODE_FFMPEG_PREPARE - 4);

    //根据流信息 配置上下文参数失败
    public static final int FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL = (ERROR_CODE_FFMPEG_PREPARE - 5);

    //打开解码器失败
    public static final int FFMPEG_OPEN_DECODER_FAIL = (ERROR_CODE_FFMPEG_PREPARE - 6);

    //没有音视频
    public static final int FFMPEG_NOMEDIA = (ERROR_CODE_FFMPEG_PREPARE - 7);

    //读取媒体数据包失败
    public static final int FFMPEG_READ_PACKETS_FAIL = (ERROR_CODE_FFMPEG_PLAY - 1);
    private OnListener onpreparedListener;
    private String url;

    public FFmplayer(String url, SurfaceView surfaceView) {
        this.url = url;
        surfaceView.getHolder().addCallback(this);
    }

    public void prepare() {
        _prepare(url);
    }

    public void start() {
        _start();
    }

    public void stop() {
        _stop();
    }

    public void release() {
        _release();
    }

    public void seek(int progress) {
        seekNative(progress);
    }

    public void onPrepare() {
        if (onpreparedListener != null) {
            this.onpreparedListener.onPrepared();
        }
    }

    public void onError(int errcode) {
        if (onpreparedListener != null) {
            this.onpreparedListener.onError(errcode);
        }
    }

    public void onProgress(int progress) {
        if (onpreparedListener != null) {
            this.onpreparedListener.onProgress(progress);
        }
    }

    /**
     * 给jni反射调用的
     */
    public void onPrepared() {
        if (null != this.onpreparedListener) {
            this.onpreparedListener.onPrepared();
        }
    }

    public void setListener(OnListener onpreparedListener) {
        this.onpreparedListener = onpreparedListener;
    }

    public int getDuration() {
        return getDurationNative();
    }

    //相关回调监听
    interface OnListener {
        void onPrepared();

        void onError(int errcode);

        void onProgress(int progress);
    }

    @Override
    public void surfaceCreated(SurfaceHolder surfaceHolder) {
    }

    @Override
    public void surfaceChanged(SurfaceHolder surfaceHolder, int i, int i1, int i2) {
        Surface surface = surfaceHolder.getSurface();
        setSurfaceNative(surface);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolder) {

    }

    /**
     * native methods
     **/
    private native void _prepare(String url);

    private native void _start();

    private native void _stop();

    private native void _release();

    private native void setSurfaceNative(Surface surface);

    private native int getDurationNative();

    private native void seekNative(int progress);
}
