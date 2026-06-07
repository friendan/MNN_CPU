cmake_minimum_required(VERSION 3.16)
project(OCR_Test LANGUAGES C)

set(CMAKE_C_STANDARD 11)

# 使用 LLVM/ClangCL 编译器
if(WIN32)
    set(CMAKE_C_COMPILER clang-cl)
endif()

# MT / MTd 运行时链接
if(MSVC)
    foreach(flag CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE)
        if(${flag} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${flag} "${${flag}}")
        endif()
    endforeach()
endif()

# 输出到项目根目录 bin/
set(OCR_OUTPUT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../bin")

add_executable(OCR_Test main.c)

set_target_properties(OCR_Test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${OCR_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OCR_OUTPUT_DIR}"
    OUTPUT_NAME_DEBUG "OCR_Test_dbg"
    OUTPUT_NAME_RELEASE "OCR_Test"
)
