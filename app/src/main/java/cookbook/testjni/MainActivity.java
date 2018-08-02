package cookbook.testjni;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("QuCore-ThirdParty");
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

                String path = "/sdcard/idlefish_video/stopwatch.mp4";

                int frameW = 720, frameH = 960;
                int[] pixels = new int[frameW * frameH];
                extractFrame2(path, 5000, pixels, frameW, frameH, false);
            }
        });

    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
//    public native String stringFromJNI();
    public native float[] setFloatArray(float[] arrInput);

    public native void getIntArray(int[] intArrays);

    public native void extractFrame2(String filePath, int timePoint, int[] pixels, int frameW, int frameH, boolean debug);
}
