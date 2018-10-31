package com.hong.mymusic;

import android.app.Activity;
import android.os.Bundle;
import android.os.Environment;
import android.support.annotation.Nullable;
import android.view.View;

import com.hong.myplayer.listener.HonPcmInfoListener;
import com.hong.myplayer.listener.HonPreparedListener;
import com.hong.myplayer.log.LogUtil;
import com.hong.myplayer.player.HPlayer;

public class CutActivity extends Activity{
    private static final String TAG = "CutActivity";

    private HPlayer player;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_cut);
        player = new HPlayer();
        player.setOnPreparedListener(new HonPreparedListener() {
            @Override
            public void onPrepared() {
                player.cutAudioPlay(20,40,true);
            }
        });

        player.setOnPcmInfoListener(new HonPcmInfoListener() {
            //pcm数据可以用来和视频数据进行合成，类似抖音
            @Override
            public void onPcmInfo(byte[] buffer, int size) {
                LogUtil.logD("size is "+size);
            }

            //可以根据这些参数生成编码器
            @Override
            public void onPcmRate(int sampleRate, int bit, int channels) {
                LogUtil.logD("sampleRate is "+sampleRate);
            }
        });
    }

    public void cutAudioPlay(View view) {
        String path = Environment.getExternalStorageDirectory().getAbsolutePath()+"/美丽的神话.flac";
        player.setSource(path);
        player.prepare();
    }

}
