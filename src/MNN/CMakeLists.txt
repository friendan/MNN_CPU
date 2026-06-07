cmake_minimum_required(VERSION 3.16)
project(MNN LANGUAGES C CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 链接方式选项: SHARED=DLL, STATIC=静态库
option(MNN_BUILD_SHARED "Build MNN as shared library (DLL)" ON)

# MT / MTd 运行时链接
if(MSVC)
    # 强制使用 MT（多线程静态链接），避免依赖 MSVCRT DLL
    foreach(flag_var
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        if(${flag_var} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endif()
    endforeach()

    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /FS /wd4267 /wd4018 /wd4251 /wd4996 /wd4244 /wd4146 /wd4129 /wd4305 /wd4275 /wd4101")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FS /wd4267 /wd4018 /wd4251 /wd4996 /wd4244 /wd4146 /wd4129 /wd4305 /wd4275 /wd4101")
endif()

if(MNN_BUILD_SHARED)
    add_definitions(-DBUILDING_MNN_DLL)
else()
    add_definitions(-DMNN_BUILD_STATIC_LIBS)
endif()
add_definitions(-DMNN_USE_THREAD_POOL)

# 头文件路径
include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/include/
    ${CMAKE_CURRENT_LIST_DIR}/source/
    ${CMAKE_CURRENT_LIST_DIR}/source/ocr/
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

# OCR 引擎源文件
file(GLOB_RECURSE MNN_OCR_SRC ${CMAKE_CURRENT_LIST_DIR}/source/ocr/*.cpp)

set(MNN_SOURCES
    ${MNN_CORE_SRC}
    ${MNN_CV_SRC}
    ${MNN_MATH_SRC}
    ${MNN_GEOMETRY_SRC}
    ${MNN_SHAPE_SRC}
    ${MNN_UTILS_SRC}
    ${MNN_EXPRESS_SRC}
    ${MNN_CPU_BACKEND_SRC}
    ${MNN_OCR_SRC}
)

# 库类型: DLL 或 静态库
if(MNN_BUILD_SHARED)
    add_library(MNN SHARED ${MNN_SOURCES})
else()
    add_library(MNN STATIC ${MNN_SOURCES})
endif()

# MNNFileUtils.cpp 使用 ANSI 版 Windows API，取消 UNICODE 避免展开为 W 版本
set_source_files_properties(
    ${CMAKE_CURRENT_LIST_DIR}/source/core/MNNFileUtils.cpp
    PROPERTIES
    COMPILE_DEFINITIONS "UNICODE=;_UNICODE="
)

# 输出到项目根目录的 bin/ 下
set(MNN_OUTPUT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../bin")

# 添加 Windows 资源文件（嵌入 OCR 模型文件）
# .rc 文件放在 src/MNN/ 下，这样 models/ 是相对路径可直接使用
if(WIN32 AND MNN_BUILD_SHARED)
    set(MNN_RESOURCE_FILE ${CMAKE_CURRENT_LIST_DIR}/ocr_resources.rc)
    set_source_files_properties(${MNN_RESOURCE_FILE} PROPERTIES LANGUAGE RC)
    list(APPEND MNN_SOURCES ${MNN_RESOURCE_FILE})
endif()

set_target_properties(MNN PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${MNN_OUTPUT_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${MNN_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${MNN_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${MNN_OUTPUT_DIR}"
    PDB_OUTPUT_DIRECTORY_DEBUG "${MNN_OUTPUT_DIR}"
    PDB_OUTPUT_DIRECTORY_RELEASE "${MNN_OUTPUT_DIR}"
    OUTPUT_NAME_DEBUG "MNN_dbg"
    OUTPUT_NAME_RELEASE "MNN"
)
