#include "/libraw/libraw.h"
#include "LibRaw_fd_datastream.h"
#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>


/**
 * Derived from https://github.com/TSGames/Libraw-Android/blob/master/app/src/main/ndk/Libraw_Open/jni/libraw/libraw.c
 */
#define WHITE_THRESHOLD 0x2000
jfieldID contextFieldID = nullptr;

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

LibRaw* getLibRaw(JNIEnv* env, jobject jLibRaw) {
    return (LibRaw*)env->GetLongField(jLibRaw, contextFieldID);
}
extern "C" JNIEXPORT jlong JNICALL Java_com_homesoft_photo_libraw_LibRaw_init(JNIEnv* env, jobject jLibRaw, int flags){
    auto libRaw = new LibRaw(flags);
    if (contextFieldID == nullptr) {
        contextFieldID = env->GetFieldID(env->GetObjectClass(jLibRaw), "mNativeContext", "J");
    }
    return reinterpret_cast<jlong>(libRaw);
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_recycle(JNIEnv* env, jobject jLibRaw){
    auto libRaw = getLibRaw(env, jLibRaw);
    libRaw->recycle();
    delete libRaw;
}
extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_open(JNIEnv* env, jobject jLibRaw, jstring file){
    const char* nativeString = env->GetStringUTFChars(file, nullptr);
    auto libRaw = getLibRaw(env, jLibRaw);
    int result = libRaw->open_file(nativeString);
    if(result==0) {
        result=libRaw->unpack();
    }
    env->ReleaseStringUTFChars(file, nativeString);
    return result;
}
extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_openBufferPtr(JNIEnv* env, jobject jLibRaw, jlong ptr, jint size) {
    auto libRaw = getLibRaw(env, jLibRaw);
    int result=libRaw->open_buffer((void*)ptr, size);
    if(result==0){
        result=libRaw->unpack();
    }
    return result;
}
extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_openBuffer(JNIEnv* env, jobject jLibRaw, jbyteArray buffer, jint size){
    auto libRaw = getLibRaw(env, jLibRaw);
    auto ptr = env->GetPrimitiveArrayCritical(buffer, nullptr);
    int result=libRaw->open_buffer(ptr, size);
    env->ReleasePrimitiveArrayCritical(buffer, ptr, 0);
    if(result==0){
        result=libRaw->unpack();
    }
    return result;
}
extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_openFd(JNIEnv* env, jobject jLibRaw, jint fd){
    LibRaw_fd_datastream stream(fd);
    if (!stream.valid()) {
        return LIBRAW_IO_ERROR;
    }
    auto libRaw = getLibRaw(env, jLibRaw);
    int result=libRaw->open_datastream(&stream);
    if(result==0){
        result=libRaw->unpack();
    }
    return result;
}
extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_getWidth(JNIEnv* env, jobject jLibRaw){
    return getLibRaw(env, jLibRaw)->imgdata.sizes.iwidth;
}
extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_getHeight(JNIEnv* env, jobject jLibRaw){
    return getLibRaw(env, jLibRaw)->imgdata.sizes.iheight;
}
extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_getLeftMargin(JNIEnv* env, jobject jLibRaw){
    return getLibRaw(env, jLibRaw)->imgdata.sizes.left_margin;
}
extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_getRightMargin(JNIEnv* env, jobject jLibRaw){
    return getLibRaw(env, jLibRaw)->imgdata.sizes.top_margin;
}

extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_getOrientation(JNIEnv* env, jobject jLibRaw){
    return getLibRaw(env, jLibRaw)->imgdata.sizes.flip;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setOrientation(JNIEnv* env, jobject jLibRaw, int orientation){
    getLibRaw(env, jLibRaw)->imgdata.params.user_flip = orientation;
}
extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_getColors(JNIEnv* env, jobject jLibRaw){
    return getLibRaw(env, jLibRaw)->imgdata.rawdata.iparams.colors;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setUseCameraMatrix(JNIEnv* env, jobject jLibRaw,jint use_camera_matrix){
    getLibRaw(env, jLibRaw)->imgdata.params.use_camera_matrix=use_camera_matrix;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setQuality(JNIEnv* env, jobject jLibRaw,jint quality){
    getLibRaw(env, jLibRaw)->imgdata.params.user_qual=quality;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setAutoBrightness(JNIEnv* env, jobject jLibRaw,jboolean autoBrightness){
    getLibRaw(env, jLibRaw)->imgdata.params.no_auto_bright=!autoBrightness;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setAutoWhiteBalance(JNIEnv* env, jobject jLibRaw,jboolean autoWhiteBalance){
    getLibRaw(env, jLibRaw)->imgdata.params.use_auto_wb=autoWhiteBalance;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setBrightness(JNIEnv* env, jobject jLibRaw,jfloat brightness){
    getLibRaw(env, jLibRaw)->imgdata.params.bright = brightness;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setCameraWhiteBalance(JNIEnv* env, jobject jLibRaw,jboolean cameraWhiteBalance){
    getLibRaw(env, jLibRaw)->imgdata.params.use_camera_wb=cameraWhiteBalance;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setOutputColorSpace(JNIEnv* env, jobject jLibRaw,jint space){
    getLibRaw(env, jLibRaw)->imgdata.params.output_color=space;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setHighlightMode(JNIEnv* env, jobject jLibRaw,jint highlight){
    getLibRaw(env, jLibRaw)->imgdata.params.highlight=highlight;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setOutputBps(JNIEnv* env, jobject jLibRaw,jint output_bps){
    getLibRaw(env, jLibRaw)->imgdata.params.output_bps=output_bps;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setHalfSize(JNIEnv* env, jobject jLibRaw,jboolean half_size){
    getLibRaw(env, jLibRaw)->imgdata.params.half_size=half_size;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setCropBox(JNIEnv* env, jobject jLibRaw,
                                                    jint left, jint top, jint width, jint height) {
    auto libRaw = getLibRaw(env, jLibRaw);
    libRaw->imgdata.params.cropbox[0] = left;
    libRaw->imgdata.params.cropbox[1] = top;
    libRaw->imgdata.params.cropbox[2] = width;
    libRaw->imgdata.params.cropbox[3] = height;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setUserMul(JNIEnv* env, jobject jLibRaw,jfloat r,jfloat g1,jfloat b,jfloat g2){
    auto libRaw = getLibRaw(env, jLibRaw);
    libRaw->imgdata.params.user_mul[0]=r;
    libRaw->imgdata.params.user_mul[1]=g1;
    libRaw->imgdata.params.user_mul[2]=b;
    libRaw->imgdata.params.user_mul[3]=g2;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setGamma(JNIEnv* env, jobject jLibRaw,jdouble g1,jdouble g2){
    auto libRaw = getLibRaw(env, jLibRaw);
    libRaw->imgdata.params.gamm[0]=g1;
    libRaw->imgdata.params.gamm[1]=g2;
}
extern "C" JNIEXPORT jfloat JNICALL Java_com_homesoft_photo_libraw_LibRaw_calcBrightness(JNIEnv* env, jobject jLibRaw){
    auto libRaw = getLibRaw(env, jLibRaw);
    //Adapted from mem_image.copy_mem_image()
    auto histogram = libRaw->get_internal_data_pointer()->output_data.histogram;
    if (histogram) {
        int val, total, t_white, c;
        int perc = libRaw->imgdata.sizes.width * libRaw->imgdata.sizes.height * libRaw->imgdata.params.auto_bright_thr;
        if (libRaw->get_internal_data_pointer()->internal_output_params.fuji_width)
            perc /= 2;
        for (t_white = c = 0; c < libRaw->imgdata.idata.colors; c++)
        {
            for (val = WHITE_THRESHOLD, total = 0; --val > 32;)
                if ((total += histogram[c][val]) >
                    perc)
                    break;
            if (t_white < val)
                t_white = val;
        }
        return WHITE_THRESHOLD / (float)t_white;
    }
    return -1;
}

extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_dcrawProcess(JNIEnv* env, jobject jLibRaw){
    getLibRaw(env, jLibRaw)->dcraw_process();
}

libraw_processed_image_t* decode(LibRaw* libRaw, int* error){
    int dcraw=libRaw->dcraw_process();
    if (dcraw == 0) {
        return libRaw->dcraw_make_mem_image(error);
    } else {
        *error = dcraw;
        __android_log_print(ANDROID_LOG_WARN,"libraw","result dcraw %d",dcraw);
        return nullptr;
    }
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

jobject createBitmap(JNIEnv* env, jobject config, jint width, jint height) {
    jclass clBitmap = env->FindClass("android/graphics/Bitmap");
    jmethodID midCreateBitmap = env->GetStaticMethodID(clBitmap, "createBitmap",
                                                       "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
    return env->CallStaticObjectMethod(clBitmap, midCreateBitmap, width, height, config);
}

extern "C" JNIEXPORT jobject JNICALL Java_com_homesoft_photo_libraw_LibRaw_getBitmap(JNIEnv* env, jobject jLibRaw, jobject bitmap) {
    auto libRaw = getLibRaw(env, jLibRaw);
    int error;
    libRaw->imgdata.params.output_bps = 8;
    auto image=decode(libRaw, &error);
    if(image== nullptr) {
        return nullptr;
    }
    if (bitmap != nullptr) {
        AndroidBitmapInfo info;
        AndroidBitmap_getInfo(env, bitmap, &info);
        if (info.width != image->width || info.height != image->height ||
                info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
            //This bitmap can't be used
            bitmap = nullptr;
        }
    }
    if (bitmap == nullptr) {
        jobject ARGB_8888 = getConfigByName(env, "ARGB_8888");
        bitmap = createBitmap(env, ARGB_8888, image->width, image->height);
    }

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
    libraw_dcraw_clear_mem(image);
    return bitmap;
}
extern "C" JNIEXPORT jobject JNICALL Java_com_homesoft_photo_libraw_LibRaw_getBitmap16(JNIEnv* env, jobject jLibRaw) {
    auto libRaw = getLibRaw(env, jLibRaw);
    int error;
    libRaw->imgdata.params.output_bps = 16;
    auto image=decode(libRaw, &error);
    if(image== nullptr) {
        return nullptr;
    }
    jobject RGBA_F16 = getConfigByName(env, "RGBA_F16");
    jobject bitmap = createBitmap(env, RGBA_F16, image->width, image->height);

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
    libraw_dcraw_clear_mem(image);
    return bitmap;
}