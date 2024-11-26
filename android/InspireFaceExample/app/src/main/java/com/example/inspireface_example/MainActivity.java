package com.example.inspireface_example;

import android.os.Bundle;
import android.util.Log;

import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

import com.insightface.sdk.inspireface.InspireFace;
import com.insightface.sdk.inspireface.base.CustomParameter;
import com.insightface.sdk.inspireface.base.Session;

public class MainActivity extends AppCompatActivity {

    private final String TAG = "InspireFace";

    void test() {
        String folder = InspireFace.copyResourceFileToApplicationDir(this);
        Log.d(TAG, "Path: "+ folder);
        boolean launchStatus = InspireFace.GlobalLaunch(folder + "/Pikachu");
        if (!launchStatus) {
            Log.e(TAG, "Failed to launch InspireFace");
            return;
        }
        CustomParameter parameter = new CustomParameter();
        parameter.enableRecognition(true).enableFaceQuality(true);
        Session session = InspireFace.CreateSession(parameter);
        Log.d(TAG, "session handle: " + session.handle);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        EdgeToEdge.enable(this);
        setContentView(R.layout.activity_main);
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main), (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });

        //

        test();
    }
}