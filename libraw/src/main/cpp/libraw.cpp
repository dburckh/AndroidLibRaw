#include "/libraw/libraw.h"
#include "LibRaw_fd_datastream.h"
#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>

/**
 * Derived from https://github.com/TSGames/Libraw-Android/blob/master/app/src/main/ndk/Libraw_Open/jni/libraw/libraw.c
 */

LibRaw iProcessor;
libraw_processed_image_t* image=NULL;

union{
    uint32_t ui32;
    struct{
        unsigned char b0;
        unsigned char b1;
        unsigned char b2;
        unsigned char b3;
    } splitter;
} argb;

union{
    uint64_t ui64;
    struct{
        __fp16 fp0;
        __fp16 fp1;
        __fp16 fp2;
        __fp16 fp3;
    } splitter;
} rgba16;

const float SHORT2FLOAT = 65535.0f;

void cleanup(){
    iProcessor.recycle();
    if(image!=NULL){
        libraw_dcraw_clear_mem(image);
        image=NULL;
    }
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_cleanup(JNIEnv* env, jclass){
    cleanup();
}
extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_open(JNIEnv* env, jclass,jstring file){
    cleanup();
    const char* nativeString = env->GetStringUTFChars(file, NULL);
    __android_log_print(ANDROID_LOG_INFO,"libraw","open %s",nativeString);
    int result = iProcessor.open_file(nativeString);
    if(result==0){
        result=iProcessor.unpack();
    }
    env->ReleaseStringUTFChars(file, nativeString);
    return result;
}
extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_openBuffer(JNIEnv* env, jclass,jlong buffer, jint size){
    cleanup();
    __android_log_print(ANDROID_LOG_INFO,"libraw","open %d", size);
    int result=iProcessor.open_buffer((void*)buffer, size);
    if(result==0){
        result=iProcessor.unpack();
    }
    return result;
}
extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_openFd(JNIEnv* env, jclass, jint fd){
    cleanup();
    __android_log_print(ANDROID_LOG_INFO,"libraw","open %d", fd);
    LibRaw_fd_datastream stream(fd);
    if (!stream.valid()) {
        return LIBRAW_IO_ERROR;
    }
    int result=iProcessor.open_datastream(&stream);
    if(result==0){
        result=iProcessor.unpack();
    }
    return result;
}
extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_getWidth(JNIEnv* env, jclass){
    return iProcessor.imgdata.sizes.iwidth;

}
extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_getBitmapWidth(JNIEnv* env, jclass){
    return image->width;
}
extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_getBitmapHeight(JNIEnv* env, jclass){
    return image->height;
}

extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_getHeight(JNIEnv* env, jclass){
    return iProcessor.imgdata.sizes.iheight;

}
extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_getOrientation(JNIEnv* env, jclass){
    return iProcessor.imgdata.sizes.flip;
}
extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_getColors(JNIEnv* env, jclass){
    return iProcessor.imgdata.rawdata.iparams.colors;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setUseCameraMatrix(JNIEnv* env, jclass,jint use_camera_matrix){
    iProcessor.imgdata.params.use_camera_matrix=use_camera_matrix;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setQuality(JNIEnv* env, jclass,jint quality){
    iProcessor.imgdata.params.user_qual=quality;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setAutoBrightness(JNIEnv* env, jclass,jboolean autoBrightness){
    iProcessor.imgdata.params.no_auto_bright=!autoBrightness;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setAutoWhitebalance(JNIEnv* env, jclass,jboolean autoWhitebalance){
    iProcessor.imgdata.params.use_camera_wb=autoWhitebalance;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setOutputColorSpace(JNIEnv* env, jclass,jint space){
    iProcessor.imgdata.params.output_color=space;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setHighlightMode(JNIEnv* env, jclass,jint highlight){
    iProcessor.imgdata.params.highlight=highlight;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setOutputBps(JNIEnv* env, jclass,jint output_bps){
    iProcessor.imgdata.params.output_bps=output_bps;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setHalfSize(JNIEnv* env, jclass,jboolean half_size){
    iProcessor.imgdata.params.half_size=half_size;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setCropBox(JNIEnv* env, jclass,jint top, jint left, jint width, int height) {
    iProcessor.imgdata.params.cropbox[0] = top;
    iProcessor.imgdata.params.cropbox[1] = left;
    iProcessor.imgdata.params.cropbox[2] = width;
    iProcessor.imgdata.params.cropbox[3] = height;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setUserMul(JNIEnv* env, jclass,jfloat r,jfloat g1,jfloat b,jfloat g2){
    iProcessor.imgdata.params.user_mul[0]=r;
    iProcessor.imgdata.params.user_mul[1]=g1;
    iProcessor.imgdata.params.user_mul[2]=b;
    iProcessor.imgdata.params.user_mul[3]=g2;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setGamma(JNIEnv* env, jclass,jdouble g1,jdouble g2){
    iProcessor.imgdata.params.gamm[0]=g1;
    iProcessor.imgdata.params.gamm[1]=g2;
}
libraw_processed_image_t* decode(int* error){
    int dcraw=iProcessor.dcraw_process();
    __android_log_print(ANDROID_LOG_INFO,"libraw","result dcraw %d",dcraw);
    return iProcessor.dcraw_make_mem_image(error);
}
extern "C" JNIEXPORT jstring JNICALL Java_com_homesoft_photo_libraw_LibRaw_getCameraList(JNIEnv* env, jclass){
    jstring result;
    char message[1024*1024];
    strcpy(message,"");
    const char** list=libraw_cameraList();
    int i;
    for(i=0;i<libraw_cameraCount();i++){
        strcat(message,list[i]);
        strcat(message,"\n");
    }
    result = env->NewStringUTF(message);
    return result;
}

jobject getConfigByName(JNIEnv* env, const char* name) {
    jclass clBitmapConfig = env->FindClass("android/graphics/Bitmap$Config");
    jfieldID fidARGB_8888 = env->GetStaticFieldID(clBitmapConfig, name,
                                                  "Landroid/graphics/Bitmap$Config;");
    return env->GetStaticObjectField(clBitmapConfig, fidARGB_8888);
}

jobject createBitmap(JNIEnv* env, jobject config) {
    jclass clBitmap = env->FindClass("android/graphics/Bitmap");
    jmethodID midCreateBitmap = env->GetStaticMethodID(clBitmap, "createBitmap",
                                                       "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
    return env->CallStaticObjectMethod(clBitmap, midCreateBitmap, image->width,
                                                 image->height, config);
}

extern "C" JNIEXPORT jobject JNICALL Java_com_homesoft_photo_libraw_LibRaw_getBitmap(JNIEnv* env, jclass) {
    int error;
    iProcessor.imgdata.params.output_bps = 8;
    image=decode(&error);
    if(image== nullptr) {
        return nullptr;
    }
    jobject ARGB_8888 = getConfigByName(env, "ARGB_8888");
    jobject bitmap = createBitmap(env, ARGB_8888);

    int pixels = image->width*image->height;
    void *addrPtr;
    AndroidBitmap_lockPixels(env,bitmap, &addrPtr);
    auto pixelArr = ((uint32_t *) addrPtr);
    argb.splitter.b3 = 0xff;
    for(int i=0,pixel=0; pixel < pixels;pixel++) {
        argb.splitter.b0 = image->data[i];
        argb.splitter.b1 = image->data[i + 1];
        argb.splitter.b2 = image->data[i + 2];
        pixelArr[pixel] = argb.ui32;
        i+=3;
    }
    AndroidBitmap_unlockPixels(env,bitmap);
    return bitmap;
}
extern "C" JNIEXPORT jobject JNICALL Java_com_homesoft_photo_libraw_LibRaw_getBitmap16(JNIEnv* env, jclass) {
    int error;
    iProcessor.imgdata.params.output_bps = 16;
    image=decode(&error);
    if(image== nullptr) {
        return nullptr;
    }
    jobject RGBA_F16 = getConfigByName(env, "RGBA_F16");
    jobject bitmap = createBitmap(env, RGBA_F16);

    int pixels = image->width*image->height;
    void *addrPtr;
    AndroidBitmap_lockPixels(env,bitmap, &addrPtr);

    //Convert from RGB (uint16[3]) to RGBA_F16 (__fp16[4])
    auto pixelArr = ((uint64_t *) addrPtr);
    rgba16.splitter.fp3 = 1.0f;
    auto data = (uint16_t*)image->data;

    for(int i=0,pixel=0; pixel < pixels;pixel++) {
        rgba16.splitter.fp0 = data[i] / SHORT2FLOAT;
        rgba16.splitter.fp1 = data[i + 1] / SHORT2FLOAT;
        rgba16.splitter.fp2 = data[i + 2] / SHORT2FLOAT;
        pixelArr[pixel] = rgba16.ui64;
        i+=3;
    }
    AndroidBitmap_unlockPixels(env,bitmap);
    return bitmap;
}