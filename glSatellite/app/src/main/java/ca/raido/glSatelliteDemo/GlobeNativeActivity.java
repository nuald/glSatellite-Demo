package ca.raido.glSatelliteDemo;

import android.app.NativeActivity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.preference.PreferenceManager;
import android.util.Log;
import android.util.Pair;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.MarginLayoutParams;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.AdView;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.Date;
import java.util.Locale;

public class GlobeNativeActivity extends NativeActivity {

    static final int LENGTH = 12;

    private String mUrl;
    private String mUsedUrl;
    private float mFps;
    private Date mDt;

    private LinearLayout mMainLayout;
    private PopupWindow mPopupWindow;
    private PopupWindow mAdsWindow;
    private TextView mLabel;
    private ProgressBar mProgressBar;

    private void setProgressBarPosition(final int progress) {
        if (mProgressBar != null) {
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mProgressBar.setProgress(progress);
                }
            });
        }
    }

    private void logException(Exception e) {
        if (GlobeApplication.DEVELOPER_MODE) {
            Log.e(getPackageName(), e.getMessage());
        }
    }

    private Pair<String, Long> downloadFileImpl() {
        try (CustomHttpUrlConnection urlConnection = new CustomHttpUrlConnection(mUrl)) {
            // Read meta-data
            String etag = urlConnection.getETag();
            if (etag != null) {
                etag = etag.replaceAll("[\":/\\\\]", "");
                final long dt = urlConnection.getLastModified();
                final File file = new File(getExternalFilesDir(null), etag);
                // ETag is unique, hence if file exists do not download
                if (!file.exists()) {
                    // Download the file
                    try (InputStream input = urlConnection.getInputStream();
                         OutputStream output = new FileOutputStream(file)) {
                        final int lengthOfFile = urlConnection.getContentLength();

                        final byte[] data = new byte[1024];
                        int total = 0;
                        int count;

                        while ((count = input.read(data)) != -1) {
                            total += count;
                            final int progress = total * 100 / lengthOfFile;
                            setProgressBarPosition(progress);
                            // writing data to file
                            output.write(data, 0, count);
                        }

                        // flushing output
                        output.flush();
                    }
                }
                return new Pair<>(etag, dt);
            }
        } catch (IOException e) {
            logException(e);
        }
        return new Pair<>(null, 0L);
    }

    // Background function to download a new file if needed
    private boolean downloadFile() {
        final Pair<String, Long> meta = downloadFileImpl();
        final String etag = meta.first;
        final long dt = meta.second;
        if (etag != null) {
            setProgressBarPosition(0);
            mDt = new Date(dt);
            mUsedUrl = mUrl;
            useTle(etag);
            return true;
        }
        return false;
    }

    @Override
    protected void onResume() {
        super.onResume();

        final SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
        mUrl = prefs.getString(SettingsActivity.PREF_URL, "");
        if (mUrl.isEmpty()) {
            final String tle = prefs.getString(SettingsActivity.PREF_TLE, SettingsActivity.DEFAULT);
            mUrl = String.format(SettingsActivity.FMT, tle);
            prefs.edit().putString(SettingsActivity.PREF_URL, mUrl).apply();
        }

        new Thread(new Runnable() {
            @Override
            public void run() {
                if (!downloadFile()) {
                    final Resources res = getResources();
                    showToast(String.format(
                        res.getString(R.string.format_fail), mUrl));
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
            default:
                // pass
        }
        return super.onOptionsItemSelected(item);
    }

    static {
        System.loadLibrary("GlobeNativeActivity");
    }

    native void showAds();

    native void useTle(String path);

    private int getStatusBarHeight() {
        int result = 0;
        final int resourceId = getResources().getIdentifier("status_bar_height",
            "dimen", "android");
        if (resourceId > 0) {
            result = getResources().getDimensionPixelSize(resourceId);
        }
        return result;
    }

    private View inflateView(int id) {
        final LayoutInflater layoutInflater = (LayoutInflater) getBaseContext()
                .getSystemService(LAYOUT_INFLATER_SERVICE);
        return layoutInflater.inflate(id, null);
    }

    public void showUI() {
        if (mPopupWindow != null) {
            return;
        }

        final GlobeNativeActivity activity = this;

        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                final View popupView = inflateView(R.layout.widgets);
                mPopupWindow = new PopupWindow(popupView,
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT);

                mMainLayout = new LinearLayout(activity);
                final MarginLayoutParams params = new MarginLayoutParams(
                        ViewGroup.LayoutParams.WRAP_CONTENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT);
                params.setMargins(0, 0, 0, 0);
                activity.setContentView(mMainLayout, params);

                // Show our UI over NativeActivity window
                mPopupWindow.showAtLocation(mMainLayout, Gravity.BOTTOM | Gravity.START, 0, 0);

                mLabel = popupView.findViewById(R.id.textViewFPS);
                mProgressBar = popupView.findViewById(R.id.progressBar);

                showAds();
            }
        });
    }

    public void runSettings(View view) {
        final Intent settings = new Intent(this, SettingsActivity.class);
        startActivity(settings);
    }

    @Override
    protected void onPause() {
        super.onPause();

        if (mPopupWindow != null) {
            mPopupWindow.dismiss();
            mPopupWindow = null;
        }
        if (mAdsWindow != null) {
            mAdsWindow.dismiss();
            mAdsWindow = null;
        }
    }

    private String formatUrl(String url) {
        String tle = "N/A";
        try {
            final URL urlObj = new URL(url);
            final String path = urlObj.getPath();
            final String name = new File(path).getName();
            if (!name.isEmpty()) {
                tle = name;
            }
            if (tle.length() > LENGTH) {
                tle = tle.substring(0, LENGTH - 1) + "…";
            }
        } catch (MalformedURLException e) {
            // do not log the error because it'll spam the log
        }
        return tle;
    }

    public void updateLabel(final String label) {
        if (mLabel == null) {
            return;
        }

        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                final Resources res = getResources();
                final String result;
                if (label == null) {
                    if (mUsedUrl == null) {
                        result = String.format(Locale.getDefault(),
                            res.getString(R.string.format_default), mFps);
                    } else {
                        result = String.format(Locale.getDefault(),
                            res.getString(R.string.format_std), mFps, mDt,
                            formatUrl(mUsedUrl));
                    }
                } else {
                    result = label;
                }
                mLabel.setText(result);
            }
        });
    }

    public void showAdsImpl() {
        if (mAdsWindow != null) {
            return;
        }

        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                final View popupView = inflateView(R.layout.ads);
                mAdsWindow = new PopupWindow(popupView,
                        android.view.ViewGroup.LayoutParams.MATCH_PARENT,
                        android.view.ViewGroup.LayoutParams.WRAP_CONTENT);

                // Show our UI over NativeActivity window
                final int height = getStatusBarHeight();
                mAdsWindow.showAtLocation(mMainLayout, Gravity.NO_GRAVITY, 0, height);

                // Look up the AdView as a resource and load a request.
                final AdView adView = popupView.findViewById(R.id.adView);
                final AdRequest adRequest = new AdRequest.Builder().addTestDevice(
                    AdRequest.DEVICE_ID_EMULATOR).build();
                adView.loadAd(adRequest);
            }
        });
    }

    public void showBeam(String name, int catnum, float lat, float lon,
        float alt) {
        final Resources res = getResources();
        final String msg = String.format(res.getString(R.string.format_info), name,
            catnum, lat, lon, alt);
        showToast(msg);
    }

    public void showToast(final CharSequence text) {
        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                final Context context = getApplicationContext();
                final int duration = Toast.LENGTH_LONG;

                final Toast toast = Toast.makeText(context, text, duration);
                toast.show();
            }
        });
    }

    public void updateFPS(final float fps) {
        mFps = fps;
        updateLabel(null);
    }

    private static class CustomHttpUrlConnection implements AutoCloseable {
        private final HttpURLConnection mConnection;

        CustomHttpUrlConnection(String urlString) throws IOException {
            final URL url = new URL(urlString);
            mConnection = (HttpURLConnection) url.openConnection();
        }

        @Override
        public void close() throws IOException {
            mConnection.disconnect();
        }

        String getETag() {
            return mConnection.getHeaderField("ETag");
        }

        long getLastModified() {
            return mConnection.getHeaderFieldDate("Last-Modified", 0);
        }

        InputStream getInputStream() throws IOException {
            return new BufferedInputStream(mConnection.getInputStream());
        }

        int getContentLength() {
            return mConnection.getContentLength();
        }
    }
}
