# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_VERBOSE_MAKEFILE on)
SET(CMAKE_BUILD_TYPE "Release")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall -mfpu=neon -mfloat-abi=softfp")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
# Specifies a path to native header files.

set(src_DIR ${CMAKE_CURRENT_SOURCE_DIR})


# Specifies a path to native header files.
include_directories(
                    src/main/cpp/ffmpeglib/include/
                    src/main/cpp/ffmpeg/
                    )

add_library( # Sets the name of the library.
             native-lib

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             src/main/cpp/native-lib.cpp
             src/main/cpp/Test.cpp
             src/main/cpp/ffmpeg/ffmpeg_util.cc
             src/main/cpp/ffmpeg/transcoding.c
              )

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )


add_library(libQuCore-ThirdParty SHARED IMPORTED)
set_target_properties(libQuCore-ThirdParty
                    PROPERTIES
                    IMPORTED_LOCATION
                    "${src_DIR}/src/main/cpp/ffmpeglib/libs/libav_thirdparty.so")

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       native-lib
                       log
                       android
                        -ldl
                         "${src_DIR}/src/main/cpp/ffmpeglib/libs/libjpeg.a"
                        libQuCore-ThirdParty

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )