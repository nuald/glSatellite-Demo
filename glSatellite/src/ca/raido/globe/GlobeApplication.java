package ca.raido.globe;

import android.app.Application;
import android.content.Context;
import android.util.Log;
import ca.raido.helper.NDKHelper;

public class GlobeApplication extends Application {
    private static Context context;

    @Override
    public void onCreate() {
        context = getApplicationContext();
        NDKHelper.setContext(context);
        Log.d(context.getPackageName(), "onCreate called");
    }
}
