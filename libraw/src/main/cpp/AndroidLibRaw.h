//
// Created by dburc on 1/2/2022.
//

#ifndef ANDROIDLIBRAW_ANDROIDLIBRAW_H
#define ANDROIDLIBRAW_ANDROIDLIBRAW_H
#include <libraw/libraw.h>
#include <jni.h>

class AndroidLibRaw: public LibRaw {
public:
    AndroidLibRaw(unsigned int flags = LIBRAW_OPTIONS_NONE);
    jobject getBitmap(JNIEnv* env, jobject bitmapConfig);
    jboolean drawSurface(JNIEnv* env, jobject surface);
    int copyImage(uint32_t width, uint32_t height, uint32_t stride, uint32_t format, void *bufferPtr);
    jobject getColorCurve(JNIEnv* env);
    void setColorCurve(JNIEnv* env, jobject byteBuffer);
    void setCaptureScaleMul(bool capture);
    void buildColorCurve();
    int dcrawProcessForced(JNIEnv* env, jobject colorCurve);
    static jobject getConfigByName(JNIEnv* env, const char* name);
    jobject createBitmap(JNIEnv *env, jobject config, jint width, jint height);

protected:
    void scale_colors_loop(float scale_mul[4]) override;

private:
    float* mScaleMul = nullptr;
    // Normally this would be done with std::function, but I'm trying to avoid pulling in the STL
    jobject doGetBitmap(JNIEnv* env, jobject bitmapConfig, void (AndroidLibRaw::*copyPtr)(void*, int));

    void copy8(void* bitmapPtr, int pixels);
    void copy16(void* bitmapPtr, int pixels);
    static void preScaleCallback(void *libRaw);
};

AndroidLibRaw* getLibRaw(JNIEnv* env, jobject jLibRaw);

#endif //ANDROIDLIBRAW_ANDROIDLIBRAW_H
