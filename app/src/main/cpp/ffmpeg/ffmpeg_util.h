#ifndef __EXTRACT_TEST_H__
#define __EXTRACT_TEST_H__

#include <iostream>
#include <vector>
#include <jni.h>

extern "C" {
int extractFrame(const char *inputFilePath, const int startTimeMs, const char *yPlaner,
                 const char *uPlaner, const char *vPlaner);

int extractFrame2(const char *inputFilePath, const int startTimeMs, jint *outputJints, jint dstW,
                  jint dstH, bool debug);
int combine_video(const std::vector<std::string> &inputFileList, const char *outputFile);
int getVideoRotate(const char *inputFilePath);

};

#endif
