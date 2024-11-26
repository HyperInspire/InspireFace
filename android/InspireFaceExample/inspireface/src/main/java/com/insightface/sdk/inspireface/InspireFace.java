package com.insightface.sdk.inspireface;

import android.content.Context;

import com.insightface.sdk.inspireface.base.CustomParameter;
import com.insightface.sdk.inspireface.base.Session;
import com.insightface.sdk.inspireface.utils.SDKUtils;

public class InspireFace {


    static {
        System.loadLibrary("InspireFace");
    }

    public static String copyResourceFileToApplicationDir(Context context) {
        String dest = context.getExternalFilesDir(null).getAbsolutePath() + "/inspireface";
        String resourceFolderPath = context.getExternalFilesDir(null).getAbsolutePath() + "/inspireface";
        SDKUtils.copyFilesFromAssets(context, "inspireface", resourceFolderPath);
        return dest;
    }

    public static native boolean GlobalLaunch(String resourcePath);

    public static native boolean GlobalTerminate();

    public static native Session CreateSession(CustomParameter customParameter);

}
