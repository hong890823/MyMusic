package com.hong.mymusic;

import android.Manifest;
import android.app.Activity;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.support.v4.app.FragmentTransaction;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.SeekBar;
import android.widget.TextView;

import com.hong.myplayer.HTimeInfoBean;
import com.hong.myplayer.listener.HonCompleteListener;
import com.hong.myplayer.listener.HonErrorListener;
import com.hong.myplayer.listener.HonLoadListener;
import com.hong.myplayer.listener.HonPauseResumeListener;
import com.hong.myplayer.listener.HonPreparedListener;
import com.hong.myplayer.listener.HonRecordListener;
import com.hong.myplayer.listener.HonTimeInfoListener;
import com.hong.myplayer.log.LogUtil;
import com.hong.myplayer.player.HPlayer;
import com.hong.myplayer.util.HTimeUtil;

import java.io.File;
import java.lang.ref.WeakReference;

public class MainActivity extends AppCompatActivity implements View.OnClickListener, HonPreparedListener, HonLoadListener, HonPauseResumeListener, HonTimeInfoListener, HonErrorListener, HonCompleteListener, HonRecordListener {
    private HPlayer player;

    private static final int REQUEST_EXTERNAL=1;
    private static String[] PERMISSIONS= {
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.INTERNET,
    };

    static class ShowTimeHandler extends Handler{
        WeakReference<MainActivity> reference;

        ShowTimeHandler(MainActivity context){
            this.reference = new WeakReference<>(context);
        }

        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            MainActivity context = reference.get();
            if(context!=null){

                if(msg.what==-1){
                    HTimeInfoBean timeInfoBean = (HTimeInfoBean) msg.obj;
                    int totalTime = timeInfoBean.getTotalTime();
                    int currentTime = timeInfoBean.getCurrentTime();
                    String timeStr = HTimeUtil.secdsToDateFormat(currentTime,totalTime)
                            +"/"+HTimeUtil.secdsToDateFormat(totalTime,totalTime);
                    context.timeShowTv.setText(timeStr);

                    int progressTime = currentTime*100/totalTime;
                    context.playSeekBar.setProgress(progressTime);
                }else if(msg.what == -2){
                    int seconds = (int) msg.obj;
                    context.recordTimeTv.setText(String.valueOf(seconds));
                }

            }

        }
    }

    private ShowTimeHandler showTimeHandler;
    private TextView timeShowTv;
    private SeekBar playSeekBar;
    private int seekTime = 0;

    private int muteSolo = 0;

    private EditText pitchEdt;
    private EditText speedEdt;
    private Button changeSoundBtn;

    private TextView recordTimeTv;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        initSourceFragment();
        verifyStoragePermissions(this);
        showTimeHandler = new ShowTimeHandler(this);

        player = new HPlayer();
        player.setSource("http://mpge.5nd.com/2015/2015-11-26/69708/1.mp3");
//        String path = Environment.getExternalStorageDirectory().getAbsolutePath()+"/myheart.mp3";
//        String path1 = Environment.getExternalStorageDirectory().getAbsolutePath()+"/Someone Like You.ape";
//        String path2 = Environment.getExternalStorageDirectory().getAbsolutePath()+"/煎熬.ape";
//        String path3 = Environment.getExternalStorageDirectory().getAbsolutePath()+"/美丽的神话.flac";
//        player.setSource(path3);
//        player.setSource("http://ngcdn004.cnr.cn/live/dszs/index.m3u8");

        findViewById(R.id.start_play).setOnClickListener(this);
        findViewById(R.id.pause_play).setOnClickListener(this);
        findViewById(R.id.resume_play).setOnClickListener(this);
        findViewById(R.id.stop_play).setOnClickListener(this);
        findViewById(R.id.next_play).setOnClickListener(this);
        findViewById(R.id.switch_mute).setOnClickListener(this);
        timeShowTv = findViewById(R.id.time_show);
        playSeekBar = findViewById(R.id.seek_play);
        playSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                int duration = player.getDuration();
                if(duration>0){
                    seekTime = duration*progress/100;
                }

            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                player.seek(seekTime);
            }
        });
        SeekBar volumeSeekBar = findViewById(R.id.seek_volume);
        volumeSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                player.setVolume(progress);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });

        pitchEdt = findViewById(R.id.pitch_edt);
        speedEdt = findViewById(R.id.speed_edt);
        changeSoundBtn = findViewById(R.id.change_sound);
        changeSoundBtn.setOnClickListener(this);

        findViewById(R.id.start_record).setOnClickListener(this);
        findViewById(R.id.pause_record).setOnClickListener(this);
        findViewById(R.id.resume_record).setOnClickListener(this);
        findViewById(R.id.stop_record).setOnClickListener(this);
        recordTimeTv = findViewById(R.id.record_time);

        player.setOnPreparedListener(this);
        player.setOnLoadListener(this);
        player.setOnPauseResumeListener(this);
        player.setOnTimeInfoListener(this);
        player.setOnErrorListener(this);
        player.setOnCompleteListener(this);
        player.setOnRecordListener(this);

    }

    private void initSourceFragment(){
        FragmentTransaction ft = getSupportFragmentManager().beginTransaction();
        ft.add(R.id.file_list_content,new SourceFragment());
        ft.commitAllowingStateLoss();
    }

    public void setDataSource(String source){
        if(player!=null){
            player.setSource(source);
            player.prepare();
        }
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
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()){
            case R.id.start_play:
                player.prepare();
                break;
            case R.id.pause_play:
                player.pause();
                break;
            case R.id.resume_play:
                player.resume();
                break;
                //释放内存有顺序：释放队列-释放OpenSL-释放Audio-释放FFmpeg
                // 还要处理数据流加载时的停止操作造成的异常
            case R.id.stop_play:
                player.stop();
                break;
            case R.id.next_play:
                player.playNext("http://ngcdn004.cnr.cn/live/dszs/index.m3u8");
                break;
            case R.id.switch_mute:
                if(muteSolo==0)muteSolo=1;
                else if(muteSolo==1)muteSolo=2;
                else if(muteSolo==2)muteSolo=0;
                player.setMuteSolo(muteSolo);
                break;
            case R.id.change_sound:
                String pitchStr = pitchEdt.getText().toString();
                String speedStr = speedEdt.getText().toString();
                float pitch = 1;
                float speed = 1;
                try{
                    pitch = Float.valueOf(pitchStr);
                }catch(Exception e){
                    pitch = 1;
                }
                try{
                    speed = Float.valueOf(speedStr);
                }catch(Exception e){
                    speed = 1;
                }
                player.setPitchAndSpeed(pitch,speed);
                break;
            case R.id.start_record:
                String rootPath = Environment.getExternalStorageDirectory().getAbsolutePath();
                player.startRecord(new File(rootPath+"/myheart.aac"));
                break;
            case R.id.pause_record:
                player.pauseRecord();
                break;
            case R.id.resume_record:
                player.resumeRecord();
                break;
            case R.id.stop_record:
                player.stopRecord();
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
        Message msg = showTimeHandler.obtainMessage();
        msg.what = -1;
        msg.obj = timeInfoBean;
        showTimeHandler.sendMessage(msg);
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

    @Override
    public void onRecordTime(int seconds) {
        Message msg = showTimeHandler.obtainMessage();
        msg.what = -2;
        msg.obj = seconds;
        showTimeHandler.sendMessage(msg);
    }



}
