#include <jni.h>
#include <string>
#include "android/log.h"
extern "C"

//JNIEXPORT jstring JNICALL
//Java_cookbook_testjni_MainActivity_stringFromJNI(
//        JNIEnv* env,
//        jobject /* this */);


JNIEXPORT jfloatArray JNICALL
Java_cookbook_testjni_MainActivity_setFloatArray(
        JNIEnv * env,
        jobject thiz,
        jfloatArray fArray);
