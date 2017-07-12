package ca.raido.glSatelliteDemo;

import android.app.Application;
import android.content.Context;
import android.os.StrictMode;
import android.util.Log;
import ca.raido.helper.NDKHelper;

public class GlobeApplication extends Application {
    public static final boolean DEVELOPER_MODE = false;

    @Override
    public void onCreate() {
        super.onCreate();
        if (DEVELOPER_MODE) {
            Log.d(getApplicationContext().getPackageName(), "onCreate called");
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
