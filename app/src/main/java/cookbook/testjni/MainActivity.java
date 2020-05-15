package cookbook.testjni;

import android.graphics.Bitmap;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import java.util.ArrayList;

public class MainActivity extends AppCompatActivity {
    private final String TAG = "MainActivity";
    private final boolean VERBOSE = true;

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("av_thirdparty");
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);


        // Example of a call to a native method
        TextView tv = (TextView) findViewById(R.id.sample_text);
//        tv.setText(stringFromJNI());


//        setFloatArray();
        findViewById(R.id.button2).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String[] paths = new String[2];
                paths[0]="/sdcard/Android/data/com.taobao.idlefish.multimedia/files/DCIM/idlefish_av/record_1535526010725.mp4";
                paths[1]="/sdcard/Android/data/com.taobao.idlefish.multimedia/files/DCIM/idlefish_av/record_1535526062139.mp4";
//        paths.add("/sdcard/av_framework/record_1517557496237.mp4");
//        paths.add("/sdcard/av_framework/record_1517557601786.mp4");
//        paths.add("/sdcard/av_framework/record_1517562471518.mp4");
//        paths.add("/sdcard/av_framework/record_1517562474777.mp4");
                String mVideoOutputPath = "/sdcard/combinate2.mp4";
                long start = System.currentTimeMillis();

                mergeMp4Clips(paths, mVideoOutputPath);

                if(VERBOSE) Log.e(TAG,"MergeVideoUse time="+(System.currentTimeMillis()-start));
            }
        });


        findViewById(R.id.button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
//                float[] floats = new float[]{
//                        12f, 23f, 31f, 14f, 5f
//                };
//                float[] outFloats = setFloatArray(floats);
//                for (int i = 0; i < outFloats.length; i++) {
//                    Log.d("andymao", "" + outFloats[i]);
//                }

                int[] intArray = new int[12];
                getIntArray(intArray);

                for (int i = 0; i < intArray.length; i++) {
                    Log.d("andymao", intArray[i] + "");
                }

                String path = "/sdcard/idlefish_video/stopwatch_gop1.mp4";
                int frameW = 720, frameH = 960;
//                String path = "/sdcard/idlefish_video/camera_1080p.mp4";
//                int frameW = 1920, frameH = 1080;
                int[] pixels = new int[frameW * frameH];
                long start = System.currentTimeMillis();

//                for (int i = 0; i < 100000; i++) {
//                    extractFrame2(path, 1000*1000, pixels, frameW, frameH, false);
//                }

                extractFrame2(path, 1000*1000, pixels, frameW, frameH, false);


                Log.e(TAG,"onClick use time="+(System.currentTimeMillis()-start));

                Bitmap bmp = Bitmap.createBitmap(frameW, frameH, Bitmap.Config.ARGB_8888);
                bmp.setPixels(pixels, 0, frameW, 0, 0, frameW, frameH);
                if(VERBOSE) Log.e(TAG,"");
            }
        });

    }

    private  native void mergeMp4Clips(String[] paths, String mVideoOutputPath);
    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
//    public native String stringFromJNI();
    public native float[] setFloatArray(float[] arrInput);

    public native void getIntArray(int[] intArrays);

    public native void extractFrame2(String filePath, int timePoint, int[] pixels, int frameW, int frameH, boolean debug);
}
