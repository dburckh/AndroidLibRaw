#include "/libraw/libraw.h"
#include "LibRaw_fd_datastream.h"
#include <jni.h>
#include <android/log.h>

/**
 * Derived from https://github.com/TSGames/Libraw-Android/blob/master/app/src/main/ndk/Libraw_Open/jni/libraw/libraw.c
 */

LibRaw iProcessor;
libraw_processed_image_t* image=NULL;

union{
    unsigned int ui32;
    struct{
        unsigned char b0;
        unsigned char b1;
        unsigned char b2;
        unsigned char b3;
    } splitter;
}combine;

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
    return iProcessor.imgdata.sizes.width;

}
extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_getBitmapWidth(JNIEnv* env, jclass){
    return image->width;
}
extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_getBitmapHeight(JNIEnv* env, jclass){
    return image->height;
}

extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_getHeight(JNIEnv* env, jclass){
    return iProcessor.imgdata.sizes.height;

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
extern "C" JNIEXPORT jintArray JNICALL Java_com_homesoft_photo_libraw_LibRaw_getPixels8(JNIEnv* env, jclass){
    int error;
    image=decode(&error);
    if(image!=NULL){
        int pixels = image->width*image->height;
        auto* image8 = (unsigned int*)malloc(sizeof(int)*pixels);
        if(image8==NULL){
            __android_log_print(ANDROID_LOG_INFO,"libraw","getPixels8 oom");
            return NULL;
        }
        __android_log_print(ANDROID_LOG_INFO,"libraw","getPixels8 image colors %d",image->colors);
        combine.splitter.b3 = 0xff;
        for(int i=0,pixel=0; pixel < pixels;pixel++) {
            combine.splitter.b2 = image->data[i];
            combine.splitter.b1 = image->data[i+1];
            combine.splitter.b0 = image->data[i+2];
            image8[pixel] =	combine.ui32;
            i+=3;
        }
        __android_log_print(ANDROID_LOG_INFO,"libraw","getPixels8 transformed");
        jintArray jintArray = env->NewIntArray(pixels);
        env->SetIntArrayRegion(jintArray, 0, pixels, (int*)image8);
        free(image8);
        return jintArray;
    }
    __android_log_print(ANDROID_LOG_INFO,"libraw","error getPixels8 %d",error);
    return NULL;
}