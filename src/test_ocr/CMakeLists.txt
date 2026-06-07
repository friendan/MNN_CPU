cmake_minimum_required(VERSION 3.16)
# 使用 LLVM/ClangCL 编译器（必须在 project() 之前设置）
if(WIN32)
    set(CMAKE_CXX_COMPILER clang-cl)
endif()

project(OCR_Test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

if(MSVC)
    foreach(flag CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE)
        if(${flag} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${flag} "${${flag}}")
        endif()
    endforeach()
endif()

set(OCR_OUTPUT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../bin")

add_executable(OCR_Test main.cpp)

set_target_properties(OCR_Test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${OCR_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OCR_OUTPUT_DIR}"
    OUTPUT_NAME_DEBUG "OCR_Test_dbg"
    OUTPUT_NAME_RELEASE "OCR_Test"
)
