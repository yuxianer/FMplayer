package com.jzy.ffmplayer;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.view.Surface;
import android.view.SurfaceView;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends AppCompatActivity {
    private static final String path = Environment.getExternalStorageDirectory().getAbsolutePath() +
            "/hello.mp4";
    private SurfaceView surfaceView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        surfaceView = findViewById(R.id.surface_view);
        final FFmplayer fFmplayer = new FFmplayer(path,surfaceView);
        fFmplayer.prepare();
        fFmplayer.setListener(new FFmplayer.OnListener() {
            @Override
            public void onPrepared() {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
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
                        Toast.makeText(getApplicationContext(), errcode, Toast.LENGTH_SHORT).show();
                    }
                });
            }

            @Override
            public void onProgress(int progress) {

            }
        });

    }
}
