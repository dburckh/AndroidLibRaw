package com.homesoft.photo.util;

import android.graphics.PointF;
import android.media.ExifInterface;

public class ImageUtil {
    public static PointF getRotationalPoint(final int width, final int height, final int degrees) {
        int mod = degrees % 360;
        if (mod < 0) {
            mod += 360;
        }
        switch (mod) {
            case 90: {
                float point = Math.min(width, height) / 2f;
                return new PointF(point, point);
            }
            case 180:
                return new PointF(width/2f, height/2f);
            case 270:
                float point = Math.max(width, height) / 2f;
                return new PointF(point, point);
            case 0:
                return new PointF();
            default:
                throw new UnsupportedOperationException("Angle must be multiple of 90");
        }
    }

    public static int getDegrees(final int orientation) {
        switch (orientation) {
            case ExifInterface.ORIENTATION_ROTATE_90:
                return 90;
            case ExifInterface.ORIENTATION_ROTATE_180:
                return 180;
            case ExifInterface.ORIENTATION_ROTATE_270:
                return 270;
            default:
                return 0;
        }
    }
}
