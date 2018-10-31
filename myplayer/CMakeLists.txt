cmake_minimum_required(VERSION 3.4.1)

#导入include文件夹路径
include_directories(src/main/cpp/include)
#导入SoundTouch所需源码文件
include_directories(src/main/cpp/soundtouch/include)
include_directories(src/main/cpp/soundtouch/source)


set(lib "${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}")
set(path "src/main/cpp")

#导入动态库
add_library(avcodec-57 SHARED IMPORTED)
#设置刚才导入动态库的实际地址链接
set_target_properties(avcodec-57 PROPERTIES IMPORTED_LOCATION
                             "${lib}/libavcodec-57.so")

add_library(avdevice-57 SHARED IMPORTED)
set_target_properties(avdevice-57 PROPERTIES IMPORTED_LOCATION
                             "${lib}/libavdevice-57.so")

add_library(avfilter-6 SHARED IMPORTED)
set_target_properties(avfilter-6 PROPERTIES IMPORTED_LOCATION
                        "${lib}/libavfilter-6.so")

add_library(avformat-57 SHARED IMPORTED)
set_target_properties(avformat-57 PROPERTIES IMPORTED_LOCATION
                       "${lib}/libavformat-57.so" )

add_library(avutil-55 SHARED IMPORTED)
set_target_properties(avutil-55 PROPERTIES IMPORTED_LOCATION
                        "${lib}/libavutil-55.so")

add_library(postproc-54 SHARED IMPORTED)
set_target_properties(postproc-54 PROPERTIES IMPORTED_LOCATION
                        "${lib}/libpostproc-54.so")

add_library(swresample-2 SHARED IMPORTED)
set_target_properties(swresample-2 PROPERTIES IMPORTED_LOCATION
                       "${lib}/libswresample-2.so" )

add_library(swscale-4 SHARED IMPORTED)
set_target_properties(swscale-4 PROPERTIES IMPORTED_LOCATION
                        "${lib}/libswscale-4.so")

add_library(native-lib SHARED
            src/main/cpp/native-lib.cpp
            src/main/cpp/HCallJava.cpp
            src/main/cpp/HFFmpeg.cpp
            src/main/cpp/HAudio.cpp
            src/main/cpp/HQueue.cpp
            src/main/cpp/HPlayStatus.cpp
            src/main/cpp/HPcmBean.cpp
            src/main/cpp/HBufferQueue.cpp

            src/main/cpp/soundtouch/source/AAfilter.cpp
            src/main/cpp/soundtouch/source/cpu_detect_x86.cpp
            src/main/cpp/soundtouch/source/FIFOSampleBuffer.cpp
            src/main/cpp/soundtouch/source/FIRFilter.cpp
            src/main/cpp/soundtouch/source/InterpolateCubic.cpp
            src/main/cpp/soundtouch/source/InterpolateLinear.cpp
            src/main/cpp/soundtouch/source/InterpolateShannon.cpp
            src/main/cpp/soundtouch/source/PeakFinder.cpp
            src/main/cpp/soundtouch/source/RateTransposer.cpp
            src/main/cpp/soundtouch/source/SoundTouch.cpp
            src/main/cpp/soundtouch/source/sse_optimized.cpp
            src/main/cpp/soundtouch/source/TDStretch.cpp
            src/main/cpp/soundtouch/source/BPMDetect.cpp
            src/main/cpp/soundtouch/source/mmx_optimized.cpp
            )

target_link_libraries( # Specifies the target library.
                       native-lib

                       avcodec-57
                       avdevice-57
                       avfilter-6
                       avformat-57
                       avutil-55
                       postproc-54
                       swresample-2
                       swscale-4

                       OpenSLES

                       log )


