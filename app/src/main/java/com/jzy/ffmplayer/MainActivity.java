package com.jzy.ffmplayer;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceView;
import android.view.View;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends AppCompatActivity implements SeekBar.OnSeekBarChangeListener {
    private static final String path = Environment.getExternalStorageDirectory().getAbsolutePath() +
            "/hello.mp4";
    private SurfaceView surfaceView;
    private TextView textView;
    private SeekBar seekBar;
    private int duration = 0;
    private boolean isTouch = false;
    private boolean isSeek;
    private FFmplayer fFmplayer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        surfaceView = findViewById(R.id.surface_view);
        textView = findViewById(R.id.tv_time);
        seekBar = findViewById(R.id.seek_bar);
        seekBar.setOnSeekBarChangeListener(this);

        fFmplayer = new FFmplayer(path, surfaceView);
        fFmplayer.prepare();
        fFmplayer.setListener(new FFmplayer.OnListener() {
            @Override
            public void onPrepared() {
                duration = fFmplayer.getDuration();
                Log.e("aaaa", duration + "");
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if (duration != 0) {
                            textView.setVisibility(View.VISIBLE);
                            seekBar.setVisibility(View.VISIBLE);
                            textView.setText( "00:00/" + getMinutes(duration) + ":" + getSeconds(duration));
                        }
                        Toast.makeText(getApplicationContext(), "开始播放", Toast.LENGTH_SHORT).show();
                        fFmplayer.start();
                    }
                });
            }

            @Override
            public void onError(final int errcode) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                Toast.makeText(getApplicationContext(), errcode, Toast.LENGTH_SHORT).show();
                            }
                        });
                    }
                });
            }

            @Override
            public void onProgress(final int progress) {
                if (!isTouch) {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            runOnUiThread(new Runnable() {
                                @Override
                                public void run() {
                                    if (duration != 0) {
                                        if (isSeek) {
                                            isSeek = false;
                                            return;
                                        }
                                        textView.setText(getMinutes(progress) + ":" + getSeconds(
                                                progress) + "/" + getMinutes(duration) + ":" + getSeconds(
                                                duration));
                                    }

                                }
                            });
                        }
                    });
                }
            }
        });

    }

    private String getMinutes(int duration) {
        int minutes = duration / 60;
        if (minutes <= 9) {
            return "0" + minutes;
        }
        return "" + minutes;
    }

    private String getSeconds(int duration) {
        int seconds = duration % 60;
        if (seconds <= 9) {
            return "0" + seconds;
        }
        return "" + seconds;
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        if (fromUser) {
            textView.setText((getMinutes(progress * duration / 100) + ":" + getSeconds(
                    progress * duration / 100) + "/" + getMinutes(duration) + ":" + getSeconds(
                    duration)));
        }
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {
        isTouch = true;
    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {
        isTouch = false;
        isSeek = false;

        int seekBarProgress = seekBar.getProgress();
        int progress = seekBarProgress * duration / 100;
        fFmplayer.seek(progress);
    }
}
