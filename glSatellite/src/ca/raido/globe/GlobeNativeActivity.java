package ca.raido.globe;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.Date;
import java.util.Locale;

import android.app.NativeActivity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup.MarginLayoutParams;
import android.view.WindowManager.LayoutParams;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;
import android.content.Context;
import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.AdView;

public class GlobeNativeActivity extends NativeActivity {

    static final String FMT = "FPS: %2.2f DT: %tF TLE: %s";
    static final String FAIL = "Downloading failed for %s";
    static final String DFL = "FPS: %2.2f Offline Iridium TLE";
    static final String BMF = "%s LAT: %f LON: %f ALT: %f";
    String url_;
    String usedUrl_;
    float fps_;
    Date dt_;

    private void setProgressBarPosition(final int progress) {
        if (_progressBar != null) {
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    _progressBar.setProgress(progress);
                }
            });
        }
    }

    // Background function to download a new file if needed
    private boolean downloadFile() {
        HttpURLConnection urlConnection = null;
        InputStream input = null;
        OutputStream output = null;
        String etag = null;
        long dt = 0;
        try {
            URL url = new URL(url_);
            urlConnection = (HttpURLConnection)url.openConnection();
            // Read meta-data
            etag = urlConnection.getHeaderField("ETag");
            if (etag == null) {
                return false;
            }
            etag = etag.replaceAll("[\":/\\\\]", "");
            dt = urlConnection.getHeaderFieldDate("Last-Modified", 0);
            File file = new File(getExternalFilesDir(null), etag);
            // Etag is unique, if file exists do not download
            if (!file.exists()) {
                // Download the file
                input = new BufferedInputStream(urlConnection.getInputStream());
                output = new FileOutputStream(file);
                int lenghtOfFile = urlConnection.getContentLength();

                byte data[] = new byte[1024];
                int total = 0, count = 0;

                while ((count = input.read(data)) != -1) {
                    total += count;
                    int progress = total * 100 / lenghtOfFile;
                    setProgressBarPosition(progress);
                    // writing data to file
                    output.write(data, 0, count);
                }

                // flushing output
                output.flush();
            }
        } catch (MalformedURLException e) {
            Log.e(getPackageName(), e.getMessage());
            return false;
        } catch (FileNotFoundException e) {
            Log.e(getPackageName(), e.getMessage());
            return false;
        } catch (IOException e) {
            Log.e(getPackageName(), e.getMessage());
            return false;
        } finally {
            try {
                if (output != null) {
                    output.close();
                }
                if (input != null) {
                    input.close();
                }
            } catch (IOException e) {
                Log.e(getPackageName(), e.getMessage());
                return false;
            } finally {
                if (urlConnection != null) {
                    urlConnection.disconnect();
                }
            }
        }
        setProgressBarPosition(0);
        dt_ = new Date(dt);
        usedUrl_ = url_;
        UseTle(etag);
        return true;
    }

    @Override
    protected void onResume() {
        super.onResume();

        SharedPreferences prefs = PreferenceManager
                .getDefaultSharedPreferences(this);
        url_ = prefs.getString(SettingsActivity.PREF_URL, "");
        if (url_.isEmpty()) {
            String tle = prefs.getString(SettingsActivity.PREF_TLE,
                    DbPickerPreference.DEFAULT);
            url_ = String.format(SettingsActivity.FMT, tle);
            prefs.edit().putString(SettingsActivity.PREF_URL, url_).commit();
        }

        new Thread(new Runnable() {
            public void run() {
                if (!downloadFile()) {
                    showToast(String.format(FAIL, url_));
                }
            }
        }).start();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
        case R.id.settings:
            runSettings(null);
            break;
        }
        return super.onOptionsItemSelected(item);
    }

    static {
        System.loadLibrary("GlobeNativeActivity");
    }

    native void ShowAds();
    native void UseTle(String path);

    LinearLayout _mainLayout;
    PopupWindow _popupWindow;
    PopupWindow _adsWindow;
    TextView _label;
    ProgressBar _progressBar;

    private int getStatusBarHeight() {
        int result = 0;
        int resourceId = getResources().getIdentifier("status_bar_height",
                "dimen", "android");
        if (resourceId > 0) {
            result = getResources().getDimensionPixelSize(resourceId);
        }
        return result;
    }

    private View inflateView(int id) {
        LayoutInflater layoutInflater = (LayoutInflater)getBaseContext()
                .getSystemService(LAYOUT_INFLATER_SERVICE);
        return layoutInflater.inflate(id, null);
    }

    public void showUI() {
        if (_popupWindow != null) {
            return;
        }

        final GlobeNativeActivity activity = this;

        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                View popupView = inflateView(R.layout.widgets);
                _popupWindow = new PopupWindow(popupView,
                        LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);

                _mainLayout = new LinearLayout(activity);
                MarginLayoutParams params = new MarginLayoutParams(
                        LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
                params.setMargins(0, 0, 0, 0);
                activity.setContentView(_mainLayout, params);

                // Show our UI over NativeActivity window
                _popupWindow.showAtLocation(_mainLayout, Gravity.BOTTOM
                        | Gravity.LEFT, 0, 0);

                _label = (TextView)popupView.findViewById(R.id.textViewFPS);
                _progressBar = (ProgressBar)popupView
                        .findViewById(R.id.progressBar);

                ShowAds();
            }
        });
    }

    public void runSettings(View view) {
        Intent settings = new Intent(this, SettingsActivity.class);
        startActivity(settings);
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (_popupWindow != null) {
            _popupWindow.dismiss();
            _popupWindow = null;
        }
        if (_adsWindow != null) {
            _adsWindow.dismiss();
            _adsWindow = null;
        }
    }

    private String formatUrl(String url) {
        String tle = "N/A";
        try {
            URL urlObj = new URL(url);
            String path = urlObj.getPath();
            String name = new File(path).getName();
            if (!name.isEmpty()) {
                tle = name;
            }
            final int LENGTH = 12;
            if (tle.length() > LENGTH) {
                tle = tle.substring(0, LENGTH - 1) + "\u2026";
            }
        } catch (MalformedURLException e) {
            // do not log the error because it'll spam the log
        }
        return tle;
    }

    public void updateLabel(final String label) {
        if (_label == null) {
            return;
        }

        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                String res = null;
                if (label == null) {
                    if (usedUrl_ == null) {
                        res = String.format(Locale.getDefault(), DFL, fps_);
                    } else {
                        res = String.format(Locale.getDefault(), FMT, fps_, dt_,
                            formatUrl(usedUrl_));
                    }
                } else {
                    res = label;
                }
                _label.setText(res);
            }
        });
    }

    public void showAds() {
        if (_adsWindow != null) {
            return;
        }

        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                View popupView = inflateView(R.layout.ads);
                _adsWindow = new PopupWindow(popupView,
                        LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);

                // Show our UI over NativeActivity window
                int height = getStatusBarHeight();
                _adsWindow.showAtLocation(_mainLayout, Gravity.NO_GRAVITY, 0,
                        height);

                // Look up the AdView as a resource and load a request.
                AdView adView = (AdView)popupView.findViewById(R.id.adView);
                AdRequest adRequest = new AdRequest.Builder()
                        .addTestDevice(AdRequest.DEVICE_ID_EMULATOR)
                        .build();
                adView.loadAd(adRequest);
            }
        });
    }

    public void showBeam(String name, float lat, float lon, float alt) {
        String msg = String.format(BMF, name, lat, lon, alt);
        showToast(msg);
    }

    public void showToast(final CharSequence text) {
        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Context context = getApplicationContext();
                int duration = Toast.LENGTH_LONG;

                Toast toast = Toast.makeText(context, text, duration);
                toast.show();
            }
        });
    }

    public void updateFPS(final float fps) {
        fps_ = fps;
        updateLabel(null);
    }

}
