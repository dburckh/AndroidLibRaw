package com.homesoft.photo.libraw;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.ColorSpace;
import android.hardware.HardwareBuffer;
import android.os.Build;
import android.system.ErrnoException;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;

import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.HashMap;
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

    public static final int COLORSPACE_RAW=0;
    public static final int COLORSPACE_SRGB=1;
    public static final int COLORSPACE_ADOBE=2;
    public static final int COLORSPACE_WIDE_GAMUT=3;
    public static final int COLORSPACE_PRO_PHOTO=4;
    public static final int COLORSPACE_XYZ=5;
    public static final int COLORSPACE_ACES=6;
    public static final int COLORSPACE_DCI_P3=7;
    public static final int COLORSPACE_REC_2020=8;

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
        final Bitmap.Config config = options == null ? Bitmap.Config.ARGB_8888 : options.inPreferredConfig;
        return getBitmap(config);
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
     * @deprecated Use {@link #getBitmap(Bitmap.Config)}
     */
    public Bitmap getBitmap() {
        return getBitmap(Bitmap.Config.ARGB_8888);
    }
    /**
     * Get a bitmap for the current image
     * @deprecated Use {@link #getBitmap(Bitmap.Config)}
     */
    @RequiresApi(26)
    public Bitmap getBitmap16() {
        return getBitmap(Bitmap.Config.RGBA_F16);
    }
    /**
     * Return a mutable {@link Bitmap}.
     * Requires that {@link #open(String)} or similar and {@link #dcrawProcess()} have been called.
     * @param bitmapConfig Currently only {@link Bitmap.Config#ARGB_8888}
     *                     and {@link Bitmap.Config#RGBA_F16}
     */
    public native Bitmap getMutableBitmap(Bitmap.Config bitmapConfig) throws IllegalArgumentException;

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
    public native int getOutputColorSpace();
    public native void setOutputColorSpace(int colorSpace);
    public native void setOutputBps(int outputBps);

    /**
     *  Controls FBDD noise reduction before demosaic.
     *  See libraw_output_params_t.fbdd_noiserd
     *
     * @param mode  0 - do not use FBDD noise reduction
     *              1 - light FBDD reduction
     *              2 (and more) - full FBDD reduction
     */
    public native void setFbddNoiseReduction(int mode);

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

    /**
     * Decode to a HardwareBuffer
     * @param format must be {@link HardwareBuffer#RGB_888} or {@link HardwareBuffer#RGBA_FP16}
     * @return null if no image is available
     */
    @RequiresApi(26)
    @Nullable
    public HardwareBuffer getHardwareBuffer(int format) {
        if (format != HardwareBuffer.RGB_888 && format != HardwareBuffer.RGBA_FP16) {
            throw new IllegalArgumentException("Unsupported format: " + format);
        }
        final HardwareBuffer hardwareBuffer = HardwareBuffer.create(getWidth(), getHeight(), format,
                1, HardwareBuffer.USAGE_GPU_SAMPLED_IMAGE| HardwareBuffer.USAGE_CPU_WRITE_RARELY);
        if (drawHardwareBuffer(hardwareBuffer)) {
            return hardwareBuffer;
        } else {
            hardwareBuffer.close();
            return null;
        }
    }


    /**
     * Map the LibRaw color space id the to the Android {@link ColorSpace}
     * This is called by C
     * @param colorSpaceId
     * @return
     */
    @RequiresApi(Build.VERSION_CODES.O)
    @Nullable
    public static ColorSpace getColorSpace(int colorSpaceId) {
        return switch (colorSpaceId) {
            case COLORSPACE_SRGB -> ColorSpace.get(ColorSpace.Named.SRGB);
            case COLORSPACE_ADOBE -> ColorSpace.get(ColorSpace.Named.ADOBE_RGB);
            case COLORSPACE_PRO_PHOTO -> ColorSpace.get(ColorSpace.Named.PRO_PHOTO_RGB);
            //case COLORSPACE_ACES -> ColorSpace.get(ColorSpace.Named.ACES);
            case COLORSPACE_DCI_P3 -> ColorSpace.get(ColorSpace.Named.DISPLAY_P3);
            case COLORSPACE_REC_2020 -> ColorSpace.get(ColorSpace.Named.BT2020);
            default -> null;
        };
    }

    /**
     * Return a {@link Bitmap.Config#HARDWARE} {@link Bitmap}.
     * Requires the image has already been loaded and decoded.
     * @param format must be {@link HardwareBuffer#RGB_888} or {@link HardwareBuffer#RGBA_FP16}
     */
    @RequiresApi(Build.VERSION_CODES.Q)
    @Nullable
    public Bitmap getHardwareBitmap(int format) {
        final HardwareBuffer hardwareBuffer = getHardwareBuffer(format);
        if (hardwareBuffer == null) {
            return null;
        }
        final ColorSpace colorSpace = getColorSpace(getOutputColorSpace());
        final Bitmap bitmap = Bitmap.wrapHardwareBuffer(hardwareBuffer, colorSpace);
        hardwareBuffer.close();
        return bitmap;
    }

    /**
     * Return a {@link Bitmap} in the most efficient form.
     * Requires that {@link #open(String)} or similar and {@link #dcrawProcess()} have been called.
     * Note: {@link Bitmap.Config#ARGB_8888} is returned as {@link HardwareBuffer#RGB_888}
     *
     * @param bitmapConfig Currently only {@link Bitmap.Config#ARGB_8888}
     *                     and {@link Bitmap.Config#RGBA_F16}
     * @return <code>null</code> if the image has not been loaded
     */
    public Bitmap getBitmap(Bitmap.Config bitmapConfig) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            switch (bitmapConfig) {
                case ARGB_8888 -> {
                    return getHardwareBitmap(HardwareBuffer.RGB_888);
                }
                case RGBA_F16 -> {
                    return getHardwareBitmap(HardwareBuffer.RGBA_FP16);
                }
                default -> throw new IllegalArgumentException("Bitmap.Config must be ARGB_8888 or RGBA_F16");
            }
        } else {
            return getMutableBitmap(bitmapConfig);
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
