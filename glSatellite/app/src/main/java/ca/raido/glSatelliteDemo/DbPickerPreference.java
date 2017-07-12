package ca.raido.glSatelliteDemo;

import android.content.Context;
import android.preference.DialogPreference;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.RadioButton;
import android.widget.RadioGroup;

public class DbPickerPreference extends DialogPreference implements
        RadioGroup.OnCheckedChangeListener {

    static final String DEFAULT = "iridium";
    private String value_;

    public DbPickerPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected View onCreateDialogView() {
        value_ = getPersistedString(DEFAULT);
        // Inflate layout
        LayoutInflater inflater = (LayoutInflater)getContext()
                .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View view = inflater.inflate(R.layout.pref, null);
        RadioButton btn = view.findViewWithTag(value_);
        if (btn != null) {
            btn.toggle();
        }
        RadioGroup rGroup = view.findViewById(R.id.radiogrp_db);
        rGroup.setOnCheckedChangeListener(this);
        return view;
    }

    @Override
    public void onCheckedChanged(RadioGroup rGroup, int checkedId) {
        RadioButton checkedRadioButton = rGroup.findViewById(checkedId);
        boolean isChecked = checkedRadioButton.isChecked();
        if (isChecked) {
            value_ = (String)checkedRadioButton.getTag();
        }
    }

    @Override
    protected void onDialogClosed(boolean positiveResult) {
        super.onDialogClosed(positiveResult);

        if (!positiveResult) {
            return;
        }
        if (shouldPersist()) {
            persistString(value_);
        }

        notifyChanged();
    }

    @Override
    public CharSequence getSummary() {
        return getPersistedString(DEFAULT);
    }
}
