cmake_minimum_required(VERSION 3.4.1)

#导入include文件夹路径
include_directories(src/main/cpp/include)

set(lib "${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}")
set(path "src/main/cpp")

#导入动态库
add_library(avcodec-56 SHARED IMPORTED)
#设置刚才导入动态库的实际地址链接
set_target_properties(avcodec-56 PROPERTIES IMPORTED_LOCATION
                             "${lib}/libavcodec-56.so")

add_library(avdevice-56 SHARED IMPORTED)
set_target_properties(avdevice-56 PROPERTIES IMPORTED_LOCATION
                             "${lib}/libavdevice-56.so")

add_library(avfilter-5 SHARED IMPORTED)
set_target_properties(avfilter-5 PROPERTIES IMPORTED_LOCATION
                        "${lib}/libavfilter-5.so")

add_library(avformat-56 SHARED IMPORTED)
set_target_properties(avformat-56 PROPERTIES IMPORTED_LOCATION
                       "${lib}/libavformat-56.so" )

add_library(avutil-54 SHARED IMPORTED)
set_target_properties(avutil-54 PROPERTIES IMPORTED_LOCATION
                        "${lib}/libavutil-54.so")

add_library(postproc-53 SHARED IMPORTED)
set_target_properties(postproc-53 PROPERTIES IMPORTED_LOCATION
                        "${lib}/libpostproc-53.so")

add_library(swresample-1 SHARED IMPORTED)
set_target_properties(swresample-1 PROPERTIES IMPORTED_LOCATION
                       "${lib}/libswresample-1.so" )

add_library(swscale-3 SHARED IMPORTED)
set_target_properties(swscale-3 PROPERTIES IMPORTED_LOCATION
                        "${lib}/libswscale-3.so")

add_library(native-lib SHARED src/main/cpp/native-lib.cpp)

target_link_libraries( # Specifies the target library.
                       native-lib

                       avcodec-56
                       avdevice-56
                       avfilter-5
                       avformat-56
                       avutil-54
                       postproc-53
                       swresample-1
                       swscale-3

                       log )