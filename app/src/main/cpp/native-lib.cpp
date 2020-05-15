#include "native-lib.h"
#include "Test.h"

#include <string>
#include <ffmpeg_util.h>
#include <android_log.h>
#include "android/log.h"

extern "C" {
#include <libavcodec/jni.h>
}

//JNIEXPORT jstring JNICALL
//Java_cookbook_testjni_MainActivity_stringFromJNI(
//        JNIEnv* env,
//        jobject /* this */) {
//    std::string hello = "Hello from C++";
//    return env->NewStringUTF(hello.c_str());
//}


JNIEXPORT jfloatArray JNICALL
Java_cookbook_testjni_MainActivity_setFloatArray(
        JNIEnv *env,
        jobject thiz,
        jfloatArray fArray) {

//        jchar * stringSource = (jchar *)"abcdef";
////        const jchar * stringSource = "abcdef";
////        const jchar * stringSource = new jchar[]{'a','b','c','d','e'};
//        jstring jstring1 = env->NewString(stringSource,6);
//
//        jchar *jcharBuffer;
//        env->GetStringRegion(jstring1,0,3,jcharBuffer);

    int fLength = env->GetArrayLength(fArray);
    const float *floatArray = env->GetFloatArrayElements(fArray, 0);

    for (int i = 0; i < fLength; i++) {
        __android_log_print(ANDROID_LOG_ERROR, "andymao", "for loop item=%d,i=%d", *floatArray, i);
    }

    Test test(1, 3, 5);
    test.setTransforamMatrix(floatArray);

//        int aaa[5] = { 1, 2, 3, 4, 5};
//
//        for(int j=0;j<5;j++){
//            __android_log_print(ANDROID_LOG_ERROR,"andymao","for loop item=%d,j=%d",aaa[j],j);
//        }

    int newLength = fLength + 2;
    jfloatArray jfloatArray1 = env->NewFloatArray(newLength);
//        env->SetFloatArrayRegion(jfloatArray1,0,fLength,floatArray);

    return jfloatArray1;

}

//JNIEXPORT jfloatArray JNICALL Java_cookbook_testjni_JNIInterfaceTest_setFloatArray
//  (JNIEnv *, jobject, jfloatArray){
//
//
//
//  }
void fillIntArray(jint*& array) {
    jint *fkJintArray = new jint[12];
    for (int i = 0; i < 12; ++i) {
        fkJintArray[i] = i;
    }

    array = fkJintArray;
}

extern "C"
JNIEXPORT void JNICALL
Java_cookbook_testjni_MainActivity_getIntArray(JNIEnv *env, jobject instance,
                                               jintArray intArrays_) {
    jint *intArrays = env->GetIntArrayElements(intArrays_, NULL);

    fillIntArray(intArrays);

    env->ReleaseIntArrayElements(intArrays_, intArrays, 0);
}



extern "C"
JNIEXPORT void JNICALL
Java_cookbook_testjni_MainActivity_extractFrame2(JNIEnv *env, jobject instance, jstring filePath_,
                                                 jint timePoint, jintArray pixels_, jint frameW,
                                                 jint frameH, jboolean debug) {
    const char *filePath = env->GetStringUTFChars(filePath_, 0);
    jint *pixels = env->GetIntArrayElements(pixels_, NULL);

    // TODO

    extractFrame2(filePath, (int) timePoint, pixels, frameW, frameH, debug);

    int rectLength = env->GetArrayLength(pixels_);
    env->SetIntArrayRegion(pixels_, 0, rectLength, pixels);

    env->ReleaseStringUTFChars(filePath_, filePath);
    env->ReleaseIntArrayElements(pixels_, pixels, 0);
}

jint JNI_OnLoad(JavaVM* vm, void* reserved)//这个类似android的生命周期，加载jni的时候会自己调用
{
    LOGE("ffmpeg JNI_OnLoad");
    av_jni_set_java_vm(vm, reserved);
    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT void JNICALL
Java_cookbook_testjni_MainActivity_mergeMp4Clips(JNIEnv *env, jobject instance, jobjectArray paths,
                                                 jstring mVideoOutputPath_) {
    const char *mVideoOutputPath = env->GetStringUTFChars(mVideoOutputPath_, 0);

    const char *output = env->GetStringUTFChars(mVideoOutputPath_, 0);
    std::vector<std::string> inputFileList;

    int stringCount = env->GetArrayLength(paths);
    for (int i = 0; i < stringCount; i++) {
        jstring pathStr = (jstring) (env->GetObjectArrayElement(paths, i));
        const char *pathChars = env->GetStringUTFChars(pathStr, 0);
        // Don't forget to call `ReleaseStringUTFChars` when you're done.

        inputFileList.push_back(std::string(pathChars));

        env->ReleaseStringUTFChars(pathStr, pathChars);
    }

    combine_video(inputFileList,output);

    env->ReleaseStringUTFChars(mVideoOutputPath_, mVideoOutputPath);
}