/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package ca.raido.helper;

import android.annotation.TargetApi;
import android.app.NativeActivity;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.opengl.GLUtils;
import android.os.Build;
import android.util.Log;

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;

import javax.microedition.khronos.opengles.GL10;

public class NDKHelper {

    private final NativeActivity activity;

    public NDKHelper(NativeActivity act) {
        activity = act;
    }

    public boolean isDeveloperMode() {
        final ApplicationInfo ai = activity.getApplicationInfo();
        return (ai.flags & ApplicationInfo.FLAG_DEBUGGABLE) != 0;
    }

    //
    // Load Bitmap
    // Java helper is useful decoding PNG, TIFF etc rather than linking libPng
    // etc separately
    //
    @SuppressWarnings("static-method")
    private int nextPOT(int i) {
        int pot = 1;
        while (pot < i) {
            pot <<= 1;
        }
        return pot;
    }

    @SuppressWarnings("static-method")
    private Bitmap scaleBitmap(Bitmap bitmapToScale, float newWidth, float newHeight) {
        if (bitmapToScale == null) {
            return null;
        }
        // get the original width and height
        final int width = bitmapToScale.getWidth();
        final int height = bitmapToScale.getHeight();
        // create a matrix for the manipulation
        final Matrix matrix = new Matrix();

        // resize the bit map
        matrix.postScale(newWidth / width, newHeight / height);

        // recreate the new Bitmap and set it back
        return Bitmap.createBitmap(bitmapToScale, 0, 0,
            bitmapToScale.getWidth(), bitmapToScale.getHeight(), matrix, true);
    }

    public Object loadTexture(String path) {
        final Bitmap bitmap;
        final TextureInformation info = new TextureInformation();
        try {
            String str = path;
            if (!path.startsWith("/")) {
                str = "/" + path;
            }

            final File file = new File(activity.getExternalFilesDir(null), str);
            if (file.canRead()) {
                bitmap = BitmapFactory.decodeStream(new FileInputStream(file));
            } else {
                final InputStream stream = activity.getResources().getAssets().open(path);
                bitmap = BitmapFactory.decodeStream(stream);
            }
        } catch (Exception e) {
            Log.w("NDKHelper", "Could not load a file:" + path);
            info.ret = false;
            return info;
        }

        if (bitmap != null) {
            GLUtils.texImage2D(GL10.GL_TEXTURE_2D, 0, bitmap, 0);
            info.ret = true;
            info.alphaChannel = bitmap.hasAlpha();
            info.originalWidth = getBitmapWidth(bitmap);
            info.originalHeight = getBitmapHeight(bitmap);
        }

        return info;
    }

    public Bitmap openBitmap(String path, boolean iScalePOT) {
        Bitmap bitmap = null;
        try {
            bitmap = BitmapFactory.decodeStream(activity.getResources()
                    .getAssets().open(path));
            if (iScalePOT) {
                final int originalWidth = getBitmapWidth(bitmap);
                final int originalHeight = getBitmapHeight(bitmap);
                final int width = nextPOT(originalWidth);
                final int height = nextPOT(originalHeight);
                if (originalWidth != width || originalHeight != height) {
                    // Scale it
                    bitmap = scaleBitmap(bitmap, width, height);
                }
            }

        } catch (Exception e) {
            Log.e(activity.getPackageName(), "Couldn't load a file: " + path);
        }

        return bitmap;
    }

    public int getBitmapWidth(Bitmap bmp) {
        return bmp.getWidth();
    }

    public int getBitmapHeight(Bitmap bmp) {
        return bmp.getHeight();
    }

    public void getBitmapPixels(Bitmap bmp, int[] pixels) {
        final int w = bmp.getWidth();
        final int h = bmp.getHeight();
        bmp.getPixels(pixels, 0, w, 0, 0, w, h);
    }

    public void closeBitmap(Bitmap bmp) {
        bmp.recycle();
    }

    public String getNativeLibraryDirectory(Context appContext) {
        final ApplicationInfo ai = activity.getApplicationInfo();

        Log.w("NDKHelper", "ai.nativeLibraryDir:" + ai.nativeLibraryDir);

        if ((ai.flags & ApplicationInfo.FLAG_UPDATED_SYSTEM_APP) != 0
            || (ai.flags & ApplicationInfo.FLAG_SYSTEM) == 0) {
            return ai.nativeLibraryDir;
        }
        return "/system/lib/";
    }

    public String getApplicationName() {
        final PackageManager pm = activity.getPackageManager();
        try {
            final ApplicationInfo ai = pm.getApplicationInfo(activity.getPackageName(), 0);
            return (String) pm.getApplicationLabel(ai);
        } catch (final PackageManager.NameNotFoundException e) {
            return "(unknown)";
        }
    }

    public int getNativeAudioBufferSize() {
        final AudioManager am = (AudioManager) activity.getSystemService(Context.AUDIO_SERVICE);
        final String framesPerBuffer = am.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER);
        return Integer.parseInt(framesPerBuffer);
    }

    public int getNativeAudioSampleRate() {
        return AudioTrack.getNativeOutputSampleRate(AudioManager.STREAM_SYSTEM);
    }

    public class TextureInformation {
        boolean ret;
        boolean alphaChannel;
        int originalWidth;
        int originalHeight;
        Object image;
    }
}
