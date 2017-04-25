package com.popsongremix.laidoff;

import android.app.NativeActivity;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.SoundPool;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;

import java.io.IOException;
import java.io.InputStream;
import java.util.Locale;

public class LaidOffNativeActivity extends NativeActivity
{
    private static boolean mBgmOn;

    static {
        System.loadLibrary("native-activity");
    }

    private static SoundPool mSoundPool;
    private static MediaPlayer mBgmPlayer;

    // SFX
    private static int mSound_Tapping01;
    private static int mSound_Completed03;
    private static int mSound_GameOver01;
    private static int mSound_GameStart01;
    private static int mSound_Point;

    public native String stringFromJNI(Class<LaidOffNativeActivity> and9NativeActivityClass);
    public native int pushTextureData(int width, int height, int[] data, int texAtlasIndex);
    public static native void registerAsset(String assetPath, int startOffset, int length);
    private native void sendApkPath(String apkPath);

    public static final String PREFS_NAME = "LaidOffPrefs";
    public static final String PREFS_KEY_HIGHSCORE = "highscore";

    private static LaidOffNativeActivity INSTANCE;
    
    private static final String LOG_TAG = "and9";

    private static AssetsLoader assetsLoader;

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        //setContentView(R.layout.main);

        getWindow().getDecorView().setKeepScreenOn(true);

        Log.i(LOG_TAG, "Device Android Version: " + Build.VERSION.SDK_INT);

        assetsLoader = new AssetsLoader(this);
        Log.i(LOG_TAG, "APK Path: " + assetsLoader.GetAPKPath());
        assetsLoader.registerAllAssetsOfType("tex");
        assetsLoader.registerAllAssetsOfType("pkm");
        assetsLoader.registerAllAssetsOfType("ktx");
        assetsLoader.registerAllAssetsOfType("vbo");
        assetsLoader.registerAllAssetsOfType("svbo");
        assetsLoader.registerAllAssetsOfType("armature");
        assetsLoader.registerAllAssetsOfType("fnt");
        assetsLoader.registerAllAssetsOfType("d");
        assetsLoader.registerAllAssetsOfType("glsles");
        assetsLoader.registerAllAssetsOfType("l");
        sendApkPath(assetsLoader.GetAPKPath());

        //noinspection deprecation
        mSoundPool = new SoundPool(5, AudioManager.STREAM_MUSIC, 0);
        mSoundPool.setOnLoadCompleteListener(new SoundPool.OnLoadCompleteListener() {
            @Override
            public void onLoadComplete(SoundPool soundPool, int sampleId,
                                       int status) {
                //loaded = true;

                //mSoundPool.play(sampleId, 1, 1, 0, 0, 1);
            }
        });

        mSound_Tapping01 = mSoundPool.load(getApplicationContext(), R.raw.jump, 1);
        mSound_Completed03 = mSoundPool.load(getApplicationContext(), R.raw.completed, 1);
        mSound_GameOver01 = mSoundPool.load(getApplicationContext(), R.raw.over, 1);
        mSound_GameStart01 = mSoundPool.load(getApplicationContext(), R.raw.start, 1);
        mSound_Point = mSoundPool.load(getApplicationContext(), R.raw.point, 1);

        mBgmPlayer = MediaPlayer.create(getApplicationContext(), R.raw.opening); // in 2nd param u have to pass your desire ringtone
        mBgmPlayer.setLooping(true);
        //mBgmPlayer.prepare();
        //mBgmPlayer.start();

        Log.i(LOG_TAG, "onCreate [JAVA] - calling JNI func result: " + stringFromJNI(LaidOffNativeActivity.class));

        INSTANCE = this;

        final int atlasCount = 3;
        for (int i = 0; i < atlasCount; i++) {
            String assetName = String.format(Locale.US, "tex/kiwi-atlas-set-%02d.png", i + 1);
            loadBitmap(assetName);
        }
    }

    static public int loadBitmap(String assetName) {
        return loadBitmapWithIndex(0, assetName);
    }

    static public int loadBitmapWithIndex(int i, String assetName) {
        Bitmap bitmap = getBitmapFromAsset(INSTANCE.getApplicationContext(), assetName);

        Log.i(LOG_TAG, String.format(Locale.US, "Tex(asset name) %s Bitmap width: %d", assetName, bitmap.getWidth()));
        Log.i(LOG_TAG, String.format(Locale.US, "Tex(asset name) %s Bitmap height: %d", assetName, bitmap.getHeight()));

        int[] pixels = new int[bitmap.getWidth()*bitmap.getHeight()];
        bitmap.getPixels(pixels, 0, bitmap.getWidth(), 0, 0, bitmap.getWidth(), bitmap.getHeight());
        int bytes_allocated_on_native = INSTANCE.pushTextureData(bitmap.getWidth(), bitmap.getHeight(), pixels, i);

        Log.i(LOG_TAG, String.format(Locale.US, "Tex(asset name) %s Bitmap copied to native side %d bytes", assetName, bytes_allocated_on_native));

        return bitmap.getWidth() * bitmap.getHeight();
    }

    public static Bitmap getBitmapFromAsset(Context context, String filePath) {
        AssetManager assetManager = context.getAssets();

        InputStream inputStr;
        Bitmap bitmap = null;
        try {
            inputStr = assetManager.open(filePath);
            bitmap = BitmapFactory.decodeStream(inputStr);
        } catch (IOException e) {
            // handle exception
        }

        return bitmap;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();

        Log.d(LOG_TAG, "onDestroy()");
    }

    @Override
    public void onPause() {
        super.onPause();

        if (mBgmOn)
        {
            mBgmPlayer.pause();
        }

        Log.d(LOG_TAG, "onPause()");
    }

    @Override
    public void onResume() {
        super.onResume();

        if (mBgmOn)
        {
            mBgmPlayer.start();
        }

        Log.d(LOG_TAG, "onResume()");
    }

}
