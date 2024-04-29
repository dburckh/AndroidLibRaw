package com.homesoft.photo.libraw;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.hardware.HardwareBuffer;
import android.os.Build;
import android.system.ErrnoException;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Derived from https://github.com/TSGames/Libraw-Android/blob/master/app/src/main/java/com/tssystems/Libraw.java
 */

public class LibRaw implements AutoCloseable {
    static {
        System.loadLibrary("androidraw");
    }
    public static final int ROTATE_0 = 0;
    public static final int ROTATE_180 = 3;
    public static final int ROTATE_270 = 5;
    public static final int ROTATE_90 = 6;
    public static final int ROTATE_DEFAULT = -1;
    public static final int USE_CAMERA_MATRIX_NEVER = 0;
    public static final int USE_CAMERA_MATRIX_DEFAULT = 1; //Use camera matrix if useCameraWhiteBalance is set
    public static final int USE_CAMERA_MATRIX_ALWAYS = 3;

    private static int COLORSPACE_RAW=0;
    private static int COLORSPACE_SRGB=1;
    private static int COLORSPACE_ADOBE=2;
    private static int COLORSPACE_WIDE_GAMUT=3;
    private static int COLORSPACE_PRO_PHOTO=4;

    long mNativeContext;

    /**
     * @deprecated Just create LibRaw directly
     */
    public static LibRaw newInstance() {
        return new LibRaw();
    }

    public static int toDegrees(final int orientation) {
        switch (orientation) {
            case ROTATE_0:
                break;
            case ROTATE_270:
                return 270;
            case ROTATE_90:
                return 90;
            case ROTATE_180:
                return 180;
        }
        return 0;
    }

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

    public boolean isClosed() {
        return mNativeContext == 0;
    }

    public Bitmap decodeBitmap(String file, BitmapFactory.Options options) throws ErrnoException {
        int result = open(file);
        if (result != 0) {
            throw new ErrnoException("open", result);
        }
        return decodeBitmap(options);
    }
    public static Bitmap decodeBitmap(long buffer, int size, BitmapFactory.Options options) throws ErrnoException {
        try (final LibRaw libRaw = new LibRaw()){
            int result = libRaw.openBufferPtr(buffer, size);
            if(result!=0) {
                throw new ErrnoException("openBufferPtr", result);
            }
            return libRaw.decodeBitmap(options);
        }
    }

    public Bitmap decodeBitmap(int fd, BitmapFactory.Options options) throws ErrnoException {
        int result=openFd(fd);
        if(result!=0) {
            throw new ErrnoException("openFd", result);
        }
        return decodeBitmap(options);
    }

    /**
     * Basic call with some reasonable defaults
     * @param options Supports {@link BitmapFactory.Options#inPreferredConfig} = [{@link Bitmap.Config#ARGB_8888} | {@link Bitmap.Config#RGBA_F16}]
     *                {@link BitmapFactory.Options#inSampleSize} anything >= 2 means half size {@link #setHalfSize(boolean)}
     * @return A {@link Bitmap} with a {@link Bitmap.Config} of {@link Bitmap.Config#HARDWARE} for
     * API >= 29 or {@link BitmapFactory.Options#inPreferredConfig} specified in {@param options}
     * @throws ErrnoException if a LibRaw error occurs
     */
    public Bitmap decodeBitmap(BitmapFactory.Options options) throws ErrnoException {
        setQuality(3);
        setHalfSize(options != null && options.inSampleSize >= 2);
        final int rc = dcrawProcess();
        if (rc != 0) {
            throw new ErrnoException("cdrawProccess", rc);
        }
        final Bitmap b;
        if (options == null || options.inPreferredConfig == Bitmap.Config.ARGB_8888) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                b = getHardwareBitmap(HardwareBuffer.RGB_888);
            } else {
                b = getBitmap();
            }
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O && options.inPreferredConfig == Bitmap.Config.RGBA_F16) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                b= getHardwareBitmap(HardwareBuffer.RGBA_FP16);
            } else {
                b = getBitmap16();
            }
        } else {
            throw new UnsupportedOperationException("Bitamp.Config must be ARGB_8888 or RGBA_F16");
        }
        return b;
    }

    /**
     * getAvailableWhiteBalanceCoefficients filters out unfilled values in getWhiteBalanceCoefficients
     * @return a Map of non-zero white balance coefficients keyed by EXIF.LightSource type
     */
    public Map<Integer, int[]> getAvailableWhiteBalanceCoefficients() {
        final HashMap<Integer, int[]> map = new HashMap<>();
        final int[][] noTempCoefficients = getWhiteBalanceCoefficients();
        for (int i=0;i<noTempCoefficients.length;i++) {
            final int[] row = noTempCoefficients[i];
            // Only skip if _all_ values are zero
            if (!(row[0] == 0 && row[1] == 0 && row[2] == 0 && row[3] == 0)) {
                map.put(i, row);
            }
        }
        return map;
    }

    /**
     * getAvailableWhiteBalanceCtCoefficients filters out unfilled values in getWhiteBalanceCoefficients
     * and places them in a map.
     * @return a Map of color temperature in kelvin to white balance coefficients
     */
    public Map<Float, float[]> getAvailableWhiteBalanceCoefficientsWithTemps() {
        final float[][] tempCoefficients = getWhiteBalanceCoefficientsWithTemps();
        final HashMap<Float, float[]> wbList = new HashMap<>();
        for (final float[] row : tempCoefficients) {
            if (row[0] != 0) {
                wbList.put(row[0], Arrays.copyOfRange(row, 1, row.length));
            }
        }
        return wbList;
    }

    public native long init(int flags);

    /**
     * Calls recycle and deletes LibRaw
     */
    public native void recycle();
    public native int open(String file);
    public native int openBufferPtr(long ptr, int size);
    public native int openBuffer(byte[] buffer, int size);
    public native int openFd(int fd);

    public native int dcrawProcess();
    public native int dcrawProcessForced(@NonNull ByteBuffer colorCurve);

    public native void clearCancelFlag();
    public native void setCancelFlag();

    public native int getColors();
    public native int getWidth();
    public native int getHeight();
    public native int getLeftMargin();
    public native int getRightMargin();
    public native int getOrientation(); // NOT the same as EXIF orientation

    public native float[] getCameraMul();

    @NonNull
    public native float[][] getWhiteBalanceCoefficientsWithTemps();
    @NonNull
    public native int[][] getWhiteBalanceCoefficients();
    /**
     * Get a bitmap for the current image
     * @return
     */
    public native Bitmap getBitmap();
    @RequiresApi(26)
    public native Bitmap getBitmap16();

    public native void setCropBox(int left, int top, int width, int height);
    public native void setAutoScale(boolean autoScale);
    public native void setAutoBrightness(boolean autoBrightness);
    public native void setAutoWhiteBalance(boolean autoWhiteBalance);

    /**
     * Brightness factor default = 1.0.  Higher numbers are brighter
     */
    public native void setBrightness(float brightness);
    public native void setCameraWhiteBalance(boolean cameraWhiteBalance);
    public native void setCaptureScaleMul(boolean capture);
    public native void setGamma(double g1,double g2);

    /**
     * If the image should be decoded with it's dimensions halved.
     * See libraw_output_params_t.half_size
     */
    public native void setHalfSize(boolean halfSize);
    public native void setHighlightMode(int highlightMode);
    public native void setOrientation(int orientation);
    public native void setOutputColorSpace(int colorSpace);
    public native void setOutputBps(int outputBps);

    /**
     * Interpolation Quality
     * See libraw_output_params_t.user_qual for a full list
     */
    public native void setQuality(int quality);
    public native void setUserBlack(int userBlack);
    public native void setUseCameraMatrix(int useCameraMatrix); // 0 = off, 1 = if auto whitebalance, 3 = always
    public native void setUserMul(float r,float g1,float b,float g2);

    /**
     * @param automaticMaximumCalculation 0.0 = off, 0.75 = default
     */
    public native void setAutomaticMaximumCalculation(float automaticMaximumCalculation);

    /**
     * @param enabled turns this on or off
     * @param shift exposure shift in linear scale. Usable range from 0.25 (2-stop darken) to 8.0 (3-stop lighter). Default: 1.0 (no exposure shift).
     * @param preservation preserve highlights when lighten the image. Usable range from 0.0 (no preservation) to 1.0 (full preservation). 0.0 is the default value.
     */
    public native void setExposureCorrectionBeforeDemosaic(boolean enabled, float shift, float preservation);

    public static native String getCameraList(); // 0 = off, 1 = if auto whitebalance, 3 = always

    public native ByteBuffer getColorCurve();

    @RequiresApi(26)
    private native boolean drawHardwareBuffer(HardwareBuffer hardwareBuffer);

    @RequiresApi(Build.VERSION_CODES.Q)
    private Bitmap getHardwareBitmap(int format) {
        final HardwareBuffer hardwareBuffer = HardwareBuffer.create(getWidth(), getHeight(), format,
                1, HardwareBuffer.USAGE_GPU_SAMPLED_IMAGE| HardwareBuffer.USAGE_CPU_WRITE_RARELY);
        if (drawHardwareBuffer(hardwareBuffer)) {
            final Bitmap bitmap = Bitmap.wrapHardwareBuffer(hardwareBuffer, null);
            hardwareBuffer.close();
            return bitmap;
        } else {
            return null;
        }
    }
    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        if (mNativeContext != 0) {
            Log.w("LibRaw", "Failed to call close()");
            close();
        }
    }
}
