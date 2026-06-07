cmake_minimum_required(VERSION 3.16)
project(OCR_Test LANGUAGES C)

set(CMAKE_C_STANDARD 11)

# MT / MTd 运行时链接（必须与 MNN.dll 保持一致）
if(MSVC)
    foreach(flag CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE)
        if(${flag} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${flag} "${${flag}}")
        endif()
    endforeach()
endif()

add_executable(OCR_Test main.c)
