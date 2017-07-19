package ca.raido.glSatelliteDemo;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import android.app.Instrumentation;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.LargeTest;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;
import android.view.WindowManager;

import static android.support.test.espresso.Espresso.onView;
import static android.support.test.espresso.action.ViewActions.click;
import static android.support.test.espresso.matcher.ViewMatchers.withText;

@RunWith(AndroidJUnit4.class)
@LargeTest
public class SettingsBehaviourTest {

    @Rule
    public ActivityTestRule<SettingsActivity> mActivityRule = new ActivityTestRule<>(
        SettingsActivity.class);

    @Test
    public void clickPreferenceDialog() throws InterruptedException {
        // Unlock the device
        final Instrumentation instrumentation = InstrumentationRegistry.getInstrumentation();
        instrumentation.runOnMainSync(new Runnable() {
            @Override
            public void run() {
                mActivityRule.getActivity().getWindow().addFlags(
                    WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED |
                        WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON |
                        WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
            }
        });
        instrumentation.waitForIdleSync();
        Thread.sleep(500);

        onView(withText("Satellites")).perform(click());
        instrumentation.waitForIdleSync();

        onView(withText("Communications")).perform(click());
        instrumentation.waitForIdleSync();
        Thread.sleep(2000);

        onView(withText("Cancel")).perform(click());
        instrumentation.waitForIdleSync();
        Thread.sleep(500);
    }
}
