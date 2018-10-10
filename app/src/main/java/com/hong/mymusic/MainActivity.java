package com.hong.mymusic;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.widget.TextView;

import com.hong.myplayer.Demo;

public class MainActivity extends AppCompatActivity {

    private Demo demo;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        demo = new Demo();

        TextView tv = (TextView) findViewById(R.id.sample_text);
        tv.setText(demo.stringFromJNI());

        demo.testFFmpeg();
    }


}
