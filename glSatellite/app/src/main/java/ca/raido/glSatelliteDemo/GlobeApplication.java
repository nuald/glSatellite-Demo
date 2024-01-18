package ca.raido.glSatelliteDemo;

import android.app.Application;
import android.os.Environment;
import android.os.StrictMode;
import android.util.Log;

import java.io.File;
import java.io.FilenameFilter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.UUID;

public class GlobeApplication extends Application {
    public static final boolean DEVELOPER_MODE = false;
    private static final boolean STRICT_MODE = false;

    @Override
    public void onCreate() {
        super.onCreate();
        if (DEVELOPER_MODE) {
            Log.d(getApplicationContext().getPackageName(), "onCreate called");

            if (STRICT_MODE) {
                StrictMode.setThreadPolicy(new StrictMode.ThreadPolicy.Builder()
                    .detectAll().penaltyLog().build());
                StrictMode.setVmPolicy(new StrictMode.VmPolicy.Builder()
                    .detectAll().penaltyLog().penaltyDeath().build());
            }
        }
    }
}
