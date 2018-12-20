package com.doom;

import android.app.Activity;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.view.View;

import java.util.Date;

public class MainActivity extends Activity {
    public Bitmap bitmap;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        final int M = 1024*1024;
        final boolean result = Doom.init(this);
        if(result){
            System.out.println("doom init : "+result+" maxMemory:"+Runtime.getRuntime().maxMemory()/M+" totalMemory:"+Runtime.getRuntime().totalMemory()/M);
            Doom.pauseGc();
        }

        findViewById(R.id.btn1).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Entity entity = new Entity();
                System.out.println("doom INFO : "+" maxMemory:"+Runtime.getRuntime().maxMemory()/M+" totalMemory:"+Runtime.getRuntime().totalMemory()/M);
                new Date().toString();
            }
        });

        findViewById(R.id.btn2).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Doom.resumeGc();
            }
        });

    }
}
