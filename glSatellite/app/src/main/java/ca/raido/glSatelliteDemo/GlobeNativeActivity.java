package ca.raido.glSatelliteDemo;

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
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup.MarginLayoutParams;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.AdView;

public class GlobeNativeActivity extends NativeActivity {

    private String url_;
    private String usedUrl_;
    private float fps_;
    private Date dt_;

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

    private void logException(Exception e) {
        if (GlobeApplication.DEVELOPER_MODE) {
            Log.e(getPackageName(), e.getMessage());
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
            // Etag is unique, hence if file exists do not download
            if (!file.exists()) {
                // Download the file
                input = new BufferedInputStream(urlConnection.getInputStream());
                output = new FileOutputStream(file);
                int lengthOfFile = urlConnection.getContentLength();

                byte data[] = new byte[1024];
                int total = 0, count;

                while ((count = input.read(data)) != -1) {
                    total += count;
                    int progress = total * 100 / lengthOfFile;
                    setProgressBarPosition(progress);
                    // writing data to file
                    output.write(data, 0, count);
                }

                // flushing output
                output.flush();
            }
        } catch (MalformedURLException e) {
            logException(e);
            return false;
        } catch (FileNotFoundException e) {
            logException(e);
            return false;
        } catch (IOException e) {
            logException(e);
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
                logException(e);
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
            String tle = prefs.getString(SettingsActivity.PREF_TLE, SettingsActivity.DEFAULT);
            url_ = String.format(SettingsActivity.FMT, tle);
            prefs.edit().putString(SettingsActivity.PREF_URL, url_).apply();
        }

        new Thread(new Runnable() {
            @Override
            public void run() {
                if (!downloadFile()) {
                    Resources res = getResources();
                    showToast(String.format(
                        res.getString(R.string.format_fail), url_));
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
                        android.view.ViewGroup.LayoutParams.MATCH_PARENT,
                        android.view.ViewGroup.LayoutParams.WRAP_CONTENT);

                _mainLayout = new LinearLayout(activity);
                MarginLayoutParams params = new MarginLayoutParams(
                        android.view.ViewGroup.LayoutParams.WRAP_CONTENT,
                        android.view.ViewGroup.LayoutParams.WRAP_CONTENT);
                params.setMargins(0, 0, 0, 0);
                activity.setContentView(_mainLayout, params);

                // Show our UI over NativeActivity window
                _popupWindow.showAtLocation(_mainLayout, Gravity.BOTTOM | Gravity.START, 0, 0);

                _label = popupView.findViewById(R.id.textViewFPS);
                _progressBar = popupView.findViewById(R.id.progressBar);

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
                Resources res = getResources();
                String result;
                if (label == null) {
                    if (usedUrl_ == null) {
                        result = String.format(Locale.getDefault(),
                            res.getString(R.string.format_default), fps_);
                    } else {
                        result = String.format(Locale.getDefault(),
                            res.getString(R.string.format_std), fps_, dt_,
                            formatUrl(usedUrl_));
                    }
                } else {
                    result = label;
                }
                _label.setText(result);
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
                        android.view.ViewGroup.LayoutParams.MATCH_PARENT,
                        android.view.ViewGroup.LayoutParams.WRAP_CONTENT);

                // Show our UI over NativeActivity window
                int height = getStatusBarHeight();
                _adsWindow.showAtLocation(_mainLayout, Gravity.NO_GRAVITY, 0,
                    height);

                // Look up the AdView as a resource and load a request.
                AdView adView = popupView.findViewById(R.id.adView);
                AdRequest adRequest = new AdRequest.Builder().addTestDevice(
                    AdRequest.DEVICE_ID_EMULATOR).build();
                adView.loadAd(adRequest);
            }
        });
    }

    public void showBeam(String name, int catnum, float lat, float lon,
        float alt) {
        Resources res = getResources();
        String msg = String.format(res.getString(R.string.format_info), name,
            catnum, lat, lon, alt);
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
