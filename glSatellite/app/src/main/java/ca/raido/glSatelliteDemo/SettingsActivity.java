package ca.raido.glSatelliteDemo;

import android.app.ActionBar;
import android.app.Activity;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.preference.EditTextPreference;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.support.v4.app.NavUtils;
import android.view.MenuItem;

public class SettingsActivity extends Activity {

    public static final String PREF_URL = "prefUrl";
    public static final String PREF_TLE = "prefSatellite";
    static final String FMT = "http://www.celestrak.com/NORAD/elements/%s.txt";

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
            SharedPreferences prefs = getPrefs();
            prefs.edit().putString(PREF_URL, url).apply();
            return url;
        }

        private void updatePrefUrl() {
            SharedPreferences prefs = getPrefs();
            String url = getUrl();
            if (prefs.getString(PREF_URL, "").isEmpty()) {
                updatePrefUrlValue();
            }
            Preference urlPref = findPreference(PREF_URL);
            urlPref.setSummary(prefs.getString(PREF_URL, url));
        }

        private String getUrl() {
            SharedPreferences prefs = getPrefs();
            String tle = prefs.getString(PREF_TLE, DbPickerPreference.DEFAULT);
            return String.format(FMT, tle);
        }

        @Override
        public void onSharedPreferenceChanged(SharedPreferences prefs,
                String key) {
            if (key.equals(PREF_URL)) {
                updatePrefUrl();
            }
            if (key.equals(PREF_TLE)) {
                updatePrefUrlValue();
            }
        }

        @Override
        public void onResume() {
            super.onResume();
            // Set up a listener whenever a key changes
            getPrefs().registerOnSharedPreferenceChangeListener(this);
        }

        @Override
        public void onPause() {
            super.onPause();
            // Set up a listener whenever a key changes
            getPrefs().unregisterOnSharedPreferenceChangeListener(this);
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
}
