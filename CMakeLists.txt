# Tested on 3.16.9 and 3.21.2
cmake_minimum_required(VERSION 3.16...3.21 FATAL_ERROR)

project(HW2)

# Executable goes to bin
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

option(BUILD_SHARED_LIBS "Build shared library" ON)

# [修正] 移除這裡的全域定義，避免汙染到 GLFW 的編譯
# add_compile_definitions(GLFW_INCLUDE_NONE) <--- 刪除這行

if (NOT (CMAKE_BUILD_TYPE OR CMAKE_CONFIGURATION_TYPES))
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Release" "RelWithDebInfo" "MinSizeRel" "Debug")
endif()

include(CheckCXXCompilerFlag)
include(CheckIPOSupported)
if (NOT COMPILER_FLAG_TEST_COMPLETE)
  check_ipo_supported(RESULT COMPILER_SUPPORT_IPO)
  check_cxx_compiler_flag("-march=native" COMPILER_SUPPORT_MARCH_NATIVE)
  if (NOT COMPILER_SUPPORT_MARCH_NATIVE)
    check_cxx_compiler_flag("-xHost" COMPILER_SUPPORT_xHOST)
    check_cxx_compiler_flag("/QxHost" COMPILER_SUPPORT_QxHOST)
  endif()
  set(COMPILER_FLAG_TEST_COMPLETE TRUE)
endif()

if (COMPILER_SUPPORT_MARCH_NATIVE)
  add_compile_options("-march=native")
elseif(COMPILER_SUPPORT_xHOST)
  add_compile_options("-xHost")
elseif(COMPILER_SUPPORT_QxHOST)
  add_compile_options("/QxHost")
endif()

if (MSVC AND NOT MSVC_SIMD_DETECTED)
  if (NOT DEFINED AVX512_RUN_RESULT)
    message(STATUS "Checking AVX512")
    check_cxx_compiler_flag("/arch:AVX512" COMPILER_SUPPORT_AVX512)
    try_run(AVX512_RUN_RESULT AVX512_COMPILE_RESULT ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/cmake/cputest/avx512.cpp)
  endif()
  if (NOT DEFINED AVX2_RUN_RESULT)
    message(STATUS "Checking AVX2 ...")
    check_cxx_compiler_flag("/arch:AVX2" COMPILER_SUPPORT_AVX2)
    try_run(AVX2_RUN_RESULT AVX2_COMPILE_RESULT ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/cmake/cputest/avx2.cpp)
  endif()
  if (NOT DEFINED AVX_RUN_RESULT)
    message(STATUS "Checking AVX ...")
    check_cxx_compiler_flag("/arch:AVX" COMPILER_SUPPORT_AVX)
    try_run(AVX_RUN_RESULT AVX_COMPILE_RESULT ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/cmake/cputest/avx.cpp)
  endif()

  if (AVX512_RUN_RESULT EQUAL 0 AND COMPILER_SUPPORT_AVX512)
    message(STATUS "Your CPU supports AVX512")
    add_compile_options("/arch:AVX512")
  elseif (AVX2_RUN_RESULT EQUAL 0 AND COMPILER_SUPPORT_AVX2)
    message(STATUS "Your CPU supports AVX2")
    add_compile_options("/arch:AVX2")
  elseif (AVX_RUN_RESULT EQUAL 0 AND COMPILER_SUPPORT_AVX)
    message(STATUS "Your CPU supports AVX")
    add_compile_options("/arch:AVX")
  else()
    message(STATUS "Your CPU does not support AVX, will use SSE")
  endif()
  set(MSVC_SIMD_DETECTED TRUE)
endif()

if (COMPILER_SUPPORT_IPO)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

if (MSVC)
  add_compile_options("/permissive-")
  add_compile_options("/Zc:__cplusplus")
  add_compile_options("/MP")
  add_compile_options("/utf-8")
  set(USE_MSVC_RUNTIME_LIBRARY_DLL ON)
endif()

set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)

# ========================================================
# 第三方庫
# ========================================================
add_subdirectory(extern/glad)
add_subdirectory(extern/glfw)
include_directories(extern/glm)
include_directories(extern/stb/include)

# ========================================================
# ImGui 設定
# ========================================================
file(GLOB IMGUI_SOURCES
    "extern/imgui/*.cpp"
    "extern/imgui/*.h"
    "extern/imgui/backends/*.cpp" 
    "extern/imgui/backends/*.h"
)
add_library(imgui_lib STATIC ${IMGUI_SOURCES})
target_include_directories(imgui_lib PUBLIC extern/imgui)
target_link_libraries(imgui_lib PRIVATE glfw) 
# ImGui 需要這個定義
target_compile_definitions(imgui_lib PUBLIC GLFW_INCLUDE_NONE)
target_compile_definitions(imgui_lib PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)

# ========================================================
# 主程式
# ========================================================
include_directories(include)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(HW2 ${SOURCES})

target_link_libraries(HW2 PRIVATE
    glad
    glfw
    imgui_lib
)

# [修正] 只針對 HW2 加入這個定義，這樣就不會影響到 glfw 的編譯
target_compile_definitions(HW2 PRIVATE GLFW_INCLUDE_NONE)

target_include_directories(HW2 PRIVATE include)

# ========================================================
# 直接在 Configure 階段強制複製資源 (比 POST_BUILD 更保險)
# ========================================================
file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_SOURCE_DIR}/bin)
message(STATUS "Assets copied to bin/assets")