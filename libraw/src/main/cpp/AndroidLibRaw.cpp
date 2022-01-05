//
// Created by dburc on 1/2/2022.
//

#include "AndroidLibRaw.h"
#include <android/log.h>
#include <android/bitmap.h>

#define P1 imgdata.idata
#define S  imgdata.sizes
#define O  imgdata.params
#define C  imgdata.color
#define IO libraw_internal_data.internal_output_params
#define SHORT2FLOAT 65535.0f;
#define COLORS 3

AndroidLibRaw::AndroidLibRaw(unsigned int flags):LibRaw(flags) {
}

jobject AndroidLibRaw::doGetBitmap(JNIEnv* env, jobject bitmap, const char* configName, int32_t configType, const std::function<void (void*, int const)>& _copy) {
    if (imgdata.idata.colors != COLORS) {
        __android_log_print(ANDROID_LOG_WARN,"libraw","expected 3 colors, got %i",P1.colors);
        return nullptr;
    }
    if (imgdata.image == nullptr) {
        __android_log_write(ANDROID_LOG_WARN,"libraw","No image data.  Did you call dcrawProcess?");
        return nullptr;
    }
    int width = imgdata.sizes.iwidth;
    int height = imgdata.sizes.iheight;

    if (bitmap != nullptr) {
        AndroidBitmapInfo info;
        AndroidBitmap_getInfo(env, bitmap, &info);
        if (info.width != width || info.height != height ||
            info.format != configType) {
            //This bitmap can't be used
            bitmap = nullptr;
        }
    }
    if (bitmap == nullptr) {
        jobject bitmapConfig = getConfigByName(env, configName);
        bitmap = createBitmap(env, bitmapConfig, width, height);
    }
    void* addrPtr;
    AndroidBitmap_lockPixels(env,bitmap, &addrPtr);
    _copy(addrPtr, width * height);
    AndroidBitmap_unlockPixels(env,bitmap);
    return bitmap;
}

jobject AndroidLibRaw::getBitmap(JNIEnv* env, jobject bitmap) {
    return doGetBitmap(env, bitmap, "ARGB_8888", ANDROID_BITMAP_FORMAT_RGBA_8888, [this](void* bitmapPtr, int const pixels) {
        auto pixelPtr = (unsigned char*)bitmapPtr;
        for(int pixel=0; pixel < pixels;pixel++) {
            for (int c=0;c<COLORS;c++) {
                *pixelPtr = imgdata.color.curve[imgdata.image[pixel][c]] >> 8;
                pixelPtr++;
            }
            *pixelPtr = 0xff;
            pixelPtr++;
        }
    });
}

jobject AndroidLibRaw::getBitmap16(JNIEnv *env, jobject bitmap) {
    return doGetBitmap(env, bitmap, "RGBA_F16", ANDROID_BITMAP_FORMAT_RGBA_F16, [this](void* bitmapPtr, int const pixels) {
        auto pixelPtr = (__fp16 *) bitmapPtr;
        for (int pixel = 0; pixel < pixels; pixel++) {
            for (int c = 0; c < COLORS; c++) {
                *pixelPtr = imgdata.color.curve[imgdata.image[pixel][c]] / SHORT2FLOAT;
                pixelPtr++;
            }
            *pixelPtr = 1.0;
            pixelPtr++;
        }
    });
}

void AndroidLibRaw::buildColorCurve() {
    //Copied from copy_mem_image()
    if (libraw_internal_data.output_data.histogram)
    {
        int perc, val, total, t_white = 0x2000, c;
        perc = S.width * S.height * O.auto_bright_thr;
        if (IO.fuji_width)
            perc /= 2;
        if (!((O.highlight & ~2) || O.no_auto_bright))
            for (t_white = c = 0; c < P1.colors; c++)
            {
                for (val = 0x2000, total = 0; --val > 32;)
                    if ((total += libraw_internal_data.output_data.histogram[c][val]) >
                        perc)
                        break;
                if (t_white < val)
                    t_white = val;
            }
        gamma_curve(O.gamm[0], O.gamm[1], 2, (t_white << 3) / O.bright);
    }
}

jobject AndroidLibRaw::getColorCurve(JNIEnv* env) {
    auto size = sizeof imgdata.color.curve;
    void* c = malloc(size);
    memcpy(c, imgdata.color.curve, size);
    return env->NewDirectByteBuffer(c, size);
}
void AndroidLibRaw::setColorCurve(JNIEnv *env, jobject byteBuffer) {
    auto c = env->GetDirectBufferAddress(byteBuffer);
    auto size = env->GetDirectBufferCapacity(byteBuffer);
    memcpy(imgdata.color.curve, c, size);
}

jobject AndroidLibRaw::getConfigByName(JNIEnv* env, const char* name) {
    jclass clBitmapConfig = env->FindClass("android/graphics/Bitmap$Config");
    jfieldID fidARGB_8888 = env->GetStaticFieldID(clBitmapConfig, name,
                                                  "Landroid/graphics/Bitmap$Config;");
    return env->GetStaticObjectField(clBitmapConfig, fidARGB_8888);
}

jobject AndroidLibRaw::createBitmap(JNIEnv* env, jobject config, jint width, jint height) {
    jclass clBitmap = env->FindClass("android/graphics/Bitmap");
    jmethodID midCreateBitmap = env->GetStaticMethodID(clBitmap, "createBitmap",
                                                       "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
    return env->CallStaticObjectMethod(clBitmap, midCreateBitmap, width, height, config);
}

void AndroidLibRaw::setCaptureScaleMul(bool capture) {
    if (capture) {
        if (mScaleMul == nullptr) {
            mScaleMul = new float[4];
        }
    } else {
        if (mScaleMul) {
            free(mScaleMul);
            mScaleMul = nullptr;
        }
    }
}

void AndroidLibRaw::scale_colors_loop(float scale_mul[4]) {
    if (mScaleMul && scale_mul != mScaleMul) {
        //store this info off for later user
        memcpy(mScaleMul, scale_mul, sizeof (float[4]));
    }
    LibRaw::scale_colors_loop(scale_mul);
}

void AndroidLibRaw::preScaleCallback(void* ctx) {
    auto androidLibRaw = (AndroidLibRaw*)ctx;
    if (androidLibRaw->mScaleMul) {
        androidLibRaw->scale_colors_loop(androidLibRaw->mScaleMul);
    }
}

/**
 * Dirty little hack to force LibRaw to use the same white balance from a previous dcraw_process
 * @param env
 * @param colorCurve
 * @return
 */
jint AndroidLibRaw::dcrawProcessForced(JNIEnv* env, jobject colorCurve) {
    callbacks.pre_scalecolors_cb = preScaleCallback;
    imgdata.params.no_auto_scale=true;
    int rc = dcraw_process();
    imgdata.params.no_auto_scale=false;
    callbacks.pre_scalecolors_cb = nullptr;
    if (rc == 0) {
        setColorCurve(env, colorCurve);
    }
    return rc;
}



