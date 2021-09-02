#include "/libraw/libraw.h"
#include <jni.h>
#include <android/log.h>

libraw_data_t* librawData=NULL;
libraw_processed_image_t* image=NULL;
void* image16=NULL;
void cleanup(){
    if(librawData!=NULL){
        libraw_recycle(librawData);
        librawData=NULL;
    }
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
    librawData=libraw_init(0);
    int result=libraw_open_file(librawData,nativeString);
    if(result==0){
        result=libraw_unpack(librawData);
    }
    env->ReleaseStringUTFChars(file, nativeString);
    return result;
}
extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_openBuffer(JNIEnv* env, jclass,jlong buffer, jint size){
    cleanup();
    __android_log_print(ANDROID_LOG_INFO,"libraw","open %d", size);
    librawData=libraw_init(0);
    int result=libraw_open_buffer(librawData, (void*)buffer, size);
    if(result==0){
        result=libraw_unpack(librawData);
    }
    return result;
}
extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_getWidth(JNIEnv* env, jclass){
    return librawData->sizes.width;

}
extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_getBitmapWidth(JNIEnv* env, jclass){
    return image->width;
}
extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_getBitmapHeight(JNIEnv* env, jclass){
    return image->height;
}

extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_getHeight(JNIEnv* env, jclass){
    return librawData->sizes.height;

}
extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_getOrientation(JNIEnv* env, jclass){
    return librawData->sizes.flip;
}
extern "C" JNIEXPORT jbyteArray JNICALL Java_com_homesoft_photo_libraw_LibRaw_getThumbnail(JNIEnv* env, jclass, jstring file){
    cleanup();
    const char *nativeString = env->GetStringUTFChars(file, NULL);
    librawData=libraw_init(0);
    int result=libraw_open_file(librawData,nativeString);
    if(result==0)
        result=libraw_unpack_thumb(librawData);

    env->ReleaseStringUTFChars(file, nativeString);
    if(result==0){
        jbyteArray jbyteArray = env->NewByteArray(librawData->thumbnail.tlength);
        if(jbyteArray==NULL)
            return NULL;
        env->SetByteArrayRegion(jbyteArray, 0, librawData->thumbnail.tlength,(jbyte*)librawData->thumbnail.thumb);
        return jbyteArray;
    }
    return NULL;
}
extern "C" JNIEXPORT jfloatArray JNICALL Java_com_homesoft_photo_libraw_LibRaw_getDaylightMultiplier(JNIEnv* env, jclass){
    float* mul = librawData->color.pre_mul;
    jfloatArray jfloatArray = env->NewFloatArray(3);
    env->SetFloatArrayRegion(jfloatArray, 0, 3, mul);
    return jfloatArray;
}
extern "C" JNIEXPORT jfloatArray JNICALL Java_com_homesoft_photo_libraw_LibRaw_getWhitebalanceMultiplier(JNIEnv* env, jclass){
    float* mul = librawData->color.cam_mul;
    jfloatArray jfloatArray = env->NewFloatArray(3);
    env->SetFloatArrayRegion(jfloatArray, 0, 3, mul);
    return jfloatArray;
}
void pseudoinverse (float (*in)[3], float (*out)[3], int size)
{
    float work[3][6], num;
    int i, j, k;

    for (i=0; i < 3; i++) {
        for (j=0; j < 6; j++)
            work[i][j] = j == i+3;
        for (j=0; j < 3; j++)
            for (k=0; k < size; k++)
                work[i][j] += in[k][i] * in[k][j];
    }
    for (i=0; i < 3; i++) {
        num = work[i][i];
        for (j=0; j < 6; j++)
            work[i][j] /= num;
        for (k=0; k < 3; k++) {
            if (k==i) continue;
            num = work[k][i];
            for (j=0; j < 6; j++)
                work[k][j] -= work[i][j] * num;
        }
    }
    for (i=0; i < size; i++)
        for (j=0; j < 3; j++)
            for (out[i][j]=k=0; k < 3; k++)
                out[i][j] += work[j][k+3] * in[i][k];
}

extern "C" JNIEXPORT jfloatArray JNICALL Java_com_homesoft_photo_libraw_LibRaw_getCamRgb(JNIEnv* env, jclass){
    if(librawData==NULL)
        return NULL;

    float inverse[4][3];
    float cam_rgb[4][3];

    int j,i;
    for (i=0; i < 4; i++)
        for (j=0; j < 3; j++)
            cam_rgb[i][j] = librawData->color.rgb_cam[j][i];

    pseudoinverse(cam_rgb,inverse,3);
    /*
    int j,i;
    for (i=0; i < 3; i++)
        for (j=0; j < 4; j++)
      rgb_cam[i][j] = inverse[j][i];
    */
    jfloatArray jfloatArray = env->NewFloatArray(12);
    env->SetFloatArrayRegion(jfloatArray, 0, 12,(jfloat*)inverse);
    return jfloatArray;
}
extern "C" JNIEXPORT jfloatArray JNICALL Java_com_homesoft_photo_libraw_LibRaw_getRgbCam(JNIEnv* env, jclass){
    float* mul = (float*)librawData->color.rgb_cam;
    jfloatArray jfloatArray = env->NewFloatArray(12);
    env->SetFloatArrayRegion(jfloatArray, 0, 12, mul);
    return jfloatArray;
}
extern "C" JNIEXPORT jfloatArray JNICALL Java_com_homesoft_photo_libraw_LibRaw_getCamMatrix(JNIEnv* env, jclass){
    float* mul = (float*)librawData->color.cmatrix;
    jfloatArray jfloatArray = env->NewFloatArray(12);
    env->SetFloatArrayRegion(jfloatArray, 0, 12, mul);
    return jfloatArray;
}
extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_getColors(JNIEnv* env, jclass){
    return librawData->rawdata.iparams.colors;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setUseCameraMatrix(JNIEnv* env, jclass,jint use_camera_matrix){
    librawData->params.use_camera_matrix=use_camera_matrix;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setQuality(JNIEnv* env, jclass,jint quality){
    librawData->params.user_qual=quality;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setAutoBrightness(JNIEnv* env, jclass,jboolean autoBrightness){
    librawData->params.no_auto_bright=!autoBrightness;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setAutoWhitebalance(JNIEnv* env, jclass,jboolean autoWhitebalance){
    librawData->params.use_camera_wb=autoWhitebalance;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setOutputColorSpace(JNIEnv* env, jclass,jint space){
    librawData->params.output_color=space;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setHighlightMode(JNIEnv* env, jclass,jint highlight){
    librawData->params.highlight=highlight;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setOutputBps(JNIEnv* env, jclass,jint output_bps){
    librawData->params.output_bps=output_bps;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setHalfSize(JNIEnv* env, jclass,jboolean half_size){
    librawData->params.half_size=half_size;
}

extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setUserMul(JNIEnv* env, jclass,jfloat r,jfloat g1,jfloat b,jfloat g2){
    librawData->params.user_mul[0]=r;
    librawData->params.user_mul[1]=g1;
    librawData->params.user_mul[2]=b;
    librawData->params.user_mul[3]=g2;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setGamma(JNIEnv* env, jclass,jdouble g1,jdouble g2){
    librawData->params.gamm[0]=g1;
    librawData->params.gamm[1]=g2;
}
libraw_processed_image_t* decode(int* error){
    int dcraw=libraw_dcraw_process(librawData);
    __android_log_print(ANDROID_LOG_INFO,"libraw","result dcraw %d",dcraw);
    return libraw_dcraw_make_mem_image(librawData,error);
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
        int* image8 = (int*)malloc(sizeof(int)*image->width*image->height);
        if(image8==NULL){
            __android_log_print(ANDROID_LOG_INFO,"libraw","getPixels8 oom");
            return NULL;
        }
        __android_log_print(ANDROID_LOG_INFO,"libraw","getPixels8 image colors %d",image->colors);
        int x,y;
        for(y=0;y<image->height;y++){
            for(x=0;x<image->width;x++){
                int pos=(x+y*image->width)*3;
                image8[x+y*image->width]=	0xff000000 | (image->data[pos]<<16) | (image->data[pos+1]<<8) | (image->data[pos+2]);
            }
        }
        jintArray jintArray = env->NewIntArray(image->width*image->height);
        env->SetIntArrayRegion(jintArray, 0, image->width*image->height, image8);
        free(image8);
        return jintArray;
    }
    __android_log_print(ANDROID_LOG_INFO,"libraw","error getPixels8 %d",error);
    return NULL;
}
extern "C" JNIEXPORT jlong JNICALL
Java_com_homesoft_photo_libraw_LibRaw_getPixels16(JNIEnv* env, jclass){
    int error;
    image=decode(&error);
    __android_log_print(ANDROID_LOG_INFO,"libraw","decode result %d data_size %d",error,image->data_size);
    if(image!=NULL && image->data_size){
        __android_log_print(ANDROID_LOG_INFO,"libraw","image length %d",image->data_size);
        libraw_recycle(librawData);
        librawData=NULL;
        image16=malloc(image->data_size);
        if(image16==NULL){
            __android_log_print(ANDROID_LOG_INFO,"libraw","getPixels16 oom");
            return 0;
        }
        __android_log_print(ANDROID_LOG_INFO,"libraw","allocated memory");
        memcpy(image16,image->data,image->data_size);
        __android_log_print(ANDROID_LOG_INFO,"libraw","copied pointer");
        return (jlong)image16;
    }
    return 0;
}