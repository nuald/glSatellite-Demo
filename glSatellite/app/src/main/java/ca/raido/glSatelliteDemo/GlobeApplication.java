package ca.raido.glSatelliteDemo;

import android.app.Application;
import android.content.Context;
import android.os.StrictMode;
import android.util.Log;
import ca.raido.helper.NDKHelper;

public class GlobeApplication extends Application {
    private static Context context;

    public static boolean DEVELOPER_MODE = false;

    @Override
    public void onCreate() {
        context = getApplicationContext();
        NDKHelper.setContext(context);
        DEVELOPER_MODE = NDKHelper.isDeveloperMode();
        if (DEVELOPER_MODE) {
            Log.d(context.getPackageName(), "onCreate called");
            // Strict mode doesn't work properly (at least on Android 4.2.1)
            // see https://code.google.com/p/android/issues/detail?id=54285
            /*
            StrictMode.setThreadPolicy(new StrictMode.ThreadPolicy.Builder()
                    .detectDiskReads().detectDiskWrites().detectAll()
                    .penaltyLog().build());
            StrictMode.setVmPolicy(new StrictMode.VmPolicy.Builder()
                    .detectLeakedSqlLiteObjects().detectLeakedClosableObjects()
                    .penaltyLog().penaltyDeath().build());
            */
        }
    }
}
