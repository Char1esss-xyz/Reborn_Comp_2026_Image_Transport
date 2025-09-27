cmake_minimum_required(VERSION 3.10)
project(hik_web_stream)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 1. 查找 OpenCV ---
find_package(OpenCV REQUIRED)

# --- 2. 设置路径 ---
set(HIK_SDK_ROOT ${CMAKE_SOURCE_DIR}/hik_sdk)
set(HIK_LIB_DIR ${HIK_SDK_ROOT}/lib/64)

# --- 3. 头文件 ---
include_directories(${HIK_SDK_ROOT}/include)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include/cpp-httplib)

# --- 4. 源文件 ---
add_executable(hik_web_stream
    src/main.cpp
    src/hik_camera.cpp
)

# --- 5. 直接指定 .so 文件路径（关键！）---
target_link_libraries(hik_web_stream
    # OpenCV
    ${OpenCV_LIBS}

    # 海康核心库（按依赖顺序，MvCameraControl 放最前）
    ${HIK_LIB_DIR}/libMvCameraControl.so
    ${HIK_LIB_DIR}/libMVGigEVisionSDK.so

    # 基础依赖库
    ${HIK_LIB_DIR}/libGCBase_gcc485_v3_0.so
    ${HIK_LIB_DIR}/libGenCP_gcc485_v3_0.so
    ${HIK_LIB_DIR}/liblog4cpp_gcc485_v3_0.so
    ${HIK_LIB_DIR}/libLog_gcc485_v3_0.so
    ${HIK_LIB_DIR}/libCLProtocol_gcc485_v3_0.so
    ${HIK_LIB_DIR}/libCLAllSerial_gcc485_v3_0.so

    # 功能库（按需保留）
    ${HIK_LIB_DIR}/libFormatConversion.so
    ${HIK_LIB_DIR}/libMediaProcess.so
    ${HIK_LIB_DIR}/libMVRender.so
    ${HIK_LIB_DIR}/libMvSDKVersion.so

    # USB 支持
    ${HIK_LIB_DIR}/libusb-1.0.so.0

    # 系统库
    pthread
    rt
    dl
)

# --- 6. 设置运行时库路径 ---
set(CMAKE_BUILD_RPATH "$ORIGIN/../hik_sdk/lib/64")