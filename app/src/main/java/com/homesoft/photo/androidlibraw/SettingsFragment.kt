package com.homesoft.photo.androidlibraw

import android.content.Context
import android.os.Build
import android.os.Bundle
import android.view.View
import androidx.appcompat.app.AppCompatActivity
import androidx.preference.ListPreference
import androidx.preference.PreferenceFragmentCompat
import androidx.preference.PreferenceManager

class SettingsFragment : PreferenceFragmentCompat() {
    private lateinit var colorSpacePref : ListPreference

    override fun onCreatePreferences(savedInstanceState: Bundle?, rootKey: String?) {
        setPreferencesFromResource(R.xml.preferences, rootKey)
        colorSpacePref = findPreference(PREF_COLOR_SPACE)!!
        updateColorSpace(getColorSpaceId(requireContext()))
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            colorSpacePref.setOnPreferenceChangeListener {_, newValue ->
                if (newValue is String) {
                    updateColorSpace(newValue)
                }
                true
            }
        } else {
            colorSpacePref.isEnabled = false
        }
    }

    private fun updateColorSpace(getColorSpaceId : String) {
        val index = colorSpacePref.entryValues.indexOf(getColorSpaceId)
        colorSpacePref.summary = if (index >= 0) {colorSpacePref.entries[index]} else {null}
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        (requireActivity() as AppCompatActivity).supportActionBar?.let {
            it.setDisplayHomeAsUpEnabled(true)
            it.title = getString(R.string.settings)
        }
    }

    companion object {
        const val PREF_COLOR_SPACE = "colorSpace"
        fun getColorSpaceId(context: Context):String {
            val prefs = PreferenceManager.getDefaultSharedPreferences(context)
            val def = context.resources.getString(R.string.defaultColorSpace)
            return prefs.getString(PREF_COLOR_SPACE, def) as String
        }
    }
}
