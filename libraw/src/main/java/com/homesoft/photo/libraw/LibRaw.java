package com.homesoft.photo.libraw;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Build;

import androidx.annotation.RequiresApi;

/**
 * Derived from https://github.com/TSGames/Libraw-Android/blob/master/app/src/main/java/com/tssystems/Libraw.java
 */

public class LibRaw {
    static {
        System.loadLibrary("libraw");
    }
    private static int COLORSPACE_RAW=0;
    private static int COLORSPACE_SRGB=1;
    private static int COLORSPACE_ADOBE=2;
    private static int COLORSPACE_WIDE_GAMUT=3;
    private static int COLORSPACE_PRO_PHOTO=4;
    public static Bitmap decodeAsBitmap(String file, BitmapFactory.Options options){
        try {
            int result = open(file);
            if (result != 0) {
                return null;
            }
            return decodeAsBitmap(options);
        } finally {
            cleanup();
        }
    }
    public static Bitmap decodeAsBitmap(long buffer, int size, BitmapFactory.Options options){
        try {
            int result = openBufferPtr(buffer, size);
            if(result!=0) {
                return null;
            }
            return decodeAsBitmap(options);
        } finally {
            cleanup();
        }
    }

    public static Bitmap decodeAsBitmap(int fd, BitmapFactory.Options options){
        //Log.d("libraw","openFd");
        try {
            int result=openFd(fd);
            if(result!=0) {
                return null;
            }
            return decodeAsBitmap(options);
        } finally {
            cleanup();
        }
    }

    public static Bitmap decodeAsBitmap(BitmapFactory.Options options) {
        //setQuality(12);
        setQuality(3);
        //Android only supports sRGB
        setHalfSize(options != null && options.inSampleSize >= 2);
        final Bitmap b;
        if (options == null || options.inPreferredConfig == Bitmap.Config.ARGB_8888) {
            b = getBitmap();
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O && options.inPreferredConfig == Bitmap.Config.RGBA_F16) {
            b = getBitmap16();
        } else {
            throw new UnsupportedOperationException("Bitamp.Config must be ARGB_8888 or RGBA_F16");
        }
        return b;
    }

    public static native int open(String file);
    public static native int openBufferPtr(long ptr, int size);
    public static native int openBuffer(byte[] buffer, int size);
    public static native int openFd(int fd);
    public static native void cleanup();
    public static native int getBitmapWidth();
    public static native int getBitmapHeight();
    public static native int getWidth();
    public static native int getHeight();
    public static native int getOrientation();
    public static native int getColors();
    public static native Bitmap getBitmap();
    @RequiresApi(26)
    public static native Bitmap getBitmap16();
    public static native void setCropBox(int top, int left, int width, int height);
    public static native void setUserMul(float r,float g1,float b,float g2);
    public static native void setAutoWhitebalance(boolean autoWhitebalance);
    public static native void setHighlightMode(int highlightMode);
    public static native void setAutoBrightness(boolean autoBrightness);
    public static native void setOrientation(int orientation);
    public static native void setOutputColorSpace(int colorSpace);
    public static native void setOutputBps(int outputBps);
    public static native void setQuality(int quality);
    public static native void setHalfSize(boolean halfSize);
    public static native void setGamma(double g1,double g2);
    public static native void setUseCameraMatrix(int useCameraMatrix); // 0 = off, 1 = if auto whitebalance, 3 = always
    public static native String getCameraList(); // 0 = off, 1 = if auto whitebalance, 3 = always
}
