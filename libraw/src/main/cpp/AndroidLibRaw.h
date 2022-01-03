//
// Created by dburc on 1/2/2022.
//

#ifndef ANDROIDLIBRAW_ANDROIDLIBRAW_H
#define ANDROIDLIBRAW_ANDROIDLIBRAW_H
#include "/libraw/libraw.h"
#include <jni.h>

class AndroidLibRaw: public LibRaw {
public:
    AndroidLibRaw(unsigned int flags = LIBRAW_OPTIONS_NONE);
    jobject getBitmap(JNIEnv* env, jobject bitmap);
    jobject getBitmap16(JNIEnv* env, jobject bitmap);
    jobject getColorCurve(JNIEnv* env);
    void setColorCurve(JNIEnv* env, jobject byteBuffer);
    void buildColorCurve();

    static jobject getConfigByName(JNIEnv* env, const char* name);
    static jobject createBitmap(JNIEnv *env, jobject config, jint width, jint height);
private:
    jobject doGetBitmap(JNIEnv* env, jobject bitmap, const char* configName, int32_t configType, const std::function<void (void* bitmapPtr, int const pixels)>& _copy);
};


#endif //ANDROIDLIBRAW_ANDROIDLIBRAW_H
