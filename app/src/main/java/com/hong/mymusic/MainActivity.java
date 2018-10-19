package com.hong.mymusic;

import android.Manifest;
import android.app.Activity;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Environment;
import android.support.v4.app.ActivityCompat;
import android.support.v7.app.AppCompatActivity;
import android.view.View;

import com.hong.myplayer.HTimeInfoBean;
import com.hong.myplayer.listener.HonCompleteListener;
import com.hong.myplayer.listener.HonErrorListener;
import com.hong.myplayer.listener.HonLoadListener;
import com.hong.myplayer.listener.HonPauseResumeListener;
import com.hong.myplayer.listener.HonPreparedListener;
import com.hong.myplayer.listener.HonTimeInfoListener;
import com.hong.myplayer.log.LogUtil;
import com.hong.myplayer.player.HPlayer;

public class MainActivity extends AppCompatActivity implements View.OnClickListener, HonPreparedListener, HonLoadListener, HonPauseResumeListener, HonTimeInfoListener, HonErrorListener, HonCompleteListener {
    private HPlayer player;

    private static final int REQUEST_EXTERNAL=1;
    private static String[] PERMISSIONS= {
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.INTERNET,
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        verifyStoragePermissions(this);

        player = new HPlayer();
        player.setSource("http://mpge.5nd.com/2015/2015-11-26/69708/1.mp3");
        String path = Environment.getExternalStorageDirectory().getAbsolutePath()+"/myheart.mp3";
        player.setSource(path);
//        player.setSource("http://ngcdn004.cnr.cn/live/dszs/index.m3u8");

        findViewById(R.id.prepare_play).setOnClickListener(this);
        findViewById(R.id.start_play).setOnClickListener(this);
        findViewById(R.id.pause_play).setOnClickListener(this);
        findViewById(R.id.resume_play).setOnClickListener(this);
        findViewById(R.id.stop_play).setOnClickListener(this);

        player.setOnPreparedListener(this);
        player.setOnLoadListener(this);
        player.setOnPauseResumeListener(this);
        player.setOnTimeInfoListener(this);
        player.setOnErrorListener(this);
        player.setOnCompleteListener(this);
    }

    /**
     * Android6.0以上校验文件读写权限
     */
    public void verifyStoragePermissions(Activity activity) {
        int writePermission = ActivityCompat.checkSelfPermission(activity, Manifest.permission.WRITE_EXTERNAL_STORAGE);
        int readPermission = ActivityCompat.checkSelfPermission(activity, Manifest.permission.READ_EXTERNAL_STORAGE);
        int internetPermission = ActivityCompat.checkSelfPermission(activity, Manifest.permission.INTERNET);

        if (writePermission != PackageManager.PERMISSION_GRANTED || readPermission != PackageManager.PERMISSION_GRANTED
                || internetPermission != PackageManager.PERMISSION_GRANTED ) {
            ActivityCompat.requestPermissions(activity,PERMISSIONS,REQUEST_EXTERNAL);
        }
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()){
            case R.id.prepare_play:
                player.prepare();
                break;
            case R.id.start_play:
                player.start();
                break;
            case R.id.pause_play:
//                player.pause();
                player.seek(145);
                break;
            case R.id.resume_play:
                player.resume();
                break;
                //释放内存有顺序：释放队列-释放OpenSL-释放Audio-释放FFmpeg
                // 还要处理数据流加载时的停止操作造成的异常
            case R.id.stop_play:
                player.stop();
                break;
        }
    }

    @Override
    public void onPrepared() {
        LogUtil.logD("解码器已经准备好了");
        player.start();
    }

    @Override
    public void onLoad(boolean load) {
        if(load){
            LogUtil.logD("加载中...");
        }else{
            LogUtil.logD("播放中...");
        }
    }

    @Override
    public void onPause(boolean pause) {
        if(pause){
            LogUtil.logD("暂停中...");
        }else{
            LogUtil.logD("播放中...");
        }
    }

    @Override
    public void onTimeInfo(HTimeInfoBean timeInfoBean) {
        LogUtil.logD(timeInfoBean.toString());
    }

    @Override
    public void onError(int code, String msg) {
        //错误信息尽量详细，方便开发者进行错误定位
        LogUtil.logE(code+msg);
    }

    @Override
    public void onComplete() {
        LogUtil.logD("播放完成");
    }

}
