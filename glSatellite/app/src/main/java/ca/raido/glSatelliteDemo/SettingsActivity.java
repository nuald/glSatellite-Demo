package ca.raido.glSatelliteDemo;

import android.app.Fragment;
import android.app.FragmentManager;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.EditTextPreference;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.support.v4.app.NavUtils;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.MenuItem;
import android.widget.TextView;

import java.util.Arrays;

public class SettingsActivity extends AppCompatActivity implements
    PreferenceFragment.OnPreferenceStartFragmentCallback,
    FragmentManager.OnBackStackChangedListener {

    public static final String DEFAULT = "iridium";
    public static final String PREF_URL = "prefUrl";
    public static final String PREF_TLE = "prefSatellite";

    static final String FMT = "http://www.celestrak.com/NORAD/elements/%s.txt";

    private static final String PREF_SYNC = "pref_sync";
    private static final String[] PREF_DB = {
        "pref_db_special",
        "pref_db_earth",
        "pref_db_comm",
        "pref_db_nav",
        "pref_db_sci",
        "pref_db_misc"
    };
    private static final String TITLE_KEY = "title";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.settings);

        final Toolbar myToolbar = findViewById(R.id.toolbar);
        setSupportActionBar(myToolbar);

        // Display the fragment as the main content.
        final FragmentManager fm = getFragmentManager();
        fm.addOnBackStackChangedListener(this);
        if (savedInstanceState == null) {
            fm.beginTransaction().replace(R.id.fragment, new SettingsFragment()).commit();
        }

        final ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.setDisplayHomeAsUpEnabled(true);
            actionBar.setDisplayShowTitleEnabled(false);
        }
    }

    @Override
    public void onRestoreInstanceState(Bundle savedInstanceState) {
        final TextView settingsTitle = findViewById(R.id.settings_title);
        settingsTitle.setText(savedInstanceState.getString(TITLE_KEY));
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        final TextView settingsTitle = findViewById(R.id.settings_title);
        outState.putString(TITLE_KEY, settingsTitle.getText().toString());

        super.onSaveInstanceState(outState);
    }

    @Override
    public void onBackStackChanged() {
        final FragmentManager fm = getFragmentManager();
        final AbstractFragment fragment = (AbstractFragment) fm.findFragmentById(R.id.fragment);
        final TextView settingsTitle = findViewById(R.id.settings_title);
        settingsTitle.setText(getString(fragment.getTitleResId()));
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            // Respond to the action bar's Up/Home button
            case android.R.id.home:
                super.onBackPressed();
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    @Override
    public boolean onPreferenceStartFragment(PreferenceFragment caller, Preference pref) {
        final String fragmentName = pref.getFragment();
        getFragmentManager()
            .beginTransaction()
            .replace(R.id.fragment, Fragment.instantiate(this, fragmentName))
            .addToBackStack(fragmentName)
            .commit();
        return true;
    }

    public abstract static class AbstractFragment extends PreferenceFragment implements
        SharedPreferences.OnSharedPreferenceChangeListener {

        abstract int getTitleResId();

        @Override
        public void onResume() {
            super.onResume();

            final SharedPreferences sharedPrefs = getPreferenceScreen().getSharedPreferences();
            sharedPrefs.registerOnSharedPreferenceChangeListener(this);
        }

        @Override
        public void onPause() {
            super.onPause();

            final SharedPreferences sharedPrefs = getPreferenceScreen().getSharedPreferences();
            sharedPrefs.unregisterOnSharedPreferenceChangeListener(this);
        }

        @Override
        public void onSharedPreferenceChanged(SharedPreferences prefs, String key) {
            if (key.equals(PREF_URL)) {
                updatePrefUrl();
            } else if (Arrays.asList(PREF_DB).contains(key)) {
                final String value = prefs.getString(key, DEFAULT);
                prefs.edit().putString(PREF_TLE, value).apply();
                getFragmentManager().popBackStack();
            }
        }

        private String getUrl() {
            final SharedPreferences prefs = getPreferenceScreen().getSharedPreferences();
            final String tle = prefs.getString(PREF_TLE, DEFAULT);
            return String.format(FMT, tle);
        }

        public void updatePrefUrlValue() {
            final String url = getUrl();
            final EditTextPreference urlPref = (EditTextPreference) findPreference(PREF_URL);
            urlPref.setText(url);
            urlPref.setSummary(url);
            final SharedPreferences prefs = getPreferenceScreen().getSharedPreferences();
            prefs.edit().putString(PREF_URL, url).apply();
        }

        protected void updatePrefUrl() {
            final SharedPreferences prefs = getPreferenceScreen().getSharedPreferences();
            final String url = getUrl();
            if (prefs.getString(PREF_URL, "").isEmpty()) {
                updatePrefUrlValue();
            } else {
                final Preference urlPref = findPreference(PREF_URL);
                urlPref.setSummary(prefs.getString(PREF_URL, url));
            }
        }
    }

    public static class SettingsFragment extends AbstractFragment {

        @Override
        int getTitleResId() {
            return R.string.settings;
        }

        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);

            // Load the preferences from an XML resource
            addPreferencesFromResource(R.xml.pref_general);
            updatePrefUrl();
        }

        @Override
        public void onResume() {
            super.onResume();

            final SharedPreferences prefs = getPreferenceScreen().getSharedPreferences();
            final Preference tlePref = findPreference(PREF_TLE);
            tlePref.setSummary(prefs.getString(PREF_TLE, DEFAULT));
            if (prefs.getBoolean(PREF_SYNC, false)) {
                updatePrefUrlValue();
            }
        }
    }

    public static class Satellite extends AbstractFragment {

        @Override
        int getTitleResId() {
            return R.string.pref_title_db;
        }

        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);

            // Load the preferences from an XML resource
            addPreferencesFromResource(R.xml.pref_satellite);
        }

        @Override
        public void onResume() {
            super.onResume();

            final SharedPreferences sharedPrefs = getPreferenceScreen().getSharedPreferences();
            final SharedPreferences.Editor editor = sharedPrefs.edit();
            editor.putBoolean(PREF_SYNC, false);
            final CheckBoxPreference pref = (CheckBoxPreference) findPreference(PREF_SYNC);
            pref.setChecked(false);
            for (String key : PREF_DB) {
                editor.putString(key, null);
            }
            editor.apply();
        }
    }
}
