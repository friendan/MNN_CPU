cmake_minimum_required(VERSION 3.16)
project(MNN LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 禁用 MNN 内部的 CRT 设置，跟随主工程
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    # /FS 允许多进程并行写入同一个 .pdb
    # /Fd 指定 .pdb 输出到 build 目录而非 lib 目录
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /FS /wd4267 /wd4018 /wd4251 /wd4996 /wd4244 /wd4146 /wd4129 /wd4305 /wd4275 /wd4101")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FS /wd4267 /wd4018 /wd4251 /wd4996 /wd4244 /wd4146 /wd4129 /wd4305 /wd4275 /wd4101")
endif()

add_definitions(-DMNN_BUILD_STATIC_LIBS)
add_definitions(-DMNN_USE_THREAD_POOL)

# 头文件路径
include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/include/
    ${CMAKE_CURRENT_LIST_DIR}/source/
    ${CMAKE_CURRENT_LIST_DIR}/express/
    ${CMAKE_CURRENT_LIST_DIR}/schema/current/
    ${CMAKE_CURRENT_LIST_DIR}/3rd_party/
    ${CMAKE_CURRENT_LIST_DIR}/3rd_party/flatbuffers/include
    ${CMAKE_CURRENT_LIST_DIR}/3rd_party/half
    ${CMAKE_CURRENT_LIST_DIR}/3rd_party/imageHelper
)

# 收集所有源文件（排除不需要的子目录）
file(GLOB_RECURSE MNN_CORE_SRC    ${CMAKE_CURRENT_LIST_DIR}/source/core/*.cpp)
file(GLOB_RECURSE MNN_CV_SRC      ${CMAKE_CURRENT_LIST_DIR}/source/cv/*.cpp)
file(GLOB_RECURSE MNN_MATH_SRC    ${CMAKE_CURRENT_LIST_DIR}/source/math/*.cpp)
file(GLOB_RECURSE MNN_GEOMETRY_SRC ${CMAKE_CURRENT_LIST_DIR}/source/geometry/*.cpp)
file(GLOB_RECURSE MNN_SHAPE_SRC   ${CMAKE_CURRENT_LIST_DIR}/source/shape/*.cpp)
file(GLOB_RECURSE MNN_UTILS_SRC   ${CMAKE_CURRENT_LIST_DIR}/source/utils/*.cpp)
file(GLOB_RECURSE MNN_EXPRESS_SRC ${CMAKE_CURRENT_LIST_DIR}/express/*.cpp)

# CPU 后端（排除 riscv、kleidiai、render 子目录）
file(GLOB_RECURSE MNN_CPU_BACKEND_SRC
    ${CMAKE_CURRENT_LIST_DIR}/source/backend/cpu/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/source/backend/cpu/compute/*.cpp
)
# 排除特定子目录
list(FILTER MNN_CPU_BACKEND_SRC EXCLUDE REGEX ".*riscv/.*")
list(FILTER MNN_CPU_BACKEND_SRC EXCLUDE REGEX ".*kleidiai/.*")
list(FILTER MNN_CPU_BACKEND_SRC EXCLUDE REGEX ".*render/.*")

set(MNN_SOURCES
    ${MNN_CORE_SRC}
    ${MNN_CV_SRC}
    ${MNN_MATH_SRC}
    ${MNN_GEOMETRY_SRC}
    ${MNN_SHAPE_SRC}
    ${MNN_UTILS_SRC}
    ${MNN_EXPRESS_SRC}
    ${MNN_CPU_BACKEND_SRC}
)

# 静态库
add_library(MNN STATIC ${MNN_SOURCES})

# MNNFileUtils.cpp 使用 ANSI 版 Windows API，取消 UNICODE 避免展开为 W 版本
set_source_files_properties(
    ${CMAKE_CURRENT_LIST_DIR}/source/core/MNNFileUtils.cpp
    PROPERTIES
    COMPILE_DEFINITIONS "UNICODE=;_UNICODE="
)

# .lib/.pdb 输出到 build 目录（不污染源码目录）
set_target_properties(MNN PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/MNN"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}/MNN"
    OUTPUT_NAME_DEBUG "MNN_dbg"
    OUTPUT_NAME_RELEASE "MNN"
)
