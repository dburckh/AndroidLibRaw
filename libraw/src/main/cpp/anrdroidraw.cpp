#include "AndroidLibRaw.h"
#include "LibRaw_fd_datastream.h"
#include <jni.h>
#include <android/log.h>
#include <android/hardware_buffer_jni.h>
#include <../common.h>

/**
 * Derived from https://github.com/TSGames/Libraw-Android/blob/master/app/src/main/ndk/Libraw_Open/jni/libraw/libraw.c
 */

extern "C" JNIEXPORT jlong JNICALL Java_com_homesoft_photo_libraw_LibRaw_init(JNIEnv* env, jobject jLibRaw, int flags){
    auto libRaw = new AndroidLibRaw(flags);
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
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_clearCancelFlag(JNIEnv* env, jobject jLibRaw){
    getLibRaw(env, jLibRaw)->clearCancelFlag();
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

extern "C" JNIEXPORT jfloatArray JNICALL Java_com_homesoft_photo_libraw_LibRaw_getCameraMul(JNIEnv* env, jobject jLibRaw) {
    auto floatArray = env->NewFloatArray(4);
    auto cam_mul = getLibRaw(env, jLibRaw)->imgdata.color.cam_mul;
    env->SetFloatArrayRegion(floatArray, 0, 4, cam_mul);
    return floatArray;
}

extern "C" JNIEXPORT jobjectArray JNICALL Java_com_homesoft_photo_libraw_LibRaw_getWhiteBalanceCoefficientsWithTemps(JNIEnv* env, jobject jLibRaw) {
    auto libRaw = getLibRaw(env, jLibRaw);
    const int arraySize = 64;
    const int subarraySize = 5;
    auto initial = env->NewFloatArray(subarraySize);

    auto floatArrayClass = env->FindClass("[F");
    auto outerArray = env->NewObjectArray(arraySize, floatArrayClass, initial);
    for (int i=0; i < arraySize; i++) {
        auto floatArray = env->NewFloatArray(subarraySize);
        // color temp, r, g1, b, g2
        env->SetFloatArrayRegion(floatArray, 0, subarraySize, libRaw->imgdata.color.WBCT_Coeffs[i]);
        env->SetObjectArrayElement(outerArray, i, floatArray);
        env->DeleteLocalRef(floatArray);
    }

    return outerArray;
}

extern "C" JNIEXPORT jobjectArray JNICALL Java_com_homesoft_photo_libraw_LibRaw_getWhiteBalanceCoefficients(JNIEnv* env, jobject jLibRaw) {
    auto libRaw = getLibRaw(env, jLibRaw);
    const int arraySize = 256;
    const int subarraySize = 4;
    auto initial = env->NewIntArray(subarraySize);

    auto intArrayClass = env->FindClass("[I");
    auto outerArray = env->NewObjectArray(arraySize, intArrayClass, initial);
    for (int i=0; i < arraySize; i++) {
        auto intArray = env->NewIntArray(subarraySize);
        // r, g1, b, g2
        env->SetIntArrayRegion(intArray, 0, subarraySize, libRaw->imgdata.color.WB_Coeffs[i]);
        env->SetObjectArrayElement(outerArray, i, intArray);
        env->DeleteLocalRef(intArray);
    }

    return outerArray;
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
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setAutoScale(JNIEnv* env, jobject jLibRaw,jboolean autoScale){
    getLibRaw(env, jLibRaw)->imgdata.params.no_auto_scale=!autoScale;
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
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setCancelFlag(JNIEnv* env, jobject jLibRaw){
    getLibRaw(env, jLibRaw)->setCancelFlag();
}
extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_getOutputColorSpace(JNIEnv* env, jobject jLibRaw) {
    return getLibRaw(env, jLibRaw)->imgdata.params.output_color;
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
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setFbddNoiseReduction(JNIEnv* env, jobject jLibRaw, jint mode){
    getLibRaw(env, jLibRaw)->imgdata.params.fbdd_noiserd=mode;
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

extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setUserBlack(JNIEnv* env, jobject jLibRaw, jint userBack) {
    auto libRaw = getLibRaw(env, jLibRaw);
    libRaw->imgdata.params.user_black = userBack;
}

extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setUserMul(JNIEnv* env, jobject jLibRaw,jfloat r,jfloat g1,jfloat b,jfloat g2){
    auto libRaw = getLibRaw(env, jLibRaw);
    libRaw->imgdata.params.user_mul[0]=r;
    libRaw->imgdata.params.user_mul[1]=g1;
    libRaw->imgdata.params.user_mul[2]=b;
    libRaw->imgdata.params.user_mul[3]=g2;
}

extern "C" JNIEXPORT void JNICALL
Java_com_homesoft_photo_libraw_LibRaw_setAutomaticMaximumCalculation(JNIEnv *env, jobject jLibRaw, jfloat automaticMaximumCalculation) {
    auto libRaw = getLibRaw(env, jLibRaw);
    libRaw->imgdata.params.adjust_maximum_thr = automaticMaximumCalculation;
}

extern "C" JNIEXPORT void JNICALL
Java_com_homesoft_photo_libraw_LibRaw_setExposureCorrectionBeforeDemosaic(JNIEnv *env, jobject jLibRaw, jboolean enabled, jfloat shift, jfloat preservation) {
    auto libRaw = getLibRaw(env, jLibRaw);
    libRaw->imgdata.params.exp_correc = enabled ? 1 : 0;
    libRaw->imgdata.params.exp_shift = shift;
    libRaw->imgdata.params.exp_preser = preservation;
}

extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setGamma(JNIEnv* env, jobject jLibRaw,jdouble g1,jdouble g2){
    auto libRaw = getLibRaw(env, jLibRaw);
    libRaw->imgdata.params.gamm[0]=g1;
    libRaw->imgdata.params.gamm[1]=g2;
}

extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_dcrawProcess(JNIEnv* env, jobject jLibRaw){
    auto libRaw = getLibRaw(env, jLibRaw);

    int rc = libRaw->dcraw_process();
    if (rc == 0) {
        libRaw->buildColorCurve();
    }
    return rc;
}

extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_dcrawProcessForced(JNIEnv* env, jobject jLibRaw, jobject colorCurve){
    auto libRaw = getLibRaw(env, jLibRaw);

    int rc = libRaw->dcrawProcessForced(env, colorCurve);
    return rc;
}
extern "C" JNIEXPORT void JNICALL Java_com_homesoft_photo_libraw_LibRaw_setCaptureScaleMul(JNIEnv* env, jobject jLibRaw, jboolean capture){
    auto libRaw = getLibRaw(env, jLibRaw);
    libRaw->setCaptureScaleMul(capture);
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

extern "C" JNIEXPORT jobject JNICALL Java_com_homesoft_photo_libraw_LibRaw_getMutableBitmap(JNIEnv* env, jobject jLibRaw, jobject bitmapConfig) {
    auto libRaw = getLibRaw(env, jLibRaw);
    return libRaw->getBitmap(env, bitmapConfig);
}
extern "C" JNIEXPORT jint JNICALL Java_com_homesoft_photo_libraw_LibRaw_drawSurface(JNIEnv* env, jobject jLibRaw, jobject surface) {
    auto libRaw = getLibRaw(env, jLibRaw);
    return libRaw->drawSurface(env, surface);
}

extern "C" JNIEXPORT jobject JNICALL Java_com_homesoft_photo_libraw_LibRaw_getColorCurve(JNIEnv* env, jobject jLibRaw) {
    auto libRaw = getLibRaw(env, jLibRaw);
    return libRaw->getColorCurve(env);
}
extern "C"
JNIEXPORT jboolean JNICALL Java_com_homesoft_photo_libraw_LibRaw_drawHardwareBuffer(JNIEnv* env, jobject jLibRaw, jobject jHardwareBuffer) {
    if (__builtin_available(android 26, *)) {
        auto hardwareBuffer = AHardwareBuffer_fromHardwareBuffer(env, jHardwareBuffer);
        if (hardwareBuffer != nullptr) {
            AHardwareBuffer_Desc desc;
            AHardwareBuffer_describe(hardwareBuffer, &desc);
            void* bufferPtr;
            auto libRaw = getLibRaw(env, jLibRaw);
            RET_CHECK(AHardwareBuffer_lock(hardwareBuffer, AHARDWAREBUFFER_USAGE_CPU_WRITE_RARELY, -1, nullptr, &bufferPtr));
            libRaw->copyImage(desc.width, desc.height, desc.stride, desc.format, bufferPtr);
            AHardwareBuffer_unlock(hardwareBuffer, nullptr);
            return true;
        }
    }
    return false;
}