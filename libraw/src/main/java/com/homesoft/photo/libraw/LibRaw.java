package com.homesoft.photo.libraw;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Build;
import android.util.Log;

import androidx.annotation.RequiresApi;

/**
 * Derived from https://github.com/TSGames/Libraw-Android/blob/master/app/src/main/java/com/tssystems/Libraw.java
 */

public class LibRaw implements AutoCloseable {
    static {
        System.loadLibrary("libraw");
    }
    public static int USE_CAMERA_MATRIX_NEVER = 0;
    public static int USE_CAMERA_MATRIX_DEFAULT = 1; //Use camera matrix if useCameraWhiteBalance is set
    public static int USE_CAMERA_MATRIX_ALWAYS = 3;

    private static int COLORSPACE_RAW=0;
    private static int COLORSPACE_SRGB=1;
    private static int COLORSPACE_ADOBE=2;
    private static int COLORSPACE_WIDE_GAMUT=3;
    private static int COLORSPACE_PRO_PHOTO=4;

    long mNativeContext;

    public LibRaw() {
        this(0);
    }
    public LibRaw(int flags) {
        mNativeContext = init(flags);
    }

    public void close() {
        recycle();
        mNativeContext = 0;
    }

    public Bitmap decodeAsBitmap(String file, BitmapFactory.Options options){
        int result = open(file);
        if (result != 0) {
            return null;
        }
        return decodeAsBitmap(options);
    }
    public static Bitmap decodeAsBitmap(long buffer, int size, BitmapFactory.Options options){
        try (final LibRaw libRaw = new LibRaw()){
            int result = libRaw.openBufferPtr(buffer, size);
            if(result!=0) {
                return null;
            }
            return libRaw.decodeAsBitmap(options);
        }
    }

    public Bitmap decodeAsBitmap(int fd, BitmapFactory.Options options){
        int result=openFd(fd);
        if(result!=0) {
            return null;
        }
        return decodeAsBitmap(options);
    }

    public Bitmap decodeAsBitmap(BitmapFactory.Options options) {
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

    public native long init(int flags);
    public native void recycle();
    public native int open(String file);
    public native int openBufferPtr(long ptr, int size);
    public native int openBuffer(byte[] buffer, int size);
    public native int openFd(int fd);

    public native int getWidth();
    public native int getHeight();
    public native int getLeftMargin();
    public native int getRightMargin();
    public native int getOrientation(); // NOT the same as EXIF orientation
    public native int getColors();
    public native Bitmap getBitmap();
    @RequiresApi(26)
    public native Bitmap getBitmap16();
    public native void setCropBox(int left, int top, int width, int height);
    public native void setUserMul(float r,float g1,float b,float g2);
    public native void setAutoWhiteBalance(boolean autoWhiteBalance);
    public native void setCameraWhiteBalance(boolean cameraWhiteBalance);
    public native void setHighlightMode(int highlightMode);
    public native void setAutoBrightness(boolean autoBrightness);
    public native void setOrientation(int orientation);
    public native void setOutputColorSpace(int colorSpace);
    public native void setOutputBps(int outputBps);
    public native void setQuality(int quality);
    public native void setHalfSize(boolean halfSize);
    public native void setGamma(double g1,double g2);
    public native void setUseCameraMatrix(int useCameraMatrix); // 0 = off, 1 = if auto whitebalance, 3 = always
    public static native String getCameraList(); // 0 = off, 1 = if auto whitebalance, 3 = always

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        if (mNativeContext != 0) {
            Log.w("LibRaw", "Failed to call close()");
            close();
        }
    }
}
