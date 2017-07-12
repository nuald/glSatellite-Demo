package ca.raido.glSatelliteDemo;

import android.app.ActionBar;
import android.app.Activity;
import android.app.Fragment;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.EditTextPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.support.v4.app.NavUtils;
import android.view.MenuItem;

import java.util.Arrays;

public class SettingsActivity extends Activity implements
    PreferenceFragment.OnPreferenceStartFragmentCallback {

    public static final String DEFAULT = "iridium";
    public static final String PREF_URL = "prefUrl";
    public static final String PREF_TLE = "prefSatellite";
    static final String FMT = "http://www.celestrak.com/NORAD/elements/%s.txt";
    private static final String PREF_SYNC = "pref_sync";
    private static String[] PREF_DB = {
        "pref_db_special",
        "pref_db_earth",
        "pref_db_comm",
        "pref_db_nav",
        "pref_db_sci",
        "pref_db_misc"
    };

    public static class SettingsFragment extends PreferenceFragment implements
            OnSharedPreferenceChangeListener {
        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);

            // Load the preferences from an XML resource
            addPreferencesFromResource(R.xml.pref_general);
            updatePrefUrl();
        }

        private SharedPreferences getPrefs() {
            return getPreferenceScreen().getSharedPreferences();
        }

        public String updatePrefUrlValue() {
            String url = getUrl();
            EditTextPreference urlPref = (EditTextPreference)findPreference(PREF_URL);
            urlPref.setText(url);
            urlPref.setSummary(url);
            SharedPreferences prefs = getPrefs();
            prefs.edit().putString(PREF_URL, url).apply();
            return url;
        }

        private void updatePrefUrl() {
            SharedPreferences prefs = getPrefs();
            String url = getUrl();
            if (prefs.getString(PREF_URL, "").isEmpty()) {
                updatePrefUrlValue();
            } else {
                Preference urlPref = findPreference(PREF_URL);
                urlPref.setSummary(prefs.getString(PREF_URL, url));
            }
        }

        private String getUrl() {
            final SharedPreferences prefs = getPrefs();
            String tle = prefs.getString(PREF_TLE, DEFAULT);
            return String.format(FMT, tle);
        }

        @Override
        public void onSharedPreferenceChanged(SharedPreferences prefs, String key) {
            if (key.equals(PREF_URL)) {
                updatePrefUrl();
            }
        }

        @Override
        public void onResume() {
            super.onResume();
            final SharedPreferences prefs = getPrefs();
            Preference tlePref = findPreference(PREF_TLE);
            tlePref.setSummary(prefs.getString(PREF_TLE, DEFAULT));
            if (prefs.getBoolean(PREF_SYNC, false)) {
                updatePrefUrlValue();
            }
            // Set up a listener whenever a key changes
            prefs.registerOnSharedPreferenceChangeListener(this);
        }

        @Override
        public void onPause() {
            super.onPause();
            // Set up a listener whenever a key changes
            getPrefs().unregisterOnSharedPreferenceChangeListener(this);
        }
    }

    public static class Satellite extends PreferenceFragment implements
        OnSharedPreferenceChangeListener {

        @Override
        public void onSharedPreferenceChanged(SharedPreferences prefs, String key) {
            if (Arrays.asList(PREF_DB).contains(key)) {
                String value = prefs.getString(key, DEFAULT);
                prefs.edit().putString(PREF_TLE, value).apply();
                getFragmentManager().popBackStack();
            }
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
            // Set up a listener whenever a key changes
            final SharedPreferences sharedPrefs = getPreferenceScreen().getSharedPreferences();
            final SharedPreferences.Editor editor = sharedPrefs.edit();
            editor.putBoolean(PREF_SYNC, false);
            final CheckBoxPreference pref = (CheckBoxPreference)findPreference(PREF_SYNC);
            pref.setChecked(false);
            for (String key: PREF_DB) {
                editor.putString(key, null);
            }
            editor.apply();
            sharedPrefs.registerOnSharedPreferenceChangeListener(this);
        }

        @Override
        public void onPause() {
            super.onPause();
            // Set up a listener whenever a key changes
            final SharedPreferences sharedPrefs = getPreferenceScreen().getSharedPreferences();
            sharedPrefs.unregisterOnSharedPreferenceChangeListener(this);
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Display the fragment as the main content.
        getFragmentManager().beginTransaction()
                .replace(android.R.id.content, new SettingsFragment()).commit();
        ActionBar actionBar = getActionBar();
        if (actionBar != null) {
            actionBar.setDisplayHomeAsUpEnabled(true);
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
        // Respond to the action bar's Up/Home button
        case android.R.id.home:
            NavUtils.navigateUpFromSameTask(this);
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public boolean onPreferenceStartFragment(PreferenceFragment caller, Preference pref) {
        final String fragmentName = pref.getFragment();
        getFragmentManager()
            .beginTransaction()
            .replace(android.R.id.content, Fragment.instantiate(this, fragmentName))
            .addToBackStack(fragmentName)
            .commit();
        return true;
    }
}
