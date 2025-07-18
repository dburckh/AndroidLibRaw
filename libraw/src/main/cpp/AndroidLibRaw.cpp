//
// Created by dburc on 1/2/2022.
//

#include "AndroidLibRaw.h"
#include "common.h"
#include <android/bitmap.h>
#include <android/native_window_jni.h>

#define P1 imgdata.idata
#define S  imgdata.sizes
#define O  imgdata.params
#define C  imgdata.color
#define IO libraw_internal_data.internal_output_params

#define SHORT2FLOAT 65535.0f
#define COLORS 3
#define CURVE_SIZE 0x10000
#define CURVE8 \
    unsigned char curve[CURVE_SIZE]; \
    for (int i=0;i<CURVE_SIZE;i++) { \
        curve[i] = imgdata.color.curve[i] >> 8; \
    } \

#define CURVE16 \
    __fp16 curve[CURVE_SIZE]; \
    for (int i=0;i<CURVE_SIZE;i++) { \
        curve[i] = imgdata.color.curve[i] / SHORT2FLOAT; \
    } \

#define PIXEL_LOOP \
    for (int c=0;c<COLORS;c++) { \
        *ptr = curve[imgdata.image[pixel][c]]; \
        ptr++; \
    }  \

#define PIXEL8A_LOOP PIXEL_LOOP \
    *ptr = 0xff; \
    ptr++; \

#define PIXEL16A_LOOP PIXEL_LOOP \
    *ptr = 1.0; \
    ptr++; \

jfieldID contextFieldID = nullptr;

AndroidLibRaw* getLibRaw(JNIEnv* env, jobject jLibRaw) {
    if (contextFieldID == nullptr) {
        contextFieldID = env->GetFieldID(env->GetObjectClass(jLibRaw), "mNativeContext", "J");
    }
    return (AndroidLibRaw*)env->GetLongField(jLibRaw, contextFieldID);
}
AndroidLibRaw::AndroidLibRaw(unsigned int flags):LibRaw(flags) {
}

jobject AndroidLibRaw::doGetBitmap(JNIEnv* env, jobject bitmapConfig, void (AndroidLibRaw::*copyPtr)(void*, int)) {
    if (imgdata.idata.colors != COLORS) {
        __android_log_print(ANDROID_LOG_ERROR,"libraw","expected 3 colors, got %i", P1.colors);
        return nullptr;
    }
    if (imgdata.image == nullptr) {
        __android_log_write(ANDROID_LOG_ERROR,"libraw","No image data.  Did you call dcrawProcess?");
        return nullptr;
    }
    int width = imgdata.sizes.iwidth;
    int height = imgdata.sizes.iheight;

    jobject bitmap = createBitmap(env, bitmapConfig, width, height);
    void* addrPtr;
    AndroidBitmap_lockPixels(env,bitmap, &addrPtr);
    (*this.*copyPtr)(addrPtr, width * height);
    AndroidBitmap_unlockPixels(env,bitmap);
    return bitmap;
}
void AndroidLibRaw::copy8(void *bitmapPtr, const int pixels) {
    auto ptr = (unsigned char*)bitmapPtr;
    CURVE8
    for(int pixel=0; pixel < pixels;pixel++) {
        PIXEL8A_LOOP
    }
}
void AndroidLibRaw::copy16(void *bitmapPtr, const int pixels) {
    auto ptr = (__fp16 *) bitmapPtr;
    CURVE16
    for (int pixel = 0; pixel < pixels; pixel++) {
        PIXEL16A_LOOP
    }
}

jobject AndroidLibRaw::getBitmap(JNIEnv* env, jobject bitmapConfig) {
    auto bitmapConfigClass = env->GetObjectClass(bitmapConfig);
    auto nameId = env->GetMethodID(bitmapConfigClass, "name", "()Ljava/lang/String;");
    jstring jName = static_cast<jstring>(env->CallObjectMethod(bitmapConfig, nameId));
    auto cName = env->GetStringUTFChars(jName, JNI_FALSE);
    jobject bitmap;
    if (strcmp("ARGB_8888", cName) == 0) {
        bitmap = doGetBitmap(env, bitmapConfig, &AndroidLibRaw::copy8);
    } else if (strcmp("RGBA_F16", cName) == 0) {
        bitmap = doGetBitmap(env, bitmapConfig, &AndroidLibRaw::copy16);
    } else {
        auto exceptionClass = env->FindClass("java/lang/IllegalArgumentException");
        env->ThrowNew(exceptionClass, cName);
        bitmap = nullptr;
    }
    env->ReleaseStringUTFChars(jName, cName);
    return bitmap;
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
    auto byteBufferClass = env->FindClass("java/nio/ByteBuffer");
    auto allocateDirectMethod = env->GetStaticMethodID(byteBufferClass, "allocateDirect", "(I)Ljava/nio/ByteBuffer;");
    auto byteBuffer = env->CallStaticObjectMethod(byteBufferClass, allocateDirectMethod, (jint)size);
    if (byteBuffer) {
        auto c = env->GetDirectBufferAddress(byteBuffer);
        memcpy(c, imgdata.color.curve, size);
    }
    return byteBuffer;
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
    if (android_get_device_api_level() >= 26 && imgdata.params.output_color != 1) {
        auto midCreateBitmap = env->GetStaticMethodID(clBitmap, "createBitmap",
                                                           "(IILandroid/graphics/Bitmap$Config;ZLandroid/graphics/ColorSpace;)Landroid/graphics/Bitmap;");
        auto classLibRaw = env->FindClass("com/homesoft/photo/libraw/LibRaw");
        auto midGetColorSpace = env->GetStaticMethodID(classLibRaw, "getColorSpace",
                                                       "(I)Landroid/graphics/ColorSpace;");
        auto colorSpace = env->CallStaticObjectMethod(classLibRaw, midGetColorSpace, imgdata.params.output_color);
        return env->CallStaticObjectMethod(clBitmap, midCreateBitmap, width, height, config, false, colorSpace);
    } else {
        jmethodID midCreateBitmap = env->GetStaticMethodID(clBitmap, "createBitmap",
                                                           "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
        return env->CallStaticObjectMethod(clBitmap, midCreateBitmap, width, height, config);
    }
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

int AndroidLibRaw::copyImage(uint32_t width, uint32_t height, uint32_t stride, uint32_t format, void *bufferPtr) {
    auto pixels = width * height;

    if (format == AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM) {
        auto skip = (stride - width) * 3;
        auto ptr = static_cast<unsigned char *>(bufferPtr);
        CURVE8
        uint32_t pixel = 0;
        while (pixel < pixels) {
            auto rowEnd = pixel + width;
            while (pixel < rowEnd) {
                PIXEL_LOOP
                pixel++;
            }
            ptr += skip;
        }
        return 0;
    } else if (format == AHARDWAREBUFFER_FORMAT_R16G16B16A16_FLOAT) {
        auto skip = (stride - width) * 4;
        auto ptr = static_cast<__fp16 *>(bufferPtr);
        CURVE16
        uint32_t pixel = 0;
        while (pixel < pixels) {
            auto rowEnd = pixel + width;
            while (pixel < rowEnd) {
                PIXEL16A_LOOP
                pixel++;
            }
            ptr += skip;
        }
        return 0;
    }
    return -1;
}

jboolean AndroidLibRaw::drawSurface(JNIEnv *env, jobject surface) {
    auto nativeWindow = ANativeWindow_fromSurface(env, surface);
    ANativeWindow_Buffer buffer;
    RET_CHECK(ANativeWindow_lock(nativeWindow, &buffer, nullptr));
    RET_CHECK(copyImage(buffer.width, buffer.height, buffer.stride, buffer.format, buffer.bits));
    RET_CHECK(ANativeWindow_unlockAndPost(nativeWindow));
    ANativeWindow_release(nativeWindow);
    return true;
}



